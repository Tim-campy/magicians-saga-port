"""Synthetic multi-resource queues -> shared backend -> SDL pixels."""
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

EXE = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None
VIEWER = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None
MISSION = Path(sys.argv.pop(1)).resolve() if len(sys.argv) > 1 else None
sys.path.insert(0, str(Path(__file__).resolve().parent))
from test_sprite_preview import png, seb, jar, bmp_pixels
from mission_scene_fixture import make_table_scene, make_shoot_scene, with_mission_map, make_boss_scene, make_gauge_scene


def command(resource, clip, x, y, depth=0, texture=-1, image_resource=-1):
    cells = [texture, resource, depth, clip, 0, x, y, -1, -1, -1, -1, -1, -1, 0, 255, -1, 0, image_resource, 0]
    return 'draw ' + ' '.join(map(str, cells)) + '\n'


@unittest.skipUnless(EXE and VIEWER, 'viewer executables supplied by CTest')
class QueuePixels(unittest.TestCase):
    def test_quest_pinch_uses_native_offscreen_source_and_presented_capture(self):
        if not MISSION:self.skipTest('mission executable required')
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'zoom.txt';replay=root/'zoom.replay'
            original=make_boss_scene(root)
            text=('mission_presentation 128 96 100 0 1 0 1500100 -1\nsystem_init 1\nmaterial_policy native\ntick_ms 1\n'+
                  '\n'.join(line for line in original.splitlines() if not line.startswith(('world ','projection ','view ','slots ','animate ','simulate ','material ')))+'\n')
            scene.write_text(text,encoding='utf8');replay.write_text('1 camera 12 0 0 40\n1 camera 1 0 0 0\n2 camera 8 12 -6 0\n',encoding='ascii')
            args=[str(MISSION),'--scene',str(scene),'--frames','3','--fps','25','--debug','0','--rng','system','--seed','42']
            def run(extra):
                result=subprocess.run([*args,*extra],capture_output=True,text=True,timeout=30)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            baseline=root/'baseline.bmp';run(['--capture',str(baseline)])
            output=root/'zoom.bmp';capture=run(['--replay',str(replay),'--capture',str(output)])
            self.assertEqual((capture['surface_width'],capture['surface_height']),(106,80))
            self.assertEqual(capture['state']['camera']['scale'],120);self.assertEqual(capture['state']['camera_component_scale'],100)
            self.assertEqual(bmp_pixels(output.read_bytes())[:2],(128,96))
            self.assertNotEqual(bmp_pixels(output.read_bytes())[2],bmp_pixels(baseline.read_bytes())[2])
            headless=run(['--replay',str(replay),'--headless'])
            for key,value in capture['state'].items():self.assertEqual(headless[key],value,key)
            again=root/'again.bmp';repeated=run(['--replay',str(replay),'--capture',str(again)])
            self.assertEqual(repeated,capture);self.assertEqual(again.read_bytes(),output.read_bytes())

    def test_resize_replay_updates_native_camera_and_gpu_target(self):
        if not MISSION:self.skipTest('mission executable required')
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);scene=root/'scene.txt';replay=root/'resize.txt'
            original=make_boss_scene(root)
            text=('mission_presentation 128 96 100 0 1 0 1500100 -1\nsystem_init 1\nmaterial_policy native\ntick_ms 1\n'+
                  '\n'.join(l for l in original.splitlines() if not l.startswith(('world ','projection ','view ','slots ','animate ','simulate ','material ')))+'\n')
            scene.write_text(text,encoding='utf-8');replay.write_text('2 viewport 161 99\n8 viewport 128 96\n',encoding='ascii')
            args=[str(MISSION),'--scene',str(scene),'--frames','12','--fps','25','--debug','0','--rng','system','--seed','42','--replay',str(replay)]
            window=subprocess.run(args,capture_output=True,text=True,timeout=30)
            headless=subprocess.run([*args,'--headless'],capture_output=True,text=True,timeout=30)
            self.assertEqual(window.returncode,0,window.stderr);self.assertEqual(headless.returncode,0,headless.stderr)
            a=json.loads(window.stdout);b=json.loads(headless.stdout)
            self.assertEqual((a['viewport']['width'],a['viewport']['height'],a['viewport']['changes']),(128,96,2))
            for key in a.keys()&b.keys():self.assertEqual(a[key],b[key],key)
            snapshot=list(args);snapshot[snapshot.index('--frames')+1]='6';output=root/'resized.bmp'
            captured=subprocess.run([*snapshot,'--capture',str(output)],capture_output=True,text=True,timeout=30)
            self.assertEqual(captured.returncode,0,captured.stderr)
            self.assertEqual(json.loads(captured.stdout)['capture_render_frames'],6)
            self.assertEqual(json.loads(captured.stdout)['capture_logic_steps'],6)
            self.assertEqual(bmp_pixels(output.read_bytes())[:2],(161,99))
            self.assertTrue(any(p[3] for p in bmp_pixels(output.read_bytes())[2]))
            state=subprocess.run([*snapshot,'--headless'],capture_output=True,text=True,timeout=30)
            self.assertEqual(state.returncode,0,state.stderr)
            self.assertEqual(json.loads(captured.stdout)['commands'],json.loads(state.stdout)['render_commands'])
            existing=output.read_bytes()
            self.assertNotEqual(subprocess.run([*snapshot,'--capture',str(output)],capture_output=True,text=True,timeout=30).returncode,0)
            self.assertEqual(output.read_bytes(),existing)
            # Bounded runs keep the declared target even without a replay; live SDL/HiDPI events are interactive only.
            plain=args[:-2]
            window=subprocess.run(plain,capture_output=True,text=True,timeout=30)
            headless=subprocess.run([*plain,'--headless'],capture_output=True,text=True,timeout=30)
            self.assertEqual(window.returncode,0,window.stderr);self.assertEqual(headless.returncode,0,headless.stderr)
            a=json.loads(window.stdout);b=json.loads(headless.stdout)
            self.assertEqual(a['viewport']['changes'],0)
            for key in a.keys()&b.keys():self.assertEqual(a[key],b[key],key)

    def test_enemy_health_pixels_use_native_saved_threshold(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for defeats, visible in [(5, False), (6, True)]:
                text = make_gauge_scene(root, defeats)
                scene = root / 'probe.txt';scene.write_text(text, encoding='utf-8')
                probe = subprocess.run([str(EXE), str(scene), '32', '16', '--material-inputs'], capture_output=True, text=True, timeout=30)
                self.assertEqual(probe.returncode, 0, probe.stderr)
                bound = {tuple(map(int, line.split()[1:4])) for line in text.splitlines() if line.startswith('material ')}
                needed = {(v['texture'], v['shader'], v['queue']) for v in json.loads(probe.stdout)['material_inputs']}
                for i, (texture, shader, queue) in enumerate(sorted(needed - bound)):
                    text += f'material {texture} {shader} {queue} {300+i} 0\n'
                result, output = self.run_capture(root, text, name=f'gauge-{defeats}.bmp', native=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                pixels = bmp_pixels(output.read_bytes())[2]
                self.assertEqual((255, 0, 255, 255) in pixels, visible)
                self.assertEqual((0, 0, 255, 255) in pixels, visible)
                automatic='material_policy native\n'+'\n'.join(l for l in text.splitlines() if not l.startswith('material '))+'\n'
                native_result,native_output=self.run_capture(root,automatic,name=f'auto-gauge-{defeats}.bmp',native=True)
                self.assertEqual(native_result.returncode,0,native_result.stderr)
                self.assertEqual(native_output.read_bytes(),output.read_bytes())

    def test_native_input_replay_runs_the_gpu_scene(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt';replay = root / 'inputs.txt'
            text = with_mission_map(root, make_shoot_scene(root, spell_attribute=0))
            text = (text.replace('human_system 10 0 100 3 1', 'human_system 10 0 100 0 0')
                    .replace('human 0 2 1 0 0 0 0 0 100', 'human 0 2 1 0 0 0 0 0 1000')
                    .replace('enemy 0 4 7 2 0 0 70', 'enemy 0 4 7 2 0 0 970'))
            text += 'input fresh 0 0 0 0 0 0 0 0 0\ntick_ms 1\n'
            replay.write_text('1 attack 0\n20 keys 131072\n30 keys 0\n40 keys 524288\n50 keys 0\n', encoding='utf-8')
            bound = {tuple(map(int, row.split()[1:4])) for row in text.splitlines() if row.startswith('material ')}
            needed = set()
            for step in range(61):
                scene.write_text(text.replace('simulate 0', f'simulate {step}'), encoding='utf-8')
                probe = subprocess.run([str(EXE), str(scene), '128', '96', '--material-inputs', '--replay', str(replay)], capture_output=True, text=True, timeout=30)
                self.assertEqual(probe.returncode, 0, probe.stderr)
                needed.update((v['texture'], v['shader'], v['queue']) for v in json.loads(probe.stdout)['material_inputs'])
            for i, (texture, shader, queue) in enumerate(sorted(needed - bound)):
                text += f'material {texture} {shader} {queue} {300 + i} 0\n'
            scene.write_text(text, encoding='utf-8')
            result = subprocess.run([str(EXE), str(scene), '128', '96', '--run-frames', '60', '--replay', str(replay)], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertTrue(report['input_enabled'] and report['replay_bound'])
            self.assertEqual(report['input_steps'], 60)
            self.assertEqual(report['replay_actions'], 5)
            self.assertEqual(report['input_held_keys'], 0)
            self.assertGreater(report['shots'], 0)
            if MISSION:
                window = subprocess.run([str(MISSION), '--scene', str(scene), '--fps', '25', '--debug', '0',
                                         '--rng', 'system', '--seed', '42', '--frames', '60', '--replay', str(replay)],
                                        capture_output=True, text=True, timeout=30)
                self.assertEqual(window.returncode, 0, window.stderr)
                self.assertEqual(window.stdout, result.stdout)
                headless = subprocess.run([str(MISSION), '--scene', str(scene), '--fps', '25', '--debug', '0',
                                           '--rng', 'system', '--seed', '42', '--frames', '60', '--headless', '--replay', str(replay)],
                                          capture_output=True, text=True, timeout=30)
                self.assertEqual(headless.returncode, 0, headless.stderr)
                state = json.loads(headless.stdout)
                for key in state.keys() & report.keys():
                    self.assertEqual(state[key], report[key], key)

    def test_native_boss_creation_reaches_gpu(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt';text = make_boss_scene(root) + 'tick_ms 1\n'
            # Explicit tall diagnostic viewport includes the barrier behind the party.
            text = (text.replace('view 128 96', 'view 128 640')
                    .replace('0.0208333333', '0.003125'))
            bound = {tuple(map(int, row.split()[1:4])) for row in text.splitlines() if row.startswith('material ')}
            needed = set()
            for step in [0, 1, 12, 31]:
                scene.write_text(text.replace('simulate 0', f'simulate {step}'), encoding='utf-8')
                probe = subprocess.run([str(EXE), str(scene), '128', '640', '--material-inputs'], capture_output=True, text=True, timeout=30)
                self.assertEqual(probe.returncode, 0, probe.stderr)
                needed.update((v['texture'], v['shader'], v['queue']) for v in json.loads(probe.stdout)['material_inputs'])
            for i, (texture, shader, queue) in enumerate(sorted(needed - bound)):
                text += f'material {texture} {shader} {queue} {500 + i} 0\n'
            scene.write_text(text, encoding='utf-8')
            result = subprocess.run([str(EXE), str(scene), '128', '640', '--run-frames', '31'], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertTrue(report['boss_entry_enabled'] and report['boss_started'])
            self.assertEqual(report['boss_creation_calls'], 1)
            self.assertEqual(report['boss_fence']['z'], 996)
            self.assertEqual(len(report['enemies']), 2)
            self.assertEqual(report['render_frames'], 31)

    def test_map_camera_and_cell_visibility_reach_gpu(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory);scene = root / 'scene.txt'
            text = with_mission_map(root, make_shoot_scene(root))
            text = text.replace('human_system 10 0 100 3 1', 'human_system 10 0 100 3 0')
            text = text.replace('human 0 2 1 0 0 0 0 0 100', 'human 0 2 1 0 0 0 0 0 1000')
            text += 'actor_state 0 1 -1\ntick_ms 1\n'
            capture = text.replace('simulate 0', 'simulate 12')
            scene.write_text(capture, encoding='utf-8')
            inputs = subprocess.run([str(EXE), str(scene), '128', '96', '--material-inputs'], capture_output=True, text=True, timeout=30)
            self.assertEqual(inputs.returncode, 0, inputs.stderr)
            bound = {tuple(map(int, row.split()[1:4])) for row in text.splitlines() if row.startswith('material ')}
            for i, key in enumerate(json.loads(inputs.stdout)['material_inputs']):
                triple = (key['texture'], key['shader'], key['queue'])
                if triple not in bound:
                    text += f'material {triple[0]} {triple[1]} {triple[2]} {200 + i} 0\n'
            scene.write_text(text, encoding='utf-8')
            loop = subprocess.run([str(EXE), str(scene), '128', '96', '--run-frames', '12'], capture_output=True, text=True, timeout=30)
            self.assertEqual(loop.returncode, 0, loop.stderr)
            report = json.loads(loop.stdout)
            self.assertEqual(report['mission_map']['chips_live'], 36)
            self.assertEqual(report['camera_steps'], 12)
            self.assertEqual(report['cell_steps'], 12)
            self.assertIn(report['positions'][0]['id'], {e['id'] for e in report['visible_entities']})
            self.assertNotIn(report['positions'][1]['id'], {e['id'] for e in report['visible_entities']})
            scene.write_text(text.replace('simulate 0', 'simulate 12'), encoding='utf-8')
            output = root / 'map.bmp'
            result = subprocess.run([str(EXE), str(scene), '128', '96', str(output), '--native-depth'], capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue(any(p[0] == 255 and p[1] == 0 and p[3] for p in bmp_pixels(output.read_bytes())[2]))

    def make_resources(self, root):
        a, b, c = root / 'first.bin', root / 'second.bin', root / 'global.bin'
        a.write_bytes(jar([('img.inf', b'2\ta.png,bin'), ('seb.inf', b'3\ta.seb'),
            ('a.seb', seb([(0, 2, 0, 0, 2, 2, 0, 0, 0, 0)])),
            ('a.png', png(4, 4, [255, 0, 0, 255] * 16))]))
        colors = [(0, 100 + x * 15, y * 40, (0, 128, 255)[x % 3]) for y in range(4) for x in range(8)]
        opt = struct.pack('>5B7h', 8, 4, 1, 1, 1, -1, 1, 1, 4, 1, 2, 2)
        b.write_bytes(jar([('img.inf', b'2\tb.png,bin,q+1'), ('seb.inf', b'5\tb.seb'),
            ('b.seb', seb([(0, 2, 0, 0, 8, 4, 0, 0, 0, 0)])),
            ('b.png', png(8, 4, [v for pixel in colors for v in pixel])), ('b.opt', opt)]))
        c.write_bytes(jar([('img.inf', b'2\tc.png,bin'), ('seb.inf', b'0\tc.seb'),
            ('c.seb', seb([(0, 2, 0, 0, 2, 2, 0, 0, 0, 0)])), ('c.png', png(2, 2, [0, 255, 255, 255] * 4))]))
        return a, b, c

    def run_capture(self, root, text, name='combined.bmp', native=False):
        scene, output = root / 'scene.txt', root / name
        scene.write_text(text, encoding='utf-8')
        args = [str(EXE), str(scene), '32', '16', str(output)]
        if native:
            args.append('--native-depth')
        result = subprocess.run(args, capture_output=True, text=True, timeout=30)
        return result, output

    def test_native_depth_overlap_and_explicit_material_inputs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.make_resources(root)
            prefix = ('world 1 0 0 0 1 0 0 0 1\n'
                      'projection 0.0625 0 0 0 0 0.125 0 0 0 0 0.00000133324445 0 -1 -1 -1 1\n'
                      'gpu_depth 24 1\n'
                      'resource 1 "first.bin" "English.lproj" 0\n'
                      'resource 3 "global.bin" "English.lproj" 0\n')
            bindings = 'material 0 65808 2000 10 0\nmaterial 1 65808 2000 11 0\n'
            for depth, expected in [(0, (255, 0, 0, 255)), (150000, (0, 255, 255, 255))]:
                result, output = self.run_capture(root, prefix + bindings + command(1, 3, 2, 2, 100) + command(3, 0, 2, 2, depth), f'native-{depth}.bmp', native=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(bmp_pixels(output.read_bytes())[2][3 * 32 + 3], expected)
                self.assertIn('native depth', json.loads(result.stdout)['profile'])
            result, output = self.run_capture(root, prefix + command(1, 3, 2, 2), 'unbound.bmp', native=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('unbound material:', result.stderr)
            self.assertFalse(output.exists())

    def test_table_backed_entities_reach_gpu_without_manual_draw_rows(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            text = make_table_scene(root)
            result, output = self.run_capture(root, text, 'entities.bmp', native=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual((report['entities'], report['commands'], report['quads']), (2, 2, 2))
            pixels = bmp_pixels(output.read_bytes())[2]
            self.assertEqual(pixels[5 * 32 + 5], (255, 0, 0, 255))
            self.assertEqual(pixels[10 * 32 + 15], (0, 255, 255, 255))
            inspected = subprocess.run([str(EXE), str(root / 'scene.txt'), '32', '16', '--material-inputs'], capture_output=True, text=True, timeout=30)
            self.assertEqual(inspected.returncode, 0, inspected.stderr)
            self.assertEqual(json.loads(inspected.stdout)['material_inputs'], [
                {'texture': 0, 'shader': 65808, 'queue': 2000}, {'texture': 1, 'shader': 65808, 'queue': 2000}])
            (root / 'scene.txt').write_text(text + 'tick_ms 1\n', encoding='utf-8')
            looped = subprocess.run([str(EXE), str(root / 'scene.txt'), '32', '16', '--run-frames', '2'], capture_output=True, text=True, timeout=30)
            self.assertEqual(looped.returncode, 0, looped.stderr)
            self.assertEqual((json.loads(looped.stdout)['render_frames'], json.loads(looped.stdout)['logic_steps']), (2, 2))
            stationary = json.loads(looped.stdout)
            self.assertEqual(stationary['movement_steps'], 0)
            self.assertEqual(stationary['positions'][1]['z'], 0)
            (root / 'scene.txt').write_text(text + 'tick_ms 1\nsimulate 0\n', encoding='utf-8')
            moving = subprocess.run([str(EXE), str(root / 'scene.txt'), '32', '16', '--run-frames', '2'], capture_output=True, text=True, timeout=30)
            self.assertEqual(moving.returncode, 0, moving.stderr)
            report = json.loads(moving.stdout)
            self.assertEqual(report['movement_steps'], 2)
            self.assertEqual(report['positions'][0], stationary['positions'][0])
            self.assertGreater(report['positions'][1]['z'], 0)
            # A native capture after movement must also change the GPU output.
            result, moved_output = self.run_capture(root, text + 'simulate 2\n', 'moving.bmp', native=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertNotEqual(bmp_pixels(moved_output.read_bytes())[2], pixels)
            (root / 'effect.bin').write_bytes(jar([
                ('img.inf', b'9\te.png,bin'), ('seb.inf', b'0\te.seb'),
                ('e.seb', seb([(0, 9, 0, 0, 2, 2, 0, 0, 0, 0), (3, 9, 0, 0, 2, 2, 0, 0, 0, 0)])),
                ('e.png', png(2, 2, [0, 255, 0, 255] * 4))]))
            effects = (text + 'resource 20 "effect.bin" "English.lproj" 0\nglobal_alias 20 20\n'
                       'material 2 65808 2000 12 0\neffect 0 9 0 18 0 0\n')
            # Header max_frame4 -> native lifetime3. Render occurs before Garbage.
            for passes, green in [(3, True), (4, False)]:
                result, capture = self.run_capture(root, effects + f'simulate {passes}\n', f'effect-{passes}.bmp', native=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual((0, 255, 0, 255) in bmp_pixels(capture.read_bytes())[2], green)
            (root / 'scene.txt').write_text(effects + 'tick_ms 1\nsimulate 0\n', encoding='utf-8')
            expired = subprocess.run([str(EXE), str(root / 'scene.txt'), '32', '16', '--run-frames', '3'], capture_output=True, text=True, timeout=30)
            self.assertEqual(expired.returncode, 0, expired.stderr)
            self.assertTrue(json.loads(expired.stdout)['positions'][2]['destroyed'])
            result, output = self.run_capture(root, text.replace('progress 2 5 13\n', ''), 'unknown-save.bmp', native=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('unbound saved character state', result.stderr)
            self.assertFalse(output.exists())

    def test_different_sizes_alpha_opt_and_same_image_ids(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            a, b, _ = self.make_resources(root)
            isolated = []
            for archive, clip, x in [(a, 3, 2), (b, 5, 12)]:
                output = root / f'single-{clip}.bmp'
                result = subprocess.run([str(VIEWER), str(archive), '--resource', str(clip), '0', '0',
                    '--language', 'English.lproj', '--dpi', '0', '--canvas', '32', '16', '--origin', str(x), '2',
                    '--headless', '--capture', str(output)], capture_output=True, text=True, timeout=30)
                self.assertEqual(result.returncode, 0, result.stderr)
                isolated.append(bmp_pixels(output.read_bytes())[2])
            text = ('world 1 0 0 0 1 0 0 0 1\n'
                    'resource 1 "first.bin" "English.lproj" 0\nresource 3 "second.bin" "English.lproj" 0\n'
                    + command(1, 3, 2, 2, 150007) + command(3, 5, 12, 2, 300009))
            result, output = self.run_capture(root, text)
            self.assertEqual(result.returncode, 0, result.stderr)
            report = json.loads(result.stdout)
            self.assertEqual((report['commands'], report['textures'], report['quads']), (2, 2, 3))
            expected = [left if left != (0, 0, 0, 0) else right for left, right in zip(*isolated)]
            self.assertEqual(bmp_pixels(output.read_bytes())[2], expected)

    def test_global_appdata_image_override_and_world_translation(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.make_resources(root)
            text = ('world 1 0 0 0 1 0 3 0 1\n'
                    'resource 1 "first.bin" "English.lproj" 0\nglobal 1 "global.bin" "English.lproj" 0\n'
                    + command(1, 3, 2, 2, texture=2, image_resource=1))
            result, output = self.run_capture(root, text)
            self.assertEqual(result.returncode, 0, result.stderr)
            pixels = bmp_pixels(output.read_bytes())[2]
            self.assertEqual(pixels[2 * 32 + 5], (0, 255, 255, 255))
            self.assertEqual(pixels[2 * 32 + 2], (0, 0, 0, 0))
            output.write_bytes(b'preserved')
            result, _ = self.run_capture(root, text)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output.read_bytes(), b'preserved')

    def test_explicit_inputs_are_required(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for text in ['# no inferred world\n', 'world 1 0 0 0 1 0 0 0 1 extra\n',
                         'resource 0 "missing.bin" "English.lproj" 0\n']:
                result, output = self.run_capture(root, text)
                self.assertNotEqual(result.returncode, 0)
                self.assertFalse(output.exists())


if __name__ == '__main__':
    unittest.main()
