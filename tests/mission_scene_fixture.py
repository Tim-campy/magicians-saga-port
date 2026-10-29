"""Synthetic table/archive profile shared by mission and GPU contracts."""
from test_sprite_preview import jar, png, seb

def make_table_scene(root, valid_png=True, human_frame=False, shoot=False, rewards=False, area=False, lethal_contact=False, treasure_flags=0, spell_attribute=3, boss_entry=False):
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


def make_shoot_scene(root, rewards=False, area=False, lethal_contact=False, treasure_flags=0, spell_attribute=3, boss_entry=False):
    text = make_table_scene(root, human_frame=True, shoot=True, rewards=rewards, area=area, lethal_contact=lethal_contact, treasure_flags=treasure_flags, spell_attribute=spell_attribute, boss_entry=boss_entry)
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


def make_boss_scene(root):
    """Explicit near-boss entry state; Stage3 creates both native enemies and the fence."""
    text = make_shoot_scene(root, rewards=True, treasure_flags=1, spell_attribute=0, boss_entry=True)
    text = '\n'.join(line for line in text.splitlines() if not line.startswith('enemy ')) + '\n'
    text = (text.replace('human 0 2 1 0 0 0 0 0 100', 'human 0 2 1 0 0 0 312 0 400')
            .replace('human_system 10 0 100 3 1', 'human_system 10 0 95 0 0'))
    text += ('enemy_system 10000 0 10 25 7\nenemy_population 0 0 1 0 0 100 0\nenemy_pool 0\n'
             'stage_notifications 1 0 0 2 0 0 -1\ncollection fresh\n'
             'stage_loop 5000 0 20 hold\nstage_events fresh 0\nstage_boss 4 7\n'
             'input fresh 0 0 0 0 0 0 0 0 0\n')
    return with_mission_map(root, text)
