"""Synthetic table/archive profile shared by mission and GPU contracts."""
from test_sprite_preview import jar, png, seb

def make_table_scene(root, valid_png=True, human_frame=False, shoot=False, rewards=False, area=False, lethal_contact=False, treasure_flags=0, spell_attribute=3, boss_entry=False, alternate_quest=False, stage_entry=False, talk_events=False):
    params = '13' + '\t2\t100\t1000' * 13
    tables = {
        'Quest.txt': ('2\tSynthetic quest\t0\tmap\t4\t0\t1\t2\t3\tunused\tunused\t7\t7\t10\t200\t25\t0\t10\t150\t'
                      '1\t0\t1\t0\t4\t1\t0\t1\t0\t3\t0\t0\t2\t1'),
        'Chara.txt': f'2\tHero\t0\tchara\t8\tchara\t1\t0\t0\t99\t0\t100\t{params}\t0\t0\t0\t-1\t0\t2',
        'Equipment.txt': f'1\tWeapon\t0\t2\t0\t0\tweapon\t12\t3\t0\t0\t0\t0\t10\t8\t80\t1\t20\t3\t0\t0\t{params}\tText\t0',
        'Cloth.txt': f'0\tCloth\t0\tcloth\t11\t0\t0\t0\t0\t{params}\tText\t0',
        'Hat.txt': f'0\tHat\t0\that\t9\t0\t0\t0\t0\t{params}\tText\t2',
        'MagicShop.txt': '\n'.join(f'{i}\tUpgrade\t0\t1\t2\tText\t0' for i in range(8)),
        'Enemy.txt': f'4\tEnemy\t0\t2\t10\t100\t0\t0\t0\tenemy-book\t7\t0\t0\t0\t{params}\t0',
        'Bullet.txt': '3\t0\t0\tbullet\t5\t6\teffect\t7\t0',
    }
    if shoot:
        weapon = tables['Equipment.txt'].split('\t')
        for i, value in {9: 3, 13: 15, 14: 4, 15: 12, 16: 3, 17: 0, 18: 1}.items():
            weapon[i] = str(value)
        if area:
            weapon[17] = '2'
        tables['Equipment.txt'] = '\t'.join(weapon)
        tables['Bullet.txt'] = f'0\t0\t{spell_attribute}\tbullet\t5\t6\teffect\t7\t0'
        enemy_params = '13' + ''.join('\t2\t100000\t100000' if i == 0 or (lethal_contact and i in (3, 7)) else '\t2\t0\t0' for i in range(13))
        tables['Enemy.txt'] = f'4\tEnemy\t0\t2\t10\t100\t0\t0\t0\tenemy-book\t7\t0\t0\t0\t{enemy_params}\t0'
    if rewards:
        enemy_params = '13' + ''.join('\t2\t1\t1' if i == 0 else '\t2\t0\t0' for i in range(13))
        tables['Enemy.txt'] = f'4\tEnemy\t0\t2\t10\t10\t1\t1\t2\t0\t1000\t0\t0\tenemy-book\t7\t0\t0\t0\t{enemy_params}\t0'
        tables['Treasure.txt'] = f'0\tGem\tquest\t77\t1\t2\t3\t1\t2\t0\t0\t{treasure_flags}'
        tables['Table.txt'] = '\n'.join(f'{i}\t{i}\t10\t0\t0\t0\t0\t0\t0\t0' for i in range(16))
    if human_frame:
        tables['Equipment.txt'] += '\n' + tables['Equipment.txt'].replace('1\tWeapon', '0\tShield', 1)
    if boss_entry:
        tables['Enemy.txt'] += '\n' + tables['Enemy.txt'].replace('4\tEnemy', '1\tExtra', 1)
    if stage_entry:
        tables['Town.txt'] = '0\t7\t0\t-1\t1\n1\t8\t1\t1\t99\t0\t0\n2\t9\t-1\t0\t1'
        tables['Area.txt'] = '5\tSynthetic area\t17\t2\t2\t3\t7\n9\tOther area\t18\t-1\t4'
        # Subcharacters use EquipmentData.bullet by id; the main hero selects Bullet0 by attribute.
        if shoot:tables['Bullet.txt'] += '\n' + tables['Bullet.txt'].replace('0\t', '3\t', 1)
        tables['Enemy.txt'] += '\n' + tables['Enemy.txt'].replace('4\tEnemy', '0\tInitial', 1)
        sub = tables['Chara.txt'].split('\t');sub[0],sub[-1] = '3','4'
        tables['Chara.txt'] += '\n' + '\t'.join(sub)
    if talk_events:
        quest=tables['Quest.txt'].split('\t')
        quest[31]='0'
        quest[24:26]=['1','1','2','212','0']
        tables['Quest.txt']='\t'.join(quest)
        tables['Event.txt']='212\tSynthetic talk event\t0\t0\t1\t2\t8\t4'
        tables['Talk.txt']='4\t2\t2\tFirst <br> page\tSecond <co=ffffff>page</co>'
    if alternate_quest:
        other = tables['Quest.txt'].split('\t')
        other[0], other[13], other[18] = '3', '12', '175'
        tables['Quest.txt'] += '\n' + '\t'.join(other)
    (root / 'tables.bin').write_bytes(jar([('English.lproj/' + name, text.encode()) for name, text in tables.items()]))
    for name, image, clip, color in [('hero', 200, 36, [255, 0, 0, 255]), ('enemy', 7, 0, [0, 255, 255, 255])]:
        clips = ([0, 1, 2, 3, 4, 8, 12, 16, 24, 28, 36, 40] if name == 'hero' else [0, 2, 24]) if human_frame else [clip]
        clips = sorted({base // 4 * 4 + direction for base in clips for direction in range(4)})
        (root / f'{name}.bin').write_bytes(jar([
            ('img.inf', f'{image}\ta.png,bin'.encode()), ('seb.inf', '\n'.join(f'{i}\ta.seb' for i in clips).encode()),
            ('a.seb', seb([(0, image, 0, 0, 2, 2, 0, 0, 0, 0), (3, image, 0, 0, 2, 2, 0, 0, 0, 0)])), ('a.png', png(8, 8, color * 64) if valid_png else b'not-png')]))
    text = ('world 1 0 0 0 1 0 0 0 1\n'
            'projection 0.0625 0 0 0 0 0.125 0 0 0 0 0.00000133324445 0 -1 -1 -1 1\ngpu_depth 24 1\n'
            'resource 1 "hero.bin" "English.lproj" 0\nresource 3 "enemy.bin" "English.lproj" 0\n'
            'global_alias 1 1\nglobal_alias 3 3\ntables "tables.bin" "English.lproj" 0\n'
            'slots 35\nview 32 16 100 -4 -4 0 0 100\nanimate 0 0 1\n'
            'resource_name "enemy-book" 3\nprogress 2 5 13\n'
            + ''.join(f'purchase {i} 0\n' for i in range(8))
            + 'human 0 2 5 0 0 13 0 0 0\nenemy 0 4 7 0 10 0 0\n'
            'material 0 65808 2000 10 0\nmaterial 1 65808 2000 11 0\n')
    return text


def make_shoot_scene(root, rewards=False, area=False, lethal_contact=False, treasure_flags=0, spell_attribute=3, boss_entry=False, alternate_quest=False, stage_entry=False, talk_events=False):
    text = make_table_scene(root, human_frame=True, shoot=True, rewards=rewards, area=area, lethal_contact=lethal_contact, treasure_flags=treasure_flags, spell_attribute=spell_attribute, boss_entry=boss_entry, alternate_quest=alternate_quest, stage_entry=stage_entry, talk_events=talk_events)
    for name, image, clips in [('bullet', 5, [0, 1]), ('effect', 7, [0, 100, 101])]:
        (root / f'{name}.bin').write_bytes(jar([
            ('img.inf', '\n'.join(f'{i}\ta.png,bin' for i in [image, 1, 2, 6, 24, 27]).encode()),
            ('seb.inf', '\n'.join(f'{clip}\ta.seb' for clip in clips).encode()),
            ('a.seb', seb([(0, image, 0, 0, 2, 2, 0, 0, 0, 0), (3, image, 0, 0, 2, 2, 0, 0, 0, 0)])),
            ('a.png', png(2, 2, [0, 255, 0, 255] * 4))]))
    if rewards:
        (root / 'quest.bin').write_bytes(jar([
            ('img.inf', b'77\ta.png,bin\n98\ta.png,bin\n99\ta.png,bin\n105\ta.png,bin'),
            ('seb.inf', '\n'.join(f'{i}\ta.seb' for i in ([41, 42, 43, 44] if boss_entry else [41, 42, 43])).encode()),
            ('a.seb', seb([(0, 98, 0, 0, 2, 2, 0, 0, 0, 0), (3, 98, 0, 0, 2, 2, 0, 0, 0, 0)])),
            ('a.png', png(2, 2, [255, 255, 0, 255] * 4))]))
        text += 'rewards 0 1 4\nresource 7 "quest.bin" "English.lproj" 0\nglobal_alias 7 7\n'
    return (text.replace('progress 2 5 13', 'progress 2 1 0')
            .replace('human 0 2 5 0 0 13 0 0 0', 'human 0 2 1 0 0 0 0 0 100')
            .replace('enemy 0 4 7 0 10 0 0', 'enemy 0 4 7 2 0 0 70')
            + 'simulate 0\ngeometry 24 24 6\nhuman_system 10 0 100 3 1\nplayer 0\n'
              'gear_levels equipment 1\ngear_levels hat 1\ngear_levels cloth 1\n'
              'resource 19 "bullet.bin" "English.lproj" 0\nglobal_alias 19 19\n'
              'resource 20 "effect.bin" "English.lproj" 0\nglobal_alias 20 20\n')


def with_mission_map(root, text):
    """Use the native InitWorld geometry and explicit selected quest chip IDs."""
    (root / 'map.bin').write_bytes(jar([
        ('img.inf', '\n'.join(f'{i}\ta.png,bin' for i in range(4)).encode()),
        ('seb.inf', b'0\ta.seb'),
        ('a.seb', seb([(0, 0, 0, 0, 8, 8, 0, 0, 0, 0), (3, 0, 0, 0, 8, 8, 0, 0, 0, 0)])),
        ('a.png', png(8, 8, [32, 64, 32, 255] * 64))]))
    return (text.replace('geometry 24 24 6', 'geometry 312 312 12')
            .replace('view 32 16 100 -4 -4 0 0 100', 'view 128 96 100 -4 -4 0 0 100')
            .replace('projection 0.0625 0 0 0 0 0.125', 'projection 0.015625 0 0 0 0 0.0208333333')
            + 'resource 18 "map.bin" "English.lproj" 0\nglobal_alias 18 18\n'
              'mission_map fresh 0 1 2 3\nruntime 25 0 system 42\n')


def make_boss_scene(root, alternate_quest=False, stage_entry=False, talk_events=False):
    """Explicit near-boss entry state; Stage3 creates both native enemies and the fence."""
    text = make_shoot_scene(root, rewards=True, treasure_flags=1, spell_attribute=0, boss_entry=True, alternate_quest=alternate_quest, stage_entry=stage_entry, talk_events=talk_events)
    text = '\n'.join(line for line in text.splitlines() if not line.startswith('enemy ')) + '\n'
    text = (text.replace('human 0 2 1 0 0 0 0 0 100', 'human 0 2 1 0 0 0 312 0 400')
            .replace('human_system 10 0 100 3 1', 'human_system 10 0 95 0 0'))
    text += ('enemy_system 10000 0 10 25 7\nenemy_population 0 0 1 0 0 100 0\nenemy_pool 0\n'
             'stage_notifications 1 0 0 2 0 0 -1\ncollection fresh\n'
             'stage_loop 5000 0 20 hold\nstage_events fresh 0\nstage_boss 4 7\n'
             'input fresh 0 0 0 0 0 0 0 0 0\n')
    return with_mission_map(root, text)


def make_gauge_scene(root, defeats=6):
    """Native enemy HP drawing threshold and a two-half synthetic gauge image."""
    text = make_shoot_scene(root, rewards=True, spell_attribute=0)
    text = (text.replace('human_system 10 0 100 3 1', 'human_system 10 0 100 0 0')
            .replace('human 0 2 1 0 0 0 0 0 100', 'human 0 2 1 0 0 0 0 0 0')
            .replace('enemy 0 4 7 2 0 0 70', 'enemy 0 4 7 2 8 0 0')
            .replace('rewards 0 1 4', f'rewards 0 1 {defeats}'))
    colors = []
    for y in range(8):
        for x in range(12):
            colors.extend([0, 0, 255, 255] if y < 4 else [255, 0, 255, 255 if 2 <= x < 10 else 0])
    (root / 'gauges.bin').write_bytes(jar([
        ('img.inf', b'1\tbar.png,bin'), ('seb.inf', b'0\tbar.seb'),
        ('bar.seb', seb([(0, 1, 0, 0, 12, 8, 0, 0, 0, 0)])), ('bar.png', png(12, 8, colors))]))
    return text + 'resource 23 "gauges.bin" "English.lproj" 0\nglobal_alias 23 23\nruntime 25 0 system 42\nenemy_gauges\n'


def make_native_entry_scene(root, alternate_quest=False, talk_events=False):
    """Table-selected native Stage.Start with explicit decoded saved inputs."""
    text=make_boss_scene(root,stage_entry=True,alternate_quest=alternate_quest,talk_events=talk_events)
    replacements={'geometry 312 312 12':'geometry quest','human_system 10 0 95 0 0':'human_system quest 0 95 0 0',
                  'enemy_system 10000 0 10 25 7':'enemy_system quest 10000','enemy_pool 0\n':'',
                  'stage_notifications 1 0 0 2 0 0 -1':'stage_notifications 1 0 0 quest 1 0 -1',
                  'stage_events fresh 0':'stage_events quest','stage_loop 5000 0 20 hold':'stage_loop quest 0 20 hold',
                  'stage_boss 4 7':'stage_boss quest','mission_map fresh 0 1 2 3':'mission_map quest'}
    for before,after in replacements.items():
        assert before in text,before
        text=text.replace(before,after)
    return text+('quest 2 0\nsystem_init 1\nstage_start 0 null\n'
                 'party_formation 2 3 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1\n'
                 'progress 3 1 0\nsorties 2 7\nsorties 3 9\n'
                 'result_wallet 10 20 0\nresult_record 0 0 0 0 0 0 0\n'
                 'result_inventory equipment 0 0 0\nresult_inventory cloth 0 0 0\n'
                 'result_inventory hat 0 0 0\nresult_recruits 0 0 0\n')


def make_data_entry_fixture(root, font_file):
    """Native bootstrap data and an equivalent diagnostic control; only synthetic/public bytes."""
    from pathlib import Path
    text=make_native_entry_scene(root,talk_events=True)
    assets=root/'textassets';assets.mkdir()
    for archive in root.glob('*.bin'):(assets/archive.name).write_bytes(archive.read_bytes())
    entries=[(-1,'tables','xls'),(1,'hero','chara'),(3,'enemy','enemy-book'),(7,'quest','quest'),
             (18,'map','chip'),(19,'bullet','bullet'),(20,'effect','effect'),(21,'hero','hat'),
             (22,'hero','weapon'),(23,'effect','gauge')]
    (root/'mission-resources.tsv').write_text('mission_resources 1\n'+''.join(
        f'{"tables" if i==-1 else "resource"} {i} "textassets/{file}.bin" "{name}"\n' for i,file,name in entries),encoding='utf8')
    (root/'new-game-save.tsv').write_text('new_game_save 1\nselection 5 2\ntown_capacity 1\nwallet 37 43\n',encoding='ascii')
    (root/'mission-initialization.tsv').write_text('mission_initialization 1\nyugami 10000\norientation 0\nevent_arguments ""\n',encoding='ascii')
    font=root/'unity/Font/different-number.ttf';font.parent.mkdir(parents=True);font.write_bytes(Path(font_file).read_bytes())
    (root/'mission-font.tsv').write_text(f'mission_font 1\nfont "unity/Font/{font.name}" {font.stat().st_size}\n',encoding='ascii')
    removed=('world ','projection ','slots ','view ','animate ','simulate ','gpu_depth ','material ',
             'tables ','resource ','resource_name ','global_alias ','progress ','purchase ','sorties ','human ','player ',
             'human_system ','rewards ','enemy_system ','enemy_population ','enemy_pool ','stage_notifications ',
             'collection ','stage_loop ','stage_events ','input ','stage_boss ','party_formation ',
             'quest ','stage_start ','result_wallet ','result_record ','result_inventory ','result_recruits ','gear_levels ')
    control='mission_presentation 600 640 100 0 1 0 1500100 -1\nmission_resources "English.lproj" 1\nmaterial_policy native\ngpu_depth 24 1\n'
    control+='\n'.join(line for line in text.splitlines() if not line.startswith(removed))+'\n'
    control+='hero fresh\ntick_ms 40\nsystem_construct empty_manager\nquest_frame 0 initial\ngear_new_game\ncharacter_shop_new_game 1\nnew_game_save\nmission_initialization\nmission_state new_game initial 0\nmission_events new_game 0 0 0\nenemy_gauges\ncollisions\n'
    control+=f'talk_font "{font.as_posix()}" 100 100 100 0 0 0 0 0\n'
    control=control.replace('system_init 1','system_init 0').replace('runtime 25 0 system 42','runtime initial initial system 42')
    return control
