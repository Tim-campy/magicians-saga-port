"""Synthetic SEB -> actual effect factory -> live animation scenario."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

EXE = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None


def empty_seb(maximum, mode):
    return (b'\xff' if mode else b'') + struct.pack('>hh', 0, maximum)


class FixtureTests(unittest.TestCase):
    def test_wire_header(self):
        self.assertEqual(empty_seb(3, 0), b'\0\0\0\3')
        self.assertEqual(empty_seb(0, 1), b'\xff\0\0\0\0')


@unittest.skipUnless(EXE, 'native integration is registered separately in CTest')
class NativeTests(unittest.TestCase):
    def scenario(self, blob, passes=5, application=1, automatic=0):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'animation.seb'
            path.write_bytes(blob)
            result = subprocess.run([str(EXE), str(path), '7', str(passes), str(application), str(automatic)],
                                    text=True, capture_output=True, timeout=20)
        return result, [json.loads(line) for line in result.stdout.splitlines()]

    def test_header_drives_effect_duration_and_both_clocks(self):
        for mode in (0, 1):
            result, trace = self.scenario(empty_seb(3, mode))
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(trace[0], dict(scenario='effect_animation', resource=20, clip=7,
                                           max_frame=3, effect_lifetime=2, entity_id=0, group_size=1))
            self.assertEqual([x['entity_frame'] for x in trace[1:]], [0, 1, 2, 0, 1, 2])
            self.assertEqual([x['shared_frame'] for x in trace[1:]], [0, 1, 2, 0, 1, 2])

    def test_effect_lifetime_zero_and_independent_gates(self):
        result, trace = self.scenario(empty_seb(0, 0), passes=3)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(trace[0]['effect_lifetime'], -1)
        self.assertEqual([x['entity_frame'] for x in trace[1:]], [0, 1, 2, 3])
        for application, automatic in ((0, 0), (1, 1)):
            result, trace = self.scenario(empty_seb(3, 0), application=application, automatic=automatic)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual([x['shared_frame'] for x in trace[1:]], [0] * 6)
            self.assertEqual([x['entity_frame'] for x in trace[1:]], [0, 1, 2, 0, 1, 2])

    def test_truncation_rejected_before_entity_creation(self):
        result, trace = self.scenario(b'\0\0\0')
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(trace)


if __name__ == '__main__':
    unittest.main()
