"""Full town controller, automatic events and modal pause on synthetic records."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXE=Path(sys.argv.pop(1)).resolve() if len(sys.argv)>1 else None
sys.path.insert(0,str(Path(__file__).resolve().parent))
from town_fixture import add_town_background


@unittest.skipUnless(EXE and os.environ.get('SAGA_TEST_FONT'),'CTest host and public font required')
class TownUpdateTests(unittest.TestCase):
    def fixture(self,root):
        add_town_background(root,update=True)
        font=root/'unity/Font/public.ttf';font.parent.mkdir(parents=True)
        font.write_bytes(Path(os.environ['SAGA_TEST_FONT']).read_bytes())
        (root/'mission-font.tsv').write_text(f'mission_font 1\nfont "unity/Font/public.ttf" {font.stat().st_size}\n')

    def run_host(self,root,n,*args):
        return subprocess.run([str(EXE),'--data',str(root),'--foreground','--update-passes',str(n),*args],capture_output=True,text=True,timeout=20)

    def state(self,root,n,*args):
        result=self.run_host(root,n,*args)
        self.assertEqual(result.returncode,0,result.stderr)
        return json.loads(result.stdout)['background']

    def test_automatic_selection_delay_and_native_parent_insertion(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            first=self.state(root,1)['update']
            self.assertEqual((first['calls'],first['base_update_counter'],first['delay_count']),(1,1,1))
            self.assertEqual(first['delays'],[1]);self.assertEqual(first['event_execution_counts'],[[0,1]])
            self.assertEqual(first['talk_forms_opened'],0)
            self.assertFalse(first['event_check_requested']);self.assertFalse(first['scene_resume'])
            self.assertEqual((first['normal_talk_character'],first['stamina'],first['platform_code']),(-7,10,2))
            self.assertEqual(first,self.state(root,1,'--motion-low-aspect','0')['update'])
            second=self.state(root,2)
            state=second['update']
            self.assertEqual(state['delay_count'],0)
            self.assertEqual(state['talk_forms_opened'],2);self.assertEqual(state['talk']['id'],0)
            self.assertEqual(state['talk_lifecycle'],['push:0','push:1'])
            self.assertTrue(state['auto_save']);self.assertEqual(state['save_requests'],0)
            self.assertEqual(second['foreground']['guide_flags'],[1,0])
            self.assertNotEqual(second['areas_draw']['marks'],[0]*len(second['areas_draw']['marks']))
            third=self.state(root,3)['update']
            self.assertEqual(third['talk_lifecycle'],['push:0','push:1','init:1','init:0','resume:0'])
            self.assertEqual((third['calls'],third['modal_dispatches'],third['talk']['frame']),(2,1,1))

    def test_modal_wait_freezes_town_motion_and_live_image_clocks(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            before=self.state(root,60);after=self.state(root,300)
            self.assertEqual(before['scenery'],after['scenery'])
            self.assertEqual(before['residents']['rows'],after['residents']['rows'])
            self.assertEqual(before['hero'],after['hero'])
            self.assertEqual(before['update']['image_clock_ticks'],after['update']['image_clock_ticks'])
            self.assertEqual(after['update']['calls'],2);self.assertEqual(after['update']['modal_dispatches'],298)
            self.assertEqual(after['update']['talk']['id'],0)
            # Existing resource clocks reach2; actors first loaded by draw1 reach1.
            clocks=after['update']['image_clock_ticks']
            self.assertTrue(clocks);self.assertTrue(all(t==2 for group in clocks[:3] for t in group))
            self.assertTrue(any(group and all(t==1 for t in group) for group in clocks[3:]))
            self.assertEqual(after,self.state(root,300))

    def test_explicit_dialogue_input_closes_in_order_and_resumes_parent(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            scene=self.state(root,160,'--talk-pulse-every','10');state=scene['update']
            self.assertEqual((state['talk_forms_opened'],state['talk_forms_closed']),(2,2))
            self.assertFalse(state['talk_pending']);self.assertFalse(state['auto_save'])
            self.assertEqual(state['save_requests'],1);self.assertGreater(state['calls'],2)
            self.assertEqual(state['calls']+state['modal_dispatches'],160)
            lifecycle=state['talk_lifecycle']
            self.assertLess(lifecycle.index('remove:0'),lifecycle.index('resume:1'))
            self.assertLess(lifecycle.index('resume:1'),lifecycle.index('remove:1'))
            self.assertFalse(state['scene_resume']);self.assertEqual(state['base_update_counter'],state['calls'])
            self.assertEqual(state['event_execution_counts'],[[0,1]])
            self.assertEqual(scene,self.state(root,160,'--talk-pulse-every','10'))

    def replay(self,root,text):
        path=root/'town.replay';path.write_text(text,encoding='ascii');return ('--replay',str(path),'--talk-pulse-every','10')

    def test_native_camera_walk_reaches_resident_and_selects_real_voice(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=self.replay(root,'3 camera 0 130 0 0 0 1\n160 camera 0 130 0 0 0 1\n161 camera 1 130 0 0 0 0\n')
            before=self.state(root,159,*args)
            self.assertEqual(before['hero']['current_x'],40)
            self.assertEqual(before['update']['replay_modal_ignored'],1)
            moving=self.state(root,160,*args)
            self.assertEqual((moving['hero']['current_x'],moving['hero']['target_x']),(46,130))
            self.assertTrue(moving['update']['touch']['enabled'])
            self.assertTrue(moving['update']['touch']['dragging'])
            self.assertEqual(moving['hero']['draws'][0][2],40)
            arrival=self.state(root,174,*args)
            self.assertEqual(arrival['hero']['current_x'],130)
            self.assertEqual(arrival['update']['touch']['collision'],0)
            self.assertFalse(arrival['update']['touch']['dragging'])
            bubble=arrival['update']['bubble']
            self.assertEqual((bubble['index'],bubble['frames']),(0,119))
            self.assertTrue(bubble['voice_catalog_bound']);self.assertIn(bubble['text_bytes'],(5,6))
            later=self.state(root,180,*args)
            self.assertEqual(later['update']['bubble']['frames'],113)
            self.assertEqual(later['update']['replay_applied'],2)
            self.assertEqual(later,self.state(root,180,*args))
            expired=self.state(root,294,*args)['update']['bubble']
            self.assertEqual((expired['index'],expired['frames']),(-1,-1))
            self.assertEqual(expired['text_bytes'],bubble['text_bytes'])

    def test_native_selection_masks_and_camera_drag_preserve_signed_rules(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=self.replay(root,'160 select 0xc0007 0\n161 select 0x22000b 1\n162 camera 0 40 0 0 0 1\n163 camera 8 40 -10 0 0 1\n164 camera 8 40 6 0 0 1\n165 camera 1 40 0 0 0 0\n')
            area=self.state(root,160,*args)
            self.assertEqual((area['areas_draw']['selected'],area['buildings_draw']['selected']),(0,-1))
            selected=self.state(root,161,*args)
            self.assertEqual((selected['areas_draw']['selected'],selected['buildings_draw']['selected']),(-1,1))
            left=self.state(root,163,*args)
            self.assertEqual(left['camera'][0],-10)
            self.assertEqual(left['buildings_draw']['selected'],1)
            right=self.state(root,164,*args)
            self.assertEqual(right['camera'][0],-4)
            self.assertEqual((right['areas_draw']['selected'],right['buildings_draw']['selected']),(-1,-1))
            released=self.state(root,165,*args)
            self.assertFalse(released['update']['touch']['dragging'])
            self.assertEqual(released['update']['touch']['history'],2)

    def test_replay_requires_ordered_bounded_native_records(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            for text in ('0 pulse\n','-18446744073709551615 pulse\n','2147483648 pulse\n','2 pulse\n1 pulse\n','1 camera 0 40\n','1 camera 0 40 0 0 0 -1\n','1 select 0x 0\n','1 buttons 4294967296\n','1 unknown\n','1 pulse extra\n'):
                with self.subTest(text=text):
                    args=self.replay(root,text);self.assertNotEqual(self.run_host(root,1,*args).returncode,0)

    def test_inputs_require_complete_typed_context(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            path=root/'mission-initialization.tsv';valid=path.read_text()
            for bad in (valid.replace('initialization 2','initialization 1').split('town_normal')[0],valid+'extra\n',valid.replace('town_normal 0 -7','town_normal 0')):
                path.write_text(bad);self.assertNotEqual(self.run_host(root,1).returncode,0)
            path.write_text(valid)
            for n,args in ((-1,()),(1,('--motion-passes','1')),(1,('--talk-pulse-every','0')),(1,('--top-form','0'))):
                self.assertNotEqual(self.run_host(root,n,*args).returncode,0)


if __name__=='__main__':unittest.main()
