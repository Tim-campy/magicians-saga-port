"""Synthetic multi-resource queues -> shared backend -> SDL pixels."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

EXE = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None
VIEWER = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None
sys.path.insert(0, str(Path(__file__).resolve().parent))
from test_sprite_preview import png, seb, jar, bmp_pixels


def command(resource, clip, x, y, depth=0, texture=-1, image_resource=-1):
    cells = [texture, resource, depth, clip, 0, x, y, -1, -1, -1, -1, -1, -1, 0, 255, -1, 0, image_resource, 0]
    return 'draw ' + ' '.join(map(str, cells)) + '\n'


@unittest.skipUnless(EXE and VIEWER, 'viewer executables supplied by CTest')
class QueuePixels(unittest.TestCase):
    def make_resources(self, root):
        a, b, c = root / 'first.bin', root / 'second.bin', root / 'global.bin'
        a.write_bytes(jar([('img.inf', b'2\ta.png,bin'), ('seb.inf', b'3\ta.seb'),
            ('a.seb', seb([(0, 2, 0, 0, 2, 2, 0, 0, 0, 0)])),
            ('a.png', png(4, 4, [255, 0, 0, 255] * 16))]))
        colors = [(0, 100 + x * 15, y * 40, (0, 128, 255)[x % 3]) for y in range(4) for x in range(8)]
        opt = struct.pack('>5B7h', 8, 4, 1, 1, 1, -1, 1, 1, 4, 1, 2, 2)
        b.write_bytes(jar([('img.inf', b'2\tb.png,bin,q+1'), ('seb.inf', b'5\tb.seb'),
            ('b.seb', seb([(0, 2, 0, 0, 8, 4, 0, 0, 0, 0)])),
            ('b.png', png(8, 4, [v for pixel in colors for v in pixel])), ('b.opt', opt)]))
        c.write_bytes(jar([('img.inf', b'2\tc.png,bin'), ('seb.inf', b'0\tc.seb'),
            ('c.seb', seb([(0, 2, 0, 0, 2, 2, 0, 0, 0, 0)])), ('c.png', png(2, 2, [0, 255, 255, 255] * 4))]))
        return a, b, c

    def run_capture(self, root, text, name='combined.bmp', native=False):
        scene, output = root / 'scene.txt', root / name
        scene.write_text(text, encoding='utf-8')
        args = [str(EXE), str(scene), '32', '16', str(output)]
        if native:
            args.append('--native-depth')
        result = subprocess.run(args, capture_output=True, text=True, timeout=30)
        return result, output

    def test_native_depth_overlap_and_explicit_material_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.make_resources(root)
            prefix = ('world 1 0 0 0 1 0 0 0 1\n'
                      'projection 0.0625 0 0 0 0 0.125 0 0 0 0 0.00000133324445 0 -1 -1 -1 1\n'
                      'gpu_depth 24 1\n'
                      'resource 1 "first.bin" "English.lproj" 0\n'
                      'resource 3 "global.bin" "English.lproj" 0\n')
            bindings = 'material 0 65808 2000 10 0\nmaterial 1 65808 2000 11 0\n'
            for depth, expected in [(0, (255, 0, 0, 255)), (150000, (0, 255, 255, 255))]:
                result, output = self.run_capture(root, prefix + bindings + command(1, 3, 2, 2, 100) + command(3, 0, 2, 2, depth), f'native-{depth}.bmp', native=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(bmp_pixels(output.read_bytes())[2][3 * 32 + 3], expected)
                self.assertIn('native depth', json.loads(result.stdout)['profile'])
            result, output = self.run_capture(root, prefix + command(1, 3, 2, 2), 'unbound.bmp', native=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('unbound material:', result.stderr)
            self.assertFalse(output.exists())

    def test_different_sizes_alpha_opt_and_same_image_ids(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            a, b, _ = self.make_resources(root)
            isolated = []
            for archive, clip, x in [(a, 3, 2), (b, 5, 12)]:
                output = root / f'single-{clip}.bmp'
                result = subprocess.run([str(VIEWER), str(archive), '--resource', str(clip), '0', '0',
                    '--language', 'English.lproj', '--dpi', '0', '--canvas', '32', '16', '--origin', str(x), '2',
                    '--headless', '--capture', str(output)], capture_output=True, text=True, timeout=30)
                self.assertEqual(result.returncode, 0, result.stderr)
                isolated.append(bmp_pixels(output.read_bytes())[2])
            text = ('world 1 0 0 0 1 0 0 0 1\n'
                    'resource 1 "first.bin" "English.lproj" 0\nresource 3 "second.bin" "English.lproj" 0\n'
                    + command(1, 3, 2, 2, 150007) + command(3, 5, 12, 2, 300009))
            result, output = self.run_capture(root, text)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual((report['commands'], report['textures'], report['quads']), (2, 2, 3))
            expected = [left if left != (0, 0, 0, 0) else right for left, right in zip(*isolated)]
            self.assertEqual(bmp_pixels(output.read_bytes())[2], expected)

    def test_global_appdata_image_override_and_world_translation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.make_resources(root)
            text = ('world 1 0 0 0 1 0 3 0 1\n'
                    'resource 1 "first.bin" "English.lproj" 0\nglobal 1 "global.bin" "English.lproj" 0\n'
                    + command(1, 3, 2, 2, texture=2, image_resource=1))
            result, output = self.run_capture(root, text)
            self.assertEqual(result.returncode, 0, result.stderr)
            pixels = bmp_pixels(output.read_bytes())[2]
            self.assertEqual(pixels[2 * 32 + 5], (0, 255, 255, 255))
            self.assertEqual(pixels[2 * 32 + 2], (0, 0, 0, 0))
            output.write_bytes(b'preserved')
            result, _ = self.run_capture(root, text)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output.read_bytes(), b'preserved')

    def test_explicit_inputs_are_required(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for text in ['# no inferred world\n', 'world 1 0 0 0 1 0 0 0 1 extra\n',
                         'resource 0 "missing.bin" "English.lproj" 0\n']:
                result, output = self.run_capture(root, text)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(output.exists())


if __name__ == '__main__':
    unittest.main()
