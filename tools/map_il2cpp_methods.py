#!/usr/bin/env python3
"""Map v31 metadata method tokens to ARM64 ELF code-module pointer slots.

Static structural evidence only. Does not execute/decompile code or imply that
the runtime invokes a method. Load bias is zero; addresses are ELF virtual
addresses. Inputs must be this project parser's metadata JSON and ELF64 LE.
Schema/lookup source: Perfare/Il2CppDumper commit 4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a,
Il2Cpp/Il2CppClass.cs (CodeGenModule) and Il2Cpp/Il2Cpp.cs (GetMethodPointer).
"""
import argparse
from bisect import bisect_right
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import struct

COMMIT = '4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a'
BASE = 'https://github.com/Perfare/Il2CppDumper/blob/' + COMMIT + '/Il2CppDumper/Il2Cpp/'


class Elf:
    def __init__(self, data):
        self.data = data
        h = self.unpack('<16sHHIQQQIHHHHHH', 0)
        if h[0][:7] != b'\x7fELF\x02\x01\x01' or h[1:3] != (3, 183):
            raise ValueError('Expected ELF64 little-endian AArch64 ET_DYN')
        if h[9] != 56 or h[11] != 64 or not h[10] or not h[12]:
            raise ValueError('Unsupported ELF header table sizes/counts')
        self.segments = [self.unpack('<IIQQQQQQ', h[5]+i*56) for i in range(h[10])]
        self.sections = [self.unpack('<IIQQQQIIQQ', h[6]+i*64) for i in range(h[12])]
        for p in self.segments:
            if p[0] == 1 and (p[5] > p[6] or p[2]+p[5] > len(data)):
                raise ValueError('Invalid PT_LOAD range')
        self.relocations = {}
        self.relative_targets = defaultdict(list)
        self.relocation_counts = Counter()
        for s in self.sections:
            if s[1] != 4:  # SHT_RELA, packed relocations not supported
                continue
            if s[9] != 24 or s[5] % 24 or s[4]+s[5] > len(data):
                raise ValueError('Invalid RELA section')
            for j in range(s[5] // 24):
                at = s[4]+24*j
                target, info, addend = self.unpack('<QQq', at)
                typ, sym = info & 0xffffffff, info >> 32
                if target in self.relocations:
                    raise ValueError('Duplicate relocation target')
                value = None
                if typ == 1027:  # R_AARCH64_RELATIVE = B + A, B=0 here
                    if sym != 0:
                        raise ValueError('RELATIVE relocation has symbol')
                    value = addend
                    self.relative_targets[value].append(target)
                elif typ == 257:  # R_AARCH64_ABS64 = S + A
                    if not 0 <= s[6] < len(self.sections):
                        raise ValueError('Invalid symbol section link')
                    symbols = self.sections[s[6]]
                    if symbols[9] != 24 or sym >= symbols[5] // 24:
                        raise ValueError('Invalid symbol index')
                    symbol = self.unpack('<IBBHQQ', symbols[4]+sym*24)
                    if symbol[3] != 0:
                        value = symbol[4] + addend
                self.relocations[target] = {'type': typ, 'symbol': sym, 'addend': addend,
                                             'value': value, 'file_offset': at}
                self.relocation_counts[typ] += 1

    def unpack(self, fmt, offset):
        if offset < 0 or offset + struct.calcsize(fmt) > len(self.data):
            raise ValueError('ELF read out of bounds')
        return struct.unpack_from(fmt, self.data, offset)

    def offset(self, va, size=1, executable=False):
        matches = [p[2]+va-p[3] for p in self.segments if p[0] == 1
                   and p[3] <= va and va+size <= p[3]+p[5]
                   and (not executable or p[1] & 1)]
        if len(matches) != 1:
            raise ValueError('VA not uniquely file-backed: ' + hex(va))
        return matches[0]

    def va(self, offset, size):
        matches = [p[3]+offset-p[2] for p in self.segments if p[0] == 1
                   and p[2] <= offset and offset+size <= p[2]+p[5]]
        if len(matches) != 1:
            raise ValueError('Offset not uniquely mapped')
        return matches[0]

    def pointer(self, va):
        try:
            raw, = self.unpack('<Q', self.offset(va, 8))
        except ValueError:
            # ELF PT_LOAD memory beyond p_filesz is zero-initialized (.bss).
            bss = [p for p in self.segments if p[0] == 1
                   and p[3]+p[5] <= va and va+8 <= p[3]+p[6]]
            if len(bss) != 1:
                raise
            raw = 0
        if va not in self.relocations:
            if raw != 0:
                raise ValueError('Nonzero pointer lacks supported relocation')
            return 0
        reloc = self.relocations[va]
        if reloc['type'] not in (1027, 257) or reloc['value'] is None:
            raise ValueError('Unsupported/unresolved pointer relocation')
        return reloc['value']

    def integer(self, va):
        if va in self.relocations:
            raise ValueError('Integer unexpectedly relocated')
        return self.unpack('<Q', self.offset(va, 8))[0]


def map_methods(elf, metadata):
    if metadata['version'] != 31:
        raise ValueError('Only metadata v31 supported')
    image_methods = defaultdict(list)
    for m in metadata['methods']:
        t = metadata['types'][m['declaring_type_index']]
        image_methods[t['image_index']].append(m)
    modules = []
    for image in metadata['images']:
        ims = image_methods[image['index']]
        rids = [int(m['token'], 16) & 0xffffff for m in ims]
        if sorted(rids) != list(range(1, len(ims)+1)):
            raise ValueError('Image tokens are not a complete unique RID range')
        needle = image['name'].encode('utf-8') + b'\0'
        candidates = []
        cursor = 0
        while (position := elf.data.find(needle, cursor)) >= 0:
            cursor = position + 1
            try:
                name_va = elf.va(position, len(needle))
            except ValueError:
                continue
            for module_va in elf.relative_targets.get(name_va, []):
                try:
                    elf.offset(module_va, 136)  # v31 ARM64 CodeGenModule
                    count = elf.integer(module_va+8)
                    if count != len(ims):
                        continue
                    table = elf.pointer(module_va+16)
                    if count:
                        if sum(p[0] == 1 and p[3] <= table and table+count*8 <= p[3]+p[6]
                               for p in elf.segments) != 1:
                            raise ValueError('Method table outside unique PT_LOAD memory')
                    nulls = 0
                    for rid in rids:
                        addr = elf.pointer(table+(rid-1)*8)
                        if not addr:
                            nulls += 1
                        elif addr % 4:
                            raise ValueError('Unaligned ARM64 method address')
                        else:
                            elf.offset(addr, 4, executable=True)
                    candidates.append({'image_index': image['index'], 'name': image['name'],
                                       'module_va': module_va, 'name_va': name_va,
                                       'method_count': count, 'method_table_va': table,
                                       'null_method_slots': nulls})
                except ValueError:
                    continue
        if len(candidates) != 1:
            raise ValueError('Expected one validated module for ' + image['name'] + ': ' + str(len(candidates)))
        modules.append(candidates[0])

    # Corroborate candidate structures against one complete 64-entry module array,
    # itself referenced by a v31 CodeRegistration whose preceding count agrees.
    module_set = {m['module_va'] for m in modules}
    hits = sorted(r for m in modules for r in elf.relative_targets.get(m['module_va'], []))
    arrays = []
    for start in hits:
        try:
            entries = [elf.pointer(start+8*i) for i in range(len(modules))]
        except ValueError:
            continue
        if len(set(entries)) == len(modules) and set(entries) == module_set:
            arrays.append(start)
    registrations = []
    for array in arrays:
        for slot in elf.relative_targets.get(array, []):
            if slot >= 128 and elf.integer(slot-8) == len(modules):
                elf.offset(slot-128, 136)
                registrations.append({'code_registration_va': slot-128,
                                      'modules_count_va': slot-8, 'modules_pointer_va': slot,
                                      'modules_array_va': array, 'module_count': len(modules)})
    if len(registrations) != 1:
        raise ValueError('Expected one code-registration module-count/array reference')

    mapped = []
    for module in modules:
        for m in image_methods[module['image_index']]:
            rid = int(m['token'], 16) & 0xffffff
            slot = module['method_table_va']+(rid-1)*8
            addr = elf.pointer(slot)
            t = metadata['types'][m['declaring_type_index']]
            mapped.append({'method_index': m['index'], 'token': m['token'],
                           'type_index': t['index'], 'namespace': t['namespace'],
                           'type_name': t['name'], 'method_name': m['name'],
                           'image_name': module['name'], 'pointer_slot_va': slot,
                           'relocation': elf.relocations.get(slot), 'elf_va': addr or None,
                           'file_offset': elf.offset(addr, 4, executable=True) if addr else None,
                           'status': 'structurally_mapped' if addr else 'null_slot_unresolved'})
    addresses = sorted({m['elf_va'] for m in mapped if m['elf_va'] is not None})
    for m in mapped:
        next_index = bisect_right(addresses, m['elf_va']) if m['elf_va'] is not None else len(addresses)
        m['next_distinct_mapped_method_va'] = addresses[next_index] if next_index < len(addresses) else None
    return {'schema_sources': [BASE+'Il2CppClass.cs', BASE+'Il2Cpp.cs'],
            'elf_sha256': hashlib.sha256(elf.data).hexdigest(), 'elf_size': len(elf.data),
            'apk_sha256': metadata['apk_sha256'], 'metadata_sha256': metadata['metadata_sha256'],
            'architecture': 'AArch64', 'load_bias': 0,
            'address_kind': 'ELF virtual address, not runtime address or file offset',
            'relocation_counts': {str(k): v for k, v in sorted(elf.relocation_counts.items())},
            'code_registration_candidate': registrations[0], 'modules': modules,
            'mapped_methods': mapped,
            'priority_callbacks': [m for m in mapped if m['method_index'] in (34537, 34538, 34539)],
            'limitations': ['Static schema and relocation validation, not runtime registration observation.',
                            'Null slots and generic method instantiations are not resolved.',
                            'No function body boundary, correctness or behaviour is inferred.',
                            'Next mapped address is only a search bound, not a function end; unmapped code may intervene.',
                            'CodeRegistration base inferred from module tail layout; other fields not validated.']}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('elf', type=Path)
    ap.add_argument('metadata_json', type=Path)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    if args.output.resolve() in (args.elf.resolve(), args.metadata_json.resolve()):
        ap.error('Refusing to overwrite an input')
    meta = json.loads(args.metadata_json.read_text(encoding='utf-8-sig'))
    result = map_methods(Elf(args.elf.read_bytes()), meta)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False, sort_keys=True)+'\n', encoding='utf-8')
    print(json.dumps({'modules': len(result['modules']), 'mapped': len(result['mapped_methods']),
                      'registration': result['code_registration_candidate'],
                      'priority': result['priority_callbacks']}, indent=2))


if __name__ == '__main__':
    main()
