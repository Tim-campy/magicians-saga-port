"""Format and filesystem boundary tests using only synthetic, non-game data."""
import importlib.util
from pathlib import Path
import struct
import json
import tempfile
import unittest
import hashlib
import io
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('extract_assets', Path(__file__).parents[1]/'tools/extract_assets.py')
extractor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(extractor)


class NewGameSaveTests(unittest.TestCase):
    def metadata(self):
        data=bytearray(512+160)
        struct.pack_into('<II',data,0,0xfab11baf,31)
        struct.pack_into('<IIII',data,64,256,24,512,160)
        struct.pack_into('<iiiiii',data,256,24359,77,0,24245,88,116)
        integers=list(range(29));integers[8]=-3;integers[9]=17;integers[19]=-2147483648
        struct.pack_into('<29i',data,512,*integers)
        struct.pack_into('<3q',data,512+116,-9223372036854775808,9223372036854775807,23)
        return data

    def test_widths_selection_and_reached_slots_only(self):
        payload=extractor.new_game_save_payload(self.metadata())
        self.assertEqual(payload,b'new_game_save\t1\nselection\t-3\t17\ntown_capacity\t-2147483648\nwallet\t-9223372036854775808\t9223372036854775807\n')

    def test_bounds_missing_and_duplicate_fields(self):
        for offset,value in ((4,30),(64,1),(68,23),(72,256),(76,99999),(264,-1),(276,150),(268,24359),(256,7)):
            with self.subTest(offset=offset,value=value):
                data=self.metadata();struct.pack_into('<i',data,offset,value)
                with self.assertRaises(ValueError):extractor.new_game_save_payload(data)
        with self.assertRaises(ValueError):extractor.new_game_save_payload(b'bad')

    def test_apk_hash_precedes_output_write(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);apk=root/'synthetic.apk';apk.write_bytes(b'synthetic')
            (root/'manifest.json').write_text(json.dumps({'schema':3,'apk_sha256':extractor.APK_SHA}))
            with self.assertRaisesRegex(ValueError,'supported APK'):extractor.write_new_game_save(apk,root)
            self.assertFalse((root/'new-game-save.tsv').exists())

    def test_synthetic_apk_round_trip_and_existing_output_preservation(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);apk=root/'synthetic.apk';metadata=self.metadata()
            with extractor.zipfile.ZipFile(apk,'w') as archive:archive.writestr(extractor.METADATA,metadata)
            checksum=hashlib.sha256(apk.read_bytes()).hexdigest()
            manifest=json.dumps({'schema':3,'apk_sha256':checksum})
            (root/'manifest.json').write_text(manifest)
            with patch.object(extractor,'APK_SHA',checksum):
                output=extractor.write_new_game_save(apk,root)
                self.assertEqual(output.read_bytes(),extractor.new_game_save_payload(metadata))
                self.assertEqual(extractor.write_new_game_save(apk,root),output)
                output.write_bytes(b'keep this user file')
                with self.assertRaisesRegex(ValueError,'replace differing'):extractor.write_new_game_save(apk,root)
                self.assertEqual(output.read_bytes(),b'keep this user file')
                self.assertEqual((root/'manifest.json').read_text(),manifest)


class MissionInitializationTests(unittest.TestCase):
    def metadata(self):
        data=bytearray(608)
        struct.pack_into('<II',data,0,0xfab11baf,31)
        struct.pack_into('<IIII',data,8,300,8,308,0)
        struct.pack_into('<IIII',data,64,256,12,512,96)
        struct.pack_into('<iii',data,256,24300,77,0)
        values=list(range(24));values[12]=-2;values[23]=-2147483648
        struct.pack_into('<24i',data,512,*values)
        return data

    def test_system_slots_and_distinct_nonnull_empty_arguments(self):
        self.assertEqual(extractor.mission_initialization_payload(self.metadata()),
                         b'mission_initialization\t1\nyugami\t-2147483648\norientation\t-2\nevent_arguments\t""\n')

    def test_initializer_and_empty_literal_bounds(self):
        for offset,value in ((4,30),(8,1),(12,7),(16,999),(300,1),(304,1),(264,-1),(256,1)):
            with self.subTest(offset=offset,value=value):
                data=self.metadata();struct.pack_into('<i',data,offset,value)
                with self.assertRaises(ValueError):extractor.mission_initialization_payload(data)

    def test_synthetic_apk_and_collision_preserve_other_seeds(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);apk=root/'synthetic.apk';metadata=self.metadata()
            with extractor.zipfile.ZipFile(apk,'w') as archive:archive.writestr(extractor.METADATA,metadata)
            checksum=hashlib.sha256(apk.read_bytes()).hexdigest()
            (root/'manifest.json').write_text(json.dumps({'schema':3,'apk_sha256':checksum}))
            other=root/'new-game-save.tsv';other.write_bytes(b'keep normal save seed')
            with patch.object(extractor,'APK_SHA',checksum):
                output=extractor.write_mission_initialization(apk,root)
                self.assertEqual(output.read_bytes(),extractor.mission_initialization_payload(metadata))
                self.assertEqual(extractor.write_mission_initialization(apk,root),output)
                output.write_bytes(b'keep user context')
                with self.assertRaisesRegex(ValueError,'replace differing'):extractor.write_mission_initialization(apk,root)
                self.assertEqual(output.read_bytes(),b'keep user context')
            self.assertEqual(other.read_bytes(),b'keep normal save seed')


class MissionFontTests(unittest.TestCase):
    def fixture(self, root, filename='991_renamed font.ttf'):
        relative='unity/Font/'+filename
        path=root/relative;path.parent.mkdir(parents=True)
        payload=b'synthetic font payload'
        path.write_bytes(payload)
        entry={'source':'assets/bin/Data/unity default resources', 'path_id':10102,
               'name':'LegacyRuntime', 'type':'Font', 'output':relative,
               'size':len(payload), 'sha256':hashlib.sha256(payload).hexdigest()}
        manifest={'schema':3, 'apk_sha256':extractor.APK_SHA, 'unity_media':[entry]}
        self.write_manifest(root, manifest)
        return manifest,entry,path,payload

    def write_manifest(self, root, manifest):
        (root/'manifest.json').write_text(json.dumps(manifest), encoding='utf-8')

    def test_identity_selection_ignores_numeric_filename_and_repeats(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);manifest,entry,path,payload=self.fixture(root,'991_renamed á.ttf')
            original=(root/'manifest.json').read_bytes()
            output=extractor.write_mission_font(root)
            self.assertEqual(output.read_text(encoding='utf-8'),
                             f'mission_font\t1\nfont\t"{entry["output"]}"\t{len(payload)}\n')
            self.assertEqual(extractor.write_mission_font(root),output)
            self.assertEqual((root/'manifest.json').read_bytes(),original)
            self.assertEqual(path.read_bytes(),payload)

    def test_missing_duplicate_and_wrong_identity_fail_without_output(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);manifest,entry,_,_=self.fixture(root)
            cases=[[],[entry,dict(entry)]]
            for key,value in (('name','Other'),('type','Texture2D'),('source','other bundle'),('path_id',3),('path_id',True)):
                changed=dict(entry);changed[key]=value;cases.append([changed])
            for media in cases+[None,{},['bad row']]:
                with self.subTest(media=media):
                    manifest['unity_media']=media;self.write_manifest(root,manifest)
                    with self.assertRaises(ValueError):extractor.write_mission_font(root)
                    self.assertFalse((root/'mission-font.tsv').exists())

    def test_version_size_and_hash_checks_preserve_existing_output(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);manifest,entry,path,payload=self.fixture(root)
            output=root/'mission-font.tsv';output.write_bytes(b'keep user index')
            cases=[('schema',2),('schema',4),('schema',3.0),('schema',True),('apk_sha256','other APK')]
            for key,value in cases:
                changed=dict(manifest);changed[key]=value;self.write_manifest(root,changed)
                with self.subTest(key=key,value=value),self.assertRaises(ValueError):extractor.write_mission_font(root)
            for changed in ([],None):
                self.write_manifest(root,changed)
                with self.assertRaises(ValueError):extractor.write_mission_font(root)
            for key,value in (('size',0),('size',-1),('size',True),('size',64*1024*1024+1),('size',1),
                              ('sha256',None),('sha256','bad'),('sha256','0'*64)):
                changed=dict(entry);changed[key]=value;manifest['unity_media']=[changed];self.write_manifest(root,manifest)
                with self.subTest(key=key,value=value),self.assertRaises(ValueError):extractor.write_mission_font(root)
                self.assertEqual(output.read_bytes(),b'keep user index')
            manifest['unity_media']=[entry];self.write_manifest(root,manifest)
            path.write_bytes(b'x'*len(payload))
            with self.assertRaisesRegex(ValueError,'hash mismatch'):extractor.write_mission_font(root)
            self.assertEqual(output.read_bytes(),b'keep user index')

    def test_paths_are_direct_bounded_font_children(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);manifest,entry,_,_=self.fixture(root)
            for relative in ('/absolute.ttf','../outside.ttf','C:/absolute.ttf','unity/Font/../x.ttf',
                             'unity/Font/nested/x.ttf','unity/Font\\x.ttf','unity/Font/x:y.ttf',
                             'unity/Font/.','unity/Font/','unity/Font/x.png','unity/Font/x\n.ttf',
                             'unity/Font/x"y.ttf','unity/Font/x?y.ttf',None):
                changed=dict(entry);changed['output']=relative;manifest['unity_media']=[changed];self.write_manifest(root,manifest)
                with self.subTest(relative=relative),self.assertRaises(ValueError):extractor.write_mission_font(root)
                self.assertFalse((root/'mission-font.tsv').exists())
            entry['output']='unity/Font/missing.ttf';manifest['unity_media']=[entry];self.write_manifest(root,manifest)
            with self.assertRaises(ValueError):extractor.write_mission_font(root)

    def test_symlink_cannot_escape_font_directory(self):
        with tempfile.TemporaryDirectory() as directory,tempfile.TemporaryDirectory() as outside:
            root=Path(directory);manifest,entry,path,payload=self.fixture(root)
            target=Path(outside)/'outside.ttf';target.write_bytes(payload);path.unlink()
            try:path.symlink_to(target)
            except OSError:self.skipTest('File symlinks are unavailable on this host')
            with self.assertRaisesRegex(ValueError,'escapes destination'):extractor.write_mission_font(root)
            path.unlink();target=root/'elsewhere.ttf';target.write_bytes(payload);path.symlink_to(target)
            with self.assertRaisesRegex(ValueError,'bounded font file'):extractor.write_mission_font(root)
            self.assertFalse((root/'mission-font.tsv').exists())

    def test_differing_index_is_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            output=extractor.write_mission_font(root);output.write_bytes(b'keep user index')
            with self.assertRaisesRegex(ValueError,'replace differing'):extractor.write_mission_font(root)
            self.assertEqual(output.read_bytes(),b'keep user index')

    def test_cli_font_mode_is_exclusive_before_reading_or_extracting(self):
        flags=('owned.apk','--check','--mission-index','--new-game-save','--mission-initialization',
               '--unity-media','--culture-signs','--culture-names','--culture-neutral','--culture-lower',
               '--runtime-tables','--collation','--collation-indexers')
        for flag in flags:
            with self.subTest(flag=flag),patch.object(extractor.sys,'argv',['extract_assets.py','--mission-font',flag]),\
                    patch.object(extractor,'write_mission_font') as writer,patch.object(extractor.sys,'stderr',io.StringIO()):
                with self.assertRaises(SystemExit) as result:extractor.main()
                self.assertEqual(result.exception.code,1);writer.assert_not_called()


class MissionIndexTests(unittest.TestCase):
    def test_manifest_names_paths_hashes_and_non_destructive_repeat(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);assets = root/'textassets';assets.mkdir()
            entries = []
            for index, name in enumerate(['xls'] + [name for _, name in extractor.MISSION_RESOURCES]):
                payload = ('synthetic:' + name).encode();relative = f'textassets/{index:02}_{name}.bin'
                (root/relative).write_bytes(payload)
                entries.append({'name':name,'output':relative,'sha256':hashlib.sha256(payload).hexdigest()})
            manifest = {'schema':2,'apk_sha256':extractor.APK_SHA,'files':entries}
            source = root/'manifest.json';source.write_text(json.dumps(manifest), encoding='utf-8')
            output = extractor.write_mission_index(root);original = output.read_bytes()
            self.assertIn(b'resource\t23\t', original)
            self.assertEqual(extractor.write_mission_index(root).read_bytes(), original)
            manifest['schema']=3;source.write_text(json.dumps(manifest), encoding='utf-8')
            self.assertEqual(extractor.write_mission_index(root).read_bytes(), original)
            (root/entries[0]['output']).write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError, 'hash mismatch'):extractor.write_mission_index(root)
            self.assertEqual(output.read_bytes(), original)
            manifest['files'].append(entries[1]);source.write_text(json.dumps(manifest), encoding='utf-8')
            manifest['files'][0]['sha256'] = hashlib.sha256(b'changed').hexdigest()
            source.write_text(json.dumps(manifest), encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'ambiguous'):extractor.write_mission_index(root)


class CollationTests(unittest.TestCase):
    def test_indexer_initializers(self):
        metadata=bytearray(256+113400)
        struct.pack_into('<II',metadata,0,0xfab11baf,31)
        struct.pack_into('<II',metadata,72,256,113400)
        offsets={6160:3,113376:6,105184:6,22232:4,90032:4,106664:4,6808:3,24720:3,12096:3,105168:3}
        for offset,count in offsets.items():
            struct.pack_into('<'+'i'*count,metadata,256+offset,*range(offset,offset+count))
        instructions=struct.pack('<II',0x52800009|(123<<5),0x52800008|(456<<5))
        result=extractor.collation_indexer_payload(metadata,instructions)
        self.assertEqual(len(result),324);self.assertEqual(result[:8],b'SCI1'+struct.pack('<I',7))
        self.assertEqual(struct.unpack_from('<iii',result,20),(0,123,456))
        self.assertEqual(struct.unpack_from('<iii',result,32),(6160,6161,6162))
        for bad,code in [(metadata[:-1],instructions),(metadata,b''),(metadata,bytes(8)),(b'',instructions)]:
            with self.assertRaises(ValueError):extractor.collation_indexer_payload(bad,code)

    def fixture(self):
        rows = [('collation.'+name+'.bin', b'\3'+bytes(size-1))
                for name, size in extractor.COLLATION_SIZES.items()]
        directory = struct.pack('<i', len(rows)) + b''.join(
            struct.pack('<ii', len(data), len(name)) + name.encode('ascii') for name, data in rows)
        return struct.pack('<i', len(directory))+directory+b''.join(data for _, data in rows), rows

    def test_indexer_manifest(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);(root/'collation').mkdir();data=bytes(324)
            path=root/'collation/indexers.bin';path.write_bytes(data)
            row={'output':'collation/indexers.bin','size':324,'sha256':extractor.digest(data)}
            manifest={'apk_sha256':extractor.APK_SHA,'files':[row]*38,'members':[row]*1963}
            def write(): (root/'manifest.json').write_text(json.dumps(manifest),encoding='utf8')
            write()
            with self.assertRaisesRegex(ValueError,'Collation indexers are required'):extractor.check(root,require_indexers=True)
            manifest['collation_indexers']=[row];write();self.assertTrue(extractor.check(root,require_indexers=True))
            manifest['collation_indexers']=[row,row];write()
            with self.assertRaisesRegex(ValueError,'Unexpected collation indexer inventory'):extractor.check(root)

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
