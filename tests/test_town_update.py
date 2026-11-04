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
