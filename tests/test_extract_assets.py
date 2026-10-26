"""Format and filesystem boundary tests using only synthetic, non-game data."""
import importlib.util
from pathlib import Path
import struct
import json
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('extract_assets', Path(__file__).parents[1]/'tools/extract_assets.py')
extractor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(extractor)


class CultureSignTests(unittest.TestCase):
    def sample(self):
        base, start = 0x880000, 0x100
        size = 0x897cfc-base+339*48
        data = bytearray(start+size)
        data[:6] = b'\x7fELF\x02\x01'
        struct.pack_into('<H', data, 18, 183)
        struct.pack_into('<Q', data, 32, 64)
        struct.pack_into('<HH', data, 54, 56, 1)
        struct.pack_into('<IIQQQQQQ', data, 64, 1, 4, start, base, base, size, size, 4096)
        record, pool = start+0x897cfc-base, start+0x886828-base
        data[pool+300:pool+305] = b'\xe2\x80\x8e+\0'
        data[pool+310:pool+312] = b'-\0'
        struct.pack_into('<HH', data, record+0x14, 310, 300)
        return data, record, pool

    def test_exact_records_and_minimal_pool_prefix(self):
        data, record, pool = self.sample()
        outputs = extractor.culture_sign_payloads(bytes(data))
        self.assertEqual(outputs['culture/number-records.bin'], data[record:record+339*48])
        self.assertEqual(outputs['culture/sign-pool.bin'], data[pool:pool+312])
        positive, = struct.unpack_from('<H', outputs['culture/number-records.bin'], 0x16)
        self.assertEqual(outputs['culture/sign-pool.bin'][positive:].split(b'\0')[0], b'\xe2\x80\x8e+')

    def test_reject_wrong_elf_and_truncated_segment(self):
        data, _, _ = self.sample()
        for bad in (b'', bytes(data[:-1]), b'BAD!'+bytes(data[4:])):
            with self.assertRaises(ValueError):
                extractor.culture_sign_payloads(bad)

    def test_check_requires_requested_culture_inventory(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            manifest = {'apk_sha256': extractor.APK_SHA, 'files': [{}]*38, 'members': [{}]*1963}
            (root/'manifest.json').write_text(json.dumps(manifest), encoding='utf8')
            with self.assertRaisesRegex(ValueError, 'Culture sign outputs are required'):
                extractor.check(root, require_culture=True)

    def test_reject_invalid_utf8(self):
        data, _, pool = self.sample()
        data[pool+300] = 255
        with self.assertRaises(UnicodeError):
            extractor.culture_sign_payloads(bytes(data))

    def test_reject_unterminated_sign(self):
        data, record, pool = self.sample()
        # Redirect every sign to a non-NUL tail without changing segment bounds.
        data[pool:] = b'x'*(len(data)-pool)
        for i in range(339):
            struct.pack_into('<HH', data, record+i*48+0x14, 65535, 65535)
        with self.assertRaises(ValueError):
            extractor.culture_sign_payloads(bytes(data))


class ContainerTests(unittest.TestCase):
    def sample(self):
        name, payload = b'example.txt', b'example'
        index = (struct.pack('>I', 1) + struct.pack('>I', len(name)) + name +
                 struct.pack('>II', 0, len(payload)) + b'\0')
        body = struct.pack('>I', len(payload)) + payload
        return struct.pack('>II', len(index)+4, len(body)) + index + body

    def test_member_and_exact_rebuild(self):
        self.assertEqual(extractor.unpack_container(self.sample()), [('example.txt', b'example')])

    def test_reject_truncation_and_trailing_data(self):
        for bad in (self.sample()[:7], self.sample()[:-1], self.sample()+b'junk'):
            with self.assertRaises((ValueError, struct.error)):
                extractor.unpack_container(bad)

    def test_reject_unknown_compression(self):
        data = bytearray(self.sample())
        end = struct.unpack_from('>I', data)[0]+4
        data[end-1] = 1
        with self.assertRaises(ValueError):
            extractor.unpack_container(data)

    def test_reject_mismatched_member_length(self):
        data = bytearray(self.sample())
        end = struct.unpack_from('>I', data)[0]+4
        struct.pack_into('>I', data, end, 99)
        with self.assertRaises(ValueError):
            extractor.unpack_container(data)

    def test_xor_reversible_and_empty_key_rejected(self):
        self.assertEqual(extractor.xor(extractor.xor(b'abcde', b'12'), b'12'), b'abcde')
        with self.assertRaises(ValueError):
            extractor.xor(b'a', b'')

    def test_path_escape_rejected(self):
        with self.assertRaises(ValueError):
            extractor.bounded(Path('data'), '../outside')


if __name__ == '__main__':
    unittest.main()
