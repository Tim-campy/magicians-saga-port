"""Retained native World ownership through town selection and native mission entry."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXE=Path(sys.argv.pop(1)).resolve() if len(sys.argv)>1 else None
sys.path.insert(0,str(Path(__file__).resolve().parent))
from town_campaign_fixture import create_campaign_fixture


@unittest.skipUnless(EXE and os.environ.get('SAGA_TEST_FONT'),'CTest host and public font required')
class TownCampaignTests(unittest.TestCase):
    opening=('160 widget 0 200 450\n161 widget 1 200 450\n'
             '162 widget 0 200 480\n163 widget 1 200 480\n')

    def fixture(self,root):
        return create_campaign_fixture(root)

    def run_host(self,root,n,*args):
        args=list(args);camera='300'
        if '--camera-x' in args:
            index=args.index('--camera-x');camera=args[index+1];del args[index:index+2]
        return subprocess.run([str(EXE),'--data',str(root),'--foreground','--shared-world',
            '--update-passes',str(n),'--physical-input','--camera-x',camera,
            '--talk-pulse-every','10','--server-time','123',*args],
            capture_output=True,text=True,timeout=30)

    def state(self,root,n,*args):
        result=self.run_host(root,n,*args)
        self.assertEqual(result.returncode,0,result.stderr)
        return json.loads(result.stdout)['background']

    def replay(self,root,tail=''):
        path=root/'campaign.replay';path.write_text(self.opening+tail,encoding='ascii')
        return ('--replay',str(path))

    def assert_waiting_world(self,scene,seed=42):
        shared=scene['shared_world']
        self.assertTrue(shared['awaiting_quest'])
        self.assertEqual(shared['initialized_systems'],32)
        self.assertTrue(shared['same_hero'] and shared['canonical_quests'] and shared['canonical_areas'])
        self.assertEqual((shared['gameplay_seed'],shared['gameplay_rng_draws']),(seed,0))
        self.assertEqual((shared['characters'],shared['formation_slots']),(10,16))
        hero=scene['hero']
        self.assertEqual((hero['character'],hero['level'],hero['experience']),(2,1,0))
        self.assertEqual(hero['formation'],[-1]*10+[2]+[-1]*5)

    def test_resident_gift_credits_shared_wallet_once_and_native_popup_returns(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            context=('--lib-seed','1') # Native gift lottery; no resident state injection.
            before=self.state(root,60,*context);gifts=[r for r in before['residents']['rows'] if r[8]==3]
            self.assertEqual(len(gifts),1)
            amount=before['residents']['present'];self.assertEqual(amount,300)
            target=next(t for t in before['residents']['touches'] if t[0]==3)
            x,y=target[1]+16,target[2]+16
            replay=root/'gift.replay';replay.write_text(f'61 widget 0 {x} {y}\n62 widget 1 {x} {y}\n80 buttons 0x02000000\n')
            args=(*context,'--replay',str(replay))
            down=self.state(root,61,*args)['update']
            self.assertTrue(down['widgets']['captured']);self.assertEqual(down['wallet'],before['update']['wallet'])
            credited=self.state(root,62,*args);state=credited['update']
            self.assertEqual(state['wallet']['money'],before['update']['wallet']['money']+amount)
            self.assertEqual(state['wallet']['crystals'],before['update']['wallet']['crystals'])
            self.assertEqual(credited['residents']['rows'][0][8],0)
            popup=state['popup_contents']
            self.assertEqual((popup['type'],popup['kind'],popup['option']),(19,3,False))
            self.assertEqual((popup['title'],popup['text']),('Synthetic gift','Gift 300G'))
            self.assertEqual((popup['production_state'],popup['production_frame'],popup['animation_frame']),(1,0,0))
            first=self.state(root,63,*args)['update'];opened=self.state(root,67,*args)['update']
            self.assertEqual(first['calls'],state['calls']);self.assertEqual(first['popup_contents']['production_frame'],1)
            self.assertEqual((opened['popup_contents']['production_state'],opened['popup_contents']['production_frame']),(0,0))
            closing=self.state(root,70,*args)['update'];near=self.state(root,74,*args)['update']
            self.assertEqual((closing['popup_contents']['production_state'],closing['popup_contents']['production_frame']),(2,0))
            self.assertEqual(near['popup_contents']['production_frame'],4);self.assertTrue(near['talk_pending'])
            resumed=self.state(root,75,*args)['update'];again=self.state(root,100,*args)['update']
            self.assertFalse(resumed['talk_pending']);self.assertNotIn('popup_contents',resumed)
            self.assertGreater(resumed['calls'],state['calls']);self.assertEqual(again['wallet'],state['wallet'])
            self.assertEqual(again['talk_lifecycle'].count('push:-1'),1)
            self.assertEqual(again['talk_lifecycle'].count('pop:-1'),1)
            self.assertTrue(credited['shared_world']['same_hero']);self.assertEqual(credited['shared_world']['initialized_systems'],32)

    def test_gift_template_translation_precedes_numeric_substitution(self):
        from town_campaign_fixture import _container_reader
        from test_sprite_preview import jar
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            archive=root/'textassets/tables.bin';members=dict(_container_reader()(archive.read_bytes()))
            members['English.lproj/text.txt']=b'Synthetic gift\tOffer\nGift <0>G\tBonus <0> credits\n'
            archive.write_bytes(jar(list(members.items())))
            replay=root/'gift.replay';replay.write_text('62 buttons 0x02000000\n')
            state=self.state(root,62,'--lib-seed','1','--replay',str(replay))['update']
            self.assertEqual((state['popup_contents']['title'],state['popup_contents']['text']),('Offer','Bonus 300 credits'))

    def test_gift_message_projection_is_lazy_and_rejects_invalid_schema(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root);context=('--lib-seed','1')
            replay=root/'gift.replay';replay.write_text('62 buttons 0x02000000\n')
            path=root/'town-messages.tsv';path.unlink()
            self.assertEqual(self.run_host(root,60,*context).returncode,0)
            failed=self.run_host(root,62,*context,'--replay',str(replay))
            self.assertNotEqual(failed.returncode,0);self.assertIn('--town-messages',failed.stderr)
            for payload in ('town_messages 2\n','town_messages 1\ngift_format "wrong order"\n',
                            'town_messages 1\ngift_title unquoted\n',
                            'town_messages 1\ngift_title "x"\ngift_format "<0>"\nextra 1\n'):
                with self.subTest(payload=payload):
                    path.write_text(payload)
                    self.assertNotEqual(self.run_host(root,62,*context,'--replay',str(replay)).returncode,0)

    def test_king_hall_grants_all_eligible_rewards_once_then_returns(self):
        from town_campaign_fixture import add_king_reward_case
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root);add_king_reward_case(root)
            before=self.state(root,60,'--high-score','20')
            touch=next(row for row in before['buildings_draw']['touches'] if row[0]==1)
            x,y=touch[2]+40,touch[3]+45
            replay=root/'king.replay';replay.write_text(
                f'61 widget 0 {x} {y}\n62 widget 1 {x} {y}\n63 widget 0 {x} {y}\n64 widget 1 {x} {y}\n'
                '112 form-back\n151 form-back\n'
                f'181 widget 0 {x} {y}\n182 widget 1 {x} {y}\n183 widget 0 {x} {y}\n184 widget 1 {x} {y}\n271 form-back\n')
            args=('--high-score','20','--replay',str(replay))
            selected=self.state(root,61,*args)['update'];self.assertTrue(selected['widgets']['captured'])
            entered=self.state(root,64,*args)['update'];self.assertEqual(entered['talk']['id'],-15)
            opening=self.state(root,65,*args)['update'];self.assertEqual(opening['king_hall']['frame'],1)
            self.assertEqual(opening['king_hall']['production_state'],1)
            ready=self.state(root,72,*args)['update'];self.assertEqual(ready['king_hall']['production_state'],0)
            self.assertEqual(ready['king_hall']['movement_frame'],0)
            threshold=self.state(root,110,*args)['update'];self.assertEqual(threshold['king_grant_passes'],0)
            granted=self.state(root,111,*args)['update'];self.assertEqual(granted['king_grant_passes'],1)
            self.assertEqual(granted['king_rewards'],[[0,1],[1,1]])
            self.assertEqual(granted['wallet'],{'money':48,'crystals':53})
            self.assertEqual(granted['talk']['id'],9)
            self.assertEqual(granted['talk_lifecycle'].count('push:9'),1)
            self.assertEqual(granted['talk_lifecycle'].count('reinsert:9'),1)
            closed=self.state(root,160,*args)['update'];self.assertFalse(closed['talk_pending'])
            self.assertEqual(closed['talk_lifecycle'].count('pop:-15'),1)
            again=self.state(root,280,*args);self.assertEqual(again['update']['wallet'],granted['wallet'])
            self.assertEqual(again['update']['king_grant_passes'],2);self.assertFalse(again['update']['talk_pending'])
            self.assertEqual(again['update']['talk_lifecycle'].count('push:9'),1)
            self.assertEqual(again['shared_world']['initialized_systems'],32)
            self.assertTrue(again['shared_world']['same_hero'])
            self.assertEqual(again['shared_world']['gameplay_rng_draws'],0)
            self.assertTrue(any(b[0]==4 for b in again['update']['building_progress']))

    def test_construction_installs_shared_plots_spends_crystals_and_consumes_stock(self):
        from town_campaign_fixture import add_construction_case
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root);add_construction_case(root)
            context=('--camera-x','0');before=self.state(root,60,*context)
            replay=root/'construction.replay';actions=[]
            for slot,start in ((2,61),(3,121),(4,181)):
                t=next(r for r in before['buildings_draw']['touches'] if r[0]==slot);x,y=t[2]+40,t[3]+45
                actions.extend(f'{start+i} widget {i%2} {x} {y}' for i in range(4))
                if slot!=4:
                    row=slot-2;x=196+106*row+52;y=214+26
                    actions.extend(f'{start+12+i} widget {i%2} {x} {y}' for i in range(4))
                else:
                    actions.extend(['193 widget 0 354 240','194 widget 1 354 240','195 form-back'])
            # Native ClearKeyBuffer discards Back received on pending Init.
            actions.append('65 form-back');actions.sort(key=lambda line:int(line.split()[0]))
            replay.write_text('\n'.join(actions)+'\n');args=(*context,'--replay',str(replay))
            first=self.state(root,72,*args)['update'];self.assertEqual(first['build_list']['entries'],[0,1])
            self.assertEqual(first['build_list']['selected'],-1)
            selected=self.state(root,74,*args)['update'];self.assertEqual(selected['build_list']['selected'],0)
            self.assertEqual(selected['wallet'],before['update']['wallet']);self.assertEqual(selected['constructions'],0)
            installed=self.state(root,76,*args);u=installed['update']
            self.assertEqual(u['wallet'],{'money':37,'crystals':36});self.assertEqual(u['constructions'],1)
            self.assertNotIn('build_list',u);self.assertEqual(u['building_complete']['building'],0)
            self.assertEqual(u['building_complete']['frame'],0)
            full=json.loads(self.run_host(root,76,*args).stdout);self.assertIn([2,0,0],full['buildings']);self.assertIn([2,0,0],full['source_buildings'])
            self.assertIn([0,1,1,1],u['building_progress'])
            fresh=self.state(root,77,*args)['update'];self.assertEqual(fresh['building_complete']['frame'],1)
            early=self.state(root,106,*args)['update'];self.assertTrue(early['talk_pending'])
            done=self.state(root,110,*args)['update'];self.assertFalse(done['talk_pending'])
            second=self.state(root,136,*args)['update'];self.assertEqual(second['constructions'],2)
            self.assertEqual(second['wallet'],{'money':37,'crystals':24});self.assertIn([1,1,1,2],second['building_progress'])
            quote=self.state(root,192,*args)['update'];self.assertIn([1,2,25,0],quote['construction_widgets']['quotes'])
            refused=self.state(root,194,*args)['update'];self.assertEqual(refused['constructions'],2)
            self.assertEqual(refused['wallet'],second['wallet']);self.assertEqual(refused['build_list']['selected'],1)
            closed=self.state(root,220,*args);self.assertFalse(closed['update']['talk_pending'])
            self.assertEqual(closed['update']['constructions'],2);self.assertEqual(closed['update']['wallet'],second['wallet'])
            self.assertIn([3,1,0],json.loads(self.run_host(root,220,*args).stdout)['buildings']);self.assertEqual(closed['shared_world']['initialized_systems'],32)
            self.assertTrue(closed['shared_world']['same_hero']);self.assertEqual(closed['shared_world']['gameplay_rng_draws'],0)
            self.assertEqual(closed['update']['talk_lifecycle'].count('push:-17'),2)
            # Construction owners also survive QuestForm entry/return, not just menu visits.
            with replay.open('a') as out:out.write('225 select 0xc0000 0\n226 buttons 0x10000000\n240 widget 0 250 250\n240 widget 1 250 250\n240 widget 0 250 250\n241 widget 1 250 250\n')
            mission=root/'build-retreat.replay';mission.write_text(''.join(f'{i} pulse 1048576\n' for i in range(30,181,5))+
                '200 pulse 33554432\n201 pulse 134217728\n202 answer 0\n')
            returned=self.state(root,550,*args,'--mission-replay',str(mission))
            self.assertEqual(returned['update']['town_initializations'],2)
            self.assertEqual(returned['update']['constructions'],2)
            self.assertEqual(returned['update']['wallet'],second['wallet'])
            self.assertIn([0,1,1,1],returned['update']['building_progress'])
            self.assertIn([1,1,1,2],returned['update']['building_progress'])
            full=json.loads(self.run_host(root,550,*args,'--mission-replay',str(mission)).stdout)
            self.assertIn([2,0,0],full['buildings']);self.assertIn([3,1,0],full['buildings'])

    def test_full_world_initializes_once_and_shared_intro_pauses_town(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            first=self.state(root,1);self.assert_waiting_world(first)
            self.assertEqual(first['update']['event_execution_counts'],[[0,1]])
            self.assertEqual(first['update']['delay_count'],1)
            self.assertEqual(first['update']['talk_forms_opened'],0)
            paused=self.state(root,3);later=self.state(root,5)
            for scene in (paused,later):
                self.assert_waiting_world(scene)
                update=scene['update']
                self.assertEqual((update['calls'],update['talk_forms_opened']),(2,2))
                self.assertTrue(update['talk_pending'])
                self.assertEqual(update['event_execution_counts'],[[0,1]])
                self.assertEqual(update['talk_lifecycle'].count('push:0'),1)
                self.assertEqual(update['talk_lifecycle'].count('push:1'),1)
            self.assertEqual(paused['shared_world'],later['shared_world'])
            self.assertEqual(paused['scenery'],later['scenery'])
            resumed=self.state(root,160);self.assert_waiting_world(resumed)
            self.assertEqual((resumed['update']['talk_forms_opened'],resumed['update']['talk_forms_closed']),(2,2))
            self.assertFalse(resumed['update']['talk_pending'])
            self.assertEqual(resumed['update']['event_execution_counts'],[[0,1]])
            self.assertIsNone(resumed['update']['stage_menu'])

    def test_town_config_debug_does_not_enable_damage_debug(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            normal=self.state(root,1)['shared_world']
            debug=self.state(root,1,'--debug','1','--debug-rescue-count','0')['shared_world']
            self.assertFalse(normal['config_debug'] or normal['damage_debug'])
            self.assertTrue(debug['config_debug']);self.assertFalse(debug['damage_debug'])
            self.assertEqual(debug['initialized_systems'],32)

    def test_menu_confirmation_starts_stage_without_recreating_the_world(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=self.replay(root,'165 widget 0 250 250\n165 widget 1 250 250\n165 widget 0 250 250\n166 widget 1 250 250\n')
            opened=self.state(root,164,*args);self.assert_waiting_world(opened)
            menu=opened['update']['stage_menu']
            self.assertEqual((menu['entries'],menu['selected'],menu['saved_quest']),([2],-1,-1))
            self.assertEqual(menu['rows'],[[10,0x20000,195,232,210,40]])
            down=self.state(root,165,*args);self.assert_waiting_world(down)
            self.assertEqual((down['update']['stage_menu']['selected'],down['update']['stamina']),(0,10))
            confirmed=self.state(root,166,*args);self.assert_waiting_world(confirmed)
            menu=confirmed['update']['stage_menu']
            self.assertEqual((menu['state'],menu['saved_quest'],menu['stamina_uses']),(1,2,1))
            self.assertEqual((confirmed['update']['stamina'],menu['stamina_timer'],menu['stamina_timestamp']),(9,900,123))
            self.assertEqual(menu['server_queries'],1)
            before=self.state(root,206,*args);self.assert_waiting_world(before)
            self.assertEqual(before['update']['stage_menu']['production_frame'],40)
            self.assertFalse(before['update']['stage_menu']['quest_handoff_pending'])
            entered=self.state(root,207,*args)
            shared=entered['shared_world'];menu=entered['update']['stage_menu'];mission=entered['update']['mission']
            self.assertFalse(shared['awaiting_quest'])
            self.assertEqual(shared['initialized_systems'],32)
            self.assertTrue(shared['same_hero'] and shared['canonical_quests'] and shared['canonical_areas'])
            self.assertEqual(entered['hero']['formation'],opened['hero']['formation'])
            self.assertEqual(menu['production_frame'],41)
            self.assertFalse(menu['quest_handoff_pending'] or menu['pending'])
            self.assertEqual((menu['closed'],menu['stamina_uses']),(1,1))
            self.assertEqual(entered['update']['mission_entry_frame'],207)
            self.assertTrue(entered['update']['mission_active'] and mission['native_stage_started'])
            self.assertEqual((mission['logic_steps'],mission['stage_entry_stage'],mission['quest_detail_count']),(0,2,1))
            self.assertEqual(mission['party_formation'],entered['hero']['formation'])
            self.assertEqual(mission['state_hash_schema'],'mission-v2-xxh64')
            self.assertEqual(entered['update']['event_execution_counts'],[[0,1]])
            self.assertEqual((entered['update']['talk_forms_opened'],entered['update']['talk_forms_closed']),(2,2))
            stepped=self.state(root,208,*args)
            self.assertEqual(stepped['update']['mission']['logic_steps'],1)
            self.assertEqual(stepped['update']['calls'],entered['update']['calls'])
            running=self.state(root,300,*args)
            self.assertEqual(running['update']['mission']['logic_steps'],93)
            self.assertEqual(running['update']['mission']['stage_state'],1)
            self.assertEqual(running['update']['mission']['quest_detail_count'],1)
            self.assertLess(running['update']['mission']['remaining_time'],mission['remaining_time'])
            self.assertEqual(running,self.state(root,300,*args))

    def test_mission_replay_uses_relative_steps_and_preserves_town_event_history(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=self.replay(root,'165 widget 0 250 250\n165 widget 1 250 250\n165 widget 0 250 250\n166 widget 1 250 250\n')
            mission_input=root/'mission.replay';mission_input.write_text('70 keys 1\n80 keys 0\n',encoding='ascii')
            neutral=self.state(root,300,*args)
            replay=self.state(root,300,*args,'--mission-replay',str(mission_input))
            selected=replay['update']['mission']
            self.assertEqual(selected['replay_actions'],2)
            self.assertEqual(selected['logic_steps'],93)
            self.assertEqual(selected['event_execution_counts'],neutral['update']['mission']['event_execution_counts'])
            self.assertNotEqual(selected['logic_state_hash'],neutral['update']['mission']['logic_state_hash'])
            self.assertEqual(replay,self.state(root,300,*args,'--mission-replay',str(mission_input)))

    def test_retreat_returns_to_the_same_world_without_repeating_entry(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=self.replay(root,'165 widget 0 250 250\n165 widget 1 250 250\n165 widget 0 250 250\n166 widget 1 250 250\n')
            mission_input=root/'retreat.replay'
            mission_input.write_text(''.join(f'{i} pulse 1048576\n' for i in range(30,181,5))+
                '200 pulse 33554432\n201 pulse 134217728\n202 answer 0\n',encoding='ascii')
            inputs=(*args,'--mission-replay',str(mission_input))
            pending=self.state(root,409,*inputs)
            self.assertTrue(pending['update']['mission']['transition_pending'])
            callback=self.state(root,410,*inputs)['update']
            self.assertFalse(callback['mission_active']);self.assertTrue(callback['town_init_pending'])
            self.assertTrue(callback['quest_return_flag'])
            self.assertEqual(callback['return_order'],['change_form','event31','return_flag'])
            returned=self.state(root,411,*inputs);update=returned['update']
            self.assertEqual(update['town_initializations'],2)
            self.assertFalse(update['town_init_pending'] or update['quest_return_flag'])
            self.assertEqual(update['return_order'],['change_form','event31','return_flag','init','return_position','clear_return_flag'])
            self.assertTrue(returned['shared_world']['same_hero'] and returned['shared_world']['awaiting_quest'])
            self.assertEqual(returned['shared_world']['initialized_systems'],32)
            completed=update['completed_missions'];self.assertEqual(len(completed),1)
            self.assertEqual((completed[0]['frame'],completed[0]['event31']),(410,[3,0,2]))
            self.assertEqual(completed[0]['mission_at_return']['logic_steps'],202)
            self.assertEqual(completed[0]['mission_at_return']['quest_detail_count'],1)
            later=self.state(root,600,*inputs)['update']
            self.assertEqual(len(later['completed_missions']),1)
            self.assertEqual((later['stamina'],later['stage_menu']['stamina_uses']),(9,1))
            self.assertEqual(later['wallet'],update['wallet'])
            self.assertGreater(later['calls'],update['calls'])
            self.assertEqual(self.state(root,600,*inputs)['update'],later)

    def test_callback_and_return_init_discard_hidden_town_input(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=self.replay(root,'165 widget 0 250 250\n165 widget 1 250 250\n165 widget 0 250 250\n166 widget 1 250 250\n')
            mission_input=root/'retreat.replay';mission_input.write_text(''.join(f'{i} pulse 1048576\n' for i in range(30,181,5))+
                '200 pulse 33554432\n201 pulse 134217728\n202 answer 0\n',encoding='ascii')
            inputs=(*args,'--mission-replay',str(mission_input))
            baseline=self.state(root,412,*inputs)
            replay=Path(args[1]);replay.write_text(replay.read_text(encoding='ascii')+
                '410 widget 0 250 500\n411 widget 1 250 500\n',encoding='ascii')
            ignored=self.state(root,412,*inputs)
            self.assertEqual(ignored['areas_draw']['selected'],baseline['areas_draw']['selected'])
            self.assertEqual(ignored['update']['widgets']['events'],baseline['update']['widgets']['events'])
            self.assertEqual(ignored['update']['replay_modal_ignored'],baseline['update']['replay_modal_ignored']+2)
            self.assertEqual(ignored['update']['replay_remaining'],0)

    def test_two_native_entries_preserve_rng_wallet_and_canonical_records(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=self.replay(root,'165 widget 0 250 250\n165 widget 1 250 250\n165 widget 0 250 250\n166 widget 1 250 250\n')
            mission_input=root/'retreat.replay';mission_input.write_text(''.join(f'{i} pulse 1048576\n' for i in range(30,181,5))+
                '200 pulse 33554432\n201 pulse 134217728\n202 answer 0\n',encoding='ascii')
            inputs=(*args,'--mission-replay',str(mission_input))
            returned=self.state(root,500,*inputs)
            region=next(r for r in returned['update']['widgets']['regions'] if r[1]==0xc0000)
            x,y=region[2]+region[4]//2,region[3]+region[5]//2
            replay=Path(args[1]);replay.write_text(replay.read_text(encoding='ascii')+f'501 widget 0 {x} {y}\n502 widget 1 {x} {y}\n',encoding='ascii')
            selected=self.state(root,502,*inputs)
            region=next(r for r in selected['update']['widgets']['regions'] if r[0]==4 and r[1]==28)
            x,y=region[2]+region[4]//2,region[3]+region[5]//2
            replay.write_text(replay.read_text(encoding='ascii')+f'503 widget 0 {x} {y}\n504 widget 1 {x} {y}\n506 widget 0 250 250\n507 widget 1 250 250\n',encoding='ascii')
            entered=self.state(root,548,*inputs)
            self.assertTrue(entered['update']['mission_active'])
            self.assertEqual(entered['update']['mission']['logic_steps'],0)
            self.assertEqual(entered['update']['mission']['quest_detail_count'],2)
            self.assertEqual(entered['update']['mission']['party_formation'],returned['hero']['formation'])
            self.assertGreater(entered['shared_world']['gameplay_rng_draws'],returned['shared_world']['gameplay_rng_draws'])
            complete=self.state(root,900,*inputs);update=complete['update']
            self.assertEqual((complete['shared_world']['initialized_systems'],update['town_initializations']),(32,3))
            self.assertTrue(complete['shared_world']['same_hero'])
            self.assertEqual(len(update['completed_missions']),2)
            self.assertEqual(update['stamina'],8)
            self.assertEqual(update['wallet'],returned['update']['wallet'])
            self.assertEqual(update['completed_missions'][1]['mission_at_return']['quest_record']['challenges'],2)
            self.assertEqual(complete,self.state(root,900,*inputs))

    def test_shared_world_requires_extracted_town_scale_but_accepts_signed_v2_input(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            path=root/'new-game-save.tsv';payload=path.read_text(encoding='ascii')
            path.write_text(payload.replace('new_game_save 2','new_game_save 1').replace('town_scale 100\n',''),encoding='ascii')
            rejected=self.run_host(root,1);self.assertNotEqual(rejected.returncode,0)
            self.assertIn('new-game-save v2',rejected.stderr)
            path.write_text(payload.replace('town_scale 100','town_scale -2147483648'),encoding='ascii')
            self.assertEqual(self.run_host(root,1).returncode,0) # Parser preserves width; SetScale is an entry-time contract.
            path.write_text(payload.replace('town_scale 100','town_scale 2147483648'),encoding='ascii')
            self.assertNotEqual(self.run_host(root,1).returncode,0)

    def test_back_returns_to_same_world_and_formation_without_consumption(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=self.replay(root,'165 widget 0 250 250\n166 stage-back\n')
            selected=self.state(root,165,*args);resumed=self.state(root,166,*args)
            self.assert_waiting_world(resumed)
            self.assertEqual(selected['shared_world'],resumed['shared_world'])
            menu=resumed['update']['stage_menu']
            self.assertFalse(menu['pending'] or menu['quest_handoff_pending'])
            self.assertEqual((menu['closed'],menu['saved_quest'],menu['stamina_uses']),(1,-1,0))
            self.assertEqual(resumed['update']['stamina'],10)
            self.assertEqual(resumed['update']['calls'],selected['update']['calls']+1)
            self.assertEqual(resumed['update']['event_execution_counts'],[[0,1]])

    def test_shared_world_and_gameplay_seed_require_typed_context(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            def direct(*args):
                return subprocess.run([str(EXE),'--data',str(root),'--foreground',*args],capture_output=True,text=True,timeout=30)
            rejected=direct('--mission-replay','missing.replay')
            self.assertNotEqual(rejected.returncode,0);self.assertIn('mission replay requires shared world',rejected.stderr)
            rejected=direct('--shared-world')
            self.assertNotEqual(rejected.returncode,0);self.assertIn('shared world requires full town Update',rejected.stderr)
            rejected=direct('--update-passes','1','--gameplay-seed','42')
            self.assertNotEqual(rejected.returncode,0);self.assertIn('gameplay seed requires shared world',rejected.stderr)
            for seed in ('2147483648','-2147483649','1.0','unknown'):
                with self.subTest(seed=seed):self.assertNotEqual(self.run_host(root,1,'--gameplay-seed',seed).returncode,0)
            for seed in (-2147483648,2147483647):
                with self.subTest(seed=seed):self.assert_waiting_world(self.state(root,1,'--gameplay-seed',str(seed)),seed)


if __name__=='__main__':unittest.main()
