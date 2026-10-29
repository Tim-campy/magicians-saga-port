"""Synthetic table/archive profile shared by mission and GPU contracts."""
from test_sprite_preview import jar, png, seb

def make_table_scene(root, valid_png=True, human_frame=False):
    params = '13' + '\t2\t100\t1000' * 13
    tables = {
        'Chara.txt': f'2\tHero\t0\tchara\t8\tchara\t1\t0\t0\t99\t0\t100\t{params}\t0\t0\t0\t-1\t0\t2',
        'Equipment.txt': f'1\tWeapon\t0\t2\t0\t0\tweapon\t12\t3\t0\t0\t0\t0\t10\t8\t80\t1\t20\t3\t0\t0\t{params}\tText\t0',
        'Cloth.txt': f'0\tCloth\t0\tcloth\t11\t0\t0\t0\t0\t{params}\tText\t0',
        'Hat.txt': f'0\tHat\t0\that\t9\t0\t0\t0\t0\t{params}\tText\t2',
        'MagicShop.txt': '\n'.join(f'{i}\tUpgrade\t0\t1\t2\tText\t0' for i in range(8)),
        'Enemy.txt': f'4\tEnemy\t0\t2\t10\t100\t0\t0\t0\tenemy-book\t7\t0\t0\t0\t{params}\t0',
        'Bullet.txt': '3\t0\t0\tbullet\t5\t6\teffect\t7\t0',
    }
    if human_frame:
        tables['Equipment.txt'] += '\n' + tables['Equipment.txt'].replace('1\tWeapon', '0\tShield', 1)
    (root / 'tables.bin').write_bytes(jar([('English.lproj/' + name, text.encode()) for name, text in tables.items()]))
    for name, image, clip, color in [('hero', 200, 36, [255, 0, 0, 255]), ('enemy', 7, 0, [0, 255, 255, 255])]:
        (root / f'{name}.bin').write_bytes(jar([
            ('img.inf', f'{image}\ta.png,bin'.encode()), ('seb.inf', ('\n'.join(f'{i}\ta.seb' for i in [0,8,12,24,28,36,40]) if human_frame and name == 'hero' else f'{clip}\ta.seb').encode()),
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
