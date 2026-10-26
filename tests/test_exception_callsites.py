import sys
import subprocess
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from inspect_exception_callsites import callsites


class CallsitesTests(unittest.TestCase):
    def test_isolated_cli(self):
        script = Path(__file__).resolve().parents[1] / 'tools' / 'inspect_exception_callsites.py'
        result = subprocess.run([sys.executable, '-I', str(script), '--help'],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn('--function', result.stdout)

    def test_ranges_and_no_landing(self):
        data = bytes([255,255,1,8, 0,4,0,0, 4,8,20,3])
        rows = callsites(data, 0, 0x1000, 32)
        self.assertIsNone(rows[0]['landing_pad_va'])
        self.assertEqual(rows[1], {'start_va':0x1004,'end_exclusive':0x100c,
                                  'landing_pad_va':0x1014,'action_index':3})

    def test_fail_closed(self):
        for data in [b'', bytes([0]), bytes([255,255,2,0]),
                     bytes([255,255,1,8,0,4]), bytes([255,255,1,3,0,4,0,0]),
                     bytes([255,255,1,4,0,40,0,0]),
                     bytes([255,255,1,4,0,4,32,0]),
                     bytes([255,255,1])+b'\x80'*10,
                     bytes([255,156,127,1,0])]:
            with self.subTest(data=data), self.assertRaises(ValueError):
                callsites(data,0,0x1000,32)


if __name__ == '__main__':
    unittest.main()
