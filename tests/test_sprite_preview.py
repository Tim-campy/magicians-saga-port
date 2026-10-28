"""Synthetic file -> SEB -> command consumer -> OPT -> UV -> SDL pixel checks."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

EXE = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None


def png(width, height, rgba):
    def chunk(kind, value):
        return struct.pack('>I', len(value)) + kind + value + struct.pack('>I', zlib.crc32(kind + value))
    rows = b''.join(b'\0' + bytes(rgba[y * width * 4:(y + 1) * width * 4]) for y in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))


def seb(records):
    return struct.pack('>hhhh', 1, 4, len(records), 0) + b''.join(struct.pack('>10h', *record) for record in records)


def bmp_pixels(blob):
    offset = struct.unpack_from('<I', blob, 10)[0]
    width, height = struct.unpack_from('<ii', blob, 18)
    bits = struct.unpack_from('<H', blob, 28)[0]
    if bits != 32:
        raise AssertionError('SDL capture must preserve RGBA32')
    stride = width * 4
    rows = [blob[offset + y * stride:offset + (y + 1) * stride] for y in range(abs(height))]
    if height > 0:
        rows.reverse()
    return width, abs(height), [tuple(row[i + n] for n in (2, 1, 0, 3)) for row in rows for i in range(0, stride, 4)]


class FixtureTests(unittest.TestCase):
    def test_png_and_seb_framing(self):
        self.assertEqual(png(1, 1, [1, 2, 3, 4])[:8], b'\x89PNG\r\n\x1a\n')
        self.assertEqual(len(seb([(0,) * 10])), 28)


@unittest.skipUnless(EXE, 'SDL integration is registered separately in CTest')
class NativeTests(unittest.TestCase):
    source = [(x * 30 + 1, y * 30 + 2, x + y + 3, (x + y) * 17) for y in range(8) for x in range(8)]

    def preview(self, records, opt=None, frame=0, slot=0, extra=()):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image, animation, capture = root / 'texture.png', root / 'clip.seb', root / 'frame.bmp'
            image.write_bytes(png(8, 8, [v for pixel in self.source for v in pixel]))
            animation.write_bytes(seb(records))
            args = [str(EXE), str(image), '--sprite', str(animation), str(frame), '0', str(slot),
                    '--canvas', '12', '12', '--origin', '3', '2', '--headless', '--capture', str(capture)]
            if opt is not None:
                grid = root / 'grid.opt'
                grid.write_bytes(opt)
                args += ['--opt', str(grid)]
            result = subprocess.run(args + list(extra), text=True, capture_output=True, timeout=20)
            pixels = bmp_pixels(capture.read_bytes()) if capture.exists() else None
        return result, pixels

    def test_direct_crop_keeps_orientation_and_rgba(self):
        result, bitmap = self.preview([(0, 0, 2, 3, 3, 2, 1, 2, 0, 0)])
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report['quad_count'], 1)
        width, height, pixels = bitmap
        self.assertEqual((width, height), (12, 12))
        expected = [(0, 0, 0, 0)] * 144
        for y in range(2):
            for x in range(3):
                expected[(4 + y) * 12 + 4 + x] = self.source[(3 + y) * 8 + 2 + x]
        self.assertEqual(pixels, expected)

    def test_opt_offsets_and_external_slot_guard(self):
        opt = struct.pack('>5B7h', 8, 8, 1, 1, 1, -1, 1, 2, 4, 5, 2, 2)
        result, bitmap = self.preview([(0, 0, 0, 0, 8, 8, 0, 0, 0, 0)], opt)
        self.assertEqual(result.returncode, 0, result.stderr)
        expected = [(0, 0, 0, 0)] * 144
        for y in range(2):
            for x in range(2):
                expected[(4 + y) * 12 + 4 + x] = self.source[(5 + y) * 8 + 4 + x]
        self.assertEqual(bitmap[2], expected)
        external = struct.pack('>5B7h', 8, 8, 1, 1, 1, 0, 0, 0, 0, 0, 2, 2)
        result, bitmap = self.preview([(0, 0, 0, 0, 8, 8, 0, 0, 0, 0)], external)
        self.assertNotEqual(result.returncode, 0)
        self.assertIsNone(bitmap)

    def test_seb_frame_interpolation_reaches_pixels(self):
        records = [(0, 0, 0, 0, 2, 2, 0, 0, 0, 0), (2, 0, 4, 4, 2, 2, 4, 4, 0, 0)]
        result, bitmap = self.preview(records, frame=1)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)['sprite'][6:8], [2, 2])
        # Interpolation changes XY; source crop is retained from the left record.
        self.assertEqual(bitmap[2][4 * 12 + 5], self.source[0])
        self.assertEqual(bitmap[2][5 * 12 + 6], self.source[9])

    def test_negative_source_clips_sampling_without_moving_destination(self):
        result, bitmap = self.preview([(0, 0, -1, -1, 3, 3, 0, 0, 0, 0)])
        self.assertEqual(result.returncode, 0, result.stderr)
        expected = [(0, 0, 0, 0)] * 144
        # The two-texel clipped source still fills the original three-pixel quad.
        for y, source_y in enumerate((0, 0, 1)):
            for x, source_x in enumerate((0, 0, 1)):
                expected[(2 + y) * 12 + 3 + x] = self.source[source_y * 8 + source_x]
        self.assertEqual(bitmap[2], expected)

    def test_empty_outside_crop_and_unknown_matrix_are_explicit(self):
        result, bitmap = self.preview([(0, 0, 20, 20, 2, 2, 0, 0, 0, 0)])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)['quad_count'], 0)
        self.assertEqual(bitmap[2], [(0, 0, 0, 0)] * 144)
        result, bitmap = self.preview([(0, 0, 0, 0, 2, 2, 0, 0, 1, 0)])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('unbound graphics service', result.stderr)
        self.assertIsNone(bitmap)

    def test_mismatched_texture_and_cli_limits_fail_without_capture(self):
        result, bitmap = self.preview([(0, 1, 0, 0, 2, 2, 0, 0, 0, 0)])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('explicit PNG slot', result.stderr)
        self.assertIsNone(bitmap)
        result, bitmap = self.preview([(0,) * 10], extra=('--canvas', '200000', '200000'))
        self.assertNotEqual(result.returncode, 0)
        self.assertIsNone(bitmap)


if __name__ == '__main__':
    unittest.main()
