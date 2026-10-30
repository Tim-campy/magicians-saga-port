"""Combat regression through saga_mission's production catalog, scheduler and GL owner."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXE=Path(sys.argv.pop(1)).resolve() if len(sys.argv)>1 else None
sys.path.insert(0,str(Path(__file__).resolve().parent))
from mission_scene_fixture import make_combat_regression_scene
from test_sprite_preview import bmp_pixels


@unittest.skipUnless(EXE,'viewer saga_mission supplied by CTest')
class CombatPixels(unittest.TestCase):
    def test_table_backed_magic_and_physical_combat(self):
        for physical in (False,True):
            with self.subTest(physical=physical),tempfile.TemporaryDirectory() as directory:
                root=Path(directory);scene=root/'combat.txt'
                scene.write_text(make_combat_regression_scene(root,physical),encoding='utf8')
                args=[str(EXE),'--scene',str(scene),'--fps','25','--debug','0','--rng','system','--seed','42']
                def run(frames,*extra):
                    result=subprocess.run([*args,'--frames',str(frames),*extra],capture_output=True,text=True,timeout=60)
                    self.assertEqual(result.returncode,0,result.stderr)
                    return json.loads(result.stdout)
                images=set()
                for frames in (1,20,40,45,120,240):
                    output=root/f'combat-{frames}.bmp';capture=run(frames,'--capture',str(output))
                    state=capture['state'];headless=run(frames,'--headless')
                    for key,value in state.items():self.assertEqual(headless[key],value,(frames,key))
                    self.assertEqual(capture['capture_render_frames'],frames)
                    self.assertEqual(capture['capture_logic_steps'],frames)
                    self.assertEqual(capture['commands'],headless['render_commands'])
                    if frames==20:self.assertEqual(capture['commands'],17 if physical else 14)
                    if frames==45:
                        self.assertEqual(state['emitter_entities'],3)
                        self.assertGreater(state['garbage_entities'],0)
                    self.assertGreater(capture['quads'],0)
                    self.assertGreater(capture['textures'],1)
                    width,height,pixels=bmp_pixels(output.read_bytes())
                    self.assertEqual((width,height),(128,96))
                    self.assertTrue(any(pixel[3] for pixel in pixels))
                    images.add(tuple(pixels))
                self.assertGreater(len(images),1)
                self.assertTrue(state['positions'][1]['destroyed'])
                self.assertGreater(state['defeated_enemies'],0)
                self.assertGreater(state['shots'],0)
                self.assertEqual(state['projectiles'],[])
                self.assertEqual((state['emitter_entities'],state['garbage_entities']),(0,0))
                self.assertEqual(state['parameter_experience_pending'],0)
                self.assertEqual(state['positions'][0]['human_experience'],0)
                self.assertTrue(state['pending_drops'])
                self.assertGreater(state['rng_draws'],0)
                self.assertTrue(state['collisions_enabled'] and state['update_schedule_complete'])
                repeated=run(240,'--headless')
                for key,value in state.items():self.assertEqual(repeated[key],value,key)


if __name__=='__main__':unittest.main(argv=[sys.argv[0]])
