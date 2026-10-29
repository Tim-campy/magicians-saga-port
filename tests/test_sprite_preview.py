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


def jar(members):
    index = struct.pack('>I', len(members))
    for name, payload in members:
        encoded = name.encode('ascii')
        index += struct.pack('>I', len(encoded)) + encoded
    offsets, cursor = [], 0
    for name, payload in members:
        offsets.append(cursor)
        cursor += 4 + len(payload)
    index += b''.join(struct.pack('>I', n) for n in offsets)
    index += b''.join(struct.pack('>I', len(payload)) for name, payload in members) + bytes(len(members))
    body = b''.join(struct.pack('>I', len(payload)) + payload for name, payload in members)
    return struct.pack('>II', len(index) + 4, len(body)) + index + body


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

    def preview(self, records, opt=None, frame=0, slot=0, extra=(), raw=True, source=None):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            image, animation, capture = root / 'texture.png', root / 'clip.seb', root / 'frame.bmp'
            image.write_bytes(png(8, 8, [v for pixel in (self.source if source is None else source) for v in pixel]))
            animation.write_bytes(seb(records))
            args = [str(EXE), str(image), '--sprite', str(animation), str(frame), '0', str(slot),
                    '--canvas', '12', '12', '--origin', '3', '2', '--headless', '--capture', str(capture)]
            if raw:
                args += ['--raw-rgba']
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

    def test_archive_catalogs_bind_texid_locale_and_opt_to_pixels(self):
        grid = struct.pack('>5B7h', 8, 8, 1, 1, 1, -1, 1, 2, 4, 5, 2, 2)
        payload = jar([
            ('img.inf', b'7\tmissing-root.png,bin\n'),
            ('English.lproj/img.inf', b'\xef\xbb\xbf 7\t../choice.gif,bin\r\n'),
            ('seb.inf', b'3\tclip.seb\n'),
            ('clip.seb', seb([(0, 7, 0, 0, 8, 8, 0, 0, 0, 0)])),
            ('English.lproj/choice.png', png(8, 8, [c for p in self.source for c in p])),
            ('English.lproj/choice.opt', grid),
        ])
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive, capture = root / 'resources.bin', root / 'frame.bmp'
            archive.write_bytes(payload)
            args = [str(EXE), str(archive), '--resource', '3', '0', '0', '--language', 'English.lproj', '--dpi', '1',
                    '--canvas', '12', '12', '--origin', '3', '2', '--raw-rgba', '--headless', '--capture', str(capture)]
            result = subprocess.run(args, text=True, capture_output=True, timeout=20)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual(report['texture_slot'], 7)
            self.assertEqual(report['resource_clip'], 3)
            self.assertEqual(report['resource_clip_slots'], 4)
            self.assertEqual(report['resource_ready_clips'], 1)
            self.assertEqual(report['image_catalog'], 'English.lproj/img.inf')
            self.assertEqual(report['animation_member'], 'clip.seb')
            self.assertEqual(report['image_member'], 'English.lproj/choice.png')
            self.assertEqual(report['opt_member'], 'English.lproj/choice.opt')
            bitmap = bmp_pixels(capture.read_bytes())
        expected = [(0, 0, 0, 0)] * 144
        for y in range(2):
            for x in range(2):
                expected[(4 + y) * 12 + 4 + x] = self.source[(5 + y) * 8 + 4 + x]
        self.assertEqual(bitmap[2], expected)

    def test_archive_unknown_loading_options_fail_before_capture(self):
        payload = jar([('img.inf', b'7\timage.png,bin,a1'), ('seb.inf', b'0\tclip.seb'),
                       ('clip.seb', seb([(0, 7, 0, 0, 1, 1, 0, 0, 0, 0)]))])
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            archive, capture = root / 'resources.bin', root / 'frame.bmp'
            archive.write_bytes(payload)
            result = subprocess.run([str(EXE), str(archive), '--resource', '0', '0', '0', '--language', 'English.lproj',
                                     '--dpi', '0', '--canvas', '2', '2', '--headless', '--capture', str(capture)],
                                    text=True, capture_output=True, timeout=20)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('unbound image loading option', result.stderr)
            self.assertFalse(capture.exists())

    def test_archive_load_reversal_and_relative_queue_reach_pixels(self):
        source = [(255, 0, 0, 255), (0, 255, 0, 255), (0, 0, 255, 255), (255, 255, 0, 255)]
        for flags, queue, opt, earlier in [(b',u,c3', b'q+1', None, False), (b'', b'q-1', struct.pack('>5B14h', 2, 2, 1, 1, 2,
                -1, 0, 0, 0, 0, 1, 2, -1, 1, 0, 1, 0, 1, 2), False), (b',u,c3', b'q+1', None, True)]:
            with self.subTest(flags=flags, queue=queue), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                archive, capture = root / 'resources.bin', root / 'frame.bmp'
                clip_id = 3 if earlier else 0
                catalog = (b'0\tclip.seb,v,c2\n3\tclip.seb' if earlier else b'0\tclip.seb') + flags
                members = [('img.inf', b'7\timage.png,bin,' + queue), ('seb.inf', catalog),
                           ('clip.seb', seb([(0, 7, 0, 0, 2, 2, 0, 0, 0, 0)])),
                           ('image.png', png(2, 2, [c for p in source for c in p]))]
                if opt is not None:
                    members.append(('image.opt', opt))
                archive.write_bytes(jar(members))
                result = subprocess.run([str(EXE), str(archive), '--resource', str(clip_id), '0', '0', '--language', 'English.lproj',
                         '--dpi', '0', '--canvas', '12', '12', '--origin', '6', '6', '--headless', '--capture', str(capture)],
                         text=True, capture_output=True, timeout=20)
                self.assertEqual(result.returncode, 0, result.stderr)
                report = json.loads(result.stdout)
                self.assertEqual(report['resource_ready_clips'], 2 if earlier else 1)
                self.assertEqual(report['image_queue_offset'], 1 if flags else -1)
                self.assertEqual(report['resource_reverse'], [1 if flags else 0, 0])
                self.assertEqual(report['render_queues'], [2001] if flags else [1999, 1999])
                expected = [(0, 0, 0, 0)] * 144
                for y in range(2):
                    for x in range(2):
                        expected[(6 + y) * 12 + (4 if flags else 6) + x] = source[y * 2 + (1 - x if flags else x)]
                self.assertEqual(bmp_pixels(capture.read_bytes())[2], expected)

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

    def test_empty_outside_crop_is_not_drawn(self):
        result, bitmap = self.preview([(0, 0, 20, 20, 2, 2, 0, 0, 0, 0)])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)['quad_count'], 0)
        self.assertEqual(bitmap[2], [(0, 0, 0, 0)] * 144)

    def test_all_seb_mirror_combinations_reach_correct_pixels(self):
        for reverse_u, reverse_v in ((1, 0), (0, 1), (1, 1)):
            with self.subTest(reverse_u=reverse_u, reverse_v=reverse_v):
                result, bitmap = self.preview([(0, 0, 2, 3, 3, 2, 1, 2, reverse_u, reverse_v)])
                self.assertEqual(result.returncode, 0, result.stderr)
                sprite = json.loads(result.stdout)['sprite']
                reverse_x, reverse_y = sprite[8] == 1, sprite[9] == 1
                self.assertTrue(reverse_x or reverse_y)
                expected = [(0, 0, 0, 0)] * 144
                for y in range(2):
                    for x in range(3):
                        sx, sy = (2 - x if reverse_x else x), (1 - y if reverse_y else y)
                        expected[(4 + y) * 12 + 4 + x] = self.source[(3 + sy) * 8 + 2 + sx]
                self.assertEqual(bitmap[2], expected)

    def test_opt_piece_translation_is_inside_the_mirror_scope(self):
        opt = struct.pack('>5B7h', 8, 8, 1, 1, 1, -1, 1, 2, 4, 5, 2, 2)
        result, bitmap = self.preview([(0, 0, 0, 0, 8, 8, 0, 0, 1, 1)], opt)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)['sprite'][8:10], [1, 1])
        expected = [(0, 0, 0, 0)] * 144
        for y in range(2):
            for x in range(2):
                expected[(6 + y) * 12 + 8 + x] = self.source[(6 - y) * 8 + 5 - x]
        self.assertEqual(bitmap[2], expected)

    def test_command_rotation_keeps_the_entity_origin_as_pivot(self):
        result, bitmap = self.preview([(0, 0, 2, 3, 3, 2, 0, 0, 0, 0)], extra=('--angle', '90'))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)['angle'], 90)
        expected = [(0, 0, 0, 0)] * 144
        for y in range(3):
            for x in range(2):
                expected[(2 + y) * 12 + 1 + x] = self.source[(4 - x) * 8 + 2 + y]
        self.assertEqual(bitmap[2], expected)

    def test_command_skew_uses_native_tangent_adapter(self):
        result, bitmap = self.preview([(0, 0, 0, 0, 2, 2, 0, 0, 0, 0)], extra=('--angle', '45', '--skew'))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue(json.loads(result.stdout)['skew'])
        self.assertEqual(json.loads(result.stdout)['quad_count'], 1)
        self.assertTrue(any(pixel != (0, 0, 0, 0) for pixel in bitmap[2]))

    def test_mismatched_texture_and_cli_limits_fail_without_capture(self):
        result, bitmap = self.preview([(0, 1, 0, 0, 2, 2, 0, 0, 0, 0)])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('explicit PNG slot', result.stderr)
        self.assertIsNone(bitmap)
        result, bitmap = self.preview([(0,) * 10], extra=('--canvas', '200000', '200000'))
        self.assertNotEqual(result.returncode, 0)
        self.assertIsNone(bitmap)

    @staticmethod
    def blend_reference(pixel, background, mode='alpha', rgb=1.0, alpha=1.0, tint=(1, 1, 1)):
        a = pixel[3] * alpha / 255
        if mode == 'alpha':
            channels = [pixel[i] * rgb * tint[i] * a + background[i] * (1 - a) for i in range(3)]
            output_alpha = pixel[3] * alpha + background[3] * (1 - a)
        elif mode == 'attenuate':
            channels = [background[i] * (1 - a) for i in range(3)]
            output_alpha = pixel[3] * alpha + background[3] * (1 - a)
        elif mode == 'add':
            channels = [background[i] + pixel[i] * a for i in range(3)]
            output_alpha = background[3] + a * a * 255
        elif mode == 'subtract':
            channels = [background[i] - pixel[i] * a for i in range(3)]
            output_alpha = background[3] * (1 - a) - pixel[3] * alpha
        else:
            raise AssertionError('unknown reference blend')
        return tuple(max(0, min(255, round(v))) for v in channels + [output_alpha])

    def assert_pixels_close(self, actual, expected, tolerance=1):
        self.assertEqual(len(actual), len(expected))
        for i, (a, b) in enumerate(zip(actual, expected)):
            self.assertTrue(all(abs(x - y) <= tolerance for x, y in zip(a, b)), (i, a, b))

    def test_native_default_transparency_tint_and_encoded_fade(self):
        background = (60, 70, 80, 90)
        for options, rgb, alpha, tint in (((), 1, 1, (1, 1, 1)),
                                          (('--alpha', '1128'), 1, 128 / 255, (1, 1, 1)),
                                          (('--alpha', '128'), 128 / 255, 1, (1, 1, 1)),
                                          (('--color', '-2130771968'), 1, 1, (128 / 255, 0, 0))):
            with self.subTest(options=options):
                args = ('--background', *(str(v) for v in background), *options)
                result, bitmap = self.preview([(0, 0, 2, 3, 3, 2, 1, 2, 0, 0)], raw=False, extra=args)
                self.assertEqual(result.returncode, 0, result.stderr)
                report = json.loads(result.stdout)
                self.assertFalse(report['raw_rgba'])
                self.assertEqual(report['image_alpha_type'], 2)
                self.assertEqual(report['quad_count'], 1)
                expected = [background] * 144
                for y in range(2):
                    for x in range(3):
                        expected[(4 + y) * 12 + 4 + x] = self.blend_reference(self.source[(3 + y) * 8 + 2 + x], background, rgb=rgb, alpha=alpha, tint=tint)
                self.assert_pixels_close(bitmap[2], expected)

    def test_native_mixed_alpha_passes_preserve_threshold_pixels(self):
        source = [(30, 60, 90, 0)] * 64
        alphas = (0, 2, 3, 128, 252, 253, 254, 255)
        source[:8] = [(30, 60, 90, a) for a in alphas]
        background = (100, 110, 120, 130)
        result, bitmap = self.preview([(0, 0, 0, 0, 8, 1, 0, 0, 0, 0)], raw=False, source=source,
                                      extra=('--background', *(str(v) for v in background)))
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertEqual(report['image_alpha_type'], 3)
        self.assertEqual(report['quad_count'], 2)
        self.assertTrue(report['shader_keys'][0] & 0x800)
        self.assertTrue(report['shader_keys'][1] & 0x1000)
        expected = [background] * 144
        for x in range(8):
            expected[2 * 12 + 3 + x] = self.blend_reference(source[x], background)
        self.assert_pixels_close(bitmap[2], expected)

    def test_native_binary_alpha_discards_values_below_point_zero_one(self):
        source = [(30, 60, 90, 255)] * 64
        source[0] = (30, 60, 90, 2)
        background = (100, 110, 120, 130)
        result, bitmap = self.preview([(0, 0, 0, 0, 2, 1, 0, 0, 0, 0)], raw=False, source=source,
                                      extra=('--background', *(str(v) for v in background)))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)['image_alpha_type'], 1)
        expected = [background] * 144
        expected[2 * 12 + 4] = source[1]
        self.assert_pixels_close(bitmap[2], expected)

    def test_native_reverse_subtract_uses_separate_alpha_factors(self):
        background = (170, 160, 150, 240)
        result, bitmap = self.preview([(0, 0, 2, 3, 3, 2, 1, 2, 0, 0)], raw=False,
                                      extra=('--alpha', '2128', '--background', *(str(v) for v in background)))
        self.assertEqual(result.returncode, 0, result.stderr)
        expected = [background] * 144
        for y in range(2):
            for x in range(3):
                expected[(4 + y) * 12 + 4 + x] = self.blend_reference(self.source[(3 + y) * 8 + 2 + x], background, mode='subtract', alpha=128 / 255)
        self.assert_pixels_close(bitmap[2], expected)

    def test_native_general_add_is_ordered_attenuation_then_addition(self):
        background = (60, 70, 80, 90)
        result, bitmap = self.preview([(0, 0, 2, 3, 3, 2, 1, 2, 0, 0)], raw=False,
                                      extra=('--alpha', '3100', '--background', *(str(v) for v in background)))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(json.loads(result.stdout)['quad_count'], 2)
        expected = [background] * 144
        for y in range(2):
            for x in range(3):
                source = self.source[(3 + y) * 8 + 2 + x]
                attenuated = self.blend_reference(source, background, mode='attenuate', alpha=155 / 255)
                expected[(4 + y) * 12 + 4 + x] = self.blend_reference(source, attenuated, mode='add')
        self.assert_pixels_close(bitmap[2], expected, tolerance=2)


if __name__ == '__main__':
    unittest.main()
