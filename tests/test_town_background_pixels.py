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
