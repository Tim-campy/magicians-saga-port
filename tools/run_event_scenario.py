"""Run native event kernels on a user-supplied extracted table.

Inject one script or perform one automatic selection scan with explicit decoded state.
Does not run a game frame, create UI, or infer new-game/save/billing state.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def fixture_tokens(state):
    """Encode explicit decoded values; omitted keys remain absent in the native host."""
    if not isinstance(state, dict):
        raise ValueError('state must be an object')
    result = []
    booleans = {'context_present', 'normal_save_present', 'print_enabled', 'debug_enabled'}
    scalars = {'context_character_count', 'debug_rescue_count', 'hero_level',
               'high_score', 'town_capacity', 'quest_data_count'}
    def number(value):
        if type(value) is not int:
            raise ValueError('expected integer')
        return str(value)
    def flag(value):
        if type(value) is not bool:
            raise ValueError('expected boolean')
        return '1' if value else '0'
    for key, value in state.items():
        result.append(key)
        if key in booleans:
            result.append(flag(value))
        elif key in scalars:
            result.append(number(value))
        elif key in {'areas', 'quests', 'characters', 'town_flags', 'buildings', 'normal_save_strings', 'execution_counts'}:
            if not isinstance(value, list):
                raise ValueError(f'{key} must be an array')
            result.append(str(len(value)))
            for entry in value:
                if key == 'normal_save_strings':
                    if entry is not None and not isinstance(entry, str):
                        raise ValueError('expected string or null')
                    result.append('null' if entry is None else 'x' + entry.encode('utf-8').hex())
                    continue
                if key == 'execution_counts':
                    if not isinstance(entry, list) or len(entry) != 2:
                        raise ValueError('execution counter needs [id, count]')
                    result.extend(map(number, entry))
                    continue
                result.append('0' if entry is None else '1')
                if entry is None:
                    continue
                if key == 'town_flags':
                    result.append(number(entry))
                    continue
                if not isinstance(entry, list):
                    raise ValueError(f'{key} entry must be an array or null')
                expected = {'areas': 2, 'quests': 5, 'characters': 2}.get(key)
                if expected is not None and len(entry) != expected:
                    raise ValueError(f'{key} entry needs {expected} values')
                if key == 'areas':
                    result.extend([number(entry[0]), flag(entry[1])])
                elif key == 'quests':
                    result.extend([number(entry[0]), flag(entry[1]), number(entry[2]), flag(entry[3])])
                    record = entry[4]
                    result.append('0' if record is None else '1')
                    if record is not None:
                        if not isinstance(record, list) or len(record) != 3:
                            raise ValueError('quest record needs [challenges, clears, rescued]')
                        result.extend(map(number, record))
                elif key == 'characters':
                    result.extend(map(number, entry))
                else:
                    result.append(str(len(entry)))
                    result.extend(map(number, entry))
        else:
            raise ValueError(f'unknown state field: {key}')
    return result + ['end']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('table', type=Path)
    p.add_argument('--exe', type=Path, required=True)
    mode = p.add_mutually_exclusive_group(required=True)
    mode.add_argument('--id', type=int, help='inject one event script by record ID')
    mode.add_argument('--auto', action='store_true', help='perform one automatic selection scan')
    p.add_argument('--state', type=Path, help='JSON with explicit decoded condition/execution state')
    p.add_argument('--passes', type=int, required=True, help='reached delay queue passes, not frames')
    for name in ('money', 'crystals', 'crystal-mode', 'stamina', 'item-state', 'talk-character'):
        p.add_argument('--' + name, type=int, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    raw = a.table.read_bytes()
    raw.decode('utf-8-sig')  # Strict validation at the tool boundary.
    if (a.id is not None and not 0 <= a.id <= 999) or not 0 <= a.passes <= 100000:
        p.error('invalid id or pass count')
    initial = dict(money=a.money, crystals=a.crystals, crystal_mode=a.crystal_mode,
                   stamina=a.stamina, item_state=a.item_state, talk_character=a.talk_character)
    state = json.loads(a.state.read_text(encoding='utf-8')) if a.state else {}
    try:
        fixture = fixture_tokens(state)
    except ValueError as error:
        p.error(str(error))
    tokens = ['SAGA_EVENT_TRACE_3', 'auto' if a.auto else 'inject', str(a.id or 0), str(a.passes)]
    tokens += [str(v) for v in initial.values()]
    tokens.append(raw.hex())  # Preserve bytes through text-mode stdin on Windows.
    tokens.extend(fixture)
    result = subprocess.run([str(a.exe.resolve())], input=' '.join(tokens), text=True,
                            capture_output=True, timeout=60)
    report = {'table_sha256': hashlib.sha256(raw).hexdigest(),
              'native_exe_sha256': hashlib.sha256(a.exe.read_bytes()).hexdigest(),
              'record_id': a.id, 'native_table_loader': True,
              'initial_fixture': initial, 'requested_queue_passes': a.passes,
              'conditions_evaluated': a.auto, 'selection_scans': int(a.auto), 'decoded_state': state, 'ui_executed': False,
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
