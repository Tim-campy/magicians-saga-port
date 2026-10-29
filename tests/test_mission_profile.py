"""Always-built headless host contracts on synthetic game records and SEBs."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXE = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None
sys.path.insert(0, str(Path(__file__).resolve().parent))
from mission_scene_fixture import make_table_scene, make_shoot_scene


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


if __name__ == '__main__':
    unittest.main()
