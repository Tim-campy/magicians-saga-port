"""Inventory immediate ARM64 calls inside exact FDE ranges; never execute the ELF."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / '.tools/python-libs'))
from elftools.elf.elffile import ELFFile


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('elf', type=Path)
    p.add_argument('code_map', type=Path)
    p.add_argument('unwind', type=Path)
    p.add_argument('--method-index', type=int, action='append', required=True)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    if args.output.resolve() in {x.resolve() for x in (args.elf, args.code_map, args.unwind)}:
        p.error('Output cannot overwrite inputs')
    mapping = json.loads(args.code_map.read_text(encoding='utf8'))
    unwind = json.loads(args.unwind.read_text(encoding='utf8'))
    digest = hashlib.sha256(args.elf.read_bytes()).hexdigest()
    if any(x['elf_sha256'] != digest for x in (mapping, unwind)):
        p.error('ELF identity mismatch')
    methods = {m['method_index']: m for m in mapping['mapped_methods']}
    names = {}
    for m in methods.values():
        if m['elf_va'] is not None:
            names.setdefault(m['elf_va'], []).append({k: m[k] for k in
                ('method_index', 'namespace', 'type_name', 'method_name', 'image_name')})
    result = {'elf_sha256': digest, 'methods': [], 'limitations': [
        'Static instruction scan, not reachability or execution proof.',
        'FDE ranges are unwind coverage; indirect targets remain unresolved.',
        'External B instructions are tail-transfer candidates, not proven calls.']}
    with args.elf.open('rb') as stream:
        elf = ELFFile(stream)
        if elf['e_machine'] != 'EM_AARCH64' or not elf.little_endian:
            p.error('Requires little-endian AArch64 ELF')
        segments = [s for s in elf.iter_segments() if s['p_type'] == 'PT_LOAD']
        for index in args.method_index:
            method = methods[index]
            start = method['elf_va']
            ranges = [r for r in unwind['ranges'] if r['elf_va'] == start]
            if len(ranges) != 1:
                p.error(f'Method {index} lacks a unique exact-start FDE')
            size = ranges[0]['size']
            if start % 4 or size % 4:
                p.error('Unaligned FDE')
            candidates = [s for s in segments if s['p_flags'] & 1 and
                          s['p_vaddr'] <= start and start + size <= s['p_vaddr'] + s['p_filesz']]
            if len(candidates) != 1:
                p.error('FDE not wholly within one file-backed executable segment')
            seg = candidates[0]
            stream.seek(seg['p_offset'] + start - seg['p_vaddr'])
            body = stream.read(size)
            transfers = []
            for offset in range(0, size, 4):
                word, = struct.unpack_from('<I', body, offset)
                site = start + offset
                if word & 0x7c000000 == 0x14000000:
                    delta = word & 0x03ffffff
                    if delta & 0x02000000:
                        delta -= 0x04000000
                    target = site + delta * 4
                    kind = 'BL' if word & 0x80000000 else 'B'
                    if kind == 'BL' or not start <= target < start + size:
                        transfers.append({'site_elf_va': site, 'kind': kind,
                                          'target_elf_va': target, 'mapped_targets': names.get(target, [])})
                elif word & 0xfffffc1f in (0xd63f0000, 0xd61f0000):
                    transfers.append({'site_elf_va': site,
                                      'kind': 'BLR' if word & 0x200000 else 'BR',
                                      'register': (word >> 5) & 31})
            result['methods'].append({'method_index': index, 'elf_va': start,
                                      'fde_size': size, 'transfers': transfers})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf8')
    print('Methods:', len(result['methods']), 'Transfers:', sum(len(m['transfers']) for m in result['methods']))


if __name__ == '__main__':
    main()
