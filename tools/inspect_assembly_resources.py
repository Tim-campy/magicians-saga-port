#!/usr/bin/env python3
"""Inspect the shipped IL2CPP assembly resource container; never emit resource bodies."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

MEMBER = 'assets/bin/Data/Managed/Resources/mscorlib.dll-resources.dat'


def extent(data, start, size):
    if start < 0 or size < 0 or start > len(data) or size > len(data) - start:
        raise ValueError('resource extent outside input')
    return data[start:start + size]


def integer(data, offset):
    return struct.unpack('<i', extent(data, offset, 4))[0]


def parse(data):
    """Strict shipped-layout reader: directory followed by contiguous resource bodies."""
    body = integer(data, 0) + 4
    count = integer(data, 4)
    if count < 0 or count > (len(data) - 8) // 8 or body < 8:
        raise ValueError('invalid resource directory')
    extent(data, 0, body)
    cursor = 8
    result = []
    for _ in range(count):
        size, length = integer(data, cursor), integer(data, cursor + 4)
        cursor += 8
        name = extent(data, cursor, length)
        cursor += length
        payload = extent(data, body, size)
        result.append((name, body, payload))
        body += size
    if cursor != integer(data, 0) + 4 or body != len(data):
        raise ValueError('directory or payload end mismatch')
    return result


def inspect_collation(core, tailoring):
    """Bound the version-3 cctor reads; this is not the comparison algorithm."""
    if extent(core, 0, 1) != b'\x03' or extent(tailoring, 0, 1) != b'\x03':
        raise ValueError('unsupported collation resource version')
    cursor = 1
    tables = []
    for _ in range(5):
        size = integer(core, cursor)
        cursor += 4
        extent(core, cursor, size)
        tables.append({'offset': cursor, 'size': size})
        cursor += size
    if cursor != len(core):
        raise ValueError('core trailing data')
    count = integer(tailoring, 1)
    extent(tailoring, 5, count * 13)
    cursor = 5 + count * 13
    # The cctor skips these two bytes without validating their value.
    extent(tailoring, cursor, 2)
    units = integer(tailoring, cursor + 2)
    extent(tailoring, cursor + 6, units * 2)
    if cursor + 6 + units * 2 != len(tailoring):
        raise ValueError('tailoring trailing data')
    return {'version': 3, 'core_tables': tables, 'tailoring_records': count,
            'tailoring_utf16_units': units}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('apk', type=Path)
    args = parser.parse_args()
    with zipfile.ZipFile(args.apk) as archive:
        rows = parse(archive.read(MEMBER))
    by_name = {}
    for name, _, data in rows:
        by_name.setdefault(name, data)  # Native lookup selects the first matching descriptor.
    report = {'member': MEMBER, 'resources': [
        {'name': name.decode('utf-8'), 'offset': offset, 'size': len(data),
         'sha256': hashlib.sha256(data).hexdigest()} for name, offset, data in rows],
        'collation': inspect_collation(by_name[b'collation.core.bin'], by_name[b'collation.tailoring.bin'])}
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
