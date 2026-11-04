"""Native Town background prefix through the existing GL mesh/bitmap adapters."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

EXE=Path(sys.argv.pop(1)).resolve() if len(sys.argv)>1 else None
sys.path.insert(0,str(Path(__file__).resolve().parent))
from town_fixture import add_town_background
from test_sprite_preview import bmp_pixels


@unittest.skipUnless(EXE,'viewer saga_town supplied by CTest')
class TownBackgroundPixels(unittest.TestCase):
    def test_resident_composition_uses_body_and_weapon_override_resources(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);add_town_background(root,residents=True)
            args=[str(EXE),'--data',str(root),'--residents','--width','600','--height','320']
            def run(*extra):
                value=subprocess.run([*args,*extra],capture_output=True,text=True,timeout=30)
                self.assertEqual(value.returncode,0,value.stderr);return json.loads(value.stdout)
            capture=root/'residents.bmp';report=run('--capture',str(capture));self.assertEqual(run(),report)
            width,height,pixels=bmp_pixels(capture.read_bytes());self.assertEqual((width,height),(600,320))
            self.assertEqual(pixels[201*width+130],(20,20,20,255))
            for y in (201,202):
                for x in (132,133,135,136):self.assertEqual(pixels[y*width+x],(200,70,20,255))
                for x in (138,139):self.assertEqual(pixels[y*width+x],(20,70,200,255))
            second=root/'again.bmp';self.assertEqual(run('--capture',str(second)),report)
            self.assertEqual(capture.read_bytes(),second.read_bytes())

    def test_area_map_and_fence_use_distinct_native_resources(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);add_town_background(root,areas=True)
            args=[str(EXE),'--data',str(root),'--areas','--width','600','--height','320','--camera-x','300']
            def run(*extra):
                value=subprocess.run([*args,*extra],capture_output=True,text=True,timeout=30)
                self.assertEqual(value.returncode,0,value.stderr);return json.loads(value.stdout)
            capture=root/'areas.bmp';report=run('--capture',str(capture));self.assertEqual(run(),report)
            width,height,pixels=bmp_pixels(capture.read_bytes());self.assertEqual((width,height),(600,320))
            for yy in (113,114):
                for xx in (158,159,160):self.assertEqual(pixels[yy*width+xx],(10,200,100,255))
            for yy in (136,137):
                for xx in (11,12):self.assertEqual(pixels[yy*width+xx],(255,0,0,255))
            second=root/'repeat.bmp';self.assertEqual(run('--capture',str(second)),report)
            self.assertEqual(capture.read_bytes(),second.read_bytes())

    def test_building_texture_override_pixels_and_no_early_notice_flush(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);add_town_background(root,buildings=True)
            args=[str(EXE),'--data',str(root),'--buildings','--width','600','--height','200']
            def run(*extra):
                value=subprocess.run([*args,*extra],capture_output=True,text=True,timeout=30)
                self.assertEqual(value.returncode,0,value.stderr);return json.loads(value.stdout)
            first=root/'buildings.bmp';report=run('--capture',str(first));self.assertEqual(run(),report)
            width,height,pixels=bmp_pixels(first.read_bytes());self.assertEqual((width,height),(600,200))
            for x,color in ((85,(20,30,40,255)),(175,(40,50,60,255)),(130,(10,100,210,255)),
                            (220,(210,100,10,255)),(310,(80,90,10,255)),(400,(80,90,10,255))):
                for yy in (73,74):
                    for xx in range(x,x+3):self.assertEqual(pixels[yy*width+xx],color)
            second=root/'reward.bmp';reward=run('--high-score','10','--capture',str(second))
            self.assertEqual(len(reward['background']['buildings_draw']['notices']),2)
            self.assertEqual(first.read_bytes(),second.read_bytes(), 'queued arrows cannot draw before area/resident passes')

    def test_native_scenery_ground_pixels_and_two_stream_repeat(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);add_town_background(root,scenery=True)
            base=[str(EXE),'--data',str(root),'--scenery','--width','320','--height','200']
            def run(*args):
                result=subprocess.run([*base,*args],capture_output=True,text=True,timeout=30)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            first=root/'scenery.bmp';report=run('--capture',str(first))
            self.assertEqual(run(),report)
            width,height,pixels=bmp_pixels(first.read_bytes());self.assertEqual((width,height),(320,200))
            ground_y=200-8+53-80 # Raw height - SEB bounds height + viewY + clamped cameraY.
            for y in range(ground_y,ground_y+8):self.assertEqual(pixels[y*width:(y+1)*width],[(80,90,100,255)]*width)
            # Floor uses Image3 despite absent TexId77; gate keeps its own offset and layer0 only.
            for y in range(73,79):self.assertEqual(pixels[y*width:(y+1)*width],[(0,100,200,255)]*width)
            for y in range(110,112):
                for x in range(45,48):self.assertEqual(pixels[y*width+x],(210,30,90,255))
            second=root/'repeat.bmp';self.assertEqual(run('--capture',str(second)),report)
            self.assertEqual(first.read_bytes(),second.read_bytes())
            scene=report['background']['scenery']
            self.assertTrue(scene['provisional_rng']);self.assertEqual((scene['app_seed'],scene['lib_seed']),(42,42))
            self.assertGreater(scene['app_draws'],scene['lib_draws'])
            self.assertTrue(any(draw[0]==14 for draw in scene['draws']))
            left=root/'left.bmp';left_report=run('--camera-x','449','--capture',str(left))
            self.assertEqual(left_report['background']['camera'][0],449)
            self.assertEqual(bmp_pixels(left.read_bytes())[2][73*width+40],(120,0,0,255))
            right=root/'right.bmp';right_report=run('--camera-x','-270','--capture',str(right))
            self.assertEqual(right_report['background']['camera'][0],-270)
            self.assertEqual(bmp_pixels(right.read_bytes())[2][73*width+217],(0,120,0,255))

    def test_foreground_and_final_notices_reach_pixels(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);add_town_background(root,foreground=True)
            base=[str(EXE),'--data',str(root),'--foreground','--width','600','--height','320','--app-seed','0']
            path=root/'foreground.bmp'
            result=subprocess.run([*base,'--capture',str(path)],capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stderr);report=json.loads(result.stdout)
            width,height,pixels=bmp_pixels(path.read_bytes());self.assertEqual((width,height),(600,320))
            self.assertEqual(pixels[281*width+230],(80,90,100,255)) # Front ground tile222, frame2 offset-100.
            front=report['background']['foreground'];self.assertEqual(front['pending_notices'],0)
            for resource,clip,frame,layer,x,y in front['notice_draws']:
                self.assertEqual((resource,clip,layer),(11,11,0))
                self.assertEqual(pixels[y*width+x],(255,0,0,255)) # Queued board reaches the final pass.
            repeat=root/'repeat.bmp';again=subprocess.run([*base,'--capture',str(repeat)],capture_output=True,text=True,timeout=30)
            self.assertEqual(again.returncode,0,again.stderr);self.assertEqual(json.loads(again.stdout),report)
            self.assertEqual(repeat.read_bytes(),path.read_bytes())
            cpu=subprocess.run(base,capture_output=True,text=True,timeout=30)
            self.assertEqual(cpu.returncode,0,cpu.stderr);self.assertEqual(json.loads(cpu.stdout),report)
            right=root/'obstacle.bmp';shifted=subprocess.run([*base,'--camera-x','-300','--capture',str(right)],capture_output=True,text=True,timeout=30)
            self.assertEqual(shifted.returncode,0,shifted.stderr)
            obstacle_pixels=bmp_pixels(right.read_bytes())[2]
            self.assertEqual(obstacle_pixels[223*width+540],(150,70,190,255))

    def test_hero_factory_hat_and_all_resident_lanes_reach_pixels(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);add_town_background(root,hero=True)
            base=[str(EXE),'--data',str(root),'--hero','--width','600','--height','320','--app-seed','0']
            first=root/'hero.bmp'
            result=subprocess.run([*base,'--capture',str(first)],capture_output=True,text=True,timeout=30)
            self.assertEqual(result.returncode,0,result.stderr);report=json.loads(result.stdout)
            width,height,pixels=bmp_pixels(first.read_bytes());self.assertEqual((width,height),(600,320))
            self.assertEqual(pixels[209*width+40],(40,190,80,255)) # First main hat image7, not equipment hat0/image5.
            self.assertEqual(pixels[209*width+42],(200,70,20,255))
            self.assertEqual(pixels[209*width+48],(20,70,200,255))
            for x,y in ((132,201),(431,210),(261,219)):
                self.assertEqual(pixels[y*width+x],(200,70,20,255))
            second=root/'repeat.bmp'
            repeat=subprocess.run([*base,'--capture',str(second)],capture_output=True,text=True,timeout=30)
            self.assertEqual(repeat.returncode,0,repeat.stderr)
            self.assertEqual(json.loads(repeat.stdout),report);self.assertEqual(first.read_bytes(),second.read_bytes())
            cpu=subprocess.run(base,capture_output=True,text=True,timeout=30)
            self.assertEqual(cpu.returncode,0,cpu.stderr);self.assertEqual(json.loads(cpu.stdout),report)

    def test_fill_tiles_parallax_repeat_and_headless_parity(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);add_town_background(root)
            base=[str(EXE),'--data',str(root),'--width','320','--height','200','--camera-y','0']
            def run(*args):
                result=subprocess.run([*base,*args],capture_output=True,text=True,timeout=30)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            first=root/'first.bmp';report=run('--capture',str(first))
            width,height,pixels=bmp_pixels(first.read_bytes());self.assertEqual((width,height),(320,200))
            expected=[(10,20,30,255)]*(320*200)
            for x0,y0,color in ((31,128,(255,0,0,255)),(271,128,(255,0,0,255)),(30,73,(0,255,0,255))):
                for y in range(y0,y0+2):
                    for x in range(x0,x0+2):expected[y*320+x]=color
            self.assertEqual(pixels,expected)
            second=root/'second.bmp';self.assertEqual(run('--capture',str(second)),report)
            self.assertEqual(first.read_bytes(),second.read_bytes())
            self.assertEqual(run('--background'),report)
            shifted=root/'shifted.bmp';changed=run('--camera-x','-60','--capture',str(shifted))
            expected[73*320+30]=expected[73*320+31]=expected[74*320+30]=expected[74*320+31]=(10,20,30,255)
            for y in (73,74):
                for x in (28,29):expected[y*320+x]=(0,255,0,255)
            self.assertEqual(bmp_pixels(shifted.read_bytes())[2],expected)
            self.assertEqual(changed['background']['camera'],[-60,0])
            rejected=subprocess.run([*base,'--capture',str(first)],capture_output=True,text=True,timeout=30)
            self.assertNotEqual(rejected.returncode,0);self.assertIn('already exists',rejected.stderr)
            self.assertEqual(first.read_bytes(),second.read_bytes())


if __name__=='__main__':unittest.main()
