"""Inspect local Talk.txt/EventMsg.txt with the recovered record layouts.

Research parser: decimal/ASCII-whitespace Int32 subset, not .NET cultures.
Preserves CR and trailing tokens; does not translate, render, or select locale.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re


def integer(token):
    if not re.fullmatch(r'[ \t\r\n\v\f]*[+-]?[0-9]+[ \t\r\n\v\f]*', token):
        raise ValueError('outside observed integer syntax')
    value = int(token)
    if not -(1 << 31) <= value < (1 << 31):
        raise ValueError('integer outside int32')
    return value


def parse_row(line, kind):
    tokens = line.split('\t')
    if len(tokens) < 3:
        raise ValueError('truncated presentation record')
    record = {'id': integer(tokens[0])}
    if kind == 'talk':
        record['character_index'] = integer(tokens[1])
        count = integer(tokens[2])
        if count > len(tokens) - 3:
            raise ValueError('truncated talk string array')
        record['texts'] = None if count < 0 else tokens[3:3 + count]
        consumed = 3 + max(count, 0)
    elif kind == 'event':
        record['type'] = integer(tokens[1])
        record['text'] = tokens[2]
        consumed = 3
    else:
        raise ValueError('unknown table kind')
    return record, tokens[consumed:]


def markup_inventory(records, kind):
    """Lexical candidates only: no translation, tag semantics or block parsing.

    Locations are record ordinals and fragment indices, so duplicate IDs do not
    collapse evidence. Unknown tokens are retained instead of being stripped.
    """
    occurrences, text_count = {}, 0
    for ordinal, record in enumerate(records):
        texts = (record['texts'] or []) if kind == 'talk' else [record['text']]
        for fragment, text in enumerate(texts):
            text_count += 1
            for match in re.finditer(r'<[^<>]*>', text):
                token = match.group()
                item = occurrences.setdefault(token, {
                    'count': 0,
                    'numeric_argument_candidate': bool(re.fullmatch(r'<[0-9]+>', token)),
                    'locations': []})
                item['count'] += 1
                item['locations'].append({'record_ordinal': ordinal, 'id': record['id'],
                                          'fragment': fragment})
    return {'scope': 'lexical_before_translation_and_substitution',
            'text_count': text_count,
            'tokens': dict(sorted(occurrences.items()))}


def inspect(path, kind):
    raw = path.read_bytes()
    records, trailing = [], []
    for number, line in enumerate(raw.decode('utf-8-sig').split('\n'), 1):
        if not line:
            continue
        try:
            record, rest = parse_row(line, kind)
        except ValueError as error:
            raise ValueError(f'line {number}: {error}') from error
        records.append(record)
        if rest:
            trailing.append({'line': number, 'tokens': rest})
    ids = Counter(record['id'] for record in records)
    return {'summary': {'sha256': hashlib.sha256(raw).hexdigest(), 'kind': kind,
                        'record_count': len(records), 'distinct_ids': len(ids),
                        'duplicate_ids': sorted(k for k, v in ids.items() if v > 1),
                        'trailing_record_count': len(trailing),
                        'null_text_arrays': sum(r.get('texts', []) is None for r in records)},
            'records': records, 'trailing': trailing,
            'markup': markup_inventory(records, kind)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('kind', choices=['talk', 'event'])
    parser.add_argument('input', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = inspect(args.input, args.kind)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result['summary']))


if __name__ == '__main__':
    main()
