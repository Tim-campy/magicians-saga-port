"""Synthetic cases derived from reader branches, without proprietary fixtures."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

spec = importlib.util.spec_from_file_location('inspect_seb', Path(__file__).resolve().parents[1]/'tools/inspect_seb.py')
seb = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = seb
spec.loader.exec_module(seb)


class SebTests(unittest.TestCase):
    def test_legacy_signed_values_and_preserved_unused_short(self):
        raw = bytes.fromhex('0001000200011234') + struct.pack('>10h', 0,-1,-32768,32767,4,5,6,7,8,9)
        parsed = seb.parse(raw)
        self.assertEqual(parsed.groups[0].legacy_value, 0x1234)
        self.assertEqual(parsed.groups[0].records[0].values[2:4], (-32768,32767))
        self.assertEqual(seb.encode(parsed), raw)

    def test_empty_structures_are_not_fabricated_records(self):
        for raw in (bytes.fromhex('00000000'), bytes.fromhex('000100000000007b'), bytes.fromhex('ff00000000')):
            self.assertEqual(seb.encode(seb.parse(raw)), raw)

    def test_mode1_conditional_boundaries(self):
        # Each wire fixture is explicitly assembled, including absent fields.
        for discriminator, control, extra, pair, flags in [
            (-9,1,None,None,None), (-8,1,None,(2,3),None),
            (-7,1,None,None,None), (-6,0,0x12345678,None,None),
            (-2,0,0x12345678,None,None), (-1,1,None,None,None),
            (0,0,None,(2,3),3), (0,1,0x12345678,(2,3),3),
        ]:
            with self.subTest(discriminator=discriminator, control=control):
                raw = bytes.fromhex('ff000100010001')
                raw += struct.pack('>4hB', 0,discriminator,-5,6,control)
                if extra is not None: raw += struct.pack('>I',extra)
                if pair is not None: raw += struct.pack('>2h',*pair)
                raw += struct.pack('>2h',7,8)
                if flags is not None: raw += bytes([flags])
                parsed = seb.parse(raw)
                record = parsed.groups[0].records[0]
                self.assertEqual((record.extra_word,record.pair,record.flags),(extra,pair,flags))
                self.assertEqual(seb.encode(parsed),raw)

    def test_all_truncations_and_appended_bytes(self):
        fixtures = [bytes.fromhex('0001000100010001') + bytes(20),
                    bytes.fromhex('ff000100010001') + struct.pack('>4hBI4hB',0,0,-5,6,1,0x12345678,2,3,7,8,3)]
        for raw in fixtures:
            for end in range(len(raw)):
                with self.subTest(size=len(raw),end=end), self.assertRaises(ValueError):
                    seb.parse(raw[:end])
            with self.assertRaises(ValueError): seb.parse(raw+b'\0')

    def test_rejects_unknown_modes_and_negative_allocations(self):
        for raw in (bytes.fromhex('fe00010001'),bytes.fromhex('ff80000000'),
                    bytes.fromhex('00018000'),bytes.fromhex('00010001ffff0000')):
            with self.subTest(raw=raw.hex()),self.assertRaises(ValueError): seb.parse(raw)

    def test_encoder_rejects_inconsistent_optional_fields(self):
        record = seb.Record((0,-9,0,0),control=0,extra_word=5,tail_pair=(1,1))
        with self.assertRaises(ValueError): seb.encode(seb.Seb(1,1,(seb.Group((record,)),)))
        with self.assertRaises(ValueError): seb.encode(seb.Seb(0,1,(seb.Group((seb.Record((0,)*9),),0),)))


if __name__ == '__main__':
    unittest.main()
