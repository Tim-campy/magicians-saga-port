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


class NeutralMappingTests(unittest.TestCase):
    def leaf(self, base=0x1000, lcid=0x1234):
        def branch(pc, target, opcode):
            return opcode | (((target-pc)//4) & 0x3ffffff)
        words = [0x90000008, 0xf9400108 | (4 << 10), 0xaa1403e0, 0xaa1f03e2,
                 0xf9400101, branch(base+20,0x4000,0x94000000),
                 0x36000000 | ((((0x2000-(base+24))//4) & 0x3fff) << 5),
                 0x52800013 | (lcid << 5), branch(base+32,0x3000,0x14000000)]
        return struct.pack('<9I',*words)

    def decode(self, code, literal=lambda got:'synthetic'):
        return extractor.decode_neutral_cases(code,0x1000,literal,0x4000,0x3000,0x2000)

    def test_leaf_address_and_lcid(self):
        seen=[]
        result=self.decode(self.leaf(),lambda got:seen.append(got) or 'synthetic')
        self.assertEqual(seen,[0x1020])
        self.assertEqual(result,[{'name':'synthetic','lcid':0x1234}])

    def test_duplicate_key_rejected(self):
        with self.assertRaisesRegex(ValueError,'Duplicate'):
            self.decode(self.leaf()+self.leaf(0x1024))

    def test_reject_damaged_registers_and_branch_targets(self):
        for word in (0,1,2,3,4,6,7,8):
            code=bytearray(self.leaf())
            struct.pack_into('<I',code,word*4,0)
            with self.assertRaises(ValueError):
                self.decode(code)
        for code in (self.leaf()[:-1],self.leaf()[:24]):
            with self.assertRaises(ValueError):
                self.decode(code)

    def test_unrelated_calls_are_not_mapping_entries(self):
        code=bytearray(self.leaf());struct.pack_into('<I',code,20,0x94000000)
        self.assertEqual(self.decode(code),[])

    def test_bad_elf_is_rejected(self):
        with self.assertRaises(ValueError):
            extractor.neutral_culture_payload(b'bad',b'bad')


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

    def named_sample(self):
        data, _, pool = self.sample()
        names, cultures = 0x100+0x18b4, 0x100+0x1e00
        for i in range(339):
            offset = 4096+i*5
            data[pool+offset:pool+offset+5] = f'x{i:03d}'.encode()+b'\0'
            struct.pack_into('<Hh', data, names+i*4, offset, 338-i)
            struct.pack_into('<h', data, cultures+(338-i)*56+0x20, i)
        return data, names, cultures, pool

    def test_name_tables_preserved_and_pool_extended(self):
        data, names, cultures, pool = self.named_sample()
        out = extractor.culture_sign_payloads(bytes(data), include_names=True)
        self.assertEqual(out['culture/name-index.bin'], data[names:names+339*4])
        self.assertEqual(out['culture/culture-records.bin'], data[cultures:cultures+339*56])
        self.assertEqual(out['culture/sign-pool.bin'], data[pool:pool+4096+339*5])

    def test_reject_bad_name_indices_and_order(self):
        for field, value in ((2,-1),(2,339)):
            data, names, _, _ = self.named_sample()
            struct.pack_into('<h', data, names+field, value)
            with self.assertRaises(ValueError):
                extractor.culture_sign_payloads(bytes(data), include_names=True)
        data, _, cultures, _ = self.named_sample()
        struct.pack_into('<h', data, cultures+0x20, -1)
        with self.assertRaises(ValueError):
            extractor.culture_sign_payloads(bytes(data), include_names=True)
        data, names, _, _ = self.named_sample()
        data[names+4:names+6] = data[names:names+2]
        with self.assertRaisesRegex(ValueError, 'sorted and unique'):
            extractor.culture_sign_payloads(bytes(data), include_names=True)

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
            with self.assertRaisesRegex(ValueError, 'Culture name outputs are required'):
                extractor.check(root, require_names=True)
            with self.assertRaisesRegex(ValueError, 'Neutral culture outputs are required'):
                extractor.check(root, require_neutral=True)

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
