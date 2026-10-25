#!/usr/bin/env python3
"""Read-only APK inventory; standard library only. Does not execute/extract code.

The small binary XML reader covers manifest declarations, not Android resource
resolution. Resource references remain numeric and signatures are not verified.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path, PurePosixPath
import struct
import zipfile


def unpack(fmt, data, offset):
    size = struct.calcsize(fmt)
    if offset < 0 or offset + size > len(data):
        raise ValueError(f"Truncated binary XML at {offset}")
    return struct.unpack_from(fmt, data, offset)


def string_pool(chunk):
    _, header, _ = unpack('<HHI', chunk, 0)
    count, styles, flags, start, style_start = unpack('<5I', chunk, 8)
    if header < 28 or header + 4 * (count + styles) > len(chunk):
        raise ValueError('Invalid string pool table')
    end = style_start or len(chunk)
    if not header + 4 * (count + styles) <= start <= end <= len(chunk):
        raise ValueError('Invalid string pool data range')
    strings = []
    def length(pos, utf8):
        if utf8:
            value, = unpack('<B', chunk[:end], pos)
            if value & 0x80:
                low, = unpack('<B', chunk[:end], pos + 1)
                return ((value & 0x7f) << 8) | low, pos + 2
            return value, pos + 1
        value, = unpack('<H', chunk[:end], pos)
        if value & 0x8000:
            low, = unpack('<H', chunk[:end], pos + 2)
            return ((value & 0x7fff) << 16) | low, pos + 4
        return value, pos + 2
    for i in range(count):
        off, = unpack('<I', chunk, header + 4 * i)
        pos = start + off
        if not start <= pos < end:
            raise ValueError('String offset outside pool')
        utf8 = bool(flags & 0x100)
        n, pos = length(pos, utf8)
        if utf8:
            n, pos = length(pos, True)
        else:
            n *= 2
        terminator = 1 if utf8 else 2
        if pos + n + terminator > end or any(chunk[pos+n:pos+n+terminator]):
            raise ValueError('Invalid string bounds/terminator')
        strings.append(chunk[pos:pos+n].decode('utf-8' if utf8 else 'utf-16-le'))
    return strings


def parse_manifest(data):
    kind, header, total = unpack('<HHI', data, 0)
    if kind != 3 or header < 8 or total != len(data):
        raise ValueError('Expected binary Android XML')
    strings, stack, roots = [], [], []
    def string(index):
        if index == 0xffffffff:
            return None
        if index >= len(strings):
            raise ValueError('String index outside pool')
        return strings[index]
    pos = header
    while pos < total:
        kind, hsize, size = unpack('<HHI', data, pos)
        if hsize < 8 or size < hsize or pos + size > total:
            raise ValueError('Invalid XML chunk bounds')
        chunk = data[pos:pos+size]
        if kind == 1:
            strings = string_pool(chunk)
        elif kind == 0x102:
            if hsize < 16:
                raise ValueError('Invalid XML node header')
            ns, name, attr_start, attr_size, count = unpack('<IIHHH', chunk, hsize)
            base = hsize + attr_start
            if attr_start < 20 or attr_size < 20 or base + count * attr_size > size:
                raise ValueError('Invalid XML attribute bounds')
            attrs = {}
            for i in range(count):
                ans, aname, raw, value_size, zero, dtype, value = unpack('<IIIHBBI', chunk, base + i * attr_size)
                if value_size != 8:
                    raise ValueError('Unsupported XML typed value size')
                key = string(aname)
                namespace = string(ans)
                if namespace:
                    key = ('android:' if namespace == 'http://schemas.android.com/apk/res/android' else '{' + namespace + '}') + key
                if raw != 0xffffffff:
                    decoded = string(raw)
                elif dtype == 3:
                    decoded = string(value)
                elif dtype == 0x12:
                    decoded = bool(value)
                elif dtype == 0x10:
                    decoded = value if value < 0x80000000 else value - 0x100000000
                elif dtype == 1:
                    decoded = f'@0x{value:08x}'
                else:
                    decoded = {'type': dtype, 'data': f'0x{value:08x}'}
                if key in attrs:
                    raise ValueError('Duplicate XML attribute')
                attrs[key] = decoded
            node = {'tag': string(name), 'attributes': attrs, 'children': []}
            (stack[-1]['children'] if stack else roots).append(node)
            stack.append(node)
        elif kind == 0x103:
            ns, name = unpack('<II', chunk, hsize)
            if not stack or stack[-1]['tag'] != string(name):
                raise ValueError('Unbalanced XML elements')
            stack.pop()
        pos += size
    if stack or len(roots) != 1 or roots[0]['tag'] != 'manifest':
        raise ValueError('Invalid manifest tree')
    return roots[0]


def elf_summary(data):
    if data[:4] != b'\x7fELF' or data[4] not in (1, 2) or data[5] not in (1, 2):
        raise ValueError('Unsupported ELF identification')
    bits = 32 if data[4] == 1 else 64
    endian = '<' if data[5] == 1 else '>'
    machine, = unpack(endian + 'H', data, 18)
    phoff, = unpack(endian + ('I' if bits == 32 else 'Q'), data, 28 if bits == 32 else 32)
    phsize, phcount = unpack(endian + 'HH', data, 42 if bits == 32 else 54)
    if phsize < (32 if bits == 32 else 56) or phoff + phcount * phsize > len(data):
        raise ValueError('Invalid ELF program headers')
    segments, dynamic = [], None
    for i in range(phcount):
        values = unpack(endian + ('8I' if bits == 32 else 'II6Q'), data, phoff + i * phsize)
        if bits == 32:
            kind, offset, vaddr, _, filesz, _, _, _ = values
        else:
            kind, _, offset, vaddr, _, filesz, _, _ = values
        if offset + filesz > len(data):
            raise ValueError('ELF segment outside file')
        if kind == 1:
            segments.append((vaddr, offset, filesz))
        if kind == 2:
            dynamic = data[offset:offset+filesz]
    tags = []
    if dynamic is not None:
        stride = 8 if bits == 32 else 16
        for pos in range(0, len(dynamic), stride):
            tag, value = unpack(endian + ('iI' if bits == 32 else 'qQ'), dynamic, pos)
            if tag == 0:
                break
            tags.append((tag, value))
    strtab = next((v for t, v in tags if t == 5), None)
    strsize = next((v for t, v in tags if t == 10), None)
    needed = []
    if any(t == 1 for t, _ in tags):
        if strtab is None or strsize is None:
            raise ValueError('Missing ELF dynamic string table')
        table = next((data[o + strtab - v:o + strtab - v + strsize] for v, o, s in segments if v <= strtab and strtab + strsize <= v + s), None)
        if table is None:
            raise ValueError('ELF string table outside file-backed segment')
        for tag, offset in tags:
            if tag == 1:
                end = table.find(b'\0', offset)
                if offset >= len(table) or end < 0:
                    raise ValueError('Invalid ELF dependency name')
                needed.append(table[offset:end].decode('utf-8'))
    return {'bits': bits, 'machine_number': machine, 'machine': {40: 'ARM', 183: 'AArch64'}.get(machine, 'other'), 'dt_needed': needed}


def inventory(path):
    with path.open('rb') as source:
        digest = hashlib.file_digest(source, 'sha256').hexdigest()
    with zipfile.ZipFile(path) as apk:
        entries = sorted(apk.infolist(), key=lambda e: e.filename)
        if len({e.filename for e in entries}) != len(entries):
            raise ValueError('Duplicate ZIP names are ambiguous')
        manifest_info = apk.getinfo('AndroidManifest.xml')
        if manifest_info.file_size > 16 * 1024 * 1024:
            raise ValueError('Manifest exceeds 16 MiB budget')
        manifest = parse_manifest(apk.read(manifest_info))
        assets = [e for e in entries if e.filename.startswith('assets/') and not e.is_dir()]
        libs = [e.filename for e in entries if e.filename.endswith('.so')]
        folded = {}
        for e in entries:
            folded.setdefault(e.filename.casefold(), []).append(e.filename)
        elf = {}
        for name in libs:
            if apk.getinfo(name).file_size > 256 * 1024 * 1024:
                elf[name] = {'error': 'ELF exceeds 256 MiB budget'}
            else:
                elf[name] = elf_summary(apk.read(name))
        samples = []
        for e in assets[:20]:
            with apk.open(e) as stream:
                prefix = stream.read(16)
            samples.append({'path': e.filename, 'prefix_hex': prefix.hex(), 'size': e.file_size})
        application = next((x for x in manifest['children'] if x['tag'] == 'application'), None)
        components = application['children'] if application else []
        launchers = []
        for c in components:
            if c['tag'] not in ('activity', 'activity-alias'):
                continue
            for intent in c['children']:
                if intent['tag'] != 'intent-filter':
                    continue
                names = {(v['tag'], v['attributes'].get('android:name')) for v in intent['children']}
                if {('action', 'android.intent.action.MAIN'), ('category', 'android.intent.category.LAUNCHER')} <= names:
                    launchers.append(c['attributes'])
        return {
            'schema_version': 1, 'apk_name': path.name, 'size_bytes': path.stat().st_size, 'sha256': digest,
            'zip_entry_count': len(entries), 'uncompressed_bytes': sum(e.file_size for e in entries),
            'dex': [e.filename for e in entries if e.filename.endswith('.dex')], 'native_libraries': libs,
            'elf_headers_and_dependencies': elf,
            'casefold_collisions': [names for names in folded.values() if len(names) > 1],
            'abis_from_lib_paths': sorted({p.split('/')[1] for p in libs if p.startswith('lib/') and len(p.split('/')) == 3}),
            'asset_summary': {'count': len(assets), 'uncompressed_bytes': sum(e.file_size for e in assets),
                'extensions': dict(sorted(Counter(PurePosixPath(e.filename).suffix or '(none)' for e in assets).items())),
                'directories': dict(sorted(Counter(str(PurePosixPath(e.filename).parent) for e in assets).items())),
                'first_twenty_headers': samples},
            'manifest_attributes': manifest['attributes'],
            'uses_sdk': [x['attributes'] for x in manifest['children'] if x['tag'] == 'uses-sdk'],
            'permissions': [x['attributes'] for x in manifest['children'] if x['tag'].startswith('uses-permission')],
            'launcher_activities': launchers, 'manifest_tree': manifest,
            'members': [{'path': e.filename, 'size': e.file_size, 'compressed_size': e.compress_size, 'crc32': f'{e.CRC:08x}'} for e in entries],
            'limitations': ['Static declarations only; APK not executed.', 'APK signatures not verified; ZIP CRC not exhaustively checked.',
                'Resource references not resolved; custom bounded binary XML reader, pending independent apktool/JADX comparison.',
                'ABI list is derived from lib paths; ELF headers/DT_NEEDED are parsed independently, not disassembled. DEX implementation is not analyzed.',
                'Headers sample only the first 20 lexicographically sorted assets; formats and offline completeness are unknown.'],
        }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('apk', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.apk.resolve() == args.output.resolve():
        parser.error('Output must not overwrite APK')
    result = inventory(args.apk)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False, sort_keys=True) + '\n', encoding='utf-8')
    print(json.dumps({k: result[k] for k in ('sha256', 'size_bytes', 'zip_entry_count', 'dex', 'abis_from_lib_paths', 'manifest_attributes', 'uses_sdk', 'launcher_activities')}, indent=2))


if __name__ == '__main__':
    main()
