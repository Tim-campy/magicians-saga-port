"""Native town data/cache host contracts on synthetic tables, without game assets."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXE=Path(sys.argv.pop(1)).resolve() if len(sys.argv)>1 else None
sys.path.insert(0,str(Path(__file__).resolve().parent))
from test_sprite_preview import jar


@unittest.skipUnless(EXE,'saga_town supplied by CTest')
class TownProfileTests(unittest.TestCase):
    def fixture(self,root,flags=(1,0,1),capacity=3):
        (root/'textassets').mkdir()
        rows=[f'{i}\t{7+i}\t'+('0' if i!=1 else '1\t1\t99')+f'\t-1\t{flag}'
              for i,flag in enumerate(flags)]
        (root/'textassets/tables.bin').write_bytes(jar([('English.lproj/Town.txt','\n'.join(rows).encode())]))
        (root/'mission-resources.tsv').write_text('mission_resources 1\ntables -1 "textassets/tables.bin" "xls"\n')
        (root/'new-game-save.tsv').write_text(f'new_game_save 1\nselection 0 -1\ntown_capacity {capacity}\nwallet 37 43\n')

    def run_host(self,root,*args):
        return subprocess.run([str(EXE),'--data',str(root),*args],capture_output=True,text=True,timeout=15)

    def test_native_init_repair_and_cache_preserve_order_and_gaps(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            result=self.run_host(root);self.assertEqual(result.returncode,0,result.stderr)
            report=json.loads(result.stdout)
            self.assertEqual(report['entry'],'fresh-town-data')
            self.assertEqual((report['town_records'],report['initial_town_capacity'],report['town_capacity']),(3,3,3))
            self.assertEqual(report['build_enable'],[1,0,1])
            self.assertEqual(report['source_buildings'],[[0,7,0],[2,9,0],[1,8,0]])
            self.assertEqual(report['buildings'],[[0,7,0],[1,8,0],[2,9,0]])
            self.assertIn('residents, quest selection and mission return unbound',report['scope'])
            self.assertEqual(self.run_host(root).stdout,result.stdout)

    def test_native_init_raises_capacity_and_v2_has_priority(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root,flags=(1,1,1),capacity=0)
            (root/'mission-resources-v2.tsv').write_text('mission_resources 2\ntables -1 "textassets/tables.bin" "xls"\n')
            (root/'mission-resources.tsv').write_text('invalid ignored v1')
            result=self.run_host(root);self.assertEqual(result.returncode,0,result.stderr)
            report=json.loads(result.stdout)
            self.assertEqual((report['initial_town_capacity'],report['town_capacity']),(0,3))
            self.assertEqual(report['source_buildings'],report['buildings'])

    def test_missing_native_defaults_and_table_fail(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            defaults=root/'new-game-save.tsv';defaults.write_text('new_game_save 1 selection 0 -1')
            result=self.run_host(root);self.assertNotEqual(result.returncode,0)
            self.assertIn('invalid new-game save defaults',result.stderr)
            (root/'textassets/tables.bin').write_bytes(jar([('Other.txt',b'')]))
            result=self.run_host(root);self.assertNotEqual(result.returncode,0)
            self.assertIn('missing town table',result.stderr)

    def test_malformed_duplicate_and_escaping_index_fail(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            path=root/'mission-resources.tsv';valid=path.read_text()
            for invalid in (valid+'tables -1 "textassets/tables.bin" "xls"\n',
                            valid.replace('textassets/tables.bin','../tables.bin'),
                            valid.replace('mission_resources 1','mission_resources 3'),
                            valid+'resource 1 "textassets/a.bin" "a"\nresource 1 "textassets/b.bin" "b"\n'):
                with self.subTest(index=invalid):
                    path.write_text(invalid)
                    self.assertNotEqual(self.run_host(root).returncode,0)

    def test_options_require_explicit_valid_input(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            for args in (('--dpi','-1'),('--dpi','1extra'),('--unknown','0'),('--data',str(root))):
                self.assertNotEqual(self.run_host(root,*args).returncode,0)


if __name__=='__main__':unittest.main()
