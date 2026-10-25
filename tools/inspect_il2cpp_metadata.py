#!/usr/bin/env python3
"""Read-only, version-31 IL2CPP metadata catalogue from the user's APK.

Layout reference (not game code): Perfare/Il2CppDumper MetadataClass.cs,
commit 4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a. Only fixed v31 records
are supported. No native method address, field offset or behaviour is inferred.
Output contains proprietary symbol names and belongs in research/local/ only.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

MEMBER = 'assets/bin/Data/Managed/Metadata/global-metadata.dat'
SOURCE = ('https://github.com/Perfare/Il2CppDumper/blob/'
          '4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a/Il2CppDumper/Il2Cpp/MetadataClass.cs')
SECTIONS = ('string_literals string_literal_data strings events properties methods '
            'parameter_defaults field_defaults default_data marshaled_sizes parameters fields '
            'generic_parameters generic_constraints generic_containers nested_types interfaces '
            'vtable_methods interface_offsets types images assemblies field_refs '
            'referenced_assemblies attribute_data attribute_ranges unresolved_parameter_types '
            'unresolved_parameter_ranges windows_type_names windows_strings exported_types').split()


def parse(data):
    if len(data) < 256 or struct.unpack_from('<II', data) != (0xFAB11BAF, 31):
        raise ValueError('Only bounded IL2CPP metadata version 31 is supported')
    sections = {}
    for i, name in enumerate(SECTIONS):
        offset, size = struct.unpack_from('<II', data, 8 + i * 8)
        if offset < 256 or offset + size > len(data):
            raise ValueError('Section outside file: ' + name)
        sections[name] = {'offset': offset, 'size_bytes': size}
    nonempty = sorted((s['offset'], s['offset'] + s['size_bytes'], n)
                      for n, s in sections.items() if s['size_bytes'])
    if any(a[1] > b[0] for a, b in zip(nonempty, nonempty[1:])):
        raise ValueError('Overlapping metadata sections')

    def records(name, fmt):
        s = sections[name]
        width = struct.calcsize(fmt)
        if s['size_bytes'] % width:
            raise ValueError('Misaligned record table: ' + name)
        s['record_size'] = width
        s['record_count'] = s['size_bytes'] // width
        return list(struct.iter_unpack(fmt, data[s['offset']:s['offset'] + s['size_bytes']]))

    string_section = sections['strings']
    strings = data[string_section['offset']:string_section['offset'] + string_section['size_bytes']]

    def string(index):
        if not 0 <= index < len(strings):
            raise ValueError('String index outside table')
        end = strings.find(b'\0', index)
        if end < 0:
            raise ValueError('Unterminated metadata string')
        return strings[index:end].decode('utf-8')

    def span(start, count, total, label):
        if count == 0 and start == -1:
            return
        if start < 0 or count < 0 or start + count > total:
            raise ValueError('Invalid index span: ' + label)

    # Explicit standard-width, little-endian records, never native Python padding.
    image_rows = records('images', '<IiiIiIiIiI')
    assembly_rows = records('assemblies', '<iIii6I4i8s')
    type_rows = records('types', '<II5iI8i8HII')
    method_rows = records('methods', '<I5iI4H')
    parameter_rows = records('parameters', '<IIi')
    field_rows = records('fields', '<IiI')
    types = []
    methods = []
    fields = []
    for i, row in enumerate(method_rows):
        name, owner, ret, ret_token, param_start, generic, token, flags, iflags, slot, count = row
        if not 0 <= owner < len(type_rows) or token >> 24 != 6:
            raise ValueError('Invalid method owner/token')
        span(param_start, count, len(parameter_rows), 'method parameters')
        methods.append({'index': i, 'offset': sections['methods']['offset'] + 36*i,
                        'name': string(name), 'declaring_type_index': owner,
                        'parameter_start': param_start, 'parameter_count': count,
                        'return_type_index': ret, 'token': f'0x{token:08x}'})
    for i, row in enumerate(field_rows):
        name, type_index, token = row
        if token >> 24 != 4:
            raise ValueError('Invalid field token')
        fields.append({'index': i, 'name': string(name), 'type_index': type_index,
                       'offset': sections['fields']['offset'] + 12*i, 'token': f'0x{token:08x}'})
    for i, row in enumerate(type_rows):
        name, namespace = row[:2]
        field_start, method_start = row[8:10]
        method_count, property_count, field_count = row[16:19]
        token = row[-1]
        if token >> 24 != 2:
            raise ValueError('Invalid type token')
        span(method_start, method_count, len(methods), 'type methods')
        span(field_start, field_count, len(fields), 'type fields')
        if any(m['declaring_type_index'] != i for m in methods[method_start:method_start+method_count]):
            raise ValueError('Method ownership mismatch')
        types.append({'index': i, 'offset': sections['types']['offset'] + 88*i,
                      'name': string(name), 'namespace': string(namespace),
                      'method_start': method_start, 'method_count': method_count,
                      'field_start': field_start, 'field_count': field_count,
                      'token': f'0x{token:08x}'})
    images = []
    owners = [-1] * len(types)
    for i, row in enumerate(image_rows):
        name, assembly, start, count = row[:4]
        if not 0 <= assembly < len(assembly_rows) or assembly_rows[assembly][0] != i:
            raise ValueError('Image/assembly backlink mismatch')
        span(start, count, len(types), 'image types')
        for j in range(start, start + count):
            if owners[j] != -1:
                raise ValueError('Overlapping image types')
            owners[j] = i
            types[j]['image_index'] = i
        images.append({'index': i, 'offset': sections['images']['offset'] + 40*i,
                       'name': string(name), 'assembly_index': assembly,
                       'assembly_name': string(assembly_rows[assembly][4]),
                       'type_start': start, 'type_count': count,
                       'method_count': sum(t['method_count'] for t in types[start:start+count])})
    if -1 in owners or sum(t['method_count'] for t in types) != len(methods):
        raise ValueError('Incomplete ownership coverage')
    selected = [im for im in images if im['name'] in
                ('Assembly-CSharp.dll', 'Assembly-CSharp-firstpass.dll', 'KairoLibrary.dll')]
    return {'metadata_sha256': hashlib.sha256(data).hexdigest(), 'metadata_size': len(data),
            'version': 31, 'layout_source': SOURCE, 'sections': sections,
            'counts': {'images': len(images), 'assemblies': len(assembly_rows),
                       'types': len(types), 'methods': len(methods), 'fields': len(fields),
                       'parameters': len(parameter_rows)},
            'images': images, 'types': types, 'methods': methods, 'fields': fields,
            'candidate_game_images': selected,
            'limitations': ['Symbols and ownership only; no recovered method bodies or native addresses.',
                            'Native type indices are not resolved; no field offsets or full signatures.',
                            'Candidate game images are selected by names, not proven behaviour.',
                            'Layout validated structurally against a tool schema, not the original runtime reader.']}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('apk', type=Path)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    if args.apk.resolve() == args.output.resolve():
        ap.error('Output must not overwrite the APK')
    with zipfile.ZipFile(args.apk) as z:
        entry = z.getinfo(MEMBER)
        if entry.file_size > 64 * 1024 * 1024:
            raise ValueError('Metadata exceeds 64 MiB budget')
        result = parse(z.read(entry))
    with args.apk.open('rb') as f:
        result['apk_sha256'] = hashlib.file_digest(f, 'sha256').hexdigest()
    result['member'] = MEMBER
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False, sort_keys=True) + '\n', encoding='utf-8')
    print(json.dumps({'counts': result['counts'], 'candidates': result['candidate_game_images']}, indent=2))


if __name__ == '__main__':
    main()
