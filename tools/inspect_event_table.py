"""Inspect extracted Event.txt using the statically recovered field layout.

Accepts only the observed decimal/ASCII-whitespace subset of Int32.Parse.
Not a complete emulator of .NET parsing, cultures, or allocation failures.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re


def parse_row(line):
    tokens = line.split('\t')
    position = 0

    def take():
        nonlocal position
        if position >= len(tokens):
            raise ValueError('truncated row')
        token = tokens[position]
        position += 1
        return token

    def integer():
        token = take()
        if not re.fullmatch(r'[ \t\r\n\v\f]*[+-]?[0-9]+[ \t\r\n\v\f]*', token):
            raise ValueError('outside observed integer syntax')
        value = int(token)
        if not -(1 << 31) <= value < (1 << 31):
            raise ValueError('integer outside int32')
        return value

    def scripts():
        count = integer()
        if count < 0:
            return None
        if count > len(tokens) - position:
            raise ValueError('truncated script counts')
        rows = []
        for _ in range(count):
            size = integer()
            if size < 0 or size > len(tokens) - position:
                raise ValueError('invalid or truncated script row')
            rows.append([integer() for _ in range(size)])
        return rows

    record = {'id': integer(), 'name': take(), 'timing': integer(),
              'terms': scripts(), 'execs': scripts()}
    # Original EventData.Load does not explicitly reject trailing tokens.
    return record, tokens[position:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    raw = args.input.read_bytes()
    # Preserve CR: this is the no-normalization branch of the original split.
    lines = [s for s in raw.decode('utf-8-sig').split('\n') if s]
    records, trailing = [], []
    for index, line in enumerate(lines):
        try:
            record, rest = parse_row(line)
        except ValueError as error:
            raise ValueError(f'line {index + 1}: {error}') from error
        records.append(record)
        if rest:
            trailing.append({'line': index + 1, 'count': len(rest)})
    summary = {'sha256': hashlib.sha256(raw).hexdigest(), 'rows': len(records),
               'trailing_tokens': trailing,
               'timings': dict(Counter(r['timing'] for r in records)),
               'null_terms': sum(r['terms'] is None for r in records),
               'null_execs': sum(r['execs'] is None for r in records),
               'exec_rows': sum(len(r['execs'] or []) for r in records)}
    args.output.write_text(json.dumps({'summary': summary, 'records': records}, indent=2,
                                     ensure_ascii=False), encoding='utf-8')
    print(json.dumps(summary))


if __name__ == '__main__':
    main()
