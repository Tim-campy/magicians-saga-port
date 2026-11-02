"""Synthetic archives and mocked host calls; never launches the game."""
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    'mission_golden', Path(__file__).parents[1] / 'tools/mission_golden.py')
golden = importlib.util.module_from_spec(spec)
spec.loader.exec_module(golden)


def synthetic_jar(members):
    names = [name.encode('ascii') for name, _ in members]
    offsets, cursor = [], 0
    for _, payload in members:
        offsets.append(cursor)
        cursor += 4 + len(payload)
    index = (struct.pack('>I', len(members))
             + b''.join(struct.pack('>I', len(name)) + name for name in names)
             + b''.join(struct.pack('>I', value) for value in offsets)
             + b''.join(struct.pack('>I', len(payload)) for _, payload in members)
             + bytes(len(members)))
    body = b''.join(struct.pack('>I', len(payload)) + payload for _, payload in members)
    return struct.pack('>II', len(index) + 4, len(body)) + index + body


def report(frames=540, **extra):
    value = {'state_hash': '0123456789abcdef', 'logic_state_hash': 'fedcba9876543210',
             'state_hash_schema': 'synthetic-v1', 'state_hash_bytes': 800,
             'logic_state_bytes': 600, 'frames': frames, 'frames_requested': 540,
             'nested': {'values': [1, None, False]}}
    value.update(extra)
    return value


def completed(value=None, code=0, stderr=''):
    stdout = json.dumps(report() if value is None else value)
    return subprocess.CompletedProcess([], code, stdout, stderr)


class QueueRunner:
    def __init__(self, values):
        self.values = iter(values)
        self.calls = []

    def __call__(self, argv, **options):
        self.calls.append((argv[:], options))
        value = next(self.values)
        if isinstance(value, Exception):
            raise value
        return value


class GoldenTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.data = self.root / 'data with spaces'
        (self.data / 'textassets').mkdir(parents=True)
        self.exe = self.root / 'synthetic executable'
        self.exe.write_bytes(b'synthetic executable identity; never executed')
        self.replay = self.root / 'inputs.replay'
        self.replay.write_bytes(b'3 keys 0\n')
        self.table = b'\xef\xbb\xbf0\tprivate synthetic row\n4\tsecond row\r\n9\tthird row\n'
        self.members = [('English.lproj/Quest.txt', self.table),
                        ('Japanese.lproj/Quest.txt', b'0\tother language\n')]
        self.write_archive()
        self.index = self.data / 'mission-resources-v2.tsv'
        self.index.write_text('mission_resources\t2\ntables\t-1\t"textassets/tables.bin"\t"xls"\n'
                              'resource\t2\t"textassets/common.bin"\t"com"\n', encoding='utf-8')

    def write_archive(self):
        (self.data / 'textassets/tables.bin').write_bytes(synthetic_jar(self.members))

    def run_quest(self, responses, **options):
        runner = QueueRunner(responses)
        result = golden.run_quest(self.exe, self.data, self.replay, 4, runner=runner, **options)
        self.assertEqual(len(runner.calls), 2)
        return result, runner

    def test_real_ids_projection_preserves_gaps_and_source_hashes(self):
        ids, source = golden.discover_quests(self.data)
        self.assertEqual(ids, [0, 4, 9])
        self.assertEqual(source['quest_table']['member'], 'English.lproj/Quest.txt')
        self.assertEqual(source['quest_table']['sha256'], hashlib.sha256(self.table).hexdigest())
        self.assertEqual(source['index']['sha256'], golden.sha256_file(self.index))
        self.assertEqual(source['tables_archive']['sha256'],
                         golden.sha256_file(self.data / 'textassets/tables.bin'))
        self.assertEqual(golden.discover_quests(self.data, 'japanese.lproj')[0], [0])

    def test_v2_precedence_and_v1_fallback(self):
        legacy = self.data / 'mission-resources.tsv'
        legacy.write_text('invalid old index', encoding='utf-8')
        self.assertEqual(golden.read_index(self.data)[1]['version'], 2)
        self.index.unlink()
        legacy.write_text('mission_resources 1\ntables -1 "textassets/tables.bin" "xls"\n', encoding='utf-8')
        self.assertEqual(golden.read_index(self.data)[1]['version'], 1)
        self.index.write_text('invalid v2', encoding='utf-8')
        with self.assertRaises(ValueError):
            golden.read_index(self.data)

    def test_strict_index_and_path_boundaries(self):
        invalid = [
            'mission_resources 3\ntables -1 textassets/tables.bin xls\n',
            'mission_resources 2 extra\ntables -1 textassets/tables.bin xls\n',
            'mission_resources 2\n',
            'mission_resources 2\ntables -1 textassets/tables.bin xls\ntables -1 textassets/tables.bin xls\n',
            'mission_resources 2\ntables 0 textassets/tables.bin xls\n',
            'mission_resources 2\ntables -1 textassets/tables.bin other\n',
            'mission_resources 2\ntables -1 textassets/../tables.bin xls\n',
            'mission_resources 2\ntables -1 /textassets/tables.bin xls\n',
            'mission_resources 2\ntables -1 textassets/sub/tables.bin xls\n',
            'mission_resources 2\ntables -1 textassets/tables.bin xls extra\n',
            'mission_resources 2\ntables -1 "unterminated\n',
        ]
        for text in invalid:
            with self.subTest(text=text):
                self.index.write_text(text, encoding='utf-8')
                with self.assertRaises(ValueError):
                    golden.read_index(self.data)

    def test_member_selection_rejects_ambiguity_and_case_collisions(self):
        for candidate in ('Quest.txt', 'drawable-hdpi/English.lproj/Quest.txt', 'drawable-mdpi/Quest.txt',
                          'english.lproj/quest.txt'):
            with self.subTest(candidate=candidate):
                self.members.append((candidate, b'3\trow'))
                self.write_archive()
                with self.assertRaises(ValueError):
                    golden.discover_quests(self.data)
                self.members.pop()
        self.members = [('English.lproj/Other.txt', b'0\trow')]
        self.write_archive()
        with self.assertRaisesRegex(ValueError, 'missing'):
            golden.discover_quests(self.data)

    def test_malformed_ids_fail_without_reconstructing_table_fields(self):
        for payload in (b'', b'-1\tx', b'0\tx\n0\ty', b'x\ty', b'2.0\ty', b'1', b'2147483647\tx'):
            with self.subTest(payload=payload):
                with self.assertRaises(ValueError):
                    golden.quest_ids(payload)
        self.assertEqual(golden.quest_ids(b' +9 \tx\n\n0\tx'), [0, 9])

    def test_matching_full_json_and_argv_tokens(self):
        result, runner = self.run_quest([completed(), completed()])
        self.assertEqual(result['status'], 'matched')
        self.assertEqual(result['runs'][0]['frames'], 540)
        self.assertEqual(result['runs'][0]['report_sha256'], result['runs'][1]['report_sha256'])
        self.assertEqual(runner.calls[0][0], [str(self.exe), '--data', str(self.data), '--quest', '4',
            '--frames', '540', '--headless', '--replay', str(self.replay), '--seed', '42',
            '--language', 'English.lproj', '--dpi', '1'])
        self.assertEqual(runner.calls[0][1], {'capture_output': True, 'text': True, 'timeout': 120})
        self.assertNotIn('nested', result['runs'][0])
        # Object key order is not report state; parsed values are compared.
        reversed_keys = dict(reversed(list(report().items())))
        result, _ = self.run_quest([completed(), completed(reversed_keys)])
        self.assertEqual(result['status'], 'matched')

    def test_full_report_changes_are_mismatches_even_when_hash_fields_match(self):
        result, _ = self.run_quest([completed(), completed(report(nested={'values': [1, None, True]}))])
        self.assertEqual(result['status'], 'mismatch')
        for change in ({'state_hash': 'different'}, {'logic_state_hash': 'different'},
                       {'state_hash_schema': 'different'}, {'state_hash_bytes': 801}, {'frames': 539}):
            with self.subTest(change=change):
                result, _ = self.run_quest([completed(), completed(report(**change))])
                self.assertEqual(result['status'], 'mismatch')

    def test_early_hold_is_explicit_and_not_complete(self):
        result, _ = self.run_quest([completed(report(12)), completed(report(12))])
        self.assertEqual(result['status'], 'held')
        self.assertEqual([run['frames'] for run in result['runs']], [12, 12])
        self.assertEqual([run['frames_requested'] for run in result['runs']], [540, 540])

    def test_failures_timeouts_invalid_json_and_stderr_bound(self):
        failures = [completed(code=3, stderr='\u00e9' * 2000),
                    subprocess.TimeoutExpired('synthetic', 120, stderr=b'timed out'),
                    FileNotFoundError('missing synthetic executable'),
                    subprocess.CompletedProcess([], 0, '{', ''),
                    completed({'frames': 540}), completed(report(frames_requested=1)),
                    completed(report(541)), completed(report(logic_state_bytes=801)),
                    completed(report(frames=True))]
        for failure in failures:
            with self.subTest(failure=failure):
                result, _ = self.run_quest([failure, completed()])
                self.assertEqual(result['status'], 'failed')
                self.assertIn('error', result['runs'][0])
                self.assertLessEqual(len(result['runs'][0]['stderr'].encode('utf-8')), 1024)

    def test_collection_defaults_subset_and_output_identity(self):
        runner = QueueRunner([completed()] * 6)
        summary = golden.collect_goldens(self.exe, self.data, self.replay, runner=runner)
        self.assertEqual(summary['quests'], [0, 4, 9])
        self.assertEqual(summary['counts'], {'matched': 3, 'held': 0, 'failed': 0, 'mismatch': 0})
        self.assertEqual(summary['replay_sha256'], golden.sha256_file(self.replay))
        self.assertEqual(summary['executable_sha256'], golden.sha256_file(self.exe))
        self.assertNotIn('private synthetic row', json.dumps(summary))
        self.assertEqual([call[0][4] for call in runner.calls], ['0', '0', '4', '4', '9', '9'])
        for invalid in ([], [4, 4], [3], [-1]):
            with self.assertRaises(ValueError):
                golden.collect_goldens(self.exe, self.data, self.replay, quests=invalid, runner=runner)

    def test_cli_refuses_overwrite_before_running_and_returns_failure_status(self):
        output = self.root / 'golden.json'
        args = ['--exe', str(self.exe), '--data', str(self.data), '--replay', str(self.replay),
                '--out', str(output), '--quests', '9,4']
        runner = QueueRunner([completed(report(12)), completed(report(12)), completed(code=1), completed()])
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(golden.main(args, runner=runner), 1)
        summary = json.loads(output.read_text(encoding='utf-8'))
        self.assertEqual(summary['quests'], [4, 9])
        self.assertEqual(summary['counts'], {'matched': 0, 'held': 1, 'failed': 1, 'mismatch': 0})
        original = output.read_bytes()
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            golden.main(args, runner=runner)
        self.assertEqual(len(runner.calls), 4)
        self.assertEqual(output.read_bytes(), original)
        output.unlink()
        runner = QueueRunner([completed(report(12)), completed(report(12)), completed(), completed()])
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(golden.main(args, runner=runner), 0)


if __name__ == '__main__':
    unittest.main()
