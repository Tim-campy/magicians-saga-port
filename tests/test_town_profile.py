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
from town_fixture import add_town_background


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

    def test_background_prefix_uses_native_area_reset_and_explicit_frames(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root);add_town_background(root)
            result=self.run_host(root,'--background','--width','320','--height','200','--camera-y','0')
            self.assertEqual(result.returncode,0,result.stderr);report=json.loads(result.stdout);scene=report['background']
            self.assertEqual(scene['area_discovery'],[1,0]);self.assertEqual(scene['camera'],[0,0])
            self.assertEqual(scene['extents'],[-449,590]);self.assertEqual(scene['view_origin'],[0,53])
            self.assertEqual((scene['resource'],scene['image'],scene['clip']),(11,9,0))
            self.assertEqual(scene['fill_color'],0xff0a141e);self.assertEqual(scene['fill_size'],[320,200])
            self.assertEqual(scene['draws'],[[0,0,x,128] for x in (-449,-209,31,271,511)]+[[0,1,30,73]])
            self.assertEqual(self.run_host(root,'--background','--width','320','--height','200','--camera-y','0').stdout,result.stdout)
            # Native integer parallax follows the clamped camera, never a replacement current frame.
            shifted=self.run_host(root,'--background','--width','320','--height','200','--camera-x','-60','--camera-y','0')
            self.assertEqual(shifted.returncode,0,shifted.stderr)
            shifted=json.loads(shifted.stdout)['background'];self.assertEqual(shifted['camera'],[-60,0])
            self.assertEqual(shifted['draws'][:-1],scene['draws'][:-1]);self.assertEqual(shifted['draws'][-1],[0,1,28,73])

    def test_background_requires_reached_data_and_bounded_viewport(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            self.assertNotEqual(self.run_host(root,'--background').returncode,0)
            add_town_background(root)
            for args in (('--width','0'),('--height','1'),('--height','8193'),('--width','8192','--height','8192')):
                self.assertNotEqual(self.run_host(root,'--background',*args).returncode,0)
            town='\n'.join(f'{i}\t{7+i}\t0\t-1\t1' for i in range(3)).encode()
            (root/'textassets/tables.bin').write_bytes(jar([('English.lproj/Town.txt',town)]))
            result=self.run_host(root,'--background');self.assertNotEqual(result.returncode,0)
            self.assertIn('Area.txt',result.stderr)

    def test_scenery_preserves_separate_streams_and_native_cloud_prefix(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);add_town_background(root,scenery=True)
            args=('--scenery','--width','320','--height','200')
            first=self.run_host(root,*args);self.assertEqual(first.returncode,0,first.stderr)
            report=json.loads(first.stdout);scene=report['background']['scenery']
            # First public System.Random(42) values; distinct AppData/Lib owners.
            self.assertEqual(scene['clouds'][:2],[[-15,1434747710%31,14,0,1,1434747710%4,0,20],
                [4,302596119%31,14,0,1,269548474%4,0,20]])
            self.assertEqual(self.run_host(root,*args).stdout,first.stdout)
            changed=self.run_host(root,*args,'--lib-seed','123');self.assertEqual(changed.returncode,0,changed.stderr)
            other=json.loads(changed.stdout)['background']['scenery']
            self.assertEqual(scene['app_draws'],other['app_draws']);self.assertEqual(scene['lib_draws'],other['lib_draws'])
            self.assertEqual([row[0] for row in scene['clouds']],[row[0] for row in other['clouds']])
            self.assertEqual([row[5] for row in scene['clouds']],[row[5] for row in other['clouds']])
            self.assertNotEqual([row[1] for row in scene['clouds']],[row[1] for row in other['clouds']])
            self.assertTrue(any(row[0]==4 and row[1]==1 for row in scene['draws']))
            self.assertIn('floor, buildings, residents',report['scope'])
            self.assertNotEqual(self.run_host(root,'--background','--app-seed','42').returncode,0)


if __name__=='__main__':unittest.main()
