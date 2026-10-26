"""Synthetic bounds and round-trip checks for the native assembly-resource layout."""
import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location('assembly_resources', Path(__file__).parents[1] / 'tools/inspect_assembly_resources.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def pack(rows):
    directory = struct.pack('<i', len(rows)) + b''.join(
        struct.pack('<ii', len(data), len(name)) + name for name, data in rows)
    return struct.pack('<i', len(directory)) + directory + b''.join(data for _, data in rows)


class Resources(unittest.TestCase):
    def test_round_trip(self):
        rows = [(b'a', b'123'), (b'long.name', b''), (b'a', b'\0\xff')]
        data = pack(rows)
        parsed = module.parse(data)
        self.assertEqual([(n, p) for n, _, p in parsed], rows)
        self.assertEqual(pack([(n, p) for n, _, p in parsed]), data)
        for _, offset, payload in parsed:
            self.assertEqual(data[offset:offset+len(payload)], payload)
        self.assertEqual(module.parse(pack([])), [])

    def test_bad_container(self):
        good = pack([(b'x', b'abc')])
        for bad in [good[:-1], good+b'x', b'', struct.pack('<ii', 4, -1),
                    struct.pack('<ii', 100, 0), good[:8]+struct.pack('<i', -1)+good[12:]]:
            with self.assertRaises(ValueError):
                module.parse(bad)

    def test_collation_layout(self):
        core = b'\3'+b''.join(struct.pack('<i', n)+b'x'*n for n in [1, 0, 3, 2, 5])
        tail = b'\3'+struct.pack('<i', 2)+bytes(26)+b'AB'+struct.pack('<i', 3)+b'x\0'*3
        result = module.inspect_collation(core, tail)
        self.assertEqual([t['size'] for t in result['core_tables']], [1, 0, 3, 2, 5])
        self.assertEqual(result['tailoring_records'], 2)
        self.assertEqual(result['tailoring_utf16_units'], 3)
        for c, t in [(b'\2'+core[1:], tail), (core[:-1], tail), (core, tail[:-1]),
                     (core+b'x', tail), (core, tail+b'x'), (core, b'\3'+struct.pack('<i', -1))]:
            with self.assertRaises(ValueError):
                module.inspect_collation(c, t)


if __name__ == '__main__':
    unittest.main()
