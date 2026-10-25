"""Format and filesystem boundary tests using only synthetic, non-game data."""
import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location('extract_assets', Path(__file__).parents[1]/'tools/extract_assets.py')
extractor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(extractor)


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
