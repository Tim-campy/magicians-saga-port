"""Always-built headless host contracts on synthetic game records and SEBs."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXE = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None
sys.path.insert(0, str(Path(__file__).resolve().parent))
from mission_scene_fixture import make_table_scene


@unittest.skipUnless(EXE, 'saga_mission supplied by CTest')
class MissionProfileTests(unittest.TestCase):
    def run_host(self, scene, *extra):
        return subprocess.run([str(EXE), '--scene', str(scene), '--frames', '12', '--headless',
                               '--fps', '25', '--debug', '0', '--rng', 'system', '--seed', '42', *extra],
                              capture_output=True, text=True, timeout=30)

    def test_relative_archives_headless_passes_and_repeatability(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            # Deliberately invalid PNG bytes: the headless runtime must not invoke image decoding.
            scene = root / 'scene.txt'
            scene.write_text(make_table_scene(root, valid_png=False) + 'simulate 0\n', encoding='utf-8')
            a, b = self.run_host(scene), self.run_host(scene)
            self.assertEqual(a.returncode, 0, a.stderr)
            self.assertEqual((a.returncode, a.stdout), (b.returncode, b.stdout))
            report = json.loads(a.stdout)
            self.assertEqual(report['frames'], 12)
            self.assertEqual(report['rng_draws'], 0)
            self.assertEqual(report['positions'][0]['z'], 0)
            self.assertGreater(report['positions'][1]['z'], 0)
            self.assertEqual(report['render_commands'], 2)

    def test_explicit_contract_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scene = root / 'scene.txt'
            text = make_table_scene(root)
            scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('explicit simulate row', result.stderr)
            for bad in ['simulate -1\n', 'simulate 0\nsimulate 0\n', 'unknown 1\n']:
                scene.write_text(text + bad, encoding='utf-8')
                self.assertNotEqual(self.run_host(scene).returncode, 0)
            for extra in [('--quest', '0'), ('--replay', 'missing'), ('--seed', '9')]:
                self.assertNotEqual(self.run_host(scene, *extra).returncode, 0)
            incomplete = subprocess.run([str(EXE), '--headless'], capture_output=True, text=True, timeout=30)
            self.assertNotEqual(incomplete.returncode, 0)


if __name__ == '__main__':
    unittest.main()
