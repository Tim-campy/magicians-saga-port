#!/usr/bin/env python3
"""Strict structural SEB modes 0/1 reader; no rendering or timing interpretation.

Layout: KairoLibrary Seb._load index 18983, ELF VA 0x1baf5a0.
Disk fields retain neutral names until consumer semantics are established.
Roundtrip reconstructs from parsed scalars, never an opaque original-byte cache.
"""
import argparse
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import struct


@dataclass(frozen=True)
class Record:
    values: tuple[int, ...]
    control: int | None = None
    extra_word: int | None = None
    pair: tuple[int, int] | None = None
    tail_pair: tuple[int, int] | None = None
    flags: int | None = None


@dataclass(frozen=True)
class Group:
    records: tuple[Record, ...]
    legacy_value: int | None = None


@dataclass(frozen=True)
class Seb:
    mode: int
    header_value: int
    groups: tuple[Group, ...]


def extra_present(discriminator, control):
    # 0x1baf904..0x1bafab0: negative takes its own branch first.
    return -6 <= discriminator <= -2 or (discriminator >= 0 and control != 0)


def pair_present(discriminator):
    return discriminator >= 0 or discriminator == -8


def parse(data):
    pos = 0

    def read(fmt):
        nonlocal pos
        size = struct.calcsize('>'+fmt)
        if pos+size > len(data):
            raise ValueError(f'truncated SEB at offset {pos}: need {size} bytes')
        values = struct.unpack_from('>'+fmt, data, pos)
        pos += size
        return values[0] if len(values) == 1 else values

    first = read('b')
    mode = -first if first < 0 else 0
    if mode not in (0, 1):
        raise ValueError(f'unsupported SEB mode {mode}')
    count = read('h') if mode else (first << 8) | read('B')
    header = read('h')
    if count < 0 or header < 0:
        raise ValueError('negative allocation count in SEB header')
    groups = []
    for _ in range(count):
        n = read('h')
        if n < 0:
            raise ValueError('negative SEB record count')
        legacy = read('h') if mode == 0 else None
        records = []
        for _ in range(n):
            if mode == 0:
                records.append(Record(read('10h')))
                continue
            values = read('4h')
            control = read('B')
            discriminator = values[1]
            extra = read('I') if extra_present(discriminator, control) else None
            pair = read('2h') if pair_present(discriminator) else None
            tail = read('2h')
            flags = read('B') if discriminator >= 0 else None
            records.append(Record(values, control, extra, pair, tail, flags))
        groups.append(Group(tuple(records), legacy))
    if pos != len(data):
        raise ValueError(f'unconsumed SEB bytes: {len(data)-pos} at offset {pos}')
    return Seb(mode, header, tuple(groups))


def encode(seb):
    if seb.mode not in (0, 1):
        raise ValueError('unsupported SEB mode')
    if not 0 <= len(seb.groups) <= 32767 or not 0 <= seb.header_value <= 32767:
        raise ValueError('header outside supported allocation range')
    out = bytearray(b'\xff' if seb.mode else b'')

    def put(fmt, *values):
        try:
            out.extend(struct.pack('>'+fmt, *values))
        except (struct.error, TypeError) as exc:
            raise ValueError('SEB field outside encoded range') from exc

    def presence(value, expected):
        if (value is not None) != expected:
            raise ValueError('SEB conditional field presence mismatch')

    put('hh', len(seb.groups), seb.header_value)
    for group in seb.groups:
        if len(group.records) > 32767:
            raise ValueError('too many records')
        put('h', len(group.records))
        presence(group.legacy_value, seb.mode == 0)
        if seb.mode == 0:
            put('h', group.legacy_value)
        for record in group.records:
            if seb.mode == 0:
                if len(record.values) != 10 or any(v is not None for v in
                    (record.control, record.extra_word, record.pair, record.tail_pair, record.flags)):
                    raise ValueError('invalid mode 0 record shape')
                put('10h', *record.values)
                continue
            if len(record.values) != 4:
                raise ValueError('invalid mode 1 record shape')
            put('4h', *record.values)
            put('B', record.control)
            discriminator = record.values[1]
            presence(record.extra_word, extra_present(discriminator, record.control))
            presence(record.pair, pair_present(discriminator))
            presence(record.tail_pair, True)
            presence(record.flags, discriminator >= 0)
            if record.extra_word is not None:
                put('I', record.extra_word)
            if record.pair is not None:
                put('2h', *record.pair)
            put('2h', *record.tail_pair)
            if record.flags is not None:
                put('B', record.flags)
    return bytes(out)


def inspect_manifest(path):
    manifest = json.loads(path.read_text(encoding='utf-8'))
    root = path.parent.resolve()
    rows = []
    for item in manifest['members']:
        if not item['name'].endswith('.seb'):
            continue
        source = (root/item['output']).resolve()
        if not source.is_relative_to(root) or source == root:
            raise ValueError('manifest member escapes extraction root')
        data = source.read_bytes()
        digest = hashlib.sha256(data).hexdigest()
        if len(data) != item['size'] or digest != item['sha256']:
            raise ValueError(f'manifest identity mismatch: {item["output"]}')
        seb = parse(data)
        if encode(seb) != data:
            raise ValueError(f'roundtrip failed: {item["output"]}')
        rows.append({'path': item['output'], 'sha256': digest, 'size': len(data),
                     'mode': seb.mode, 'header_value': seb.header_value,
                     'group_count': len(seb.groups),
                     'record_counts': [len(g.records) for g in seb.groups],
                     'roundtrip': True})
    return {'scope': 'SEB disk structure; no animation/rendering semantics',
            'manifest_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
            'apk_sha256': manifest['apk_sha256'], 'files': rows}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    report = inspect_manifest(args.manifest)
    if args.output:
        # Report must stay outside the source extraction tree.
        if args.output.resolve().is_relative_to(args.manifest.parent.resolve()):
            parser.error('report must not overwrite files in extraction tree')
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n', encoding='utf-8')
    print(f'SEB structural roundtrips: {len(report["files"])}')


if __name__ == '__main__':
    main()
