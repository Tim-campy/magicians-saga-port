"""Full town controller, automatic events and modal pause on synthetic records."""
import importlib.util
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
from test_sprite_preview import jar


@unittest.skipUnless(EXE and os.environ.get('SAGA_TEST_FONT'),'CTest host and public font required')
class TownUpdateTests(unittest.TestCase):
    def fixture(self,root,stage_entries=False):
        add_town_background(root,update=True,stage_entries=stage_entries)
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

    def test_event_notice_uses_draw_clock_and_closes_without_text_reveal(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            spec=importlib.util.spec_from_file_location('notice_extractor',Path(__file__).resolve().parents[1]/'tools/extract_assets.py')
            extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
            path=root/'textassets/tables.bin';tables=dict(extractor.unpack_container(path.read_bytes()))
            tables['English.lproj/Event.txt']=b'0\tNotice\t0\t1\t3\t2\t0\t1\t1\t2\t12\t0'
            tables['English.lproj/EventMsg.txt']=b'0\t2\tSynthetic notice'
            path.write_bytes(jar(list(tables.items())))
            first=self.state(root,1)['update']
            self.assertEqual((first['talk_forms_opened'],first['talk_lifecycle']),(1,['push:0','reinsert:0']))
            before=self.state(root,17,'--talk-pulse-every','10')['update']
            self.assertEqual((before['event_notice']['frame'],before['event_notice']['page']),(16,0))
            ready=self.state(root,18,'--talk-pulse-every','10')['update']
            self.assertEqual(ready['event_notice']['frame'],17)
            closing=self.state(root,20,'--talk-pulse-every','10')['update']
            self.assertEqual((closing['event_notice']['page'],closing['talk']['close_counter']),(1,8))
            self.assertEqual(closing['event_notice']['frame'],18) # No end-page Draw while closing.
            closed=self.state(root,28,'--talk-pulse-every','10')['update']
            self.assertFalse(closed['talk_pending']);self.assertEqual(closed['talk_forms_closed'],1)
            self.assertEqual(closed['talk_lifecycle'].count('init:0'),1)
            self.assertGreater(closed['calls'],first['calls'])

    def test_immediate_automatic_talk_reinserts_same_form_without_second_init(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            spec=importlib.util.spec_from_file_location('town_test_extractor',Path(__file__).resolve().parents[1]/'tools/extract_assets.py')
            extractor=importlib.util.module_from_spec(spec);spec.loader.exec_module(extractor)
            path=root/'textassets/tables.bin'
            tables=dict(extractor.unpack_container(path.read_bytes()))
            # Keep Event0's eligible money term, replacing its delay/two talks
            # with one immediate opcode8. AppData.Push and SelectForm's returned
            # form path must both push this single factory-created identity.
            tables['English.lproj/Event.txt']=b'0\tIntro\t0\t1\t3\t2\t0\t1\t1\t2\t8\t0'
            path.write_bytes(jar(list(tables.items())))
            first=self.state(root,1)['update']
            self.assertEqual(first['event_execution_counts'],[[0,1]])
            self.assertEqual((first['delay_count'],first['talk_forms_opened'],first['talk_forms_closed']),(0,1,0))
            self.assertEqual(first['talk_lifecycle'],['push:0','reinsert:0'])
            self.assertTrue(first['talk_pending']);self.assertTrue(first['auto_save'])
            initialized=self.state(root,2)['update']
            self.assertEqual(initialized['talk_lifecycle'],['push:0','reinsert:0','init:0','resume:0'])
            self.assertEqual((initialized['calls'],initialized['modal_dispatches'],initialized['talk']['frame']),(1,1,1))
            before=self.state(root,60);paused=self.state(root,120)
            self.assertEqual(paused['update']['calls'],1)
            self.assertEqual(paused['update']['modal_dispatches'],119)
            self.assertEqual(paused['scenery'],before['scenery'])
            self.assertEqual(paused['residents']['rows'],before['residents']['rows'])
            self.assertEqual(paused['update']['image_clock_ticks'],before['update']['image_clock_ticks'])
            resumed=self.state(root,160,'--talk-pulse-every','10')['update']
            self.assertFalse(resumed['talk_pending'])
            self.assertEqual((resumed['talk_forms_opened'],resumed['talk_forms_closed']),(1,1))
            self.assertEqual(resumed['talk_lifecycle'].count('init:0'),1)
            self.assertEqual(resumed['talk_lifecycle'].count('remove:0'),1)
            self.assertEqual(resumed['event_execution_counts'],[[0,1]])
            self.assertGreater(resumed['calls'],1)
            self.assertEqual(resumed['calls']+resumed['modal_dispatches'],160)
            self.assertEqual(resumed['save_requests'],1)

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

    def test_physical_camera_region_capture_and_outside_release(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=(*self.replay(root,'3 pointer 0 130 550 0 0\n160 pointer 0 -1 550 0 0\n161 pointer 8 130 550 0 0\n162 pointer 0 130 550 0 0\n163 pointer 8 1000 550 1 0\n164 pointer 1 5000 5000 0 0\n'),'--physical-input')
            down=self.state(root,162,*args)
            self.assertEqual(down['hero']['target_x'],130)
            self.assertTrue(down['update']['pointer']['captured'])
            self.assertEqual(down['update']['pointer']['region'],[-1000,-1000,2000,2000])
            self.assertEqual(down['update']['pointer']['flags'],40)
            released=self.state(root,176,*args)
            pointer=released['update']['pointer']
            self.assertEqual((pointer['events'],pointer['rejected']),(2,4))
            self.assertFalse(pointer['captured']);self.assertEqual(pointer['region'],[0,0,600,640])
            self.assertEqual(released['hero']['current_x'],130)
            self.assertEqual(released['update']['bubble']['frames'],119)
            self.assertEqual(pointer['view_content_width'],600)
            self.assertEqual(released,self.state(root,176,*args))

    def test_modal_and_focus_policy_cancel_physical_capture(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=(*self.replay(root,'1 pointer 0 130 550 0 0\n'),'--physical-input')
            modal=self.state(root,2,*args)['update']
            self.assertTrue(modal['talk_pending']);self.assertFalse(modal['pointer']['captured'])
            self.assertEqual(modal['pointer']['cancels'],1);self.assertFalse(modal['touch']['dragging'])
            self.assertTrue(modal['touch']['enabled'])
            self.assertTrue(modal['scene_resume']) # Outer Draw sets it while the parent is hidden.
            args=(*self.replay(root,'160 pointer 0 130 550 0 0\n161 pointer cancel\n'),'--physical-input')
            focused=self.state(root,161,*args)['update']
            self.assertFalse(focused['pointer']['captured']);self.assertEqual(focused['pointer']['cancels'],1)
            self.assertTrue(focused['touch']['enabled'])

    def test_physical_drag_preserves_selection_sign_rule_and_release_inertia(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=(*self.replay(root,'161 select 0x220000 1\n162 pointer 0 40 550 0 0\n163 pointer 8 30 550 -10 0\n164 pointer 8 36 550 6 0\n165 pointer 1 5000 5000 0 0\n'),'--physical-input')
            left=self.state(root,163,*args)
            self.assertEqual(left['camera'][0],-10)
            self.assertEqual(left['buildings_draw']['selected'],1)
            right=self.state(root,164,*args)
            self.assertEqual(right['camera'][0],-4)
            self.assertEqual(right['buildings_draw']['selected'],-1)
            released=self.state(root,165,*args)
            self.assertEqual(released['camera'][0],1) # Last delta6 decays to5.4 before int32 movement.
            self.assertFalse(released['update']['pointer']['captured'])
            self.assertEqual(released['update']['pointer']['events'],4)
            self.assertEqual(released['update']['touch']['history'],2)

    def test_replay_requires_ordered_bounded_native_records(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            for text in ('0 pulse\n','-18446744073709551615 pulse\n','2147483648 pulse\n','2 pulse\n1 pulse\n','1 camera 0 40\n','1 camera 0 40 0 0 0 -1\n','1 select 0x 0\n','1 buttons 4294967296\n','1 unknown\n','1 pulse extra\n','1 pointer 12 0 0 0 0\n','1 pointer 0 40\n','1 pointer cancel extra\n','1 widget 0 200\n','1 widget 8 200 450\n','1 stage-back extra\n','1 stage-back\n'):
                with self.subTest(text=text):
                    args=self.replay(root,text);self.assertNotEqual(self.run_host(root,1,*args).returncode,0)

    def test_physical_area_go_opens_retained_menu_and_back_resumes_town(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            text='3 widget 0 200 450\n160 widget 0 200 450\n161 widget 1 200 450\n162 widget 0 200 480\n163 widget 1 200 480\n184 stage-back\n'
            args=(*self.replay(root,text),'--physical-input','--camera-x','300')
            selected=self.state(root,160,*args)
            self.assertEqual(selected['areas_draw']['selected'],0)
            self.assertIn([4,28],[r[:2] for r in selected['update']['widgets']['regions']])
            pushed=self.state(root,163,*args)['update']
            self.assertTrue(pushed['stage_menu']['pending']);self.assertFalse(pushed['stage_menu']['initialized'])
            opened=self.state(root,164,*args)['update']
            self.assertEqual(opened['system_flags'],256)
            self.assertEqual(opened['stage_menu']['normal_flags'],0)
            self.assertEqual(opened['stage_menu']['selected'],-1)
            paused=self.state(root,183,*args)['update'];menu=paused['stage_menu']
            self.assertEqual(paused['calls'],pushed['calls'])
            self.assertEqual((menu['frame'],menu['scroll_color'],menu['scroll_key_queries']),(20,128,120))
            self.assertEqual(menu['entries'],[]) # Empty lists still execute native geometry/fade queries.
            self.assertEqual(paused['widgets']['rejected'],1)
            closed=self.state(root,184,*args)['update']
            self.assertFalse(closed['stage_menu']['pending']);self.assertEqual(closed['stage_menu']['closed'],1)
            self.assertEqual(closed['calls'],paused['calls']+1)
            self.assertEqual(closed['save_requests'],paused['save_requests']+1)
            self.assertEqual(closed,self.state(root,184,*args)['update'])

    def test_inputs_require_complete_typed_context(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            path=root/'mission-initialization.tsv';valid=path.read_text()
            for bad in (valid.replace('initialization 2','initialization 1').split('town_normal')[0],valid+'extra\n',valid.replace('town_normal 0 -7','town_normal 0')):
                path.write_text(bad);self.assertNotEqual(self.run_host(root,1).returncode,0)
            path.write_text(valid)
            for n,args in ((-1,()),(1,('--motion-passes','1')),(1,('--talk-pulse-every','0')),(1,('--top-form','0'))):
                self.assertNotEqual(self.run_host(root,n,*args).returncode,0)

    def stage_args(self,root,extra=''):
        opening='160 widget 0 200 450\n161 widget 1 200 450\n162 widget 0 200 480\n163 widget 1 200 480\n'
        return (*self.replay(root,opening+extra),'--physical-input','--camera-x','300','--server-time','123','--stamina-up-state','0')

    def test_stage_row_down_selects_without_debit_and_cancel_keeps_selection(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root,stage_entries=True)
            args=self.stage_args(root,'165 widget 0 250 250\n166 widget 3 0 0\n')
            opened=self.state(root,164,*args)['update']
            self.assertEqual(opened['stage_menu']['entries'],[0])
            self.assertEqual(opened['stage_menu']['rows'],[[10,0x20000,195,232,210,40]])
            down=self.state(root,165,*args)['update'];menu=down['stage_menu']
            self.assertEqual((menu['selected'],menu['last_touched'],menu['state']),(0,0,0))
            self.assertTrue(menu['row_captured']);self.assertGreater(menu['text_draws'],opened['stage_menu']['text_draws'])
            self.assertEqual((down['stamina'],menu['stamina_uses'],menu['saved_quest']),(10,0,-1))
            cancelled=self.state(root,166,*args)['update'];menu=cancelled['stage_menu']
            self.assertFalse(menu['row_captured']);self.assertEqual(menu['selected'],0)
            self.assertEqual((cancelled['stamina'],menu['stamina_uses'],menu['server_queries']),(10,0,0))
            args=self.stage_args(root,'165 widget 0 250 250\n166 widget 1 -1 -1\n')
            outside=self.state(root,166,*args)['update'];menu=outside['stage_menu']
            self.assertFalse(menu['row_captured']);self.assertEqual((menu['selected'],menu['state']),(0,0))
            self.assertEqual((outside['stamina'],menu['stamina_uses'],menu['saved_quest']),(10,0,-1))

    def test_stage_confirmation_debits_once_then_holds_production41_handoff(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root,stage_entries=True)
            args=self.stage_args(root,'165 widget 0 250 250\n166 widget 1 250 250\n167 widget 1 250 250\n168 widget 0 250 250\n169 widget 1 250 250\n')
            confirmed=self.state(root,166,*args)['update'];menu=confirmed['stage_menu']
            self.assertEqual((menu['state'],menu['production_frame'],menu['saved_quest']),(1,0,0))
            self.assertEqual((confirmed['stamina'],menu['stamina_uses'],menu['stamina_timer'],menu['stamina_timestamp']),(9,1,900,123))
            self.assertEqual(menu['server_queries'],1);self.assertFalse(menu['quest_handoff_pending'])
            before=self.state(root,206,*args)['update']
            self.assertEqual(before['stage_menu']['production_frame'],40)
            # Production keeps the native row callback live; its Update ignores
            # the extra confirmation pulse rather than charging stamina again.
            self.assertEqual(before['stage_menu']['row_events'],4)
            self.assertFalse(before['stage_menu']['quest_handoff_pending'])
            held=self.state(root,207,*args)['update'];menu=held['stage_menu']
            self.assertEqual(menu['production_frame'],41);self.assertTrue(menu['quest_handoff_pending'])
            self.assertTrue(menu['camera_reset']);self.assertTrue(menu['shared_owner_retained'])
            self.assertEqual((held['stamina'],menu['stamina_uses'],menu['server_queries']),(9,1,1))
            self.assertEqual(menu['row_rejected'],1)
            self.assertEqual(held['save_requests'],before['save_requests']+1)
            later=self.state(root,300,*args)['update']
            self.assertEqual(later,held) # A held handoff stops dispatch, saves and consumption.

    def test_selected_stage_back_resumes_shared_town_without_spending(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root,stage_entries=True)
            args=self.stage_args(root,'165 widget 0 250 250\n166 stage-back\n')
            before=self.state(root,165,*args)['update'];after=self.state(root,166,*args)['update']
            menu=after['stage_menu'];self.assertFalse(menu['pending']);self.assertEqual(menu['closed'],1)
            self.assertFalse(menu['row_captured']) # Closing a form clears its live pointer capture.
            self.assertEqual((menu['selected'],menu['saved_quest'],menu['stamina_uses'],after['stamina']),(0,-1,0,10))
            self.assertEqual(after['calls'],before['calls']+1);self.assertFalse(menu['quest_handoff_pending'])

    def test_stage_projection_and_provider_inputs_are_bounded(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=self.stage_args(root);path=root/'stage-menu.tsv';valid=path.read_text()
            for bad in (valid.replace('stage_menu 1','stage_menu 2'),valid.replace('heading "Quests"\n',''),
                        valid+'extra "value"\n',valid.replace('", +17, , 4"','"unsupported"'),valid+' '*65536):
                with self.subTest(projection=bad[:80]):
                    path.write_text(bad);self.assertNotEqual(self.run_host(root,164,*args).returncode,0)
            path.unlink();self.assertNotEqual(self.run_host(root,164,*args).returncode,0)
            path.write_text(valid)
            for option,value in (('--menu-font','0'),('--menu-font','101'),('--menu-font','bad'),
                                 ('--stamina-up-state','2147483648'),('--server-time','9223372036854775808')):
                with self.subTest(option=option,value=value):
                    self.assertNotEqual(self.run_host(root,1,option,value).returncode,0)
            for option,value in (('--menu-font','10'),('--stamina-up-state','0'),('--server-time','123')):
                result=subprocess.run([str(EXE),'--data',str(root),'--foreground',option,value],capture_output=True,text=True,timeout=20)
                self.assertNotEqual(result.returncode,0);self.assertIn('stage providers require full town Update',result.stderr)


if __name__=='__main__':unittest.main()
