"""Synthetic resources for the reached Town drawing prefix; no game bytes."""
import struct
from test_sprite_preview import jar,png,seb


def add_town_background(root,scenery=False):
    if not (root/'textassets').exists():
        (root/'textassets').mkdir()
        (root/'mission-resources.tsv').write_text('mission_resources 1\ntables -1 "textassets/tables.bin" "xls"\n')
        (root/'new-game-save.tsv').write_text('new_game_save 1\nselection 0 -1\ntown_capacity 3\nwallet 37 43\n')
    rows=[f'{i}\t{7+i}\t0\t-1\t1' for i in range(3)]
    area='0\tFirst\t0\t0\t1\n1\tSecond\t0\t0\t0'
    (root/'textassets/tables.bin').write_bytes(jar([
        ('English.lproj/Town.txt','\n'.join(rows).encode()),('English.lproj/Area.txt',area.encode())]))
    atlas=png(4,2,[c for _ in range(2) for p in [(255,0,0,255)]*2+[(0,255,0,255)]*2 for c in p])
    members=[
        ('img.inf',b'0\tatlas.png,bin\n9\tbackground.png,bin\n'),
        ('seb.inf',b'0\tbackground.seb\n'),
        ('background.seb',seb([(0,0,0,0,2,2,0,0,0,0),(1,0,2,0,2,2,0,0,0,0)])),
        ('atlas.png',atlas),('background.png',png(1,2,[10,20,30,255,70,80,90,255]))]
    if scenery:
        def layers():
            return struct.pack('>hh',3,10)+b''.join(struct.pack('>hh10h',1,0,0,2,i*2,0,2,2,0,0,0,0) for i in range(3))
        members[0]=('img.inf',b'0\tatlas.png,bin\n1\tground.png,bin\n2\tbirds.png,bin\n9\tbackground.png,bin\n')
        members[1]=('seb.inf',b'0\tbackground.seb\n4\tground.seb\n14\tcloud.seb\n22\tbirds.seb\n23\tleaves.seb\n')
        members[2]=('background.seb',seb([(0,0,0,0,2,2,0,0,0,0),(1,0,2,0,2,2,0,0,0,0),(3,0,0,0,2,2,0,0,0,0)]))
        members.extend([
            ('ground.seb',seb([(0,1,0,0,40,8,0,0,0,0),(1,1,0,0,40,8,0,0,0,0)])),
            ('cloud.seb',seb([(0,0,0,0,2,2,0,0,0,0)])),
            ('birds.seb',layers()),('leaves.seb',layers()),
            ('ground.png',png(40,8,[80,90,100,255]*(40*8))),
            ('birds.png',png(6,2,[channel for _ in range(2) for color in [(255,0,255,255)]*2+[(255,255,0,255)]*2+[(0,255,255,255)]*2 for channel in color]))])
    (root/'textassets/select.bin').write_bytes(jar(members))
    (root/'town-resources.tsv').write_text('town_resources 1\ntables -1 "textassets/tables.bin" "xls"\nresource 11 "textassets/select.bin" "select"\n')
