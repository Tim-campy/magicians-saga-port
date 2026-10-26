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


class CollationTests(unittest.TestCase):
    def fixture(self):
        rows = [('collation.'+name+'.bin', b'\3'+bytes(size-1))
                for name, size in extractor.COLLATION_SIZES.items()]
        directory = struct.pack('<i', len(rows)) + b''.join(
            struct.pack('<ii', len(data), len(name)) + name.encode('ascii') for name, data in rows)
        return struct.pack('<i', len(directory))+directory+b''.join(data for _, data in rows), rows

    def test_payloads_and_corruption(self):
        data, rows = self.fixture()
        self.assertEqual(extractor.collation_payloads(data), {'collation/'+name: payload for name, payload in rows})
        directory = struct.unpack_from('<i', data)[0]+4
        for bad in [data[:7], data[:-1], data+b'x', data[:directory]+b'\2'+data[directory+1:],
                    data[:4]+struct.pack('<i', 8)+data[8:], data[:8]+struct.pack('<i', -1)+data[12:]]:
            with self.assertRaises(ValueError):extractor.collation_payloads(bad)

    def test_manifest_inventory_integrity(self):
        _, rows = self.fixture()
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);(root/'collation').mkdir()
            inventory=[]
            for name, data in rows:
                (root/'collation'/name).write_bytes(data)
                inventory.append({'output':'collation/'+name,'size':len(data),'sha256':extractor.digest(data)})
            manifest={'apk_sha256':extractor.APK_SHA,'files':[inventory[0]]*38,'members':[inventory[0]]*1963}
            def write(): (root/'manifest.json').write_text(json.dumps(manifest),encoding='utf8')
            write();self.assertTrue(extractor.check(root))
            with self.assertRaisesRegex(ValueError,'Collation outputs are required'):extractor.check(root,require_collation=True)
            manifest['collation']=inventory;write();self.assertTrue(extractor.check(root,require_collation=True))
            path=root/inventory[-1]['output'];original=path.read_bytes();path.write_bytes(b'bad')
            self.assertFalse(extractor.check(root,require_collation=True));path.write_bytes(original)
            manifest['collation']=inventory[:-1]+[inventory[0]];write()
            with self.assertRaisesRegex(ValueError,'Unexpected collation inventory'):extractor.check(root)


class RuntimeTests(unittest.TestCase):
    def test_payload_and_bounds(self):
        data=bytearray(512+50056+288)
        struct.pack_into('<II',data,0,0xfab11baf,31)
        struct.pack_into('<II',data,72,512,50056+288)
        payload=bytes(range(256))+bytes(range(32))
        data[-288:]=payload
        self.assertEqual(extractor.runtime_payloads(data),{'runtime/hashtable-primes.bin':payload})
        for bad in (data[:255],data[:-1],b'BAD!'+data[4:]):
            with self.assertRaises(ValueError):extractor.runtime_payloads(bad)
        struct.pack_into('<II',data,72,512,50056+287)
        with self.assertRaises(ValueError):extractor.runtime_payloads(data)

    def test_manifest_runtime_required_and_integrity(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);(root/'runtime').mkdir();data=bytes(288)
            (root/'runtime/hashtable-primes.bin').write_bytes(data)
            row={'output':'runtime/hashtable-primes.bin','size':288,'sha256':extractor.digest(data)}
            manifest={'apk_sha256':extractor.APK_SHA,'files':[row]*38,'members':[row]*1963}
            def write(): (root/'manifest.json').write_text(json.dumps(manifest),encoding='utf8')
            write()
            self.assertTrue(extractor.check(root))
            with self.assertRaisesRegex(ValueError,'Runtime tables are required'):extractor.check(root,require_runtime=True)
            manifest['runtime_tables']=[row];write();self.assertTrue(extractor.check(root,require_runtime=True))
            (root/'runtime/hashtable-primes.bin').write_bytes(b'bad')
            self.assertFalse(extractor.check(root,require_runtime=True))
            manifest['runtime_tables']=[row,row];write()
            with self.assertRaisesRegex(ValueError,'Unexpected runtime table inventory'):extractor.check(root)


class LowercaseTests(unittest.TestCase):
    def sample(self):
        base,size=512,106392
        data=bytearray(base+size)
        struct.pack_into('<II',data,0,0xfab11baf,31)
        struct.pack_into('<II',data,72,base,size)
        for i,(offset,count) in enumerate(extractor.LOWER_LAYOUT):
            for j in range(count):
                struct.pack_into('<H',data,base+offset+2*j,(i*257+j*19)&65535)
        return data

    def test_exact_arrays_and_byte_order(self):
        data=self.sample();outputs=extractor.culture_lower_payloads(data)
        self.assertEqual(len(outputs),9)
        self.assertEqual(sum(map(len,outputs.values())),4270)
        for i,(offset,count) in enumerate(extractor.LOWER_LAYOUT):
            expected=struct.pack('<'+'H'*count,*[(i*257+j*19)&65535 for j in range(count)])
            self.assertEqual(outputs[f'culture/lower-{i}.bin'],expected)

    def test_section_and_version_bounds(self):
        data=self.sample()
        for bad in (data[:255],data[:-1],b'BAD!'+data[4:]):
            with self.assertRaises(ValueError):extractor.culture_lower_payloads(bad)
        for base,size in ((0,106392),(512,106391),(512,0),(0xffffffff,106392)):
            bad=bytearray(data);struct.pack_into('<II',bad,72,base,size)
            with self.assertRaises(ValueError):extractor.culture_lower_payloads(bad)
        bad=bytearray(data);struct.pack_into('<I',bad,4,30)
        with self.assertRaises(ValueError):extractor.culture_lower_payloads(bad)

    def test_manifest_inventory_hashes_and_legacy(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp)
            def row(name,data):
                path=root/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
                return {'output':name,'size':len(data),'sha256':extractor.digest(data)}
            base=row('synthetic.bin',b'synthetic')
            culture=[row(name,b'synthetic') for name in ('culture/number-records.bin','culture/sign-pool.bin',
                'culture/name-index.bin','culture/culture-records.bin','culture/neutral-map.json')]
            lower=[row(name,data) for name,data in extractor.culture_lower_payloads(self.sample()).items()]
            manifest={'apk_sha256':extractor.APK_SHA,'files':[base]*38,'members':[base]*1963,'culture_signs':culture+lower}
            def write(): (root/'manifest.json').write_text(json.dumps(manifest),encoding='utf8')
            write()
            self.assertTrue(extractor.check(root,True,True,True,True))
            (root/'culture/lower-8.bin').write_bytes(b'changed')
            self.assertFalse(extractor.check(root,require_lower=True))
            manifest['culture_signs']=culture;write()
            self.assertTrue(extractor.check(root,require_neutral=True))
            with self.assertRaisesRegex(ValueError,'Lowercase culture outputs are required'):
                extractor.check(root,require_lower=True)
            manifest['culture_signs']=culture+lower[:-1];write()
            with self.assertRaisesRegex(ValueError,'Unexpected culture sign inventory'):
                extractor.check(root)
            manifest['culture_signs']=culture+lower+[lower[0]];write()
            with self.assertRaisesRegex(ValueError,'Unexpected culture sign inventory'):
                extractor.check(root)


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
