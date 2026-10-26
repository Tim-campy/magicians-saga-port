"""Synthetic culture inventory and optional C++ desktop loader integration."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
EXE = Path(sys.argv.pop(1)).resolve() if len(sys.argv)>1 else None

def fixture(root):
    directory=root/'culture';directory.mkdir()
    pool=bytearray(b'\0P\0N\0ZZ\0')
    names=bytearray(339*4);records=bytearray(339*56);numbers=bytearray(339*48)
    for i in range(339):
        offset=len(pool);pool+=f'x{i:03d}'.encode()+b'\0'
        struct.pack_into('<HH',names,4*i,offset,i)
        struct.pack_into('<H',records,56*i,i+1)
        struct.pack_into('<H',records,56*i+8,offset)
        struct.pack_into('<H',records,56*i+0x14,5)
        struct.pack_into('<H',records,56*i+0x20,i)
        struct.pack_into('<HH',numbers,48*i+0x14,3,1)
    for name,data in [('name-index.bin',names),('culture-records.bin',records),('number-records.bin',numbers),('sign-pool.bin',pool)]:
        (directory/name).write_bytes(data)
    for i,length in enumerate((2350,76,1018,32,52,94,262,174,212)):
        (directory/f'lower-{i}.bin').write_bytes(bytes(length))
    mapping={'schema':1,'entries':[{'name':f'neutral{i:03d}','lcid':i+1} for i in range(130)]}
    (directory/'neutral-map.json').write_text(json.dumps(mapping),encoding='utf8')
    return directory

class FixtureTests(unittest.TestCase):
    def test_inventory(self):
        with tempfile.TemporaryDirectory() as temp:
            d=fixture(Path(temp))
            self.assertEqual(len(list(d.iterdir())),14)
            self.assertEqual((d/'culture-records.bin').stat().st_size,339*56)

@unittest.skipUnless(EXE,'native integration registered in CTest')
class NativeTests(unittest.TestCase):
    def run_loader(self,root,name='X338',value='N42'):
        return subprocess.run([str(EXE),str(root),name,value],capture_output=True,text=True,timeout=20)
    def test_lookup_lowercase_signs_and_lifetime(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);fixture(root)
            result=self.run_loader(root)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual(result.stdout.strip(),'339 -42')
    def test_reject_incomplete_corrupt_inventory(self):
        mutations=[('lower-8.bin',b''),('name-index.bin',b'bad'),('neutral-map.json',b'{}'),
                   ('sign-pool.bin',b'\xff'),('number-records.bin',b'bad')]
        for name,data in mutations:
            with self.subTest(name=name),tempfile.TemporaryDirectory() as temp:
                root=Path(temp);d=fixture(root);(d/name).write_bytes(data)
                self.assertNotEqual(self.run_loader(root).returncode,0)
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);d=fixture(root);(d/'lower-0.bin').unlink()
            self.assertNotEqual(self.run_loader(root).returncode,0)
    def test_reject_duplicate_rows_and_unsorted_lcids(self):
        for name,offset,value in [('name-index.bin',6,0),('culture-records.bin',56,0),('culture-records.bin',56+0x20,0)]:
            with self.subTest(name=name,offset=offset),tempfile.TemporaryDirectory() as temp:
                root=Path(temp);d=fixture(root);path=d/name;data=bytearray(path.read_bytes());struct.pack_into('<H',data,offset,value);path.write_bytes(data)
                self.assertNotEqual(self.run_loader(root).returncode,0)
    def test_preserve_missing_neutral_target(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);d=fixture(root);path=d/'neutral-map.json';mapping=json.loads(path.read_text());mapping['entries'][0]['lcid']=9999;path.write_text(json.dumps(mapping))
            result=self.run_loader(root)
            self.assertEqual(result.returncode,0,result.stderr)

if __name__=='__main__':unittest.main()
