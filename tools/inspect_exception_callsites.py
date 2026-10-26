"""Read this ARM64 artifact's bounded LSDA call-site table without execution.

Supports omitted LPStart, omitted or indirect PC-relative sdata8 type table,
and ULEB128 call sites. Does not resolve catch types or execute handlers.
Uses the already installed local pyelftools, like inspect_unwind_ranges.py.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

from map_il2cpp_methods import Elf


def callsites(data, offset, function_start, function_size):
    pos = offset

    def byte():
        nonlocal pos
        if pos < 0 or pos >= len(data):
            raise ValueError('truncated LSDA')
        value = data[pos]
        pos += 1
        return value

    def uleb():
        value = 0
        for shift in range(0, 70, 7):
            part = byte()
            value |= (part & 127) << shift
            if not part & 128:
                if value >= 1 << 64:
                    raise ValueError('ULEB128 exceeds uint64')
                return value
        raise ValueError('unterminated ULEB128')

    if byte() != 0xff:
        raise ValueError('unsupported LPStart encoding')
    types = byte()
    if types == 0x9c:
        delta = uleb()
        if pos + delta > len(data):
            raise ValueError('type table outside input')
    elif types != 0xff:
        raise ValueError('unsupported type table encoding')
    if byte() != 1:
        raise ValueError('unsupported call-site encoding')
    size = uleb()
    end = pos + size
    if end > len(data):
        raise ValueError('call-site table outside input')
    rows = []
    previous_end = 0
    while pos < end:
        start, length, landing, action = (uleb(), uleb(), uleb(), uleb())
        if pos > end:
            raise ValueError('entry crosses call-site table end')
        if start < previous_end or start + length > function_size or landing >= function_size:
            raise ValueError('call-site range outside FDE or unordered')
        rows.append({'start_va': function_start + start,
                     'end_exclusive': function_start + start + length,
                     'landing_pad_va': function_start + landing if landing else None,
                     'action_index': action})
        previous_end = start + length
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf', type=Path)
    parser.add_argument('--function', type=lambda s: int(s, 0), required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.elf.resolve() == args.output.resolve():
        parser.error('Output cannot overwrite ELF')
    sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'external/python-libs'))
    from elftools.elf.elffile import ELFFile
    from elftools.dwarf.callframe import FDE
    with args.elf.open('rb') as stream:
        entries = ELFFile(stream).get_dwarf_info().EH_CFI_entries()
        matches = [e for e in entries if isinstance(e, FDE) and e['initial_location'] == args.function]
    if len(matches) != 1 or matches[0].lsda_pointer is None:
        parser.error('Need exactly one FDE with LSDA at the specified function start')
    fde = matches[0]
    data = args.elf.read_bytes()
    elf = Elf(data)
    rows = callsites(data, elf.offset(fde.lsda_pointer, 1), args.function, fde['address_range'])
    result = {'elf_sha256': hashlib.sha256(data).hexdigest(),
              'function_va': args.function, 'lsda_va': fde.lsda_pointer,
              'fde_size': fde['address_range'], 'callsites': rows,
              'limitations': ['Call-site coverage and landing addresses only; catch types, action chains, handler behavior and runtime outcomes are not inferred.']}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(f'{len(rows)} call-site ranges; LSDA {fde.lsda_pointer:#x}')


if __name__ == '__main__':
    main()
