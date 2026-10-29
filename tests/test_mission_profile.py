"""Always-built headless host contracts on synthetic game records and SEBs."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXE = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None
sys.path.insert(0, str(Path(__file__).resolve().parent))
from mission_scene_fixture import make_table_scene, make_shoot_scene, with_mission_map, make_boss_scene


@unittest.skipUnless(EXE, 'saga_mission supplied by CTest')
class MissionProfileTests(unittest.TestCase):
    def run_host(self, scene, *extra, fps=25, frames=12):
        return subprocess.run([str(EXE), '--scene', str(scene), '--frames', str(frames), '--headless',
                               '--fps', str(fps), '--debug', '0', '--rng', 'system', '--seed', '42', *extra],
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
            self.assertFalse(report['update_schedule_complete'])
            self.assertEqual(report['bound_update_systems'], [0,2,3,4,5,8,9,10,11,12,14,15,17,18,19,20,21,23,31])
            self.assertEqual(report['unbound_update_systems'], [13,16,22,24,25,26,30])
            self.assertEqual(report['last_update_order'], [2,9,11,14,15,18,19,21,23,31])
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

    def test_quest_table_drives_map_boss_and_preserves_saved_fields(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt'
            original = make_boss_scene(root)
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
