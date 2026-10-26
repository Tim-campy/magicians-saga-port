"""Record ELF unwind FDE ranges using local pyelftools; no code execution.

Install: python -m pip install --target external/python-libs pyelftools==0.32
FDE ranges describe unwind coverage, not proof of every logical function boundary.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'external/python-libs'))
from elftools.elf.elffile import ELFFile
from elftools.dwarf.callframe import FDE


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.elf.resolve() == args.output.resolve():
        parser.error('Output cannot overwrite ELF')
    with args.elf.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
        stream.seek(0)
        elf = ELFFile(stream)
        dwarf = elf.get_dwarf_info()
        rows = []
        for entry in dwarf.EH_CFI_entries():
            if isinstance(entry, FDE):
                start, size = entry['initial_location'], entry['address_range']
                rows.append({'elf_va': start, 'size': size, 'end_exclusive': start+size, 'eh_frame_offset': entry.offset})
    result = {'elf_sha256': digest, 'provider': 'pyelftools 0.32', 'ranges': rows,
              'limitations': ['FDE unwind coverage, not original symbols or verified execution.']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf8')
    print('FDE ranges:', len(rows))


if __name__ == '__main__':
    main()
