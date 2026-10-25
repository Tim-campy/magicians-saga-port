"""Run native event kernels on a user-supplied extracted table.

Explicit injection of one catalog entry; does not evaluate event conditions,
run a game frame, create UI, or infer a new-game/save/billing state.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from inspect_event_table import parse_row


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('table', type=Path)
    p.add_argument('--exe', type=Path, required=True)
    p.add_argument('--index', type=int, required=True, help='catalog position, not record ID')
    p.add_argument('--passes', type=int, required=True, help='reached delay queue passes, not frames')
    for name in ('money', 'crystals', 'crystal-mode', 'stamina', 'item-state', 'talk-character'):
        p.add_argument('--' + name, type=int, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    raw = a.table.read_bytes()
    records = [parse_row(line)[0] for line in raw.decode('utf-8-sig').split('\n') if line]
    if not 0 <= a.index < len(records) or not 0 <= a.passes <= 100000:
        p.error('invalid index or pass count')
    initial = dict(money=a.money, crystals=a.crystals, crystal_mode=a.crystal_mode,
                   stamina=a.stamina, item_state=a.item_state, talk_character=a.talk_character)
    tokens = ['SAGA_EVENT_TRACE_1', str(a.index), str(a.passes)]
    tokens += [str(v) for v in initial.values()]
    tokens.append(str(len(records)))
    for record in records:
        script = record['execs']
        tokens.append(str(-1 if script is None else len(script)))
        for row in script or []:
            tokens.append(str(len(row)))
            tokens.extend(map(str, row))
    result = subprocess.run([str(a.exe.resolve())], input=' '.join(tokens), text=True,
                            capture_output=True, timeout=60)
    report = {'table_sha256': hashlib.sha256(raw).hexdigest(),
              'native_exe_sha256': hashlib.sha256(a.exe.read_bytes()).hexdigest(),
              'catalog_index': a.index, 'record_id': records[a.index]['id'],
              'initial_fixture': initial, 'requested_queue_passes': a.passes,
              'conditions_evaluated': False, 'ui_executed': False,
              'original_apk_executed': False, 'exit_code': result.returncode,
              'trace': [json.loads(line) for line in result.stdout.splitlines()],
              'stderr': result.stderr}
    a.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    if result.returncode:
        raise SystemExit(f'native trace failed; details in {a.output}')
    print(json.dumps({'record_id': report['record_id'], 'last': report['trace'][-1],
                      'report': str(a.output)}))


if __name__ == '__main__':
    main()
