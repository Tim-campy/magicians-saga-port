"""Audit local extracted table bytes; does not emulate the game's decoder."""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    records = []
    for path in sorted(args.directory.glob('*.txt')):
        raw = path.read_bytes()
        raw.decode('utf-8-sig', errors='strict')
        records.append({
            'file': path.name, 'sha256': hashlib.sha256(raw).hexdigest(),
            'bytes': len(raw), 'utf8_valid': True,
            'utf8_bom': raw.startswith(b'\xef\xbb\xbf'),
            'crlf': raw.count(b'\r\n'), 'cr': raw.count(b'\r'),
            'lf': raw.count(b'\n'),
            'lf_nonempty_segments': sum(bool(s) for s in raw.split(b'\n')),
        })
    if not records:
        parser.error('No .txt files found')
    args.output.write_text(json.dumps({'scope': 'local byte audit, not runtime oracle',
                                       'files': records}, indent=2), encoding='utf-8')
    print(f'{len(records)} UTF-8 files audited; report: {args.output}')


if __name__ == '__main__':
    main()
