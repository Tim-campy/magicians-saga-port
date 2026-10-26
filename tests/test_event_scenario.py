"""Synthetic fixture validation and optional native scenario integration."""
import json
from pathlib import Path
import subprocess
import sys
import unittest
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from run_event_scenario import fixture_tokens
EXE = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None

class FixtureTests(unittest.TestCase):
    def test_absence_and_types(self):
        self.assertEqual(fixture_tokens({}), ['end'])
        self.assertEqual(fixture_tokens({'sys_save_strings': [None, '']}), ['sys_save_strings', '2', 'null', 'x', 'end'])
        for fixture in ({'typo': 1}, {'print_enabled': 0}, {'hero_level': True}, {'quests': [[0, False]]}):
            with self.assertRaises(ValueError):
                fixture_tokens(fixture)

@unittest.skipUnless(EXE, 'native integration is registered separately in CTest')
class NativeTests(unittest.TestCase):
    def run_scenario(self, table, state, passes=0):
        tokens = ['SAGA_EVENT_TRACE_3', 'auto', '0', str(passes), '0', '0', '10000', '3', '0', '2', table.encode().hex()]
        result = subprocess.run([str(EXE)], input=' '.join(tokens + fixture_tokens(state)), text=True, capture_output=True, timeout=20)
        return result, [json.loads(line) for line in result.stdout.splitlines()]

    def test_discovery_delay_and_counters(self):
        table = ('0\ta\t0\t1\t3\t1\t0\t4\t2\t2\t4\t0\t2\t1\t5\n'
                 '2\tb\t0\t2\t2\t4\t0\t3\t1\t5\t4\t2\t2\t10\t1\t2\t1\t7\n'
                 '3\tc\t0\t0\t0')
        state = {'context_present': True, 'context_character_count': 1,
                 'sys_save_present': False, 'areas': [[0, False]]}
        result, trace = self.run_scenario(table, state, 1)
        self.assertEqual(result.returncode, 0, result.stderr)
        before = next(x for x in trace if x.get('pass') == 0)
        self.assertEqual(before['money'], 5)
        self.assertEqual(before['pending'], 1)
        self.assertEqual(before['execution_counts'], [1, None, 1, 0])
        self.assertEqual(trace[-1]['money'], 12)
        self.assertEqual(trace[-1]['pending'], 0)

    def test_missing_context_fails(self):
        result, _ = self.run_scenario('0\ta\t0\t1\t3\t1\t0\t4\t0', {})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('context', result.stderr)

    def test_form_stops_scan_and_existing_counter_skips(self):
        table = ('0\ta\t0\t1\t3\t1\t0\t4\t1\t2\t1\t50\n'
                 '1\tb\t0\t1\t3\t1\t0\t4\t1\t2\t8\t7\n'
                 '2\tc\t0\t1\t3\t1\t0\t4\t1\t2\t1\t99')
        result, trace = self.run_scenario(table, {'context_present': True, 'context_character_count': 1,
            'sys_save_present': True, 'sys_save_strings': ['example'], 'execution_counts': [[0, 1]]})
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn({'selection_returned_form': True}, trace)
        self.assertEqual(trace[-1]['money'], 0)
        self.assertEqual(trace[-1]['execution_counts'], [1, 1, 0])

if __name__ == '__main__':
    unittest.main()
