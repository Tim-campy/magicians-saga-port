#!/usr/bin/env python3
"""Record local, repeated native mission checkpoints from the maintainer's data.

This runner projects quest IDs only; the host retains all gameplay table loading
and full resource-index validation. Raw tables and reports are never copied into
the golden output. A match demonstrates local determinism, not Android fidelity.
"""
import argparse
from collections import Counter
import hashlib
import importlib.util
import json
import math
from pathlib import Path, PurePosixPath
import re
import shlex
import subprocess


def _jar_members(blob):
    spec = importlib.util.spec_from_file_location(
        'mission_golden_extractor', Path(__file__).with_name('extract_assets.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    # Unity dependencies are imported only by the extractor's extraction route.
    return module.unpack_container(blob)


def sha256_file(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def _integer(token):
    if not re.fullmatch(r'[ \t\r\v\f]*[+-]?[0-9]+[ \t\r\v\f]*', token):
        raise ValueError('malformed decimal integer')
    value = int(token)
    if not -(1 << 31) <= value < (1 << 31):
        raise ValueError('integer outside int32')
    return value


def read_index(data_root):
    """Read the selected v2/v1 index and locate its sole tables/-1/xls archive."""
    root = Path(data_root).resolve()
    index = root / 'mission-resources-v2.tsv'
    if not index.exists():
        index = root / 'mission-resources.tsv'
    blob = index.read_bytes()
    if len(blob) > 16384:
        raise ValueError('mission resource index exceeds16KiB')
    rows = [shlex.split(line, comments=False, posix=True)
            for line in blob.decode('utf-8').splitlines() if line.strip()]
    if not rows or len(rows[0]) != 2 or rows[0][0] != 'mission_resources':
        raise ValueError('invalid mission resource index header')
    version = _integer(rows[0][1])
    if version not in (1, 2):
        raise ValueError('unsupported mission resource index version')
    tables = []
    for row in rows[1:]:
        if len(row) != 4:
            raise ValueError('invalid mission resource index row')
        kind, ident, relative, name = row
        ident = _integer(ident)
        if kind not in ('tables', 'resource') or not name:
            raise ValueError('unexpected mission resource index row')
        path = PurePosixPath(relative)
        if ('\\' in relative or path.parent != PurePosixPath('textassets')
                or path.name in ('', '.', '..') or ':' in relative):
            raise ValueError('mission archive must be a direct textassets child')
        resolved = (root / path).resolve()
        if not resolved.is_relative_to(root) or resolved.parent != (root / 'textassets').resolve():
            raise ValueError('mission archive escapes the data root')
        if kind == 'tables':
            if ident != -1 or name != 'xls':
                raise ValueError('expected tables/-1/xls index row')
            tables.append((relative, resolved))
    if len(tables) != 1:
        raise ValueError('mission resource index requires exactly one tables/-1 row')
    relative, archive = tables[0]
    return archive, {'path': index.name, 'version': version,
                     'sha256': hashlib.sha256(blob).hexdigest(), 'tables_path': relative}


def quest_ids(blob):
    """Project the first TAB token, without duplicating QuestData's field parser."""
    result = set()
    for line in blob.decode('utf-8-sig').split('\n'):
        if not line:
            continue
        if '\t' not in line:
            raise ValueError('quest row lacks a TAB-delimited ID')
        ident = _integer(line.split('\t', 1)[0])
        if ident < 0 or ident == (1 << 31) - 1:
            raise ValueError('quest ID outside native catalog bounds')
        if ident in result:
            raise ValueError('duplicate quest ID')
        result.add(ident)
    if not result:
        raise ValueError('selected quest table is empty')
    return sorted(result)


def discover_quests(data_root, language='English.lproj', dpi=1):
    """Support the exact language member; reject competing root/DPI candidates.

    ResourceArchive uses ASCII/invariant case folding. Its broader precedence
    remains in native resource_lookup.cpp; this tool does not approximate it.
    """
    if (not language or not language.isascii() or '/' in language or '\\' in language
            or language in ('.', '..') or not 0 <= dpi < (1 << 31)):
        raise ValueError('unsupported resource language/dpi profile')
    archive, index = read_index(data_root)
    blob = archive.read_bytes()
    members = {}
    for name, payload in _jar_members(blob):
        if not name.isascii():
            raise ValueError('resource archive requires ASCII member names')
        key = name.lower()
        if key in members:
            raise ValueError('case-folded archive member collision')
        members[key] = (name, payload)
    wanted = language.lower() + '/quest.txt'
    if wanted not in members:
        raise ValueError('exact language/Quest.txt member is missing')
    for key in members:
        parts = key.split('/')
        if parts[-1] != 'quest.txt' or key == wanted:
            continue
        if len(parts) == 1 or parts[-2] == language.lower() or not parts[-2].endswith('.lproj'):
            raise ValueError('ambiguous root/DPI Quest.txt candidates require native lookup')
    name, table = members[wanted]
    return quest_ids(table), {
        'index': index,
        'tables_archive': {'path': index['tables_path'], 'bytes': len(blob),
                           'sha256': hashlib.sha256(blob).hexdigest()},
        'quest_table': {'member': name, 'bytes': len(table),
                        'sha256': hashlib.sha256(table).hexdigest()},
    }


def _short_text(value):
    if value is None:
        return ''
    if isinstance(value, bytes):
        value = value.decode('utf-8', errors='replace')
    return value.encode('utf-8', errors='replace')[:1024].decode('utf-8', errors='ignore')


def _canonical(report):
    return json.dumps(report, ensure_ascii=True, sort_keys=True,
                      separators=(',', ':'), allow_nan=False).encode('utf-8')


def _nonfinite(_):
    raise ValueError('nonfinite JSON number')


def _parse_report(stdout, frames):
    report = json.loads(stdout, parse_constant=_nonfinite)
    if not isinstance(report, dict):
        raise ValueError('host report is not an object')
    for name in ('state_hash', 'logic_state_hash', 'state_hash_schema'):
        if not isinstance(report.get(name), str) or not report[name]:
            raise ValueError('host report lacks checkpoint identities')
    for name in ('state_hash_bytes', 'logic_state_bytes', 'frames'):
        if type(report.get(name)) is not int or report[name] < 0:
            raise ValueError('host report lacks checkpoint sizes/frame count')
    if report['frames'] > frames or report['logic_state_bytes'] > report['state_hash_bytes']:
        raise ValueError('invalid checkpoint sizes/frame count')
    if type(report.get('frames_requested')) is not int or report['frames_requested'] != frames:
        raise ValueError('host requested frame count differs')
    return report


def _run_once(argv, frames, runner, timeout):
    try:
        process = runner(argv, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired as error:
        return {'returncode': None, 'error': 'timeout', 'stderr': _short_text(error.stderr)}, None
    except OSError as error:
        return {'returncode': None, 'error': 'launch failed', 'stderr': _short_text(str(error))}, None
    result = {'returncode': process.returncode, 'stderr': _short_text(process.stderr)}
    if process.returncode:
        result['error'] = 'host failed'
        return result, None
    try:
        report = _parse_report(process.stdout, frames)
        canonical = _canonical(report)
    except (ValueError, TypeError, OverflowError):
        result['error'] = 'invalid checkpoint JSON'
        return result, None
    result.update({key: report[key] for key in (
        'state_hash', 'logic_state_hash', 'state_hash_schema',
        'state_hash_bytes', 'logic_state_bytes', 'frames', 'frames_requested')})
    result['report_sha256'] = hashlib.sha256(canonical).hexdigest()
    return result, canonical


def run_quest(exe, data, replay, quest, frames=540, seed=42,
              language='English.lproj', dpi=1, *, runner=None, timeout=120):
    runner = runner or subprocess.run
    argv = [str(exe), '--data', str(data), '--quest', str(quest), '--frames', str(frames),
            '--headless', '--replay', str(replay), '--seed', str(seed),
            '--language', language, '--dpi', str(dpi)]
    runs = [_run_once(argv, frames, runner, timeout) for _ in range(2)]
    records = [record for record, _ in runs]
    if any(report is None for _, report in runs):
        status = 'failed'
    elif runs[0][1] != runs[1][1]:
        status = 'mismatch'
    elif records[0]['frames'] < frames:
        status = 'held'
    else:
        status = 'matched'
    return {'quest': quest, 'status': status, 'runs': records}


def collect_goldens(exe, data, replay, frames=540, seed=42, language='English.lproj',
                    dpi=1, quests=None, *, runner=None, timeout=120):
    if (not 1 <= frames <= 100000 or not -(1 << 31) <= seed < (1 << 31)
            or not math.isfinite(timeout) or timeout <= 0):
        raise ValueError('invalid frames, seed or timeout')
    available, source = discover_quests(data, language, dpi)
    selected = available if quests is None else sorted(quests)
    if not selected or len(selected) != len(set(selected)) or not set(selected) <= set(available):
        raise ValueError('requested quests must be a nonempty unique subset of the table IDs')
    replay_hash = sha256_file(replay)
    executable_hash = sha256_file(exe)
    results = [run_quest(exe, data, replay, ident, frames, seed, language, dpi,
                         runner=runner, timeout=timeout) for ident in selected]
    counts = Counter(record['status'] for record in results)
    return {'schema': 'mission-local-golden-v1',
            'scope': 'local repeated checkpoints; not original runtime verification',
            'frames_requested': frames, 'seed': seed, 'language': language, 'dpi': dpi,
            'timeout_seconds': timeout, 'replay_sha256': replay_hash,
            'executable_sha256': executable_hash, 'source': source,
            'available_quests': available, 'quests': selected,
            'counts': {status: counts[status] for status in ('matched', 'held', 'failed', 'mismatch')},
            'results': results}


def main(argv=None, *, runner=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--data', type=Path, required=True)
    parser.add_argument('--replay', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--frames', type=int, default=540)
    parser.add_argument('--seed', type=int, default=42)
    parser.add_argument('--language', default='English.lproj')
    parser.add_argument('--dpi', type=int, default=1)
    parser.add_argument('--quests', help='comma-separated subset of real quest IDs')
    parser.add_argument('--timeout', type=float, default=120)
    args = parser.parse_args(argv)
    try:
        if args.out.exists():
            raise ValueError('refusing to overwrite golden output')
        quests = None if args.quests is None else [_integer(token) for token in args.quests.split(',')]
        report = collect_goldens(args.exe, args.data, args.replay, args.frames, args.seed,
                                 args.language, args.dpi, quests, runner=runner, timeout=args.timeout)
        args.out.parent.mkdir(parents=True, exist_ok=True)
        with args.out.open('x', encoding='utf-8', newline='\n') as stream:
            json.dump(report, stream, ensure_ascii=True, indent=2, allow_nan=False)
            stream.write('\n')
    except (OSError, ValueError) as error:
        parser.error(str(error))
    print(json.dumps(report['counts'], sort_keys=True))
    return 1 if report['counts']['failed'] or report['counts']['mismatch'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
