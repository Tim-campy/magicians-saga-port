"""Native Town background prefix through the existing GL mesh/bitmap adapters."""
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
from test_sprite_preview import bmp_pixels


@unittest.skipUnless(EXE,'viewer saga_town supplied by CTest')
class TownBackgroundPixels(unittest.TestCase):
    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public CTest font required')
    def test_equipment_grid_stock_notice_pages_and_return_match_window(self):
        from town_campaign_fixture import create_campaign_fixture,add_equipment_shop_case
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);create_campaign_fixture(root);add_equipment_shop_case(root)
            replay=root/'equipment.replay';replay.write_text('')
            base=[str(EXE),'--data',str(root),'--foreground','--shared-world','--physical-input',
                  '--camera-x','0','--talk-pulse-every','10','--replay',str(replay)]
            def run(frame,*extra):
                result=subprocess.run([*base,'--update-passes',str(frame),*extra],capture_output=True,text=True,encoding='utf-8',timeout=45)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            before=run(60);touch=next(t for t in before['background']['buildings_draw']['touches'] if t[0]==0)
            x,y=touch[2]+40,touch[3]+45
            replay.write_text(''.join(f'{61+i} widget {i%2} {x} {y}\n' for i in range(4)))
            opened=run(140)['background']['update'];cell=next(r for r in opened['equipment_shop_widgets']['regions'] if r[:2]==[10,0x20000])
            x,y=cell[2]+cell[4]//2,cell[3]+cell[5]//2
            with replay.open('a') as out:out.write(f'141 widget 0 {x} {y}\n142 widget 1 {x} {y}\n')
            selected=run(142)['background']['update'];buy=next(r for r in selected['equipment_shop_widgets']['regions'] if r[:2]==[4,25])
            bx,by=buy[2]+buy[4]//2,buy[3]+buy[5]//2
            with replay.open('a') as out:out.write(f'145 widget 0 {bx} {by}\n146 widget 1 {bx} {by}\n')
            resumed=run(190)['background']['update'];right=next(r for r in resumed['equipment_shop_widgets']['regions'] if r[:2]==[5,0x10012])
            rx,ry=right[2]+right[4]//2,right[3]+right[5]//2
            with replay.open('a') as out:out.write(f'191 widget 0 {rx} {ry}\n192 widget 1 {rx} {ry}\n200 form-back\n')
            captures={}
            for frame in (140,142,146,166,190,192,210):
                with self.subTest(frame=frame):
                    cpu=run(frame);still=root/f'equipment-still-{frame}.bmp';window=root/f'equipment-window-{frame}.bmp'
                    self.assertEqual(run(frame,'--capture',str(still)),cpu)
                    self.assertEqual(run(frame,'--interactive','--hidden','--capture',str(window)),cpu)
                    self.assertEqual(still.read_bytes(),window.read_bytes());captures[frame]=still.read_bytes()
                    update=cpu['background']['update']
                    if frame>=146:
                        self.assertEqual(update['wallet']['money'],25)
                        self.assertEqual(next(r for r in update['equipment_stock'] if r[0]==14),[14,1,1,1])
                        self.assertEqual(cpu['background']['hero']['equipment'],before['background']['hero']['equipment'])
                    if frame==192:self.assertEqual(update['equipment_shop']['page'],1)
                    if frame==210:self.assertNotIn('equipment_shop',update)
            self.assertNotEqual(captures[140],captures[142]);self.assertNotEqual(captures[190],captures[192])

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public CTest font required')
    def test_hat_and_cloth_icons_purchase_and_return_match_window(self):
        from town_campaign_fixture import create_campaign_fixture,add_apparel_shop_case
        for kind in (3,4):
            with self.subTest(kind=kind),tempfile.TemporaryDirectory() as directory:
                root=Path(directory);create_campaign_fixture(root);add_apparel_shop_case(root,kind)
                replay=root/'apparel.replay';replay.write_text('')
                base=[str(EXE),'--data',str(root),'--foreground','--shared-world','--physical-input',
                      '--camera-x','0','--talk-pulse-every','10','--replay',str(replay)]
                def run(frame,*extra):
                    r=subprocess.run([*base,'--update-passes',str(frame),*extra],capture_output=True,text=True,encoding='utf8',timeout=45)
                    self.assertEqual(r.returncode,0,r.stderr);return json.loads(r.stdout)
                before=run(60)['background'];t=next(t for t in before['buildings_draw']['touches'] if t[0]==0)
                x,y=t[2]+40,t[3]+45
                replay.write_text(''.join(f'{61+i} widget {i%2} {x} {y}\n' for i in range(4)))
                opened=run(140)['background']['update'];t=next(t for t in opened['equipment_shop_widgets']['regions'] if t[:2]==[10,0x20000])
                x,y=t[2]+t[4]//2,t[3]+t[5]//2
                with replay.open('a') as out:out.write(f'141 widget 0 {x} {y}\n142 widget 1 {x} {y}\n')
                selected=run(142)['background']['update'];t=next(t for t in selected['equipment_shop_widgets']['regions'] if t[:2]==[4,25])
                x,y=t[2]+t[4]//2,t[3]+t[5]//2
                with replay.open('a') as out:out.write(f'145 widget 0 {x} {y}\n146 widget 1 {x} {y}\n201 form-back\n')
                for frame in (140,142,146,190,210):
                    cpu=run(frame);still=root/f'still-{frame}.bmp';window=root/f'window-{frame}.bmp'
                    self.assertEqual(run(frame,'--capture',str(still)),cpu)
                    self.assertEqual(run(frame,'--interactive','--hidden','--capture',str(window)),cpu)
                    self.assertEqual(still.read_bytes(),window.read_bytes())
                    scene=cpu['background'];update=scene['update']
                    if frame>=146:
                        self.assertEqual(update['wallet']['money'],25 if kind==3 else 0)
                        self.assertEqual(next(r for r in update['hat_stock' if kind==3 else 'cloth_stock'] if r[0]==2),[2,1,1,1])
                        self.assertEqual(scene['hero']['equipment'],before['hero']['equipment'])
                    if frame==210:self.assertNotIn('equipment_shop',update)

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public CTest font required')
    def test_recruitment_portraits_crystal_purchase_and_return_match_window(self):
        from town_campaign_fixture import create_campaign_fixture,add_recruitment_shop_case
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);create_campaign_fixture(root);add_recruitment_shop_case(root)
            replay=root/'recruit.replay';replay.write_text('')
            base=[str(EXE),'--data',str(root),'--foreground','--shared-world','--physical-input',
                  '--camera-x','0','--talk-pulse-every','10','--replay',str(replay)]
            def run(frame,*extra):
                r=subprocess.run([*base,'--update-passes',str(frame),*extra],capture_output=True,text=True,encoding='utf8',timeout=45)
                self.assertEqual(r.returncode,0,r.stderr);return json.loads(r.stdout)
            before=run(60)['background'];t=next(t for t in before['buildings_draw']['touches'] if t[0]==0)
            x,y=t[2]+40,t[3]+45
            replay.write_text(''.join(f'{61+i} widget {i%2} {x} {y}\n' for i in range(4)))
            opened=run(140)['background']['update'];t=next(t for t in opened['recruitment_shop_widgets']['regions'] if t[:2]==[10,0x20000])
            x,y=t[2]+t[4]//2,t[3]+t[5]//2
            with replay.open('a') as out:out.write(f'141 widget 0 {x} {y}\n142 widget 1 {x} {y}\n')
            selected=run(142)['background']['update'];t=next(t for t in selected['recruitment_shop_widgets']['regions'] if t[:2]==[4,25])
            x,y=t[2]+t[4]//2,t[3]+t[5]//2
            with replay.open('a') as out:out.write(f'145 widget 0 {x} {y}\n146 widget 1 {x} {y}\n201 form-back\n')
            for frame in (140,142,146,190,210):
                cpu=run(frame);still=root/f'still-{frame}.bmp';window=root/f'window-{frame}.bmp'
                self.assertEqual(run(frame,'--capture',str(still)),cpu)
                self.assertEqual(run(frame,'--interactive','--hidden','--capture',str(window)),cpu)
                self.assertEqual(still.read_bytes(),window.read_bytes())
                scene=cpu['background'];update=scene['update']
                if frame>=146:
                    self.assertEqual(update['wallet']['money'],37);self.assertEqual(update['wallet']['crystals'],31)
                    self.assertEqual(next(r for r in update['recruitment_state'] if r[0]==10),[10,1,1,1,-1])
                    self.assertEqual(update['recruitment_progress'],before['update']['recruitment_progress'])
                    self.assertEqual(scene['hero']['formation'],before['hero']['formation'])
                if frame==210:self.assertNotIn('recruitment_shop',update)

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public CTest font required')
    def test_magic_shop_rows_purchase_notice_and_town_return_match_window(self):
        from town_campaign_fixture import create_campaign_fixture,add_magic_shop_case
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);create_campaign_fixture(root);add_magic_shop_case(root)
            replay=root/'magic.replay';replay.write_text('')
            base=[str(EXE),'--data',str(root),'--foreground','--shared-world','--physical-input',
                  '--camera-x','0','--talk-pulse-every','10','--replay',str(replay)]
            def run(*args):
                result=subprocess.run([*base,*args],capture_output=True,text=True,encoding='utf-8',timeout=45)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            before=run('--update-passes','60');touch=next(t for t in before['background']['buildings_draw']['touches'] if t[0]==0)
            x,y=touch[2]+40,touch[3]+45
            replay.write_text(''.join(f'{61+i} widget {i%2} {x} {y}\n' for i in range(4))+
                              '141 widget 0 240 244\n142 widget 1 240 244\n')
            selected=run('--update-passes','142')['background']['update']
            buy=next(t for t in selected['magic_shop_widgets']['regions'] if t[:2]==[4,25]);bx,by=buy[2]+buy[4]//2,buy[3]+buy[5]//2
            with replay.open('a') as out:out.write(f'145 widget 0 {bx} {by}\n146 widget 1 {bx} {by}\n200 form-back\n')
            images={}
            for frame in (140,142,146,147,166,190,210):
                with self.subTest(frame=frame):
                    args=['--update-passes',str(frame)];cpu=run(*args)
                    still=root/f'magic-still-{frame}.bmp';window=root/f'magic-window-{frame}.bmp'
                    self.assertEqual(run(*args,'--capture',str(still)),cpu)
                    self.assertEqual(run(*args,'--interactive','--hidden','--capture',str(window)),cpu)
                    self.assertEqual(still.read_bytes(),window.read_bytes());images[frame]=still.read_bytes()
                    update=cpu['background']['update']
                    if frame>=146:self.assertEqual(update['wallet']['money'],25);self.assertEqual(dict(update['shop_purchases'])[0],1)
                    if frame==142:self.assertEqual(update['wallet']['money'],37)
                    if frame==210:self.assertEqual(update['magic_returns'],1);self.assertNotIn('magic_shop',update)
            self.assertNotEqual(images[140],images[142]);self.assertNotEqual(images[140],images[210])

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public CTest font required')
    def test_construction_grid_parent_pop_and_changed_town_pixels_match_window(self):
        from town_campaign_fixture import create_campaign_fixture,add_construction_case
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);create_campaign_fixture(root);add_construction_case(root)
            replay=root/'construction.replay';replay.write_text('')
            base=[str(EXE),'--data',str(root),'--foreground','--shared-world','--physical-input',
                  '--camera-x','0','--talk-pulse-every','10','--replay',str(replay)]
            def run(*args):
                r=subprocess.run([*base,*args],capture_output=True,text=True,encoding='utf-8',timeout=45)
                self.assertEqual(r.returncode,0,r.stderr);return json.loads(r.stdout)
            before=run('--update-passes','60');t=next(t for t in before['background']['buildings_draw']['touches'] if t[0]==2)
            x,y=t[2]+40,t[3]+45
            replay.write_text('\n'.join([*(f'{61+i} widget {i%2} {x} {y}' for i in range(4)),
                *(f'{73+i} widget {i%2} 248 240' for i in range(4))])+'\n')
            images={}
            for frame in (60,72,74,76,77,91,110):
                with self.subTest(frame=frame):
                    args=['--update-passes',str(frame)];cpu=run(*args)
                    still=root/f'build-still-{frame}.bmp';window=root/f'build-window-{frame}.bmp'
                    self.assertEqual(run(*args,'--capture',str(still)),cpu)
                    self.assertEqual(run(*args,'--interactive','--hidden','--capture',str(window)),cpu)
                    self.assertEqual(still.read_bytes(),window.read_bytes());images[frame]=still.read_bytes()
                    u=cpu['background']['update']
                    if frame>=76:self.assertEqual(u['wallet']['crystals'],36);self.assertIn([2,0,0],cpu['buildings'])
                    if frame==74:self.assertEqual(u['build_list']['selected'],0);self.assertEqual(u['constructions'],0)
            self.assertNotEqual(images[60],images[72]);self.assertNotEqual(images[60],images[110])

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public CTest font required')
    def test_king_hall_selection_nested_reward_and_return_match_window(self):
        from town_campaign_fixture import create_campaign_fixture,add_king_reward_case
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);create_campaign_fixture(root);add_king_reward_case(root)
            replay=root/'king.replay';replay.write_text('')
            base=[str(EXE),'--data',str(root),'--foreground','--shared-world','--physical-input',
                  '--camera-x','300','--high-score','20','--talk-pulse-every','10','--replay',str(replay)]
            def run(*args):
                r=subprocess.run([*base,*args],capture_output=True,text=True,encoding='utf-8',timeout=45)
                self.assertEqual(r.returncode,0,r.stderr);return json.loads(r.stdout)
            # Read native geometry to avoid a guessed screen target.
            before=run('--update-passes','60');t=next(t for t in before['background']['buildings_draw']['touches'] if t[0]==1)
            x,y=t[2]+40,t[3]+45
            replay.write_text(f'61 widget 0 {x} {y}\n62 widget 1 {x} {y}\n63 widget 0 {x} {y}\n64 widget 1 {x} {y}\n151 form-back\n')
            for frame in (61,64,65,72,111,160):
                with self.subTest(frame=frame):
                    args=['--update-passes',str(frame)];cpu=run(*args)
                    still=root/f'king-still-{frame}.bmp';window=root/f'king-window-{frame}.bmp'
                    self.assertEqual(run(*args,'--capture',str(still)),cpu)
                    self.assertEqual(run(*args,'--interactive','--hidden','--capture',str(window)),cpu)
                    self.assertEqual(still.read_bytes(),window.read_bytes())
                    if frame>=111:self.assertEqual(cpu['background']['update']['wallet'],{'money':48,'crystals':53})

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public CTest font required')
    def test_resident_gift_capture_popup_and_resume_match_cpu_and_window(self):
        from town_campaign_fixture import create_campaign_fixture
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);create_campaign_fixture(root)
            replay=root/'gift.replay';replay.write_text('61 widget 0 430 505\n62 widget 1 430 505\n80 buttons 0x02000000\n')
            base=[str(EXE),'--data',str(root),'--foreground','--shared-world','--physical-input',
                  '--camera-x','300','--lib-seed','1','--talk-pulse-every','10','--replay',str(replay)]
            def run(*args):
                r=subprocess.run([*base,*args],capture_output=True,text=True,timeout=45)
                self.assertEqual(r.returncode,0,r.stderr);return json.loads(r.stdout)
            images={}
            for frame in (61,62,63,67,70,75,100):
                with self.subTest(frame=frame):
                    args=['--update-passes',str(frame)];cpu=run(*args)
                    still=root/f'gift-still-{frame}.bmp';window=root/f'gift-window-{frame}.bmp'
                    self.assertEqual(run(*args,'--capture',str(still)),cpu)
                    self.assertEqual(run(*args,'--interactive','--hidden','--capture',str(window)),cpu)
                    self.assertEqual(still.read_bytes(),window.read_bytes());images[frame]=still.read_bytes()
                    u=cpu['background']['update'];self.assertEqual(u['wallet']['money'],37 if frame==61 else 337)
                    if 62<=frame<75:self.assertEqual(u['popup_contents']['type'],19)
                    elif frame>=75:self.assertFalse(u['talk_pending'])
            self.assertNotEqual(images[61],images[62]) # Gift icon removed only after the native release callback.

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public CTest font required')
    def test_hidden_area_menu_open_pause_and_back_match_cpu_and_still_capture(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);add_town_background(root,update=True)
            font=root/'unity/Font/public.ttf';font.parent.mkdir(parents=True)
            font.write_bytes(Path(os.environ['SAGA_TEST_FONT']).read_bytes())
            (root/'mission-font.tsv').write_text(f'mission_font 1\nfont "unity/Font/public.ttf" {font.stat().st_size}\n')
            replay=root/'menu.replay';replay.write_text('160 widget 0 200 450\n161 widget 1 200 450\n162 widget 0 200 480\n163 widget 1 200 480\n184 stage-back\n')
            base=[str(EXE),'--data',str(root),'--foreground','--camera-x','300','--talk-pulse-every','10','--replay',str(replay)]
            def run(*args):
                result=subprocess.run([*base,*args],capture_output=True,text=True,timeout=45)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            for count in (163,183,184):
                with self.subTest(count=count):
                    args=['--update-passes',str(count)];cpu=run(*args,'--physical-input')
                    still=root/f'still-{count}.bmp';window=root/f'window-{count}.bmp'
                    self.assertEqual(run(*args,'--physical-input','--capture',str(still)),cpu)
                    self.assertEqual(run(*args,'--interactive','--hidden','--capture',str(window)),cpu)
                    self.assertEqual(still.read_bytes(),window.read_bytes())
                    self.assertEqual(cpu['background']['update']['stage_menu']['pending'],count<184)

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public CTest font required')
    def test_shared_world_menu_and_mission_preserve_logic_and_pixels(self):
        from town_campaign_fixture import create_campaign_fixture
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);create_campaign_fixture(root)
            replay=root/'world.replay';replay.write_text('160 widget 0 200 450\n161 widget 1 200 450\n162 widget 0 200 480\n163 widget 1 200 480\n165 widget 0 250 250\n165 widget 1 250 250\n165 widget 0 250 250\n166 widget 1 250 250\n')
            base=[str(EXE),'--data',str(root),'--foreground','--shared-world','--camera-x','300','--talk-pulse-every','10','--server-time','123','--replay',str(replay)]
            def run(*args):
                result=subprocess.run([*base,*args],capture_output=True,text=True,timeout=45)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            def semantic(report):
                # S1 hashes CPU logic separately from GPU-warmed SEB/render work.
                result=json.loads(json.dumps(report));mission=result['background']['update']['mission']
                if mission:
                    mission.pop('state_hash');mission.pop('state_hash_bytes')
                return result
            for count in (1,166,207,300):
                with self.subTest(count=count):
                    args=['--update-passes',str(count)];cpu=run(*args,'--physical-input')
                    still=root/f'world-still-{count}.bmp';window=root/f'world-window-{count}.bmp'
                    self.assertEqual(semantic(run(*args,'--physical-input','--capture',str(still))),semantic(cpu))
                    self.assertEqual(semantic(run(*args,'--interactive','--hidden','--capture',str(window))),semantic(cpu))
                    self.assertEqual(still.read_bytes(),window.read_bytes())
                    self.assertTrue(cpu['background']['shared_world']['same_hero'])
                    self.assertEqual(cpu['background']['shared_world']['initialized_systems'],32)
                    self.assertEqual(cpu['background']['update']['mission_active'],count>=207)
                    if count>=207:
                        mission=cpu['background']['update']['mission']
                        self.assertEqual(mission['logic_steps'],count-207)
                        self.assertEqual(mission['quest_detail_count'],1)
            mission_replay=root/'viewport.replay';mission_replay.write_text('1 viewport 320 240\n',encoding='ascii')
            resize=root/'resize.bmp';resized=run('--update-passes','208','--interactive','--hidden','--mission-replay',str(mission_replay),'--capture',str(resize))
            cpu=run('--update-passes','208','--physical-input','--mission-replay',str(mission_replay))
            self.assertEqual(semantic(resized),semantic(cpu))
            width,height,_=bmp_pixels(resize.read_bytes());self.assertEqual((width,height),(320,240))

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public CTest font required')
    def test_retained_return_and_reentry_match_cpu_window_and_still_pixels(self):
        from town_campaign_fixture import create_campaign_fixture
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);create_campaign_fixture(root)
            replay=root/'return.replay';replay.write_text('160 widget 0 200 450\n161 widget 1 200 450\n162 widget 0 200 480\n163 widget 1 200 480\n165 widget 0 250 250\n165 widget 1 250 250\n165 widget 0 250 250\n166 widget 1 250 250\n')
            mission=root/'retreat.replay';mission.write_text(''.join(f'{i} pulse 1048576\n' for i in range(30,181,5))+
                '200 pulse 33554432\n201 pulse 134217728\n202 answer 0\n')
            base=[str(EXE),'--data',str(root),'--foreground','--shared-world','--camera-x','300','--talk-pulse-every','10','--server-time','123','--replay',str(replay),'--mission-replay',str(mission)]
            def run(n,*extra):
                result=subprocess.run([*base,'--update-passes',str(n),*extra],capture_output=True,text=True,timeout=45)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            def semantic(value):
                if isinstance(value,dict):return {k:semantic(v) for k,v in value.items() if k not in ('state_hash','state_hash_bytes')}
                if isinstance(value,list):return [semantic(v) for v in value]
                return value
            returned=run(500,'--physical-input')['background']
            region=next(r for r in returned['update']['widgets']['regions'] if r[1]==0xc0000)
            x,y=region[2]+region[4]//2,region[3]+region[5]//2
            replay.write_text(replay.read_text()+f'501 widget 0 {x} {y}\n502 widget 1 {x} {y}\n')
            selected=run(502,'--physical-input')['background']
            region=next(r for r in selected['update']['widgets']['regions'] if r[0]==4 and r[1]==28)
            x,y=region[2]+region[4]//2,region[3]+region[5]//2
            replay.write_text(replay.read_text()+f'503 widget 0 {x} {y}\n504 widget 1 {x} {y}\n506 widget 0 250 250\n507 widget 1 250 250\n')
            for count in (409,410,411,548,900):
                with self.subTest(count=count):
                    cpu=run(count,'--physical-input');still=root/f'return-still-{count}.bmp';window=root/f'return-window-{count}.bmp'
                    self.assertEqual(semantic(run(count,'--physical-input','--capture',str(still))),semantic(cpu))
                    self.assertEqual(semantic(run(count,'--interactive','--hidden','--capture',str(window))),semantic(cpu))
                    self.assertEqual(still.read_bytes(),window.read_bytes())
                    self.assertTrue(cpu['background']['shared_world']['same_hero'])
                    self.assertEqual(cpu['background']['shared_world']['initialized_systems'],32)

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public CTest font required')
    def test_stage_row_statistics_and_held_entry_match_window(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);add_town_background(root,update=True,stage_entries=True)
            font=root/'unity/Font/public.ttf';font.parent.mkdir(parents=True)
            font.write_bytes(Path(os.environ['SAGA_TEST_FONT']).read_bytes())
            (root/'mission-font.tsv').write_text(f'mission_font 1\nfont "unity/Font/public.ttf" {font.stat().st_size}\n')
            replay=root/'row.replay';replay.write_text('160 widget 0 200 450\n161 widget 1 200 450\n162 widget 0 200 480\n163 widget 1 200 480\n165 widget 0 250 250\n165 widget 1 250 250\n165 widget 0 250 250\n166 widget 1 250 250\n')
            base=[str(EXE),'--data',str(root),'--foreground','--camera-x','300','--talk-pulse-every','10','--replay',str(replay),'--server-time','123']
            def run(*args):
                result=subprocess.run([*base,*args],capture_output=True,text=True,timeout=45)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            for count in (165,166,207):
                with self.subTest(count=count):
                    args=['--update-passes',str(count)];cpu=run(*args,'--physical-input')
                    still=root/f'row-still-{count}.bmp';window=root/f'row-window-{count}.bmp'
                    self.assertEqual(run(*args,'--physical-input','--capture',str(still)),cpu)
                    self.assertEqual(run(*args,'--interactive','--hidden','--capture',str(window)),cpu)
                    self.assertEqual(still.read_bytes(),window.read_bytes())
                    update=cpu['background']['update'];menu=update['stage_menu']
                    self.assertEqual(menu['rows'],[[10,131072,195,232,210,40]])
                    self.assertEqual((menu['text_draws'],menu['missing_text'],menu['selected']),(17,0,0))
                    self.assertEqual(update['stamina'],10 if count==165 else 9)
                    self.assertEqual(menu['quest_handoff_pending'],count==207)

    @unittest.skipUnless(os.environ.get('SAGA_TEST_FONT'),'public CTest font required')
    def test_hidden_interactive_loop_matches_physical_headless_state_and_pixels(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);add_town_background(root,update=True)
            font=root/'unity/Font/public.ttf';font.parent.mkdir(parents=True)
            font.write_bytes(Path(os.environ['SAGA_TEST_FONT']).read_bytes())
            (root/'mission-font.tsv').write_text(f'mission_font 1\nfont "unity/Font/public.ttf" {font.stat().st_size}\n')
            replay=root/'pointer.replay'
            replay.write_text('160 pointer 0 130 550 0 0\n161 pointer 1 130 550 0 0\n')
            base=[str(EXE),'--data',str(root),'--foreground','--talk-pulse-every','10','--replay',str(replay)]
            def run(*args):
                result=subprocess.run([*base,*args],capture_output=True,text=True,timeout=45)
                self.assertEqual(result.returncode,0,result.stderr);return json.loads(result.stdout)
            # Stop once while captured and once after reaching the resident.
            for count in (160,174):
                with self.subTest(count=count):
                    args=['--update-passes',str(count)]
                    cpu=run(*args,'--physical-input')
                    still=root/f'still-{count}.bmp';window=root/f'window-{count}.bmp'
                    self.assertEqual(run(*args,'--physical-input','--capture',str(still)),cpu)
                    self.assertEqual(run(*args,'--interactive','--hidden','--capture',str(window)),cpu)
                    self.assertEqual(still.read_bytes(),window.read_bytes())
                    state=cpu['background']['update']
                    self.assertEqual(state['pointer']['captured'],count==160)
                    self.assertEqual(state['pointer']['cancels'],0)
                    if count==174:
                        self.assertEqual(cpu['background']['hero']['current_x'],130)
                        self.assertEqual(state['bubble']['frames'],119)

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

    def test_selected_motion_changes_pixels_and_retains_cpu_gl_parity(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);add_town_background(root,motion=True)
            base=[str(EXE),'--data',str(root),'--foreground','--width','600','--height','320','--app-seed','0']
            captures=[]
            for passes in (0,1,60):
                path=root/f'motion-{passes}.bmp'
                command=[*base,'--motion-passes',str(passes)]
                gpu=subprocess.run([*command,'--capture',str(path)],capture_output=True,text=True,timeout=30)
                self.assertEqual(gpu.returncode,0,gpu.stderr)
                cpu=subprocess.run(command,capture_output=True,text=True,timeout=30)
                self.assertEqual(cpu.returncode,0,cpu.stderr);self.assertEqual(json.loads(cpu.stdout),json.loads(gpu.stdout))
                repeat=root/f'repeat-{passes}.bmp'
                again=subprocess.run([*command,'--capture',str(repeat)],capture_output=True,text=True,timeout=30)
                self.assertEqual(again.returncode,0,again.stderr);self.assertEqual(again.stdout,gpu.stdout)
                self.assertEqual(path.read_bytes(),repeat.read_bytes());captures.append(path.read_bytes())
            self.assertNotEqual(captures[0],captures[1]);self.assertNotEqual(captures[1],captures[2])

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
