"""Synthetic resources for the reached Town drawing prefix; no game bytes."""
import struct
from test_sprite_preview import jar,png,seb


def add_town_background(root,scenery=False,buildings=False,areas=False,residents=False,hero=False,foreground=False,motion=False,update=False):
    motion=motion or update
    foreground=foreground or motion
    hero=hero or foreground
    residents=residents or hero
    areas=areas or residents
    buildings=buildings or areas
    scenery=scenery or buildings
    if not (root/'textassets').exists():
        (root/'textassets').mkdir()
        (root/'mission-resources.tsv').write_text('mission_resources 1\ntables -1 "textassets/tables.bin" "xls"\n')
        (root/'new-game-save.tsv').write_text('new_game_save 1\nselection 0 -1\ntown_capacity 3\nwallet 37 43\n')
    rows=[f'{i}\t{7+i}\t0\t-1\t1' for i in range(3)]
    area='0\tFirst\t0\t0\t1\n1\tSecond\t0\t0\t0'
    tables=[('English.lproj/Town.txt','\n'.join(rows).encode()),('English.lproj/Area.txt',area.encode())]
    if buildings:
        (root/'new-game-save.tsv').write_text('new_game_save 1\nselection 0 -1\ntown_capacity 4\nwallet 37 43\n')
        tables[0]=('English.lproj/Town.txt',b'0\t12\t0\t-1\t1\n1\t4\t0\t-1\t1\n2\t-1\t1\t1\t99\t-1\t0\n3\t-1\t1\t1\t99\t-1\t0')
        if foreground:tables[0]=(tables[0][0],tables[0][1]+b'\n4\t-1\t0\t-1\t0')
        tables.extend([
            ('English.lproj/Building.txt',b'4\tKing "Hall"\\Name\tselect\t14\t50\t7\t0\t0\tGift\t2\n12\tTree\tselect\t47\t-1\t8\t0\t0\tDecoration\t2'),
            ('English.lproj/KingReward.txt',b'0\tFirst\t10\t0\t0\t0\t0'),
            ('English.lproj/Chara.txt',b'0\tResident\t7\tres\t8\tres2\t1\t2\t3\t4\t5\t150\t1\t2\t11\t12\t6\t7\t8\t-1\t9\t5')])
    if residents:
        def character(index,town,terms,flags=32):
            tokens=[index,'Person',0,'chara',10,'',0,0,0,1,0,100,0,0,town,-1,*terms,100,flags]
            return '\t'.join(map(str,tokens))
        tables[-1]=('English.lproj/Chara.txt','\n'.join([character(0,0,[0],96),character(1,-1,[1,2,4,1]),character(2,-1,[1,3,7,1,1])]).encode())
        gear=['0','Wand','0','0','0','0','weapon','3']+['0']*13+['0','Description','0']
        quest=[0,'Quest',0,'quest',0,'','',1,1,1,100,1,1,1,100,0,0,-1,0,0,0,-1,-1,1,0]
        tables.extend([('English.lproj/Equipment.txt','\t'.join(gear).encode()),('English.lproj/Quest.txt','\t'.join(map(str,quest)).encode())])
        if hero:
            # Initial saved level7 deliberately differs from World.CreateHero's live level1.
            main=[3,'Hero',0,'chara',10,'',0,0,0,7,0,100,13,*([2,10,20]*13),0,-1,-1,0,100,2]
            for i,(name,data) in enumerate(tables):
                if name.endswith('/Chara.txt'):
                    extra=['\t'.join(map(str,main)),*[character(index,-1,[0],96) for index in range(4,10)]]
                    tables[i]=(name,data+b'\n'+'\n'.join(extra).encode())
            def wearable(index,image,flags):
                return '\t'.join(map(str,[index,'Gear',0,'hat',image,0,0,1,0,0,'Description',flags]))
            # First main hat is image7, while equipped hat ID0 would resolve image5.
            tables.extend([('English.lproj/Hat.txt',(wearable(0,5,0)+'\n'+wearable(1,7,2)).encode()),
                           ('English.lproj/Cloth.txt',wearable(0,12,0).encode())])
    if update:
        tables[0]=(tables[0][0],tables[0][1].replace(b'1\t1\t99',b'1\t3\t7\t1\t1'))
        # Synthetic Event0 has delay1 followed by two single-page talks. Later
        # events are ineligible; this tests the full selector rather than injection.
        event='0\tIntro\t0\t1\t3\t2\t0\t1\t3\t3\t10\t1\t-1\t2\t8\t0\t2\t8\t1'
        tables.extend([('English.lproj/Event.txt',event.encode()),
                       ('English.lproj/Talk.txt',b'0\t0\t1\tFirst<br>page\n1\t1\t1\tSecond<co=4a86e8>page</co>')])
        (root/'mission-initialization.tsv').write_text('mission_initialization 2\nyugami 10000\norientation 0\nevent_arguments ""\ntown_normal 0 -7\ntown_system 0 0 0 10\n')
    (root/'textassets/tables.bin').write_bytes(jar(tables))
    atlas=png(4,2,[c for _ in range(2) for p in [(255,0,0,255)]*2+[(0,255,0,255)]*2 for c in p])
    members=[
        ('img.inf',b'0\tatlas.png,bin\n9\tbackground.png,bin\n'),
        ('seb.inf',b'0\tbackground.seb\n'),
        ('background.seb',seb([(0,0,0,0,2,2,0,0,0,0),(1,0,2,0,2,2,0,0,0,0)])),
        ('atlas.png',atlas),('background.png',png(1,2,[10,20,30,255,70,80,90,255]))]
    if scenery:
        def layers():
            count=10 if motion else 1
            return struct.pack('>hh',3,10)+b''.join(struct.pack('>hh',count,0)+b''.join(struct.pack('>10h',frame,2,i*2,0,2,2,0,0,0,0) for frame in range(count)) for i in range(3))
        members[0]=('img.inf',b'0\tatlas.png,bin\n1\tground.png,bin\n2\tbirds.png,bin\n3\tfloor.png,bin\n4\tgate.png,bin\n9\tbackground.png,bin\n')
        members[1]=('seb.inf',b'0\tbackground.seb\n2\tfloor.seb\n3\tgate.seb\n4\tground.seb\n14\tcloud.seb\n22\tbirds.seb\n23\tleaves.seb\n')
        members[2]=('background.seb',seb([(0,0,0,0,2,2,0,0,0,0),(1,0,2,0,2,2,0,0,0,0),(3,0,0,0,2,2,0,0,0,0)]))
        members.extend([
            ('ground.seb',seb([(0,1,0,0,40,8,0,0,0,0),(1,1,0,0,40,8,0,0,0,0)]+([(2,1,0,0,40,8,0,-100,0,0)] if foreground else []))),
            ('cloud.seb',seb([(0,0,0,0,2,2,0,0,0,0)])),
            # TexId77 deliberately has no image slot; the floor helper must use Image3.
            ('floor.seb',seb([(frame,77,frame*32,0,32,6,0,0,0,0) for frame in range(3)])),
            ('floor.png',png(96,6,[c for _ in range(6) for color in [(120,0,0,255)]*32+[(0,100,200,255)]*32+[(0,120,0,255)]*32 for c in color])),
            ('gate.seb',struct.pack('>hh',2,4)+struct.pack('>hh10h',1,0,0,4,0,0,3,2,5,7,0,0)+struct.pack('>hh10h',1,0,0,0,0,0,2,2,5,7,0,0)),
            ('gate.png',png(3,2,[210,30,90,255]*6)),
            ('birds.seb',layers()),('leaves.seb',layers()),
            ('ground.png',png(40,8,[80,90,100,255]*(40*8))),
            ('birds.png',png(6,2,[channel for _ in range(2) for color in [(255,0,255,255)]*2+[(255,255,0,255)]*2+[(0,255,255,255)]*2 for channel in color]))])
    if buildings:
        # Chip TexId77 deliberately has no image; every call must honor the override.
        members[0]=('img.inf',members[0][1]+b''.join(f'{i}\tchip{i}.png,bin\n'.encode() for i in (14,17,18,32,33,47)))
        members[1]=('seb.inf',members[1][1]+b'1\tchip.seb\n9\tbutton.seb\n11\tboard.seb\n17\tarrow.seb\n')
        members.extend([('chip.seb',seb([(frame,77,0,0,3,2,0,0,0,0) for frame in range(4 if motion else 1)])),
                        ('button.seb',seb([(0,0,0,0,2,2,0,0,0,0)])),
                        ('board.seb',seb([(0,0,0,0,2,2,0,0,0,0),(1,0,0,0,2,2,0,0,0,0)])),
                        ('arrow.seb',seb([(0,0,0,0,2,2,0,0,0,0)]))])
        for i,color in ((14,(210,100,10,255)),(17,(20,30,40,255)),(18,(40,50,60,255)),
                        (32,(80,90,10,255)),(33,(70,80,10,255)),(47,(10,100,210,255))):
            members.append((f'chip{i}.png',png(3,2,list(color)*6)))
        if foreground:
            members[0]=('img.inf',members[0][1]+b'31\tobstacle.png,bin\n')
            members[1]=('seb.inf',members[1][1]+b'26\tboundary.seb\n')
            members.extend([('obstacle.png',png(3,2,[150,70,190,255]*6)),
                            ('boundary.seb',seb([(0,33,0,0,3,2,0,0,0,0),(1,33,0,0,3,2,-80,0,0,0)]))])
        if update:
            members[1]=('seb.inf',members[1][1]+b'29\tguide.seb\n30\tguide.seb\n')
            members.append(('guide.seb',seb([(frame,0,0,0,2,2,0,0,0,0) for frame in range(4)])))
    if areas:
        members[1]=('seb.inf',members[1][1]+b'10\tarea-name.seb\n12\tfence.seb\n')
        members.extend([('area-name.seb',seb([(0,0,0,0,2,2,0,0,0,0),(1,0,2,0,2,2,0,0,0,0)])),
                        ('fence.seb',seb([(0,0,0,0,2,2,0,0,0,0)]))])
        (root/'textassets/bg.bin').write_bytes(jar([('img.inf',b'0\tmap.png,bin\n'),('seb.inf',b'3\tmap.seb\n'),
            ('map.png',png(3,2,[10,200,100,255]*6)),('map.seb',seb([(0,0,0,0,3,2,0,0,0,0)]))]))
        (root/'textassets/com.bin').write_bytes(jar([('img.inf',b'0\ticon.png,bin\n'),('seb.inf',b'15\ticon.seb\n'),
            ('icon.png',png(2,2,[200,100,10,255]*4)),('icon.seb',seb([(0,0,0,0,2,2,0,0,0,0)]))]))
    (root/'textassets/select.bin').write_bytes(jar(members))
    (root/'town-resources.tsv').write_text('town_resources 1\ntables -1 "textassets/tables.bin" "xls"\nresource 11 "textassets/select.bin" "select"\n')
    if areas:
        (root/'town-resources-v2.tsv').write_text('town_resources 2\ntables -1 "textassets/tables.bin" "xls"\nresource 0 "textassets/bg.bin" "bg"\nresource 2 "textassets/com.bin" "com"\nresource 11 "textassets/select.bin" "select"\n')
    if residents:
        animation=struct.pack('>hh',5,4)
        for layer,offset in enumerate((0,2,5,0,8)):
            size=1 if layer==0 else 2
            animation+=struct.pack('>hh',4,0)+b''.join(struct.pack('>10h',frame,77,0,0,size,size,offset,0,0,0) for frame in range(4))
        (root/'textassets/chara.bin').write_bytes(jar([('img.inf',b'10\tbody.png,bin\n200\tshadow.png,bin\n'),('seb.inf',b'38\thuman.seb\n'+(b'37\thuman.seb\n' if hero else b'')),
            ('human.seb',animation),('body.png',png(2,2,[200,70,20,255]*4)),('shadow.png',png(1,1,[20,20,20,255]))]))
        (root/'textassets/weapon.bin').write_bytes(jar([('img.inf',b'3\tweapon.png,bin\n'),('seb.inf',b''),('weapon.png',png(2,2,[20,70,200,255]*4))]))
        with (root/'mission-resources.tsv').open('a') as out:
            out.write('resource 1 "textassets/chara.bin" "chara"\nresource 22 "textassets/weapon.bin" "weapon"\n')
            if hero:out.write('resource 21 "textassets/hat.bin" "hat"\n')
        if hero:
            (root/'textassets/hat.bin').write_bytes(jar([('img.inf',b'5\twrong.png,bin\n7\that.png,bin\n'),('seb.inf',b''),
                ('wrong.png',png(2,2,[255,0,0,255]*4)),('hat.png',png(2,2,[40,190,80,255]*4))]))
