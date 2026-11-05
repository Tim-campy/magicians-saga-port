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
        return subprocess.run([str(EXE),'--data',str(root),'--foreground','--shared-world',
            '--update-passes',str(n),'--physical-input','--camera-x','300',
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
            args=self.replay(root,'165 widget 0 250 250\n166 widget 1 250 250\n')
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
            args=self.replay(root,'165 widget 0 250 250\n166 widget 1 250 250\n')
            mission_input=root/'mission.replay';mission_input.write_text('70 keys 1\n80 keys 0\n',encoding='ascii')
            neutral=self.state(root,300,*args)
            replay=self.state(root,300,*args,'--mission-replay',str(mission_input))
            selected=replay['update']['mission']
            self.assertEqual(selected['replay_actions'],2)
            self.assertEqual(selected['logic_steps'],93)
            self.assertEqual(selected['event_execution_counts'],neutral['update']['mission']['event_execution_counts'])
            self.assertNotEqual(selected['logic_state_hash'],neutral['update']['mission']['logic_state_hash'])
            self.assertEqual(replay,self.state(root,300,*args,'--mission-replay',str(mission_input)))

    def test_retreat_holds_the_same_world_at_return_without_repeating_entry(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);self.fixture(root)
            args=self.replay(root,'165 widget 0 250 250\n166 widget 1 250 250\n')
            mission_input=root/'retreat.replay'
            mission_input.write_text(''.join(f'{i} pulse 1048576\n' for i in range(30,181,5))+
                '200 pulse 33554432\n201 pulse 134217728\n202 answer 0\n',encoding='ascii')
            held=self.state(root,600,*args,'--mission-replay',str(mission_input));update=held['update'];mission=update['mission']
            self.assertEqual((update['dispatches'],mission['logic_steps']),(409,202))
            self.assertEqual(mission['quest_exit'],{'code':3,'outcome':'retreat','handoff':'held','rewards_granted':False})
            self.assertTrue(mission['transition_pending'] and held['shared_world']['same_hero'])
            self.assertEqual((update['stamina'],update['stage_menu']['stamina_uses'],mission['quest_detail_count']),(9,1,1))
            self.assertEqual(held,self.state(root,900,*args,'--mission-replay',str(mission_input)))

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
