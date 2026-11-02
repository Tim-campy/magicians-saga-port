"""Synthetic resources for the reached Town drawing prefix; no game bytes."""
from test_sprite_preview import jar,png,seb


def add_town_background(root):
    if not (root/'textassets').exists():
        (root/'textassets').mkdir()
        (root/'mission-resources.tsv').write_text('mission_resources 1\ntables -1 "textassets/tables.bin" "xls"\n')
        (root/'new-game-save.tsv').write_text('new_game_save 1\nselection 0 -1\ntown_capacity 3\nwallet 37 43\n')
    rows=[f'{i}\t{7+i}\t0\t-1\t1' for i in range(3)]
    area='0\tFirst\t0\t0\t1\n1\tSecond\t0\t0\t0'
    (root/'textassets/tables.bin').write_bytes(jar([
        ('English.lproj/Town.txt','\n'.join(rows).encode()),('English.lproj/Area.txt',area.encode())]))
    atlas=png(4,2,[c for _ in range(2) for p in [(255,0,0,255)]*2+[(0,255,0,255)]*2 for c in p])
    (root/'textassets/select.bin').write_bytes(jar([
        ('img.inf',b'0\tatlas.png,bin\n9\tbackground.png,bin\n'),
        ('seb.inf',b'0\tbackground.seb\n'),
        ('background.seb',seb([(0,0,0,0,2,2,0,0,0,0),(1,0,2,0,2,2,0,0,0,0)])),
        ('atlas.png',atlas),('background.png',png(1,2,[10,20,30,255,70,80,90,255]))]))
    (root/'town-resources.tsv').write_text('town_resources 1\ntables -1 "textassets/tables.bin" "xls"\nresource 11 "textassets/select.bin" "select"\n')
