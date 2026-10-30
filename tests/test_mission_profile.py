"""Always-built headless host contracts on synthetic game records and SEBs."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXE = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None
sys.path.insert(0, str(Path(__file__).resolve().parent))
from mission_scene_fixture import make_table_scene, make_shoot_scene, with_mission_map, make_boss_scene, make_gauge_scene, make_native_entry_scene, make_data_entry_fixture, make_combat_regression_scene


@unittest.skipUnless(EXE, 'saga_mission supplied by CTest')
class MissionProfileTests(unittest.TestCase):
    def test_legacy_retry_snapshot_keeps_unsupported_lifecycle_guard(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'legacy.txt'
            text=make_boss_scene(root).replace('stage_loop 5000 0 20 hold','stage_loop 0 0 20 hold')
            scene.write_text(text,encoding='utf8')
            result=self.run_host(scene,frames=2)
            self.assertNotEqual(result.returncode,0)
            self.assertIn('retry/defeat form lifecycle and recovery',result.stderr)

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public font fixture supplied by CTest')
    def test_native_retry_choices_gate_world_and_use_live_wallet(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);make_data_entry_fixture(root,os.environ['SAGA_TEST_FONT'],quest_time_limit=0)
            seed=root/'new-game-save.tsv';original=seed.read_text();replay=root/'retry.replay'
            opening='28 pulse 1048576\n29 pulse 1048576\n30 pulse 1048576\n31 pulse 1048576\n'
            def run(frames,tail='',crystals=143,*extra):
                seed.write_text(original.replace('wallet 37 43',f'wallet 37 {crystals}'))
                replay.write_text(opening+tail,encoding='ascii')
                result=subprocess.run([str(EXE),'--data',str(root),'--quest','2','--frames',str(frames),'--headless','--replay',str(replay),*extra],capture_output=True,text=True,timeout=30)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            waiting=run(60)
            self.assertEqual(waiting['stage_modal_stack'],['retry','defeat'])
            self.assertFalse(waiting['stage_retried'] or waiting['stage_dialog_answer_received'])
            self.assertEqual(waiting['world_update_calls'],23)
            ignored=run(60,'41 attack 1\n42 click 1 2\n')
            self.assertEqual(ignored['world_update_calls'],23);self.assertEqual(ignored['replay_modal_ignored'],2)
            prefix='40 answer 0\n'
            choice=run(60,prefix);self.assertEqual(choice['stage_modal_stack'],['retry'])
            self.assertEqual(choice['wallet']['crystals'],143);self.assertEqual(choice['world_update_calls'],23)
            paid=run(41,prefix+'41 answer 0\n')
            self.assertTrue(paid['stage_retried']);self.assertIsNone(paid['retry_dialog'])
            self.assertEqual(paid['stage_modal_stack'],[]);self.assertEqual(paid['wallet']['crystals'],43)
            self.assertEqual(paid['positions'][0]['invincible_time'],89)
            self.assertGreater(paid['positions'][0]['mp'],waiting['positions'][0]['mp'])
            self.assertEqual(run(41,prefix+'41 answer 0\n'),paid)
            static=run(41,prefix+'41 answer 0\n',143,'--human-static-fps','17')
            self.assertEqual(static['retry_invincibility_frames'],51);self.assertEqual(static['positions'][0]['invincible_time'],50)
            declined=run(43,prefix+'41 answer 2\n')
            self.assertTrue(declined['transition_pending']);self.assertFalse(declined['stage_retried'])
            self.assertEqual(declined['wallet']['crystals'],143);self.assertEqual(declined['stage_state'],6)
            insufficient=run(41,prefix+'41 answer 0\n',43)
            self.assertEqual(insufficient['stage_modal_stack'],['retry','insufficient_crystals'])
            self.assertFalse(insufficient['stage_retried'] or insufficient['stage_dialog_answer_received'])
            notice=run(60,prefix+'41 answer 0\n42 answer 0\n',43)
            self.assertEqual(notice['stage_modal_stack'],['retry']);self.assertEqual(notice['wallet']['crystals'],43)
            self.assertEqual(notice['world_update_calls'],24)
            ad=run(60,prefix+'41 answer 1\n',43)
            self.assertTrue(ad['stage_retried']);self.assertEqual(ad['wallet']['crystals'],43)
            self.assertEqual(ad['stage_modal_stack'],['ad_request']);self.assertEqual(ad['positions'][0]['invincible_time'],89)
            # CheckCrystal uses the nominal100; the distinct SubCrystal call applies live Yugami scaling.
            initial=root/'mission-initialization.tsv';initial.write_text(initial.read_text().replace('yugami 10000','yugami 9999'))
            scaled=run(41,prefix+'41 answer 0\n');self.assertTrue(scaled['stage_retried'])
            self.assertEqual(scaled['wallet']['crystals'],0)

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public font fixture supplied by CTest')
    def test_native_notice_victory_and_result_modal_acknowledgement(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);make_data_entry_fixture(root,os.environ['SAGA_TEST_FONT'])
            replay=root/'forms.replay'
            def run(mask,ack=False):
                replay.write_text(f'1 keys 16\n1 pulse {mask}\n'+('11 answer 0\n12 keys 16\n' if ack else ''),encoding='ascii')
                result=subprocess.run([str(EXE),'--data',str(root),'--quest','2','--frames','12','--headless','--ui-debug','1','--replay',str(replay)],capture_output=True,text=True,timeout=30)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            for mask,kind in ((128,'defeat'),(256,'victory'),(32,'result')):
                with self.subTest(kind=kind):
                    waiting=run(mask);self.assertEqual(waiting['stage_modal_stack'],[kind])
                    self.assertEqual(waiting['world_update_calls'],1)
                    closed=run(mask,True);self.assertEqual(closed['stage_modal_stack'],[])
                    self.assertEqual(closed['last_world_passes'],[0,1,2]);self.assertEqual(closed['world_update_calls'],5)

    def test_shared_catalog_combat_regression(self):
        for physical in (False,True):
            with self.subTest(physical=physical),tempfile.TemporaryDirectory() as directory:
                root=Path(directory);scene=root/'combat.txt'
                scene.write_text(make_combat_regression_scene(root,physical),encoding='utf8')
                result=self.run_host(scene,frames=240)
                self.assertEqual(result.returncode,0,result.stderr);state=json.loads(result.stdout)
                self.assertTrue(state['positions'][1]['destroyed'])
                self.assertEqual(state['shots'],1);self.assertEqual(state['defeated_enemies'],1)
                self.assertEqual(state['saved_defeats'],[{'enemy_id':4,'count':5}])
                self.assertEqual([(d['type'],d['value']) for d in state['pending_drops']],[(0,10),(3,0)])
                self.assertEqual(state['projectiles'],[])
                self.assertEqual((state['emitter_entities'],state['garbage_entities']),(0,0))
                self.assertEqual(state['parameter_experience_pending'],0)
                self.assertEqual(state['positions'][0]['human_experience'],0)
                self.assertTrue(state['update_schedule_complete'] and state['collisions_enabled'])
                self.assertEqual(state['last_update_order'],[2,3,4,5,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,30,31])
                self.assertEqual(self.run_host(scene,frames=240).stdout,result.stdout)
                early=self.run_host(scene,frames=20);self.assertEqual(early.returncode,0,early.stderr)
                # Category1 retains the physical impact modifier; ordinary shots expire immediately.
                self.assertEqual(json.loads(early.stdout)['render_commands'],17 if physical else 14)
                landing=self.run_host(scene,frames=45);self.assertEqual(landing.returncode,0,landing.stderr)
                landing=json.loads(landing.stdout)
                self.assertEqual(landing['emitter_entities'],3)
                self.assertGreater(landing['garbage_entities'],0)
                # Landing at44 retains the corpse through53; native maxFrame10 destroys it at54.
                for frames,emitters,destroyed in ((43,0,False),(44,3,False),(53,3,False),(54,3,True)):
                    boundary=self.run_host(scene,frames=frames)
                    self.assertEqual(boundary.returncode,0,boundary.stderr);boundary=json.loads(boundary.stdout)
                    self.assertEqual((boundary['emitter_entities'],boundary['positions'][1]['destroyed']),(emitters,destroyed),frames)

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'), 'public font fixture supplied by CTest')
    def test_data_entry_has_no_scene_dependency_and_matches_native_control(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);control=make_data_entry_fixture(root,os.environ['SAGA_TEST_FONT'])
            def run(*extra,frames=22):
                return subprocess.run([str(EXE),'--data',str(root),'--quest','2','--frames',str(frames),'--headless',*extra],capture_output=True,text=True,timeout=30)
            result=run();self.assertEqual(result.returncode,0,result.stderr);data=json.loads(result.stdout)
            self.assertEqual(data['entry_source'],'data');self.assertTrue(data['bootstrap_context']['provisional'])
            self.assertEqual(data['bootstrap_context']['font_source'],'manifest-index')
            self.assertEqual(data['fps'],30);self.assertEqual(data['wallet'],{'money':37,'crystals':43})
            self.assertTrue(data['native_mission_state'] and data['native_party_formation'] and data['talk_pending'])
            self.assertEqual(data['event_execution_counts'],[[212,1]])
            self.assertEqual(run().stdout,result.stdout)
            scene=root/'control.txt';scene.write_text(control,encoding='utf8')
            reference=self.run_host(scene,'--data',str(root),fps='initial',debug='initial',frames=22)
            self.assertEqual(reference.returncode,0,reference.stderr);reference=json.loads(reference.stdout)
            for key,value in reference.items():
                if key!='entry_source':self.assertEqual(data[key],value,key)
            scene.unlink() # Data entry still works when the only diagnostic file is absent.
            self.assertEqual(run().stdout,result.stdout)
            explicit=run('--font',os.environ['SAGA_TEST_FONT'],'--width','640','--height','600','--tick-ms','33','--ui-debug','1','--short-words','1',frames=21)
            self.assertEqual(explicit.returncode,0,explicit.stderr);explicit=json.loads(explicit.stdout)
            self.assertEqual(explicit['viewport']['width'],640);self.assertEqual(explicit['bootstrap_context']['tick_ms'],33)
            self.assertTrue(explicit['bootstrap_context']['debug_controls'] and explicit['bootstrap_context']['short_words'])
            self.assertEqual(explicit['bootstrap_context']['font_source'],'explicit')
            for extra in (('--language','Japanese.lproj'),('--japanese','1'),('--ui-debug','2'),('--app-ratio','nan'),
                          ('--tick-ms','0'),('--kerning','2'),('--width','0'),('--depth-bits','16')):
                self.assertNotEqual(run(*extra).returncode,0,extra)
            index=root/'mission-font.tsv';valid=index.read_text()
            for bad in (valid+'font "unity/Font/other.ttf" 1\n',valid.replace('mission_font 1','mission_font 2'),
                        valid.replace('unity/Font/different-number.ttf','../outside.ttf'),valid.replace(str((root/'unity/Font/different-number.ttf').stat().st_size),'1')):
                index.write_text(bad);self.assertNotEqual(run().returncode,0)
            index.unlink();self.assertNotEqual(run().returncode,0)
            self.assertEqual(run('--font',os.environ['SAGA_TEST_FONT']).returncode,0)
            # Diagnostic input and native context options have separate ownership.
            scene.write_text(control,encoding='utf8')
            self.assertNotEqual(self.run_host(scene,'--data',str(root),'--width','640',fps=30).returncode,0)

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'), 'public font fixture supplied by CTest')
    def test_talk_uses_font_blocks_instead_of_provisional_lengths(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt';replay=root/'talk.replay'
            text=make_native_entry_scene(root,talk_events=True).replace('stage_loop quest 0 20 hold','stage_loop quest 0 0 hold')
            text=text.replace('stage_notifications 1 0 0 quest 1 0 -1','stage_notifications 1 0 0 quest 0 0 -1')
            font=Path(os.environ['SAGA_TEST_FONT']).resolve().as_posix()
            text+=f'system_construct empty_manager\nquest_frame 0 0\ntick_ms 40\nmission_events fresh null 0 0 0\ntalk_font "{font}" 100 100 100 0 0 0 0 0\n'
            def run(frames,script=None):
                scene.write_text(text,encoding='utf-8');args=[]
                if script is not None:replay.write_text(script,encoding='ascii');args=['--replay',str(replay)]
                result=self.run_host(scene,*args,frames=frames);self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            opened=run(22);self.assertTrue(opened['talk_font_bound']);self.assertGreater(opened['talk_font_measurements'],0)
            self.assertEqual(opened['talk_layout_samples'],[[4,0,11,28,2]])
            self.assertEqual(opened['talk']['reveal_length'],11);self.assertIn('FreeType metrics unverified',opened['talk']['layout_provider'])
            waiting=run(200);self.assertTrue(waiting['talk_pending']);self.assertEqual(waiting['world_update_calls'],22)
            script='28 pulse 1048576\n29 pulse 1048576\n30 pulse 1048576\n31 pulse 1048576\n'
            closed=run(39,script);self.assertFalse(closed['talk_pending']);self.assertEqual(closed['world_update_calls'],23)
            self.assertEqual([sample[2] for sample in closed['talk_layout_samples']],[11,11]);self.assertEqual(run(39,script),closed)
            for bad in (text+'talk_layout 4 2 100 100\n',text.replace('100 100 100 0 0 0 0 0','100 100 100 1 0 0 0 0')):
                scene.write_text(bad,encoding='utf-8');self.assertNotEqual(self.run_host(scene,frames=22).returncode,0)

    def test_stage_event_talk_holds_world_and_requires_page_confirmation(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt';replay=root/'talk.replay'
            text=make_native_entry_scene(root,talk_events=True).replace('stage_loop quest 0 20 hold','stage_loop quest 0 0 hold')
            text=text.replace('stage_notifications 1 0 0 quest 1 0 -1','stage_notifications 1 0 0 quest 0 0 -1')
            text+='system_construct empty_manager\nquest_frame 0 0\ntick_ms 40\nmission_events fresh null 0 0 0\ntalk_layout 4 2 100 100\n'
            scene.write_text(text,encoding='utf-8')
            def run(frames,replaying=False):
                result=self.run_host(scene,*(['--replay',str(replay)] if replaying else []),frames=frames)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            opened=run(22);self.assertTrue(opened['talk_pending']);self.assertEqual(opened['talk']['form_state'],0)
            self.assertEqual(opened['executed_stage_events'],[212]);self.assertEqual(opened['event_execution_counts'],[[212,1]])
            held=run(200);self.assertTrue(held['talk_pending']);self.assertEqual(held['talk']['page'],0)
            self.assertEqual(held['world_update_calls'],22);self.assertEqual(held['remaining_time'],5000)
            replay.write_text('24 attack 2\n25 touch 0 35 65553\n26 cursor 3 4\n27 click 5 6\n',encoding='ascii')
            blocked=run(30,True);baseline=run(30)
            for key in ['attack_buttons','attack_attribute','last_active_button','move_front_pushed','cursor','cursor_touches','cursor_clicks']:
                self.assertEqual(blocked[key],baseline[key],key)
            self.assertEqual(blocked['replay_modal_ignored'],4)
            # Init clears input, opening ignores input, then explicit flush/next/flush/close and eight Updates.
            replay.write_text('23 pulse 1048576\n24 pulse 1048576\n28 pulse 1048576\n29 pulse 1048576\n30 pulse 1048576\n31 pulse 1048576\n',encoding='ascii')
            flushing=run(28,True);self.assertEqual(flushing['talk']['page'],0);self.assertEqual(flushing['talk_flushes'],1)
            closing=run(31,True);self.assertEqual(closing['talk']['close_counter'],8);self.assertEqual(closing['talk_flushes'],2)
            self.assertTrue(run(38,True)['talk_pending'])
            resumed=run(39,True);self.assertFalse(resumed['talk_pending']);self.assertEqual(resumed['talk_forms_closed'],1)
            self.assertEqual(resumed['world_update_calls'],23);self.assertEqual(resumed['remaining_time'],4999)
            self.assertEqual(resumed['event_execution_counts'],[[212,1]])
            self.assertEqual(resumed['talk_lifecycle'],['push:4','init:4','resume:4','pop:4','finish:4','remove:4'])
            self.assertEqual(run(39,True),resumed)
            for bad in (text.replace('talk_layout 4 2 100 100\n',''),text.replace('talk_layout 4 2 100 100','talk_layout 4 1 100'),text.replace('mission_events fresh null 0 0 0','mission_events fresh missing 0 0 0')):
                scene.write_text(bad,encoding='utf-8');self.assertNotEqual(self.run_host(scene,frames=22).returncode,0)

    def test_artifact_initial_context_and_runtime_defaults_reach_native_mission(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt'
            original=make_native_entry_scene(root,talk_events=True).replace('stage_loop quest 0 20 hold','stage_loop quest 0 0 hold')+'system_construct empty_manager\nquest_frame 0 initial\n'
            fresh=original.replace('human 0 2 1 0 0 0 312 0 400\n','hero fresh\n').replace('player 0\n','').replace('system_init 1','system_init 0')
            assets=root/'textassets';assets.mkdir()
            for archive in root.glob('*.bin'):(assets/archive.name).write_bytes(archive.read_bytes())
            (root/'new-game-save.tsv').write_text('new_game_save 1\nselection 5 2\ntown_capacity 1\nwallet 37 43\n',encoding='ascii')
            context=root/'mission-initialization.tsv';payload='mission_initialization 1\nyugami 9999\norientation 2\nevent_arguments ""\n';context.write_text(payload,encoding='ascii')
            snapshots=('human_system ','rewards ','enemy_system ','enemy_population ','enemy_pool ','stage_notifications ',
                       'collection ','stage_loop ','stage_events ','input ','stage_boss ','party_formation ',
                       'quest ','stage_start ','result_wallet ','result_record ')
            text='\n'.join(line for line in fresh.splitlines() if not line.startswith(snapshots))+'\nnew_game_save\nmission_initialization\nmission_state new_game initial 0\nmission_events new_game 0 0 0\ntalk_layout 4 2 100 100\ntick_ms 40\n'
            text=text.replace('runtime 25 0 system 42','runtime initial initial system 42')
            def run(value,**kwargs):
                scene.write_text(value,encoding='utf8');return self.run_host(scene,'--data',str(root),frames=22,**kwargs)
            result=run(text,fps='initial',debug='initial');self.assertEqual(result.returncode,0,result.stderr);data=json.loads(result.stdout)
            self.assertEqual(data['fps'],30);self.assertEqual(data['debug'],0)
            self.assertTrue(data['mission_initialization'] and data['new_game_event_context'] and data['talk_pending'])
            self.assertEqual(data['event_save_string0_kind'],'empty')
            self.assertEqual(data['initial_system_yugami'],9999);self.assertEqual(data['quest_saved_orientation'],2)
            self.assertIsNone(data['application_orientation']);self.assertEqual(data['event_execution_counts'],[[212,1]])
            self.assertEqual(run(text,fps='initial',debug='initial').stdout,result.stdout)
            control=text.replace('mission_initialization\n','').replace('new_game initial 0','new_game 9999 0').replace('quest_frame 0 initial','quest_frame 0 2')
            control=control.replace('mission_events new_game 0 0 0','mission_events fresh "" 0 0 0').replace('runtime initial initial','runtime 30 0')
            reference=run(control,fps=30);self.assertEqual(reference.returncode,0,reference.stderr);reference=json.loads(reference.stdout)
            for key,value in reference.items():
                if key not in {'mission_initialization','new_game_event_context'}:self.assertEqual(data[key],value,key)
            for bad in (text.replace('mission_initialization\n',''),text+'mission_initialization\n',text.replace('new_game_save\n',''),text.replace('system_init 0','system_init 1')):
                self.assertNotEqual(run(bad,fps='initial',debug='initial').returncode,0)
            for bad in (payload+'extra\n',payload.replace('initialization 1','initialization 2'),payload.replace('yugami 9999','yugami 2147483648'),payload.replace('event_arguments ""','event_arguments null')):
                context.write_text(bad,encoding='ascii');self.assertNotEqual(run(text,fps='initial',debug='initial').returncode,0)
            context.unlink();self.assertNotEqual(run(text,fps='initial',debug='initial').returncode,0)

    def test_native_mission_state_uses_init_start_without_snapshot_overrides(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt'
            original=make_native_entry_scene(root,alternate_quest=True).replace('stage_loop quest 0 20 hold','stage_loop quest 0 0 hold')+'system_construct empty_manager\nquest_frame 0 0\n'
            fresh=original.replace('human 0 2 1 0 0 0 312 0 400\n','hero fresh\n').replace('player 0\n','').replace('system_init 1','system_init 0')
            assets=root/'textassets';assets.mkdir()
            for archive in root.glob('*.bin'):(assets/archive.name).write_bytes(archive.read_bytes())
            (root/'new-game-save.tsv').write_text('new_game_save 1\nselection 5 2\ntown_capacity 1\nwallet 37 43\n',encoding='ascii')
            snapshots=('human_system ','rewards ','enemy_system ','enemy_population ','enemy_pool ','stage_notifications ',
                       'collection ','stage_loop ','stage_events ','input ','stage_boss ','party_formation ',
                       'quest ','stage_start ','result_wallet ','result_record ')
            text='\n'.join(line for line in fresh.splitlines() if not line.startswith(snapshots))+'\nnew_game_save\nmission_state new_game 10000 0\n'
            def run(value,frames=21):
                scene.write_text(value,encoding='utf8');return self.run_host(scene,'--data',str(root),frames=frames)
            result=run(text);self.assertEqual(result.returncode,0,result.stderr);data=json.loads(result.stdout)
            self.assertTrue(data['native_mission_state'] and data['native_party_formation'] and data['original_world_entry'])
            self.assertEqual(data['new_game_enemy_records'],3)
            expected=[-1]*16;expected[10]=2;self.assertEqual(data['party_formation'],expected)
            self.assertEqual(data['attack_buttons'],[{'state':1,'frame':4},{'state':0,'frame':4},{'state':0,'frame':4}])
            self.assertEqual(data['remaining_time'],5000);self.assertEqual(data['max_z_rate'],95)
            self.assertEqual(data['input_held_keys'],0);self.assertEqual(data['saved_defeats'],[])
            self.assertEqual(run(text).stdout,result.stdout)
            # Same decoded inputs and native slot10 formation through the legacy explicit adapters.
            control='\n'.join(line for line in fresh.splitlines() if not line.startswith(('quest ','stage_start ','result_wallet ','result_record ','party_formation ')))+'\nnew_game_save\nparty_formation '+' '.join(map(str,expected))+'\n'
            control=control.replace('stage_notifications 1 0 0 quest 1 0 -1','stage_notifications 1 0 0 quest new_game 0 -1')
            control=control.replace('rewards 0 1 4','rewards 0 1 0')
            reference=run(control);self.assertEqual(reference.returncode,0,reference.stderr);reference=json.loads(reference.stdout)
            excluded={'native_mission_state','native_party_formation'}
            for key,value in reference.items():
                if key not in excluded:self.assertEqual(data[key],value,key)
            held=run(text.replace('new_game 10000 0','new_game 10000 131072'))
            self.assertEqual(held.returncode,0,held.stderr);self.assertEqual(json.loads(held.stdout)['input_held_keys'],131072)
            scene.write_text(text,encoding='utf8');replay=root/'native.replay';replay.write_text('25 attack 0\n',encoding='ascii')
            touched=self.run_host(scene,'--data',str(root),'--replay',str(replay),frames=30)
            self.assertEqual(touched.returncode,0,touched.stderr);self.assertEqual(json.loads(touched.stdout)['input_touches'],1)
            for suffix in ('human_system quest 0 95 0 0','rewards 0 1 0','enemy_population 0 0 1 0 0 100 0',
                           'stage_notifications 1 0 0 quest new_game 0 -1','collection fresh','stage_loop quest 0 0 hold',
                           'stage_events quest','input fresh 0 0 0 0 0 0 0 0 0','stage_boss quest','mission_state new_game 10000 0'):
                self.assertNotEqual(run(text+suffix+'\n').returncode,0,suffix)
            for bad in (text.replace('new_game_save\n',''),text.replace('quest_frame 0 0\n',''),text.replace('system_init 0','system_init 1'),
                        text.replace('new_game 10000 0','new_game 10000 4294967296'),text+'party_formation '+' '.join(map(str,expected))+'\n'):
                self.assertNotEqual(run(bad).returncode,0)

    def test_native_fresh_formation_survives_member_init_into_stage_start(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt'
            original=make_native_entry_scene(root).replace('stage_loop quest 0 20 hold','stage_loop quest 0 0 hold')+'system_construct empty_manager\nquest_frame 0 0\n'
            fresh=original.replace('human 0 2 1 0 0 0 312 0 400\n','hero fresh\n').replace('player 0\n','').replace('system_init 1','system_init 0')+'town_save19 0\n'
            snapshot='party_formation 2 3 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1'
            self.assertIn(snapshot,fresh);text=fresh.replace(snapshot,'party_formation new_game')
            def run(value):
                scene.write_text(value,encoding='utf8');return self.run_host(scene,frames=21)
            result=run(text);self.assertEqual(result.returncode,0,result.stderr);data=json.loads(result.stdout)
            self.assertTrue(data['native_party_formation'] and data['original_world_entry'])
            expected=[-1]*16;expected[10]=2;self.assertEqual(data['party_formation'],expected)
            self.assertEqual(len(data['entry_members']),1)
            self.assertEqual(data['sortie_counts'],[{'id':2,'count':8},{'id':3,'count':9}])
            # Compare against the same native formation supplied explicitly, including placement/RNG/render state.
            control=fresh.replace(snapshot,'party_formation '+' '.join(map(str,expected)))
            reference=run(control);self.assertEqual(reference.returncode,0,reference.stderr);reference=json.loads(reference.stdout)
            self.assertEqual({k:v for k,v in data.items() if k!='native_party_formation'},
                             {k:v for k,v in reference.items() if k!='native_party_formation'})
            self.assertEqual(run(text).stdout,result.stdout)
            for bad in (text+'party_formation new_game\n',text.replace('party_formation new_game','party_formation new_game 2'),
                        text.replace('system_init 0','system_init 1'),text.replace('system_construct empty_manager\n',''),
                        original.replace(snapshot,'party_formation new_game')):
                self.assertNotEqual(run(bad).returncode,0)

    def test_new_game_save_initializes_wallet_selection_town_and_quest_records(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt';seeds=root/'new-game-save.tsv'
            original=make_native_entry_scene(root,alternate_quest=True).replace('stage_loop quest 0 20 hold','stage_loop quest 0 0 hold')+'system_construct empty_manager\nquest_frame 0 0\n'
            fresh=original.replace('human 0 2 1 0 0 0 312 0 400\n','hero fresh\n').replace('player 0\n','').replace('system_init 1','system_init 0')
            assets=root/'textassets';assets.mkdir()
            for archive in root.glob('*.bin'):(assets/archive.name).write_bytes(archive.read_bytes())
            text='\n'.join(line for line in fresh.splitlines() if not line.startswith(('quest ','stage_start ','result_wallet ','result_record ')))+'\nnew_game_save\n'
            text=text.replace('stage_notifications 1 0 0 quest 1 0 -1','stage_notifications 1 0 0 quest new_game 0 -1')
            payload='new_game_save 1\nselection 5 2\ntown_capacity 99\nwallet 5000000000 -6000000000\n'
            seeds.write_text(payload,encoding='ascii');scene.write_text(text,encoding='utf-8')
            result=self.run_host(scene,'--data',str(root),frames=21)
            self.assertEqual(result.returncode,0,result.stderr);report=json.loads(result.stdout)
            self.assertTrue(report['new_game_save'] and report['original_world_entry'])
            self.assertEqual(report['new_game_quest_counts'],[2,0])
            self.assertEqual(report['wallet'],{'money':5000000000,'crystals':-6000000000})
            self.assertEqual(report['town_save19'],99);self.assertEqual(report['town_save19_writes'],0)
            self.assertEqual(report['decoded_mission_selection'],{'area_slot8':5,'quest_slot9':2,'effective_quest_slot9':2})
            self.assertFalse(report['quest_completion_byte']);self.assertEqual(report['rescued_prisoners'],0)
            self.assertTrue(all(value==0 for value in report['quest_record'].values()))
            self.assertEqual(self.run_host(scene,'--data',str(root),frames=21).stdout,result.stdout)
            overridden=self.run_host(scene,'--data',str(root),'--quest','3',frames=21)
            self.assertEqual(overridden.returncode,0,overridden.stderr)
            self.assertEqual(json.loads(overridden.stdout)['decoded_mission_selection']['effective_quest_slot9'],3)
            for bad in (text+'result_wallet 1 2 0\n',text+'result_record 0 0 0 0 0 0 0\n',text+'town_save19 0\n',text+'mission_selection 5 2 0\n',text+'new_game_save\n',
                        text.replace('quest new_game','quest 0'),text.replace('system_init 0','system_init 1'),text.replace('new_game_save\n','')):
                scene.write_text(bad,encoding='utf-8');self.assertNotEqual(self.run_host(scene,'--data',str(root),frames=21).returncode,0)
            scene.write_text(text,encoding='utf-8')
            for bad in (payload.replace('new_game_save 1','new_game_save 2'),payload.replace('5000000000','9223372036854775808'),payload.replace('selection 5 2','selection -1 2'),payload+'extra\n'):
                seeds.write_text(bad,encoding='ascii');self.assertNotEqual(self.run_host(scene,'--data',str(root),frames=21).returncode,0)
            seeds.unlink();self.assertNotEqual(self.run_host(scene,'--data',str(root),frames=21).returncode,0)

    def test_new_game_character_shop_state_replaces_saved_snapshot_rows(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt'
            original=make_native_entry_scene(root).replace('stage_loop quest 0 20 hold','stage_loop quest 0 0 hold')+'system_construct empty_manager\nquest_frame 0 0\n'
            fresh=original.replace('human 0 2 1 0 0 0 312 0 400\n','hero fresh\n').replace('player 0\n','').replace('system_init 1','system_init 0')+'town_save19 0\n'
            text='\n'.join(line for line in fresh.splitlines() if not line.startswith(('progress ','purchase ','sorties ','result_recruits ')))+'\ncharacter_shop_new_game 1\n'
            scene.write_text(text,encoding='utf-8');result=self.run_host(scene,frames=22)
            self.assertEqual(result.returncode,0,result.stderr);report=json.loads(result.stdout)
            self.assertTrue(report['character_shop_new_game'] and report['original_world_entry'])
            self.assertEqual(report['new_game_record_counts'],[[2,0],[8,0]])
            self.assertEqual(report['saved_characters'],[{'id':2,'level':99,'experience':0},{'id':3,'level':99,'experience':0}])
            self.assertEqual(report['positions'][0]['human_level'],1)
            self.assertEqual(report['recruit_changes'],[])
            self.assertEqual(self.run_host(scene,frames=22).stdout,result.stdout)
            for bad in (text+'progress 2 1 0\n',text+'purchase 0 0\n',text+'sorties 2 0\n',text+'result_recruits 0 0 0\n',text+'character_shop_new_game 1\n',text.replace('system_init 0','system_init 1')):
                scene.write_text(bad,encoding='utf-8');self.assertNotEqual(self.run_host(scene,frames=22).returncode,0)

    def test_new_game_gear_uses_record_levels_and_individual_inventory(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt'
            original=make_native_entry_scene(root).replace('stage_loop quest 0 20 hold','stage_loop quest 0 0 hold')+'system_construct empty_manager\nquest_frame 0 0\n'
            fresh=original.replace('human 0 2 1 0 0 0 312 0 400\n','hero fresh\n').replace('player 0\n','').replace('system_init 1','system_init 0')+'town_save19 0\n'
            text='\n'.join(line for line in fresh.splitlines() if not line.startswith(('gear_levels ','result_inventory ')))+'\ngear_new_game\n'
            scene.write_text(text,encoding='utf-8');result=self.run_host(scene,frames=22)
            self.assertEqual(result.returncode,0,result.stderr);report=json.loads(result.stdout)
            self.assertTrue(report['gear_new_game'] and report['original_world_entry'])
            self.assertEqual(report['new_game_gear_counts'],[[2,0,0],[1,0,0],[1,0,1]])
            self.assertEqual(report['result_gear_levels'],{'weapon':0,'hat':0,'cloth':0})
            # The initially owned hat belongs to its per-record baseline, not a fake reward change.
            self.assertEqual(report['inventory_changes'],[])
            self.assertEqual(self.run_host(scene,frames=22).stdout,result.stdout)
            for bad in (text+'gear_levels hat 1\n',text+'result_inventory hat 0 0 0\n',text+'gear_new_game\n',text.replace('system_init 0','system_init 1')):
                scene.write_text(bad,encoding='utf-8');self.assertNotEqual(self.run_host(scene,frames=22).returncode,0)

    def test_fresh_world_initializers_create_hero_and_town_from_tables(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt'
            original=make_native_entry_scene(root).replace('stage_loop quest 0 20 hold','stage_loop quest 0 0 hold')+'system_construct empty_manager\nquest_frame 0 0\n'
            fresh=original.replace('human 0 2 1 0 0 0 312 0 400\n','hero fresh\n').replace('player 0\n','').replace('system_init 1','system_init 0')+'town_save19 0\n'
            self.assertIn('hero fresh\n',fresh)
            def run(text,frames=22):
                scene.write_text(text,encoding='utf-8');return self.run_host(scene,frames=frames)
            r=run(fresh);self.assertEqual(r.returncode,0,r.stderr);data=json.loads(r.stdout)
            self.assertTrue(data['fresh_hero_bootstrap']);self.assertTrue(data['fresh_town_initialized'])
            self.assertTrue(data['original_world_entry'])
            self.assertEqual(data['world_entry_order'],['create_systems','add_systems','create_map','init_world'])
            self.assertEqual(data['town_build_enable'],[1,0,1]);self.assertEqual(data['town_buildings'],[[0,7,0],[2,9,0]])
            self.assertEqual(data['town_save19'],2);self.assertEqual(data['town_save19_writes'],1)
            self.assertEqual(data['positions'][0]['human_level'],1);self.assertEqual(data['positions'][0]['human_experience'],0)
            self.assertEqual(data['stage_state'],1);self.assertEqual(data['remaining_time'],4999)
            self.assertEqual(json.loads(run(fresh).stdout),data)
            high=run(fresh.replace('town_save19 0','town_save19 99'));self.assertEqual(high.returncode,0,high.stderr)
            high=json.loads(high.stdout);self.assertEqual(high['town_save19'],99);self.assertEqual(high['town_save19_writes'],0)
            for bad in (fresh.replace('town_save19 0\n',''),fresh.replace('system_construct empty_manager\n',''),
                        fresh.replace('system_init 0','system_init 1'),fresh+'player 0\n',fresh+'hero fresh\n',
                        fresh.replace('progress 2 1 0\n',''),original+'town_save19 0\n'):
                self.assertNotEqual(run(bad).returncode,0)

    def test_saved_selection_uses_global_quest_id_for_both_quest_and_stage(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt'
            original=make_native_entry_scene(root,alternate_quest=True).replace('stage_loop quest 0 20 hold','stage_loop quest 0 0 hold')+'system_construct empty_manager\nquest_frame 0 0\n'
            base=original.replace('quest 2 0\n','').replace('stage_start 0 null\n','')
            def run(text,*args):
                scene.write_text(text,encoding='utf-8');return self.run_host(scene,*args,frames=22)
            native=base+'mission_selection 5 2 0\n'
            manual=original.replace('stage_start 0 null','stage_start 2 area 5')
            a=run(native);b=run(manual)
            self.assertEqual(a.returncode,0,a.stderr);self.assertEqual(b.returncode,0,b.stderr)
            data=json.loads(a.stdout);control=json.loads(b.stdout)
            self.assertTrue(data['native_mission_selection']);self.assertEqual(data['stage_entry_stage'],2)
            self.assertEqual(data['decoded_mission_selection'],{'area_slot8':5,'quest_slot9':2,'effective_quest_slot9':2})
            for key in control.keys()-{'native_mission_selection'}:self.assertEqual(data[key],control[key],key)
            changed=run(native,'--quest','3');expected=run(original.replace('quest 2 0','quest 3 0').replace('stage_start 0 null','stage_start 3 area 5'))
            self.assertEqual(changed.returncode,0,changed.stderr);self.assertEqual(expected.returncode,0,expected.stderr)
            data=json.loads(changed.stdout);control=json.loads(expected.stdout)
            self.assertEqual(data['stage_entry_stage'],3);self.assertEqual(data['decoded_mission_selection']['effective_quest_slot9'],3)
            for key in control.keys()-{'native_mission_selection'}:self.assertEqual(data[key],control[key],key)
            # A valid cached area index can hold null; StartExplore checks only the array length.
            gap=run(base+'mission_selection 0 2 0\n');self.assertEqual(gap.returncode,0,gap.stderr)
            self.assertIsNone(json.loads(gap.stdout)['stage_area'])
            for suffix in ('mission_selection -1 2 0\n','mission_selection 99 2 0\n','mission_selection 5 -1 0\n',
                           'mission_selection 5 1 0\n','mission_selection 5 99 0\n','mission_selection 5 2\n',
                           'mission_selection 5 2 0 extra\n','mission_selection 5 2 0\nquest 2 0\n',
                           'mission_selection 5 2 0\nstage_start 2 area 5\n','stage_start 2 area 5\nmission_selection 5 2 0\n',
                           'mission_selection 5 2 0\nmission_selection 5 3 0\n'):
                self.assertNotEqual(run(base+suffix).returncode,0,suffix)

    def test_native_area_entry_retains_real_record_and_explicit_stage(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt';text=make_native_entry_scene(root)
            scene.write_text(text,encoding='utf-8');result=self.run_host(scene,frames=1)
            self.assertEqual(result.returncode,0,result.stderr);baseline=json.loads(result.stdout)
            self.assertIsNone(baseline['stage_area']);self.assertEqual(baseline['stage_entry_stage'],0)
            def run(stage,area):
                scene.write_text(text.replace('stage_start 0 null',f'stage_start {stage} area {area}'),encoding='utf-8')
                return self.run_host(scene,frames=1)
            result=run(37,5);self.assertEqual(result.returncode,0,result.stderr);data=json.loads(result.stdout)
            self.assertEqual(data['stage_area'],{'id':5,'image':17,'flags':7,'stages':[2,3]})
            self.assertEqual(data['stage_entry_stage'],37) # Start stores the argument without inferring an index into Area.stages.
            for key in baseline.keys()-{'stage_area','stage_entry_stage'}:self.assertEqual(data[key],baseline[key],key)
            result=run(-7,9);self.assertEqual(result.returncode,0,result.stderr);data=json.loads(result.stdout)
            self.assertEqual(data['stage_entry_stage'],-7);self.assertEqual(data['stage_area'],{'id':9,'image':18,'flags':4,'stages':None})
            for area in (-1,0,6,10,'bad','5 extra',''):
                self.assertNotEqual(run(0,area).returncode,0,area)
            # Legacy null-area profiles need no Area table; selecting an area requires it explicitly.
            make_boss_scene(root)
            missing=run(0,5);self.assertNotEqual(missing.returncode,0);self.assertIn('missing gameplay table: Area.txt',missing.stderr)

    def test_native_quest_frames_intro_and_fast_forward_passes(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt';replay=root/'frames.txt'
            text=make_native_entry_scene(root).replace('stage_loop quest 0 20 hold','stage_loop quest 0 0 hold')+'system_construct empty_manager\n'
            scene.write_text(text+'quest_frame 0 0\n',encoding='utf-8')
            replay.write_text('1 keys 16\n',encoding='ascii')
            def run(frames):
                r=self.run_host(scene,'--replay',str(replay),frames=frames)
                self.assertEqual(r.returncode,0,r.stderr);return json.loads(r.stdout)
            before=run(20);self.assertEqual(before['stage_state'],0);self.assertEqual(before['remaining_time'],5000)
            self.assertEqual(before['quest_form_fade_frame'],20);self.assertEqual(before['world_update_calls'],20)
            intro=run(21);self.assertEqual(intro['stage_state'],1);self.assertEqual(intro['quest_form_fade_frame'],0)
            self.assertFalse(intro['quest_form_result']);self.assertEqual(intro['quest_form_fade_type'],-1)
            after=run(22);self.assertEqual(after['frames'],22);self.assertEqual(after['remaining_time'],4999)
            self.assertEqual(after['quest_form_update'],21);self.assertEqual(after['quest_form_frame'],22)
            self.assertEqual(after['image_clock_resets'],1);self.assertEqual(after['image_clock_updates'],22)
            scene.write_text(text+'quest_frame 1 0\n',encoding='utf-8')
            fast=run(22)
            self.assertEqual(fast['world_update_calls'],66);self.assertEqual(fast['stage_steps'],66)
            self.assertEqual(fast['last_world_passes'],[0,1,2]);self.assertEqual(fast['input_steps'],22)
            self.assertEqual(fast['camera_steps'],22);self.assertEqual(fast['cell_steps'],66)
            self.assertEqual(fast['remaining_time'],4995);self.assertEqual(fast['image_clock_updates'],22)
            self.assertEqual(fast['last_update_order'].count(21),1);self.assertEqual(fast['last_update_order'].count(19),3)
            replay.write_text('1 keys 16\n23 pulse 33554432\n24 pulse 134217728\n27 answer -1\n28 pulse 67108864\n',encoding='ascii')
            waiting=run(26)
            self.assertTrue(waiting['exit_dialog_pending']);self.assertEqual(waiting['last_world_passes'],[0])
            self.assertEqual(waiting['world_update_calls'],72);self.assertEqual(waiting['input_steps'],26)
            resumed=run(28)
            self.assertFalse(resumed['init_manager_paused']);self.assertEqual(resumed['stage_state'],1)
            self.assertEqual(resumed['world_update_calls'],78);self.assertEqual(resumed['input_steps'],28)
            self.assertEqual(resumed['movement_steps'],69);self.assertEqual(resumed['battle_steps'],69)
            self.assertEqual(resumed['remaining_time'],4992);self.assertEqual(resumed['last_world_passes'],[0,1,2])
            self.assertEqual(run(28),resumed)
            for bad in [text+'quest_frame 2 0\n',text+'quest_frame 0 0 extra\n',text+'quest_frame 0 0\nquest_frame 1 0\n',
                        text.replace('system_construct empty_manager\n','')+'quest_frame 0 0\n',
                        text.replace('stage_loop quest 0 0 hold','stage_loop quest 0 20 hold')+'quest_frame 0 0\n']:
                scene.write_text(bad,encoding='utf-8');self.assertNotEqual(self.run_host(scene).returncode,0)

    def test_factory_construction_runs_enable_hooks_before_init_and_keeps_menu_duplicates(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt';replay=root/'construct.txt'
            text=make_native_entry_scene(root)
            scene.write_text(text,encoding='utf-8');baseline=self.run_host(scene,frames=1)
            self.assertEqual(baseline.returncode,0,baseline.stderr);old=json.loads(baseline.stdout)
            scene.write_text(text+'system_construct empty_manager\n',encoding='utf-8')
            replay.write_text('2 touch 5 35 0\n3 pulse 33554432\n4 touch 5 35 0\n6 pulse 67108864\n7 touch 5 35 0\n',encoding='ascii')
            def run(frames):
                result=self.run_host(scene,'--replay',str(replay),frames=frames)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            first=run(1)
            self.assertFalse(first['original_world_entry']);self.assertEqual(first['world_entry_order'],[])
            self.assertTrue(first['system_construction_complete']);self.assertEqual(first['construction_order'],list(range(32)))
            self.assertTrue(first['construction_pause_before_init']);self.assertFalse(first['init_manager_paused'])
            self.assertEqual(first['pause_requests'],1);self.assertTrue(all(first['system_enabled']))
            self.assertFalse(first['menu_visible']);self.assertEqual(first['menu_button_receivers'],2)
            self.assertEqual(first['button_receiver_order'],[28,25,27,28])
            self.assertTrue(first['debug_path_visualize']);self.assertEqual(first['debug_path_wait_ms'],0)
            for key in ('positions','entry_members','sortie_counts','remaining_time','rng_draws','shots'):
                self.assertEqual(first[key],old[key],key)
            paused=run(5)
            self.assertTrue(paused['init_manager_paused']);self.assertEqual(paused['menu_button_receivers'],1)
            self.assertEqual(paused['button_receiver_order'],[25,27,28])
            self.assertEqual(paused['menu_button_events'],3);self.assertEqual(paused['info_button_events'],2)
            resumed=run(7)
            self.assertFalse(resumed['init_manager_paused']);self.assertEqual(resumed['menu_button_receivers'],2)
            self.assertEqual(resumed['button_receiver_order'],[25,27,28,28])
            self.assertEqual(resumed['menu_button_events'],5);self.assertEqual(resumed['submenu_button_events'],3)
            self.assertEqual(run(7),resumed)
            for bad in [text+'system_construct restore\n',text+'system_construct empty_manager extra\n',
                        text+'system_construct empty_manager\nsystem_construct empty_manager\n',
                        text.replace('stage_start 0 null\n','')+'system_construct empty_manager\n',
                        text+'system_construct empty_manager\nsystem_enabled 9 0\n',
                        text+'system_construct empty_manager\nmenu_visible 1\n',text+'menu_visible 2\n',
                        text+'menu_visible 0\nmenu_visible 1\n']:
                scene.write_text(bad,encoding='utf-8');self.assertNotEqual(self.run_host(scene).returncode,0)
            scene.write_text(text+'menu_visible 0\n',encoding='utf-8')
            legacy=self.run_host(scene,'--replay',str(replay),frames=7)
            self.assertEqual(legacy.returncode,0,legacy.stderr);self.assertFalse(json.loads(legacy.stdout)['menu_visible'])

    def test_native_pause_resume_and_explicit_exit_answers(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt';replay=root/'pause.txt'
            text=make_boss_scene(root)+('system_init 1\ncollisions\ninfo_popup 1 0 0 5 0 20 -1\ninfo_popup 2 1 0 3 0 12 -1\n'
                                       'result_wallet 10 20 4\nresult_record 0 0 0 0 0 0 0\n'
                                       'result_inventory equipment 0 0 0\nresult_inventory cloth 0 0 0\n'
                                       'result_inventory hat 0 0 0\nresult_recruits 0 0 0\n')
            scene.write_text(text,encoding='utf-8')
            replay.write_text('2 pulse 33554432\n3 touch 5 99 123\n3 click 0 0\n7 pulse 67108864\n8 touch 5 99 123\n'
                              '9 pulse 33554432\n10 pulse 134217728\n12 pulse 67108864\n14 answer -1\n15 pulse 67108864\n'
                              '16 pulse 33554432\n17 pulse 134217728\n20 answer 0\n',encoding='ascii')
            def run(frames):
                result=self.run_host(scene,'--replay',str(replay),frames=frames)
                self.assertEqual(result.returncode,0,result.stderr)
                return json.loads(result.stdout)
            paused=run(6)
            self.assertTrue(paused['init_manager_paused']);self.assertEqual(paused['stage_state'],2)
            self.assertEqual(paused['pause_requests'],1);self.assertEqual(paused['movement_steps'],1)
            self.assertEqual(paused['battle_steps'],2);self.assertEqual(paused['input_steps'],6)
            self.assertEqual(paused['remaining_time'],4998);self.assertEqual(paused['info_steps'],6)
            self.assertEqual(paused['info_popup_disposals'],0);self.assertEqual(paused['info_popups'][0]['state'],2)
            self.assertEqual(paused['submenu_button_events'],1);self.assertEqual(paused['menu_button_events'],0)
            cohort={2,4,5,9,10,11,13,14,15,19,23,28}
            self.assertEqual(paused['system_enabled'],[i not in cohort for i in range(32)])
            self.assertEqual(paused['last_update_order'],[0,3,8,12,16,17,18,20,21,22,24,25,26,30,31])
            resumed=run(7)
            self.assertFalse(resumed['init_manager_paused']);self.assertEqual(resumed['movement_steps'],2)
            self.assertEqual(resumed['battle_steps'],2);self.assertEqual(resumed['remaining_time'],paused['remaining_time'])
            self.assertEqual(run(8)['menu_button_events'],1)
            waiting=run(13)
            self.assertEqual(waiting['info_popup_disposals'],1);self.assertEqual([p['id'] for p in waiting['info_popups']],[2])
            self.assertTrue(waiting['exit_dialog_pending']);self.assertEqual(waiting['retry_dialog']['kind'],99)
            self.assertFalse(waiting['transition_pending']);self.assertEqual(waiting['exit_dialog_answers'],0)
            self.assertEqual(waiting['exit_dialog_wait_steps'],3)
            cancelled=run(14)
            self.assertFalse(cancelled['exit_dialog_pending']);self.assertIsNone(cancelled['retry_dialog'])
            self.assertEqual(cancelled['stage_state'],2);self.assertTrue(cancelled['init_manager_paused'])
            self.assertFalse(cancelled['transition_pending']);self.assertEqual(cancelled['exit_dialog_answers'],1)
            finished=run(25)
            self.assertTrue(finished['transition_pending']);self.assertEqual(finished['exit_dialog_answers'],2)
            self.assertEqual(finished['frames'],20);self.assertEqual(finished['replay_actions'],13)
            self.assertFalse(finished['result_granted']);self.assertEqual(finished['quest_record']['challenges'],1)
            self.assertEqual(finished['quest_record']['clears'],0);self.assertEqual(finished['inventory_changes'],[])
            self.assertEqual(finished['notification_events'].count('make_exit'),2)
            self.assertIn('answer_exit:-1',finished['notification_events']);self.assertIn('answer_exit:0',finished['notification_events'])
            self.assertEqual(json.loads(self.run_host(scene,'--replay',str(replay),frames=25).stdout),finished)

    def test_pause_resume_enables_cohort_without_restoring_old_flags(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt';replay=root/'pause.txt'
            text=make_boss_scene(root)+'system_init 1\nsystem_enabled 9 0\nsystem_enabled 22 0\n'
            scene.write_text(text,encoding='utf-8');replay.write_text('2 pulse 33554432\n4 pulse 67108864\n',encoding='ascii')
            result=self.run_host(scene,'--replay',str(replay),frames=5)
            self.assertEqual(result.returncode,0,result.stderr);d=json.loads(result.stdout)
            self.assertTrue(d['system_enabled'][9]);self.assertFalse(d['system_enabled'][22])
            self.assertEqual(d['parameter_experience_steps'],2);self.assertEqual(d['gauge_steps'],0)
            for bad in ['system_enabled 9 0\n','system_enabled 32 0\n','system_enabled -1 1\n',
                        'system_enabled 10 2\n','system_enabled 10 0 extra\n']:
                scene.write_text(text+bad,encoding='utf-8');self.assertNotEqual(self.run_host(scene).returncode,0)
            scene.write_text(text.replace('system_init 1\n',''),encoding='utf-8')
            self.assertIn('requires system_init',self.run_host(scene).stderr)

    def test_dialog_answers_require_a_pending_dialog_and_signed_int32(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt';replay=root/'answer.txt'
            text=make_boss_scene(root)+'system_init 1\n';scene.write_text(text,encoding='utf-8')
            for bad in ['1 answer 0\n','1 answer -2147483649\n','1 answer 2147483648\n','1 answer 0 extra\n',
                        '2 pulse 33554432\n3 pulse 134217728\n4 answer 1\n4 answer 0\n']:
                replay.write_text(bad,encoding='ascii');self.assertNotEqual(self.run_host(scene,'--replay',str(replay),frames=6).returncode,0)
            replay.write_text('2 pulse 33554432\n3 pulse 134217728\n4 answer 1\n5 pulse 67108864\n',encoding='ascii')
            ok=self.run_host(scene,'--replay',str(replay),frames=6)
            self.assertEqual(ok.returncode,0,ok.stderr);d=json.loads(ok.stdout)
            self.assertFalse(d['exit_dialog_pending']);self.assertIsNone(d['retry_dialog']);self.assertEqual(d['stage_state'],1)
            scene.write_text(text.replace('system_init 1\n',''),encoding='utf-8')
            self.assertIn('answer replay requires',self.run_host(scene,'--replay',str(replay)).stderr)

    def test_native_presentation_and_viewport_replay(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt';replay=root/'resize.txt'
            original=make_boss_scene(root)+'system_init 1\n'
            scene.write_text(original,encoding='utf-8');baseline=self.run_host(scene,frames=12)
            self.assertEqual(baseline.returncode,0,baseline.stderr)
            text='mission_presentation 128 96 100 0 1 0 1500100 -1\n'+'\n'.join(l for l in original.splitlines() if not l.startswith(('world ','projection ','slots ','view ','animate ','simulate ')))+'\n'
            scene.write_text(text,encoding='utf-8');result=self.run_host(scene,frames=12)
            self.assertEqual(result.returncode,0,result.stderr);a=json.loads(baseline.stdout);b=json.loads(result.stdout)
            for key in a.keys()&b.keys():self.assertEqual(a[key],b[key],key)
            self.assertEqual((b['viewport']['width'],b['viewport']['height'],b['viewport']['changes']),(128,96,0))
            replay.write_text('2 viewport 161 99\n8 viewport 128 96\n',encoding='ascii')
            resized=self.run_host(scene,'--replay',str(replay),frames=12)
            self.assertEqual(resized.returncode,0,resized.stderr);self.assertEqual(json.loads(resized.stdout)['viewport']['changes'],2)
            self.assertEqual(self.run_host(scene,'--replay',str(replay),frames=12).stdout,resized.stdout)
            for bad in [text+'world 1 0 0 0 1 0 0 0 1\n',text.replace('system_init 1\n',''),
                        text.replace('128 96 100','128 1 100'),text+'slots 35\n']:
                scene.write_text(bad,encoding='utf-8');self.assertNotEqual(self.run_host(scene).returncode,0)
    def test_resource_index_replaces_manual_archive_and_alias_bindings(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt';text=make_gauge_scene(root)
            scene.write_text(text,encoding='utf-8');control=self.run_host(scene,frames=12)
            self.assertEqual(control.returncode,0,control.stderr)
            data=root/'data';assets=data/'textassets';assets.mkdir(parents=True)
            for path in root.glob('*.bin'):(assets/path.name).write_bytes(path.read_bytes())
            entries=[(-1,'tables','xls'),(1,'hero','chara'),(3,'enemy','enemy-book'),(7,'quest','quest'),
                     (18,'hero','chip'),(19,'bullet','bullet'),(20,'effect','effect'),(21,'hero','hat'),
                     (22,'hero','weapon'),(23,'gauges','gauge')]
            index=data/'mission-resources.tsv'
            content='mission_resources 1\n'+''.join(f'{"tables" if i==-1 else "resource"} {i} "textassets/{file}.bin" "{name}"\n' for i,file,name in entries)
            index.write_text(content,encoding='utf-8')
            automatic='mission_resources "English.lproj" 0\n'+'\n'.join(l for l in text.splitlines() if not l.startswith(('tables ','resource ','resource_name ','global_alias ')))+'\n'
            scene.write_text(automatic,encoding='utf-8')
            result=self.run_host(scene,'--data',str(data),frames=12)
            self.assertEqual(result.returncode,0,result.stderr);self.assertEqual(result.stdout,control.stdout)
            for bad in [content.replace('resource 23 ', 'resource 22 '),content.replace('textassets/hero.bin','../hero.bin'),
                        '\n'.join(content.splitlines()[:-1])+'\n',content.replace('mission_resources 1','mission_resources 2')]:
                index.write_text(bad,encoding='utf-8');self.assertNotEqual(self.run_host(scene,'--data',str(data)).returncode,0)
            index.write_text(content,encoding='utf-8')
            self.assertNotEqual(self.run_host(scene).returncode,0)

    def test_loaded_world_dispatches_all_32_native_initializers_before_updates(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt';replay = root / 'init.txt'
            text = make_boss_scene(root)
            replay.write_text('1 touch 5 99 123\n1 click 0 0\n', encoding='ascii')
            scene.write_text(text+'system_init 1\n', encoding='utf-8')
            result = self.run_host(scene, '--replay', str(replay), frames=100)
            self.assertEqual(result.returncode, 0, result.stderr);d = json.loads(result.stdout)
            self.assertTrue(d['init_schedule_complete']);self.assertTrue(d['update_schedule_complete'])
            self.assertEqual(d['init_order'], list(range(32)));self.assertEqual(d['init_world_loaded_byte'], 1)
            self.assertFalse(d['init_manager_paused']);self.assertTrue(d['init_town_return_registered'])
            self.assertEqual(d['submenu_button_events'], 1);self.assertEqual(d['menu_button_events'], 1)
            self.assertEqual(d['info_button_events'], 1);self.assertEqual(d['button_events'], 1)
            self.assertEqual(d['positions'][0]['id'], 1)
            self.assertEqual(self.run_host(scene, '--replay', str(replay), frames=100).stdout, result.stdout)
            # Unknown loaded state is never inferred; any nonzero native byte takes the loaded branch.
            scene.write_text(text+'system_init 128\n', encoding='utf-8')
            other = self.run_host(scene, '--replay', str(replay), frames=100)
            self.assertEqual(other.returncode, 0, other.stderr)
            alternate = json.loads(other.stdout);self.assertEqual(alternate['init_world_loaded_byte'], 128)
            self.assertEqual(alternate['positions'], d['positions'])
            # The old selected-profile path stays explicit and does not claim complete Init coverage.
            scene.write_text(text, encoding='utf-8');legacy = self.run_host(scene, '--replay', str(replay), frames=100)
            self.assertEqual(legacy.returncode, 0, legacy.stderr);old = json.loads(legacy.stdout)
            self.assertFalse(old['init_schedule_complete']);self.assertEqual(old['init_order'], [])
            for key in ['positions','shots','rng_draws','total_damage','enemy_created','attack_buttons']:
                self.assertEqual(d[key], old[key], key)
            for row in ['system_init 0\n','system_init -1\n','system_init 256\n','system_init 1 extra\n','system_init 1\nsystem_init 1\n']:
                scene.write_text(text+row, encoding='utf-8');self.assertNotEqual(self.run_host(scene).returncode, 0)
            scene.write_text(make_table_scene(root)+'simulate 0\nsystem_init 1\n', encoding='utf-8')
            self.assertIn('Human, Input and mission_map', self.run_host(scene).stderr)

    def test_gauge_factory_updates_native_images_and_expiry_in_complete_schedule(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt'
            text = make_gauge_scene(root) + ('gauge 0 0 0 0 100 100 1 -1 1 0 0\n'
                                            'gauge 0 0 6 0 100 100 1 11 0 0 0\n'
                                            'gauge 0 0 1 0 100 100 1 2 0 0 0\n')
            scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene, frames=12)
            self.assertEqual(result.returncode, 0, result.stderr);d = json.loads(result.stdout)
            self.assertTrue(d['update_schedule_complete']);self.assertEqual(d['unbound_update_systems'], [])
            self.assertEqual(d['gauge_steps'], 12);self.assertEqual(d['gauge_destroy_calls'], 0)
            self.assertEqual(len(d['gauges']), 3);a,b,_ = d['gauges']
            self.assertEqual((a['frame'],a['frame_max'],a['parent']), (0,-1,d['positions'][0]['id']))
            self.assertEqual(a['x'], int(d['positions'][0]['x'])-50)
            self.assertEqual(a['z'], int(d['positions'][0]['z'])-20)
            self.assertEqual((a['u'],a['v'],a['width'],a['height']), ([0,2],[0,4],[12,8],[4,4]))
            self.assertEqual((b['frame'],b['width']), (12,[12,8]))
            self.assertLess(d['last_update_order'].index(21), d['last_update_order'].index(22))
            self.assertLess(d['last_update_order'].index(22), d['last_update_order'].index(23))
            self.assertEqual(self.run_host(scene, frames=12).stdout, result.stdout)
            later = self.run_host(scene, frames=18);self.assertEqual(later.returncode, 0, later.stderr)
            self.assertEqual(len(json.loads(later.stdout)['gauges']), 2)
            final = self.run_host(scene, frames=28);self.assertEqual(final.returncode, 0, final.stderr)
            self.assertEqual(len(json.loads(final.stdout)['gauges']), 1)
            self.assertEqual(json.loads(final.stdout)['gauge_destroy_calls'], 2)
            scene.write_text(text.replace('gauge 0 0 ', 'gauge 3 0 '), encoding='utf-8')
            delayed = self.run_host(scene, frames=2);self.assertEqual(delayed.returncode, 0, delayed.stderr)
            self.assertEqual(json.loads(delayed.stdout)['gauges'], [])
            delayed = self.run_host(scene, frames=3);self.assertEqual(delayed.returncode, 0, delayed.stderr)
            self.assertEqual([g['frame'] for g in json.loads(delayed.stdout)['gauges']], [0,1,1])
            for bad in ['gauge 0 -1 0 0 100 100 1 -1 1 0 0\n', 'gauge 0 90 0 0 100 100 1 -1 1 0 0\n',
                        'gauge 0 0 8 0 100 100 1 -1 1 0 0\n', 'gauge 0 0 0 0 100 100 1 -1 1 0\n']:
                scene.write_text(make_gauge_scene(root)+bad, encoding='utf-8')
                self.assertNotEqual(self.run_host(scene).returncode, 0)

    def test_bubble_controller_deduplicates_expires_and_runs_debug_pass(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt';replay = root / 'walk.txt'
            text = make_boss_scene(root)
            text += ('bubble 0 100 0 3 1 40 312 0 400\n'
                     'bubble 0 101 0 -1 1 40 312 0 400\n'
                     'bubble -1 102 1 -1 1 40 0 0 0\n')
            scene.write_text(text, encoding='utf-8');replay.write_text('2 keys 131072\n', encoding='ascii')
            result = self.run_host(scene, '--replay', str(replay), frames=12)
            self.assertEqual(result.returncode, 0, result.stderr);d = json.loads(result.stdout)
            self.assertEqual(d['bubble_steps'], 12);self.assertEqual(d['bubble_destroy_calls'], 2)
            self.assertEqual(d['bubble_pending'], 0);self.assertEqual(d['bubble_seen'], 0)
            self.assertEqual(d['debug_visualization_steps'], 12);self.assertEqual(d['debug_visualization_flags'], 0)
            self.assertEqual(len(d['bubbles']), 1);bubble = d['bubbles'][0]
            self.assertEqual((bubble['data_id'],bubble['frame'],bubble['frame_max']), (101,12,-1))
            self.assertEqual(bubble['parent'], d['positions'][0]['id'])
            self.assertEqual(bubble['x'], int(d['positions'][0]['x'])-36)
            self.assertEqual(bubble['y'], int(d['positions'][0]['y']))
            self.assertEqual(bubble['z'], int(d['positions'][0]['z'])-30)
            self.assertLess(d['last_update_order'].index(15), d['last_update_order'].index(16))
            self.assertLess(d['last_update_order'].index(16), d['last_update_order'].index(17))
            self.assertLess(d['last_update_order'].index(26), d['last_update_order'].index(30))
            self.assertEqual(self.run_host(scene, '--replay', str(replay), frames=12).stdout, result.stdout)
            scene.write_text(make_boss_scene(root) + 'bubble 0 1 0 2 1 40 312 0 400\n', encoding='utf-8')
            expired = self.run_host(scene, frames=3);self.assertEqual(expired.returncode, 0, expired.stderr)
            self.assertEqual(json.loads(expired.stdout)['bubbles'], [])
            for bad in ['bubble 90 1 0 -1 1 40 0 0 0\n', 'bubble 0 1 9 -1 1 40 0 0 0\n', 'bubble 0 1 0 -1 1 40 0 0\n']:
                scene.write_text(make_boss_scene(root) + bad, encoding='utf-8');self.assertNotEqual(self.run_host(scene).returncode, 0)

    def test_info_controller_updates_bars_and_receives_native_events(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt';replay = root / 'info.txt'
            text = make_boss_scene(root)
            text += ('info_popup 1 0 0 5 0 20 -1\ninfo_popup 2 1 0 3 0 12 3\n'
                     'info_slide 3 80 12 3\ninfo_slide 4 40 14 -1\n')
            scene.write_text(text, encoding='utf-8')
            replay.write_text('1 click 0 0\n1 touch 5 99 123\n', encoding='ascii')
            first = self.run_host(scene, '--replay', str(replay), frames=9)
            self.assertEqual(first.returncode, 0, first.stderr)
            d = json.loads(first.stdout)
            self.assertEqual(d['info_steps'], 9);self.assertEqual(d['info_clicks'], 1)
            self.assertEqual(d['cursor_clicks'], 1);self.assertEqual(d['info_button_events'], 1)
            self.assertEqual(d['info_emphasize_frame'], -1);self.assertFalse(d['info_dialog_pending'])
            self.assertEqual([(p['id'],p['y'],p['state']) for p in d['info_popups']], [(1,76,1),(2,64,1)])
            self.assertEqual([(p['id'],p['x'],p['y'],p['state']) for p in d['info_slides']], [(3,0,65,1),(4,0,79,1)])
            self.assertLess(d['last_update_order'].index(24), d['last_update_order'].index(25))
            self.assertLess(d['last_update_order'].index(25), d['last_update_order'].index(26))
            later = self.run_host(scene, '--replay', str(replay), frames=30)
            self.assertEqual(later.returncode, 0, later.stderr);d = json.loads(later.stdout)
            self.assertEqual(d['info_popup_disposals'], 1)
            self.assertEqual([p['id'] for p in d['info_popups']], [1])
            self.assertEqual([(p['id'],p['x'],p['y']) for p in d['info_slides']], [(4,0,65)])
            self.assertEqual(self.run_host(scene, '--replay', str(replay), frames=30).stdout, later.stdout)
            for bad in ['1 click 0\n', '1 click 0 2147483648\n', '1 click 0 0 extra\n']:
                replay.write_text(bad, encoding='ascii')
                self.assertNotEqual(self.run_host(scene, '--replay', str(replay)).returncode, 0)
            for bad in ['info_popup 1 0 0 0 2 20 -1\n', 'info_popup 1 0 0 0 0 20\n', 'info_slide 1 80 20\n']:
                scene.write_text(text + bad, encoding='utf-8');self.assertNotEqual(self.run_host(scene).returncode, 0)

    def test_entity_buttons_follow_native_camera_and_route_touch(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt';replay = root / 'buttons.txt'
            text = make_boss_scene(root)
            # Explicit empty-text diagnostic widgets; the real quest widget producer remains separate.
            text += 'button 0 7 9 1 40 0 99 123 0 0 100 8 -9 0\n'
            text += 'button 0 7 9 1 40 0 99 124 1 0 100 8 -9 0\n'
            text += 'button -1 7 9 1 40 0 99 125 0 1 100 8 -9 0\n'
            scene.write_text(text, encoding='utf-8');replay.write_text('1 touch 13 99 123\n', encoding='ascii')
            result = self.run_host(scene, '--replay', str(replay), frames=1)
            self.assertEqual(result.returncode, 0, result.stderr)
            d = json.loads(result.stdout);buttons = d['entity_buttons'];self.assertEqual(len(buttons), 2)
            self.assertEqual(d['button_steps'], 1);self.assertEqual(d['button_removals'], 1)
            self.assertEqual(d['last_button_target'], buttons[0]['entity']);self.assertEqual(d['last_button_event_type'], 13)
            self.assertTrue(buttons[0]['hover']);self.assertFalse(buttons[1]['hover'])
            self.assertEqual(buttons[0]['parent'], d['positions'][0]['id'])
            self.assertEqual((buttons[0]['x'], buttons[0]['y']), (buttons[1]['x'], buttons[1]['y']))
            self.assertAlmostEqual(buttons[0]['x'], -88 - d['camera']['x'], places=4)
            self.assertAlmostEqual(buttons[0]['y'], 356 - d['camera']['y'], places=4)
            self.assertLess(d['last_update_order'].index(24), d['last_update_order'].index(26))
            self.assertLess(d['last_update_order'].index(26), d['last_update_order'].index(31))
            self.assertEqual(self.run_host(scene, '--replay', str(replay), frames=1).stdout, result.stdout)
            replay.write_text('1 touch 13 99 123\n2 touch 2 99 123\n3 touch 13 99 999\n', encoding='ascii')
            later = self.run_host(scene, '--replay', str(replay), frames=3)
            self.assertEqual(later.returncode, 0, later.stderr);after = json.loads(later.stdout)
            self.assertFalse(after['entity_buttons'][0]['hover']);self.assertEqual(after['last_button_target'], -1)
            self.assertEqual(after['button_events'], 3)
            # Factory uniqueness is by value, even across different group IDs, and removes only the first match.
            scene.write_text(text + 'button -1 1 2 1 40 0 100 123 1 0 100 0 0 1\n', encoding='utf-8')
            replaced = self.run_host(scene, frames=1);self.assertEqual(replaced.returncode, 0, replaced.stderr)
            # Native subset traversal reuses the removed hash slot for the replacement.
            self.assertEqual([b['group'] for b in json.loads(replaced.stdout)['entity_buttons']], [100,99])
            for bad in ['1 touch 13 99\n', '1 touch 13 99 2147483648\n', '1 touch 13 99 1 extra\n',
                        '2 touch 13 99 1\n1 touch 13 99 1\n']:
                replay.write_text(bad, encoding='ascii')
                self.assertNotEqual(self.run_host(scene, '--replay', str(replay)).returncode, 0)
            for invalid in ['button 50 0 0 1 40 0 99 123 0 0 100 0 0 0\n',
                            'button 0 0 0 1 40 0 99 123 2 0 100 0 0 0\n']:
                scene.write_text(text + invalid, encoding='utf-8');self.assertNotEqual(self.run_host(scene).returncode, 0)
            scene.write_text(text.replace('mission_map fresh 0 1 2 3\n', ''), encoding='utf-8')
            self.assertIn('button scenario requires mission_map fresh', self.run_host(scene).stderr)

    def test_cursor_selects_live_human_and_follows_camera_cells(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt';replay = root / 'cursor.txt'
            scene.write_text(make_boss_scene(root), encoding='utf-8')
            replay.write_text('1 cursor -88 341\n2 keys 131072\n', encoding='ascii')
            result = self.run_host(scene, '--replay', str(replay), frames=12)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual(report['cursor_touches'], 1)
            self.assertEqual(report['cursor_steps'], 12)
            self.assertEqual(report['cursor']['focus'], report['positions'][0]['id'])
            self.assertEqual(report['cursor']['effect_arg'], report['cursor']['focus'])
            self.assertEqual(report['cursor']['focus_effects'], 1)
            self.assertEqual(report['cursor']['wait'], 6)
            self.assertEqual(report['cursor']['x'], report['camera']['saved_x'])
            self.assertEqual(report['cursor']['y'], report['camera']['saved_y'])
            self.assertLess(report['last_update_order'].index(23), report['last_update_order'].index(24))
            self.assertLess(report['last_update_order'].index(24), report['last_update_order'].index(31))
            self.assertEqual(self.run_host(scene, '--replay', str(replay), frames=12).stdout, result.stdout)
            replay.write_text('1 cursor -88 341\n2 cursor -88 341\n3 cursor 10000 10000\n', encoding='ascii')
            cleared = json.loads(self.run_host(scene, '--replay', str(replay), frames=3).stdout)
            self.assertEqual(cleared['cursor']['focus'], -1)
            self.assertEqual(cleared['cursor']['focus_effects'], 1)
            self.assertEqual(cleared['cursor_touches'], 3)
            for bad in ['1 cursor 0\n', '1 cursor 2147483648 0\n', '1 cursor 0 -2147483649\n',
                        '1 cursor 0 0 1\n', '2 cursor 0 0\n1 cursor 0 0\n']:
                replay.write_text(bad, encoding='ascii')
                self.assertNotEqual(self.run_host(scene, '--replay', str(replay)).returncode, 0)
            replay.write_text('1 cursor 0 0\n', encoding='ascii')
            scene.write_text(make_boss_scene(root).replace('mission_map fresh 0 1 2 3\n', ''), encoding='utf-8')
            self.assertIn('mission_map fresh', self.run_host(scene, '--replay', str(replay)).stderr)

    def test_enemy_gauge_uses_saved_defeat_threshold(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt'
            scene.write_text(make_gauge_scene(root, 5), encoding='utf-8')
            result = self.run_host(scene, frames=1);self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(json.loads(result.stdout)['enemy_gauge_requests'], [])
            text = make_gauge_scene(root, 6);scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene, frames=1);self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout);self.assertTrue(report['enemy_gauges_enabled'])
            self.assertEqual(report['enemy_gauge_requests'], [{'entity': 1, 'x': 12, 'y': 11, 'image': 1, 'parameter': 0, 'anchor': 2, 'force': False}])
            for invalid in [text + 'enemy_gauges\n', text.replace('rewards 0 1 6\n', ''),
                            text.replace('resource 23 "gauges.bin" "English.lproj" 0\n', '').replace('global_alias 23 23\n', '')]:
                scene.write_text(invalid, encoding='utf-8');self.assertNotEqual(self.run_host(scene, frames=1).returncode, 0)
    def run_host(self, scene, *extra, fps=25, debug=0, frames=12):
        return subprocess.run([str(EXE), '--scene', str(scene), '--frames', str(frames), '--headless',
                               '--fps', str(fps), '--debug', str(debug), '--rng', 'system', '--seed', '42', *extra],
                              capture_output=True, text=True, timeout=30)

    def test_relative_archives_headless_passes_and_repeatability(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            # Deliberately invalid PNG bytes: the headless runtime must not invoke image decoding.
            scene = root / 'scene.txt'
            scene.write_text(make_table_scene(root, valid_png=False) + 'simulate 0\n', encoding='utf-8')
            a, b = self.run_host(scene), self.run_host(scene)
            self.assertEqual(a.returncode, 0, a.stderr)
            self.assertEqual((a.returncode, a.stdout), (b.returncode, b.stdout))
            report = json.loads(a.stdout)
            self.assertEqual(report['frames'], 12)
            self.assertEqual(report['rng_draws'], 0)
            self.assertEqual(report['positions'][0]['z'], 0)
            self.assertGreater(report['positions'][1]['z'], 0)
            self.assertEqual(report['render_commands'], 2)
            self.assertEqual(report['parameter_experience_steps'], 12)
            self.assertEqual(report['parameter_steps'], 12)
            self.assertEqual(report['parameter_experience_pending'], 0)
            self.assertEqual(report['effect_steps'], 12)
            self.assertEqual(report['effect_updates'], [])
            self.assertTrue(report['update_schedule_complete'])
            self.assertEqual(report['bound_update_systems'], [0,2,3,4,5,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,30,31])
            self.assertEqual(report['unbound_update_systems'], [])
            self.assertEqual(report['last_update_order'], [2,9,11,13,14,15,16,18,19,21,22,23,25,26,30,31])
            self.assertEqual(report['rotation_steps'], 12)
            self.assertEqual(report['battle_steps'], 12)

    def test_live_human_walking_uses_saved_gear_fps_and_rng(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scene = root / 'scene.txt'
            text = make_table_scene(root, valid_png=False, human_frame=True)
            text = text.replace('human 0 2 5 0 0 13 0 0 0', 'human 0 2 5 0 0 13 0 0 100')
            text += ('simulate 0\ngeometry 24 24 6\n'
                     'human_system 10 0 100 0 0\nplayer 0\nactor_state 0 1 -1\n'
                     'gear_levels equipment 1\ngear_levels hat 1\ngear_levels cloth 1\n')
            scene.write_text(text, encoding='utf-8')
            a = self.run_host(scene)
            self.assertEqual(a.returncode, 0, a.stderr)
            report = json.loads(a.stdout)
            self.assertEqual(report['positions'][0]['human_state'], 1)
            self.assertEqual(report['positions'][0]['human_frame'], 12)
            self.assertLess(report['positions'][0]['z'], 100)
            self.assertEqual(report['rng_draws'], 1)
            fast = self.run_host(scene, fps=5)
            self.assertEqual(fast.returncode, 0, fast.stderr)
            self.assertEqual(json.loads(fast.stdout)['rng_draws'], 3)
            # Total-stat provider requires all saved gear levels, never a silent level1.
            scene.write_text(text.replace('gear_levels cloth 1\n', ''), encoding='utf-8')
            self.assertNotEqual(self.run_host(scene).returncode, 0)
            scene.write_text(text + 'runtime 60 0 system 42\n', encoding='utf-8')
            self.assertIn('conflicts', self.run_host(scene).stderr)

    def test_rotate_pass_follows_input_and_respects_human_attack_facing(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt';replay = root / 'replay.txt'
            text = make_shoot_scene(root, spell_attribute=0).replace('human_system 10 0 100 3 1', 'human_system 10 0 100 0 0')
            text += 'input fresh 0 0 0 0 0 0 0 0 0\n'
            scene.write_text(text, encoding='utf-8')
            replay.write_text('2 keys 524288\n8 keys 0\n', encoding='ascii')
            moving = self.run_host(scene, '--replay', str(replay), frames=6)
            self.assertEqual(moving.returncode, 0, moving.stderr)
            a = json.loads(moving.stdout)
            self.assertEqual(a['positions'][0]['direction'], 2)
            self.assertEqual(a['positions'][0]['animation_clip'] % 4, 2)
            self.assertGreater(a['positions'][0]['z'], 100)
            self.assertFalse(a['positions'][0]['direction_locked'])
            self.assertLess(a['last_update_order'].index(11), a['last_update_order'].index(13))
            self.assertLess(a['last_update_order'].index(13), a['last_update_order'].index(19))
            released = self.run_host(scene, '--replay', str(replay), frames=10)
            self.assertEqual(released.returncode, 0, released.stderr)
            b = json.loads(released.stdout)
            # Release restores attack selection; Human4 faces the acquired target before Rotate13.
            self.assertEqual(b['positions'][0]['human_state'], 2)
            self.assertEqual(b['positions'][0]['direction'], 0)
            self.assertEqual(b['positions'][0]['animation_clip'] % 4, 0)
            self.assertEqual(b['rotation_steps'], 10)

    def test_actual_shoot_flight_impact_and_nonlethal_damage(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scene = root / 'scene.txt'
            scene.write_text(make_shoot_scene(root), encoding='utf-8')
            result = self.run_host(scene, frames=80)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertGreater(report['shots'], 0)
            self.assertGreater(report['total_damage'], 0)
            self.assertGreater(report['rng_draws'], 0)
            self.assertEqual(report['positions'][0]['mp'], 100 - report['shots'])
            self.assertEqual(report['positions'][1]['hp'], 100000 - report['total_damage'])
            repeated = self.run_host(scene, frames=80)
            self.assertEqual(repeated.stdout, result.stdout)

    def test_explicit_contract_failures(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scene = root / 'scene.txt'
            text = make_table_scene(root)
            scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('explicit simulate row', result.stderr)
            for bad in ['simulate -1\n', 'simulate 0\nsimulate 0\n', 'unknown 1\n']:
                scene.write_text(text + bad, encoding='utf-8')
                self.assertNotEqual(self.run_host(scene).returncode, 0)
            for extra in [('--quest', '0'), ('--replay', 'missing'), ('--seed', '9')]:
                self.assertNotEqual(self.run_host(scene, *extra).returncode, 0)
            incomplete = subprocess.run([str(EXE), '--headless'], capture_output=True, text=True, timeout=30)
            self.assertNotEqual(incomplete.returncode, 0)

    def test_native_stage_start_positions_party_and_places_table_drops(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'entry.txt'
            original = make_boss_scene(root, stage_entry=True)
            substitutions = {
                'geometry 312 312 12': 'geometry quest',
                'human_system 10 0 95 0 0': 'human_system quest 0 95 0 0',
                'enemy_system 10000 0 10 25 7': 'enemy_system quest 10000',
                'enemy_pool 0\n': '',
                'stage_notifications 1 0 0 2 0 0 -1': 'stage_notifications 1 0 0 quest 1 0 -1',
                'stage_events fresh 0': 'stage_events quest',
                'stage_loop 5000 0 20 hold': 'stage_loop quest 0 20 hold',
                'stage_boss 4 7': 'stage_boss quest',
                'mission_map fresh 0 1 2 3': 'mission_map quest',
            }
            text = original
            for before, after in substitutions.items():
                self.assertIn(before, text);text = text.replace(before, after)
            text += ('quest 2 0\nsystem_init 1\nstage_start 0 null\n'
                     'party_formation 2 3 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1\n'
                     'progress 3 1 0\nsorties 2 7\nsorties 3 9\n'
                     'result_wallet 10 20 0\nresult_record 0 0 0 0 0 0 0\n'
                     'result_inventory equipment 0 0 0\nresult_inventory cloth 0 0 0\n'
                     'result_inventory hat 0 0 0\nresult_recruits 0 0 0\n')
            scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene, frames=1)
            self.assertEqual(result.returncode, 0, result.stderr)
            d = json.loads(result.stdout)
            self.assertTrue(d['native_stage_started']);self.assertTrue(d['init_schedule_complete'])
            self.assertEqual(len(d['entry_members']), 2)  # Empty entry roster, one hero and one native sub-character.
            self.assertEqual(d['quest_form_scale'], 100)
            self.assertEqual(d['sortie_counts'], [{'id':2,'count':8},{'id':3,'count':10}])
            self.assertEqual((d['positions'][0]['x'], d['positions'][0]['z']), (240,2964))
            self.assertEqual(d['positions'][0]['human_state'], 2)  # Human4 consumes the startup waiting state.
            self.assertEqual(d['remaining_time'], 5000)  # First frame only changes Stage0 -> Stage1.
            self.assertGreater(d['rng_draws'], 80)
            self.assertEqual(self.run_host(scene, frames=1).stdout, result.stdout)
            scene.write_text(text.replace('sorties 2 7', 'sorties 2 2147483647'), encoding='utf-8')
            wrapped = self.run_host(scene, frames=1);self.assertEqual(wrapped.returncode, 0, wrapped.stderr)
            self.assertEqual(json.loads(wrapped.stdout)['sortie_counts'][0]['count'], -2147483648)
            # Repeated character slots create distinct native sub-characters; do not deduplicate the saved formation.
            larger = text.replace('party_formation 2 3 -1 -1', 'party_formation 2 3 3 3')
            scene.write_text(larger, encoding='utf-8');four = self.run_host(scene, frames=1)
            self.assertEqual(four.returncode, 0, four.stderr)
            self.assertEqual(len(json.loads(four.stdout)['entry_members']), 4)
            self.assertEqual(json.loads(four.stdout)['quest_form_scale'], 75)
            self.assertEqual(json.loads(four.stdout)['sortie_counts'][1]['count'], 12)
            for bad in [text.replace('sorties 3 9\n',''), text.replace('party_formation 2 3', 'party_formation 2'),
                        text+'stage_start 1 null\n', text.replace('stage_start 0 null','stage_start 0 inferred'),
                        text.replace('system_init 1\n',''), text+'actor_state 0 1 -1\n']:
                scene.write_text(bad, encoding='utf-8')
                self.assertNotEqual(self.run_host(scene, frames=1).returncode, 0)

    def test_quest_table_drives_map_boss_and_preserves_saved_fields(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt'
            original = make_boss_scene(root, alternate_quest=True)
            substitutions = {
                'geometry 312 312 12': 'geometry quest',
                'human_system 10 0 95 0 0': 'human_system quest 0 95 0 0',
                'enemy_system 10000 0 10 25 7': 'enemy_system quest 10000',
                'enemy_pool 0\n': '',
                'stage_notifications 1 0 0 2 0 0 -1': 'stage_notifications 1 0 0 quest 1 0 -1',
                'stage_events fresh 0': 'stage_events quest',
                'stage_loop 5000 0 20 hold': 'stage_loop quest 0 20 hold',
                'stage_boss 4 7': 'stage_boss quest',
                'mission_map fresh 0 1 2 3': 'mission_map quest',
            }
            text = original
            for before, after in substitutions.items():
                self.assertIn(before, text);text = text.replace(before, after)
            text += 'quest 2 2\n'
            scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene, frames=1)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual(report['selected_quest'], 2)
            self.assertEqual(report['quest_distance'], 10)
            self.assertEqual(report['quest_flags'], 2)
            self.assertEqual(report['quest_money_rate'], 150)
            self.assertEqual((report['quest_completion_byte'], report['quest_rescued']), (1, 2))
            self.assertEqual(report['boss_creation_calls'], 1)
            self.assertEqual(report['remaining_time'], 4999)
            self.assertEqual(report['mission_map']['height'], 12)
            self.assertEqual(self.run_host(scene, frames=1).stdout, result.stdout)
            override = self.run_host(scene, '--quest', '3', frames=1)
            self.assertEqual(override.returncode, 0, override.stderr)
            changed = json.loads(override.stdout)
            self.assertEqual((changed['selected_quest'], changed['quest_distance'], changed['quest_money_rate']), (3,12,175))
            self.assertEqual(changed['quest_rescued'], 2)
            self.assertEqual(changed['mission_map']['height'], 14)
            for value in ['-1', 'bad', '1', '99999']:
                self.assertNotEqual(self.run_host(scene, '--quest', value, frames=1).returncode, 0)
            scene.write_text(text.replace('quest 2 2\n', ''), encoding='utf-8')
            self.assertIn('scenario quest row', self.run_host(scene, '--quest', '2', frames=1).stderr)
            scene.write_text(text, encoding='utf-8')
            # Relocation must ignore stale absolute paths and never fall back to the original directory.
            data = root / 'data';assets = data / 'textassets';assets.mkdir(parents=True)
            for archive in root.glob('*.bin'):
                (assets / archive.name).write_bytes(archive.read_bytes())
            stale = text.replace('"tables.bin"', json.dumps(r'C:\unavailable\tables.bin'))
            scene.write_text(stale, encoding='utf-8')
            relocated = self.run_host(scene, '--data', str(data), '--quest', '3', frames=1)
            self.assertEqual(relocated.returncode, 0, relocated.stderr)
            self.assertEqual(relocated.stdout, override.stdout)
            (assets / 'tables.bin').unlink()
            self.assertIn('selected data archive missing', self.run_host(scene, '--data', str(data), frames=1).stderr)
            self.assertNotEqual(self.run_host(scene, '--data', str(root / 'absent'), frames=1).returncode, 0)
            for invalid in [text.replace('quest 2 2', 'quest 1 2'),
                            text.replace('quest 2 2\n', ''),
                            text + 'enemy_pool 0\n',
                            text.replace('mission_map quest', 'mission_map fresh 0 1 2 3'),
                            text.replace('stage_events quest', 'stage_events fresh 0')]:
                scene.write_text(invalid, encoding='utf-8')
                self.assertNotEqual(self.run_host(scene, frames=1).returncode, 0)

    def test_lethal_shot_pending_rewards_and_cleanup(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scene = root / 'scene.txt'
            text = make_shoot_scene(root, rewards=True)
            scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene, frames=240)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertTrue(report['positions'][1]['destroyed'])
            self.assertTrue(report['rewards_enabled'])
            self.assertEqual(report['defeated_enemies'], 1)
            self.assertEqual(report['remaining_enemies'], 1)  # Native ordinary-enemy floor.
            self.assertEqual(report['saved_defeats'], [{'enemy_id': 4, 'count': 5}])
            self.assertEqual([(d['type'], d['value']) for d in report['pending_drops']], [(0, 10), (3, 0)])
            self.assertEqual(report['projectiles'], [])
            self.assertGreaterEqual(report['rng_draws'], 19)
            self.assertEqual(self.run_host(scene, frames=240).stdout, result.stdout)
            scene.write_text(text + 'enemy_system 10000 3 10 2 7\nenemy_population 0 0 1 0 0 100 0\nenemy_pool 1 4 1\n', encoding='utf-8')
            full = self.run_host(scene, frames=240)
            self.assertEqual(full.returncode, 0, full.stderr)
            full_report = json.loads(full.stdout)
            self.assertEqual(full_report['enemies'], [])
            # Two in-flight hits can reach Defeat before corpse expiry in this partial schedule.
            # Defeat has no local already-dead guard; do not invent host reward deduplication.
            self.assertEqual(full_report['defeated_enemies'], 2)
            self.assertEqual(full_report['saved_defeats'], [{'enemy_id': 4, 'count': 6}])
            self.assertEqual([(d['type'], d['value']) for d in full_report['pending_drops']], [(0, 10), (3, 0)] * 2)
            # Lethal rules require explicit initial counters; no fabricated reward state.
            scene.write_text(text.replace('rewards 0 1 4\n', ''), encoding='utf-8')
            missing = self.run_host(scene, frames=240)
            self.assertNotEqual(missing.returncode, 0)
            self.assertIn('enemy defeat requires explicit counters', missing.stderr)
            scene.write_text(text + 'enemy_system 10000 3 10 2 7\nenemy_population 0 0 10 0 0 100 0\nenemy_pool 0\n', encoding='utf-8')
            self.assertIn('counters conflict', self.run_host(scene).stderr)

    def test_enemy_activation_population_targets_and_repeatability(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scene = root / 'scene.txt'
            text = make_shoot_scene(root).replace('human_system 10 0 100 3 1', 'human_system 10 0 100 3 0')
            config = 'enemy_system 10000 3 10 2 7\nenemy_pool 1 4 1\n'
            initial = 'enemy_population 0 0 1 0 0 100 0\n'
            scene.write_text(text + config + initial, encoding='utf-8')
            result = self.run_host(scene, frames=12)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertTrue(report['enemy_system_enabled'])
            self.assertEqual(report['enemy_created'], 0)
            self.assertEqual(report['enemy_loop_counter'], 12)
            self.assertEqual(report['positions'][1]['enemy_state'], 1)
            self.assertEqual(report['positions'][1]['enemy_flags'], 6)
            self.assertEqual(report['positions'][1]['enemy_frame'], 11)
            self.assertAlmostEqual(report['positions'][1]['z'], 70 + 12 * 0.5575, places=4)
            self.assertEqual(report['shots'], 0)
            # Run the actual population prefix, lottery and CreateSystemEnemy target assignment.
            initial = 'enemy_population 0 0 10 0 0 100 0\n'
            scene.write_text(text + config + initial, encoding='utf-8')
            result = self.run_host(scene, frames=6)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual(report['enemy_created'], 2)
            self.assertEqual(len(report['enemies']), 3)
            spawned = report['enemies'][1:]
            self.assertEqual([e['target'] for e in spawned], [0, 0])
            self.assertTrue(all(e['state'] == 1 and e['level'] == 7 for e in spawned))
            self.assertGreater(report['rng_draws'], 0)
            self.assertEqual(self.run_host(scene, frames=6).stdout, result.stdout)
            # The alternate save branch changes cadence/limits and the link level after factory stats.
            scene.write_text(text + config.replace('10000', '9999') + initial, encoding='utf-8')
            alternate = self.run_host(scene, frames=2)
            self.assertEqual(alternate.returncode, 0, alternate.stderr)
            alternate_report = json.loads(alternate.stdout)
            self.assertEqual(alternate_report['enemy_created'], 2)
            self.assertEqual([e['level'] for e in alternate_report['enemies'][1:]], [350, 350])
            for bad in [config, config + initial + 'enemy_pool 0\n',
                        config.replace('enemy_pool 1 4 1', 'enemy_pool 2 4 1') + initial]:
                scene.write_text(text + bad, encoding='utf-8')
                self.assertNotEqual(self.run_host(scene).returncode, 0)

    def test_collision_contacts_and_neighboring_impact_damage(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            scene = root / 'scene.txt'
            text = make_shoot_scene(root, area=True)
            text += ('enemy 0 4 7 2 36 0 70\ncollisions\n'
                     'enemy_system 10000 3 10 25 7\nenemy_population 0 0 1 0 0 100 0\nenemy_pool 0\n')
            scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene, frames=80)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertTrue(report['collisions_enabled'])
            self.assertGreater(report['collision_events'], 0)
            self.assertGreater(report['area_collision_events'], 0)
            self.assertLess(report['positions'][1]['hp'], 100000)
            # Shots travel at X0; the X36 enemy is outside their radius and takes area damage.
            self.assertLess(report['positions'][2]['hp'], 100000)
            self.assertEqual(self.run_host(scene, frames=80).stdout, result.stdout)
            scene.write_text(text + 'collisions\n', encoding='utf-8')
            self.assertIn('one row', self.run_host(scene).stderr)
            scene.write_text(text.replace('human_system 10 0 100 3 1\n', ''), encoding='utf-8')
            self.assertIn('event receivers', self.run_host(scene).stderr)

    def test_main_death_dispatches_game_over_before_retarget(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt'
            text = make_shoot_scene(root, lethal_contact=True)
            text += ('collisions\nenemy_system 10000 3 10 25 7\n'
                     'enemy_population 0 0 1 0 0 100 0\nenemy_pool 0\n')
            scene.write_text(text + 'stage_notifications 1 13 0 0 0 655360 -1\n', encoding='utf-8')
            result = self.run_host(scene, frames=80)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual(report['positions'][0]['hp'], 0)
            self.assertEqual(report['positions'][0]['human_state'], 5)
            self.assertEqual(report['retry_dialog'], {'kind': 98, 'reason': 1, 'answer': -1})
            self.assertEqual((report['stage_state'], report['stage_frame']), (1, 13))
            self.assertEqual(report['notification_events'], ['make_retry:100', 'push:retry', 'stop_bgm', 'make_defeat:1', 'push:defeat'])
            scene.write_text(text + 'stage_notifications 1 13 1 0 0 655360 -1\n', encoding='utf-8')
            retried = self.run_host(scene, frames=80)
            self.assertEqual(retried.returncode, 0, retried.stderr)
            report = json.loads(retried.stdout)
            self.assertEqual((report['stage_state'], report['stage_frame']), (5, 0))
            self.assertIsNone(report['retry_dialog'])
            self.assertEqual(report['notification_events'], ['stop_bgm', 'make_defeat:1', 'push:defeat'])
            scene.write_text(text, encoding='utf-8')
            self.assertIn('stage_notifications', self.run_host(scene, frames=80).stderr)
            for invalid in ['stage_notifications 1 13 2 0 0 0 -1\n',
                            'stage_notifications 1 13 0 0 0 4294967296 -1\n',
                            'stage_notifications 1 13 0 0 0 0\n']:
                scene.write_text(text + invalid, encoding='utf-8')
                self.assertNotEqual(self.run_host(scene).returncode, 0)

    def test_boss_landing_notifies_and_releases_keys(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt'
            text = make_shoot_scene(root, rewards=True).replace('enemy 0 4 7 2', 'enemy 0 4 7 1')
            text += ('enemy_system 10000 3 10 25 7\nenemy_population 0 0 1 0 0 100 0\nenemy_pool 0\n'
                     'stage_notifications 1 13 0 2 0 655361 -1\n')
            scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene, frames=240)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertTrue(report['boss_defeated'])
            self.assertEqual(report['max_z_rate'], 100)
            self.assertEqual(report['held_keys'], 1)
            self.assertEqual(report['notification_events'], ['key_up:524288', 'make_victory:4:1', 'push:victory', 'key_up:131072', 'key_up:524288', 'control_lookup:35:65553', 'control_lookup:35:65555'])
            self.assertEqual(report['stage_state'], 1)  # Notification alone does not clear a quest.
            self.assertIsNone(report['retry_dialog'])
            self.assertEqual(self.run_host(scene, frames=240).stdout, result.stdout)

    def test_collection_credits_stage_totals_and_expires_sources_without_character_grants(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt'
            text = make_shoot_scene(root, rewards=True)
            text += ('collisions\nenemy_system 10000 3 10 25 7\nenemy_population 0 0 1 0 0 100 0\nenemy_pool 0\n'
                     'stage_notifications 1 13 0 0 0 0 -1\ncollection fresh\n')
            scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene, frames=240)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            count = report['defeated_enemies']
            self.assertGreater(count, 0)
            self.assertTrue(report['collection_enabled'])
            self.assertEqual(report['collection_steps'], 240)
            self.assertEqual(report['stage_rewards'], {'experience': 10 * count, 'money': 0, 'prisoners': 0, 'treasures': [0] * count})
            self.assertEqual(report['collection_sounds'], 2 * count)
            self.assertEqual(report['pending_drops'], [])
            self.assertEqual(report['collecting_drops'], 0)
            self.assertEqual(report['numeric_effects_live'], 0)
            self.assertEqual(report['positions'][0]['human_experience'], 0)
            self.assertEqual(self.run_host(scene, frames=240).stdout, result.stdout)
            scene.write_text(text.replace('stage_notifications 1 13', 'stage_notifications 0 13'), encoding='utf-8')
            inactive = self.run_host(scene, frames=240)
            self.assertEqual(inactive.returncode, 0, inactive.stderr)
            self.assertEqual(json.loads(inactive.stdout)['collection_steps'], 0)
            for invalid in [text + 'collection fresh\n', text.replace('collection fresh', 'collection saved'),
                            text.replace('stage_notifications 1 13 0 0 0 0 -1\n', '')]:
                scene.write_text(invalid, encoding='utf-8')
                self.assertNotEqual(self.run_host(scene).returncode, 0)

    def test_selected_money_prisoner_and_treasure_collection(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt'
            text = make_shoot_scene(root, rewards=True).replace('human_system 10 0 100 3 1', 'human_system 10 0 100 3 0')
            text += ('stage_notifications 1 0 0 0 0 0 -1\ncollection fresh\n'
                     'drop 1 7 25 200 0 100\ndrop 2 999 25 -200 0 100\ndrop 3 0 25 0 0 100\n')
            scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene, frames=80)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual(report['stage_rewards'], {'experience': 0, 'money': 7, 'prisoners': 1, 'treasures': [0]})
            self.assertEqual(report['collection_sounds'], 3)
            self.assertEqual(report['pending_drops'], [])
            self.assertTrue(all(e['destroyed'] for e in report['positions'][2:]))

    def test_boss_box_waits_for_defeat_then_collects_immediately(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt'
            text = make_shoot_scene(root, rewards=True, treasure_flags=1).replace('human_system 10 0 100 3 1', 'human_system 10 0 100 3 0')
            text += ('stage_notifications 1 0 0 0 0 0 -1\ncollection fresh\ndrop 3 0 25 0 0 100\n'
                     'enemy_system 10000 3 10 25 7\nenemy_pool 0\n')
            population = 'enemy_population 0 0 1 0 0 100 0\n'
            scene.write_text(text + population, encoding='utf-8')
            blocked = self.run_host(scene, frames=1)
            self.assertEqual(blocked.returncode, 0, blocked.stderr)
            report = json.loads(blocked.stdout)
            self.assertFalse(report['reward_box_picked_up'])
            self.assertEqual(report['stage_rewards']['treasures'], [])
            self.assertEqual(len(report['pending_drops']), 1)
            scene.write_text(text + population.replace('1 0 0 100', '1 0 1 100'), encoding='utf-8')
            collected = self.run_host(scene, frames=1)
            self.assertEqual(collected.returncode, 0, collected.stderr)
            report = json.loads(collected.stdout)
            self.assertTrue(report['reward_box_picked_up'])
            self.assertEqual(report['stage_rewards']['treasures'], [0])
            self.assertEqual(report['collection_jingles'], 1)
            self.assertEqual(report['collecting_drops'], 0)
            self.assertEqual(report['pending_drops'], [])

    def test_native_input_replays_movement_attack_and_dash(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt';replay = root / 'inputs.txt'
            text = make_shoot_scene(root, spell_attribute=0).replace('human_system 10 0 100 3 1', 'human_system 10 0 100 0 0')
            text += 'input fresh 0 0 0 0 0 0 0 0 0\n'
            scene.write_text(text, encoding='utf-8')
            replay.write_text('2 keys 131072\n10 keys 0\n', encoding='utf-8')
            moving = self.run_host(scene, '--replay', str(replay), frames=9)
            self.assertEqual(moving.returncode, 0, moving.stderr)
            a = json.loads(moving.stdout)
            self.assertEqual(a['input_steps'], 9)
            self.assertEqual(a['replay_actions'], 1)
            self.assertEqual(a['positions'][0]['human_state'], 1)
            self.assertLess(a['positions'][0]['z'], 100)
            self.assertEqual(a['shots'], 0)
            stopped = self.run_host(scene, '--replay', str(replay), frames=10)
            self.assertEqual(stopped.returncode, 0, stopped.stderr)
            b = json.loads(stopped.stdout)
            self.assertEqual(a['positions'][0]['z'], b['positions'][0]['z'])
            self.assertEqual(b['attack_buttons'][0]['state'], 1)
            self.assertTrue(b['move_front_pushed'])
            replay.write_text('1 attack 0\n', encoding='utf-8')
            attack = self.run_host(scene, '--replay', str(replay), frames=80)
            self.assertEqual(attack.returncode, 0, attack.stderr)
            self.assertGreater(json.loads(attack.stdout)['shots'], 0)
            self.assertEqual(self.run_host(scene, '--replay', str(replay), frames=80).stdout, attack.stdout)
            replay.write_text('2 keys 131072\n71 keys 0\n', encoding='utf-8')
            charged = self.run_host(scene, '--replay', str(replay), frames=70)
            self.assertEqual(charged.returncode, 0, charged.stderr)
            self.assertEqual(json.loads(charged.stdout)['positions'][0]['human_dash_speed'], 2)
            reset = self.run_host(scene, '--replay', str(replay), frames=71)
            self.assertEqual(reset.returncode, 0, reset.stderr)
            self.assertEqual(json.loads(reset.stdout)['positions'][0]['human_dash_speed'], 0)
            replay.write_text('1 keys 131072\n', encoding='utf-8')
            first = self.run_host(scene, '--replay', str(replay), frames=70)
            self.assertEqual(first.returncode, 0, first.stderr)
            self.assertEqual(json.loads(first.stdout)['positions'][0]['human_dash_speed'], 0)
            for bad in ['0 keys 0\n', '-1 keys 0\n', '2 keys 0\n1 keys 0\n', '1 attack 3\n', '1 keys 4294967296\n', '1 teleport 0\n', '1 pulse 1048576\n']:
                replay.write_text(bad, encoding='utf-8')
                self.assertNotEqual(self.run_host(scene, '--replay', str(replay)).returncode, 0)
            replay.write_text('1 keys 0\n', encoding='utf-8')
            scene.write_text(text.replace('input fresh 0 0 0 0 0 0 0 0 0\n', ''), encoding='utf-8')
            self.assertIn('input fresh', self.run_host(scene, '--replay', str(replay)).stderr)

    def test_map_camera_cells_and_live_visibility(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt'
            text = with_mission_map(root, make_shoot_scene(root))
            text = text.replace('human_system 10 0 100 3 1', 'human_system 10 0 100 3 0').replace('0 0 0 0 0 100\n', '0 0 0 0 0 1000\n')
            text += 'actor_state 0 1 -1\n'
            scene.write_text(text, encoding='utf-8')
            before = self.run_host(scene, frames=11)
            self.assertEqual(before.returncode, 0, before.stderr)
            result = self.run_host(scene, frames=12)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout);old = json.loads(before.stdout)['positions'][0]
            self.assertTrue(report['spatial_enabled'])
            self.assertEqual(report['mission_map'], {'width': 3, 'height': 12, 'size_width': 624, 'size_height': 312,
                                                    'step_x': 312, 'step_z': 312, 'chips_live': 36})
            self.assertEqual(report['camera_steps'], 12)
            self.assertEqual(report['cell_steps'], 12)
            self.assertEqual(report['visibility_steps'], 13)  # Initial LoadMap viewport plus twelve passes.
            self.assertGreaterEqual(report['cell_change_events'], 2)
            self.assertEqual(report['camera']['saved_x'], int(old['x']) - int(old['z']) + 20)
            self.assertEqual(report['camera']['saved_y'], int((int(old['x']) + int(old['z']) + 144) / 2) - 60)
            self.assertEqual(report['camera']['x'], report['camera']['saved_x'] - 64)
            self.assertEqual(report['camera']['y'], report['camera']['saved_y'] - 48)
            self.assertEqual(report['positions'][0]['cell_yi'], int((int(report['positions'][0]['z']) + 156) / 312))
            visible = {e['id'] for e in report['visible_entities']}
            self.assertIn(report['positions'][0]['id'], visible)
            self.assertNotIn(report['positions'][1]['id'], visible)
            self.assertLess(report['visible_count'], 38)
            self.assertEqual(self.run_host(scene, frames=12).stdout, result.stdout)
            for broken in [text + 'mission_map fresh 0 1 2 3\n', text.replace('fresh 0 1 2 3', 'saved 0 1 2 3'),
                           text.replace('geometry 312 312 12', 'geometry 24 24 12'), text.replace('player 0\n', '')]:
                scene.write_text(broken, encoding='utf-8')
                self.assertNotEqual(self.run_host(scene).returncode, 0)

    def test_native_boss_startup_creates_shared_world_enemies_and_fence(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt';text = make_boss_scene(root)
            scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene, frames=1)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertTrue(report['boss_entry_enabled'] and report['boss_started'])
            self.assertEqual(report['boss_creation_calls'], 1)
            self.assertEqual(report['min_z_rate'], 75)
            self.assertEqual(report['boss_fence']['x'], 140)
            self.assertEqual(report['boss_fence']['z'], 996)
            self.assertEqual(report['positions'][0]['x'], 312)
            self.assertEqual(report['rng_draws'], 0)
            boss, extra = report['enemies']
            self.assertEqual((boss['enemy_id'], boss['level'], boss['state'], boss['frame'], boss['flags']), (4, 7, 1, 0, 5))
            self.assertEqual((extra['enemy_id'], extra['level'], extra['frame']), (1, 1, 1))
            self.assertEqual(boss['target'], report['positions'][0]['id'])
            self.assertAlmostEqual(boss['z'], 213.5575, places=4)
            continued = self.run_host(scene, frames=12)
            self.assertEqual(continued.returncode, 0, continued.stderr)
            self.assertEqual(json.loads(continued.stdout)['boss_creation_calls'], 1)
            self.assertEqual(continued.stdout, self.run_host(scene, frames=12).stdout)
            scene.write_text(text.replace('312 0 400', '312 0 1000'), encoding='utf-8')
            before = self.run_host(scene, frames=1)
            self.assertEqual(before.returncode, 0, before.stderr)
            waiting = json.loads(before.stdout)
            self.assertFalse(waiting['boss_started'])
            self.assertEqual(waiting['enemies'], [])
            for broken in [text + 'stage_boss 4 7\n', text.replace('stage_events fresh 0\n', ''),
                           text.replace('enemy_pool 0\n', ''), text.replace('stage_loop 5000 0 20 hold\n', '')]:
                scene.write_text(broken, encoding='utf-8')
                self.assertNotEqual(self.run_host(scene, frames=1).returncode, 0)
            scene.write_text(text.replace('stage_boss 4 7\n', ''), encoding='utf-8')
            self.assertIn('stage_boss inputs', self.run_host(scene, frames=1).stderr)
            result_text = text + ('drop 3 0 25 312 0 450\n'
                                  'result_wallet 10 20 4\nresult_record 0 0 0 0 0 0 0\n'
                                  'result_inventory equipment 0 0 0\nresult_inventory cloth 0 0 0\nresult_inventory hat 0 0 0\n'
                                  'result_recruits 0 0 0\n')
            scene.write_text(result_text, encoding='utf-8')
            replay = root / 'boss-input.txt';replay.write_text('1 attack 0\n100 keys 131072\n130 keys 0\n400 pulse 1048576\n', encoding='utf-8')
            ended = self.run_host(scene, '--replay', str(replay), frames=450)
            self.assertEqual(ended.returncode, 0, ended.stderr)
            final = json.loads(ended.stdout)
            self.assertEqual(final['frames'], 401)
            self.assertTrue(final['boss_defeated'] and final['reward_box_picked_up'])
            self.assertTrue(final['result_granted'] and final['transition_pending'])
            self.assertIsNone(final['boss_fence'])
            self.assertEqual(final['boss_creation_calls'], 1)


    def test_native_stage_clear_grants_and_deferred_return_writeback(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt'
            text = make_shoot_scene(root, rewards=True, treasure_flags=1)
            text = text.replace('enemy 0 4 7 2', 'enemy 0 4 7 1').replace('progress 2 1 0', 'progress 2 1 9').replace('human 0 2 1 0 0 0 ', 'human 0 2 1 0 0 9 ')
            text += ('collisions\nenemy_system 10000 3 10 25 7\nenemy_population 0 0 1 0 0 100 0\nenemy_pool 0\n'
                     'stage_notifications 1 0 0 0 0 0 -1\ncollection fresh\ndrop 3 0 25 0 0 150\n'
                     'drop 1 7 25 100 0 100\ndrop 2 999 25 -100 0 100\n'
                     'result_wallet 10 20 4\nresult_record 0 0 0 0 0 0 0\n'
                     'result_inventory equipment 0 0 0\nresult_inventory cloth 0 0 0\nresult_inventory hat 0 0 0\n'
                     'result_recruits 0 0 0\nstage_loop 5000 1 20 hold\nstage_events fresh 0\nstage_pulse 200 1048576\n')
            scene.write_text(text, encoding='utf-8')
            result = self.run_host(scene, frames=240)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual(report['frames_requested'], 240)
            self.assertEqual(report['frames'], 201)
            self.assertEqual(report['movement_steps'], 200)
            self.assertGreater(report['collection_steps'], 0)
            self.assertTrue(report['transition_pending'])
            self.assertTrue(report['quest_completed'])
            self.assertEqual(report['stage_state'], 4)
            self.assertTrue(report['result_granted'])
            self.assertEqual(report['wallet'], {'money': 19, 'crystals': 23})
            self.assertEqual(report['rescued_prisoners'], 5)
            self.assertEqual(report['result_gear_levels'], {'weapon': 2, 'hat': 2, 'cloth': 2})
            self.assertEqual(report['quest_record']['challenges'], 1)
            self.assertEqual(report['quest_record']['clears'], 1)
            gained = report['quest_result']['experience']
            self.assertGreater(gained, 0)
            self.assertEqual(report['saved_characters'], [{'id': 2, 'level': 1 + (9 + gained) // 10, 'experience': (9 + gained) % 10}])
            self.assertEqual({(v['kind'], v['id'], v['stock']) for v in report['inventory_changes']}, {(0, 1, 1), (1, 0, 1), (2, 0, 1)})
            self.assertEqual(report['recruit_changes'], [{'id': 2, 'found': 1, 'is_new': True, 'recruited': 1}])
            self.assertEqual(report['notification_events'][-3:], ['make_result', 'push:result', 'transition:1'])
            self.assertEqual(self.run_host(scene, frames=240).stdout, result.stdout)
            # The same cleared state must wait for an explicit confirmation, without repeating grants.
            scene.write_text(text.replace('stage_pulse 200 1048576\n', ''), encoding='utf-8')
            waiting = self.run_host(scene, frames=240)
            self.assertEqual(waiting.returncode, 0, waiting.stderr)
            pending = json.loads(waiting.stdout)
            self.assertEqual(pending['stage_state'], 3)
            self.assertFalse(pending['result_granted'])
            self.assertEqual(pending['wallet'], {'money': 10, 'crystals': 20})
            self.assertEqual(pending['saved_characters'], [{'id': 2, 'level': 1, 'experience': 9}])
            scene.write_text(text.replace('stage_pulse 200', 'stage_pulse 1'), encoding='utf-8')
            early = self.run_host(scene, frames=240)
            self.assertEqual(early.returncode, 0, early.stderr)
            self.assertFalse(json.loads(early.stdout)['result_granted'])
            scene.write_text(text.replace('stage_pulse 200', 'stage_pulse -1'), encoding='utf-8')
            self.assertIn('positive logic step', self.run_host(scene).stderr)
            scene.write_text(text, encoding='utf-8')
            held = self.run_host(scene, frames=1000)
            self.assertEqual(held.returncode, 0, held.stderr)
            terminal = json.loads(held.stdout)
            self.assertEqual(terminal['frames'], 201)
            self.assertEqual(terminal['wallet'], report['wallet'])
            self.assertEqual(terminal['notification_events'].count('transition:1'), 1)
            scene.write_text(text.replace('stage_events fresh 0', 'stage_events fresh 1 99 0'), encoding='utf-8')
            self.assertIn('stage event catalog dispatch', self.run_host(scene).stderr)
            scene.write_text(text.replace('result_inventory hat 0 0 0\n', ''), encoding='utf-8')
            self.assertNotEqual(self.run_host(scene).returncode, 0)
            # Keep the encounter above the native312-unit grid's minimum stage Z.
            map_text = (text.replace('human 0 2 1 0 0 9 0 0 100', 'human 0 2 1 0 0 9 0 0 1000')
                        .replace('enemy 0 4 7 1 0 0 70', 'enemy 0 4 7 1 0 0 970')
                        .replace('drop 3 0 25 0 0 150', 'drop 3 0 25 0 0 1050')
                        .replace('drop 1 7 25 100 0 100', 'drop 1 7 25 100 0 1000')
                        .replace('drop 2 999 25 -100 0 100', 'drop 2 999 25 -100 0 1000'))
            scene.write_text(with_mission_map(root, map_text), encoding='utf-8')
            mapped = self.run_host(scene, frames=240)
            self.assertEqual(mapped.returncode, 0, mapped.stderr)
            cleaned = json.loads(mapped.stdout)
            self.assertEqual(cleaned['mission_map']['chips_live'], 0)
            self.assertEqual(cleaned['camera_steps'], 200)
            self.assertEqual(cleaned['saved_characters'], report['saved_characters'])
            self.assertTrue(cleaned['transition_pending'])
            # Native nonclear return records the attempt, but never runs result grants/completion.
            scene.write_text(text.replace('stage_notifications 1 0', 'stage_notifications 5 0'), encoding='utf-8')
            lost = self.run_host(scene, frames=240)
            self.assertEqual(lost.returncode, 0, lost.stderr)
            losing = json.loads(lost.stdout)
            self.assertEqual(losing['frames'], 1)
            self.assertFalse(losing['result_granted'])
            self.assertFalse(losing['quest_completed'])
            self.assertEqual(losing['quest_record']['challenges'], 1)
            self.assertEqual(losing['quest_record']['clears'], 0)
            self.assertEqual(losing['wallet'], {'money': 10, 'crystals': 20})
            # A native result confirmation can arrive from replay instead of a scene-timed pulse.
            make_shoot_scene(root, rewards=True, treasure_flags=1, spell_attribute=0)
            pulse_text = (text.replace('human_system 10 0 100 3 1', 'human_system 10 0 100 0 1')
                          .replace('stage_pulse 200 1048576\n', '') + 'input fresh 0 0 0 1 0 0 0 0 0\n')
            scene.write_text(pulse_text, encoding='utf-8')
            replay = root / 'result-input.txt';replay.write_text('200 pulse 1048576\n', encoding='utf-8')
            confirmed = self.run_host(scene, '--replay', str(replay), frames=240)
            self.assertEqual(confirmed.returncode, 0, confirmed.stderr)
            cleared = json.loads(confirmed.stdout)
            self.assertEqual(cleared['frames'], 201)
            self.assertTrue(cleared['result_granted'] and cleared['transition_pending'])
            self.assertEqual(cleared['parameter_experience_steps'], 200)
            self.assertEqual(cleared['parameter_steps'], 200)
            self.assertEqual(cleared['effect_steps'], 200)
            self.assertEqual(cleared['battle_steps'], 201)
            self.assertEqual(cleared['last_update_order'], [0,2,3,21])
            self.assertEqual(cleared['replay_actions'], 1)
            no_confirmation = self.run_host(scene, frames=240)
            self.assertEqual(no_confirmation.returncode, 0, no_confirmation.stderr)
            self.assertEqual(json.loads(no_confirmation.stdout)['stage_state'], 3)
            self.assertFalse(json.loads(no_confirmation.stdout)['result_granted'])


if __name__ == '__main__':
    unittest.main()
