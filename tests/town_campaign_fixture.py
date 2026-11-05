"""Compose the existing synthetic town and native mission data for S2 contracts.

Quest2 is the mission fixture's real synthetic combat quest; Area0 lists it.
The public font comes from SAGA_TEST_FONT or an explicit caller argument.
No APK, extracted game data or independent gameplay implementation is used.
"""
import importlib.util
import os
from pathlib import Path

from mission_scene_fixture import make_data_entry_fixture
from test_sprite_preview import jar
from town_fixture import add_town_background


def _container_reader():
    # Reuse the production container reader and its round-trip checks.
    path=Path(__file__).resolve().parents[1]/'tools'/'extract_assets.py'
    spec=importlib.util.spec_from_file_location('campaign_fixture_extractor',path)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module.unpack_container


def _merge_rows(preferred,additional):
    """Merge synthetic numeric-ID table/index rows, preserving preferred records."""
    rows={}
    for payload in (preferred,additional):
        for line in payload.decode('utf8').splitlines():
            if line:rows.setdefault(int(line.split('\t',1)[0]),line)
    return ('\n'.join(rows[key] for key in sorted(rows))+'\n').encode('utf8')


def _merge_resources(preferred,additional):
    result=dict(preferred)
    for name,payload in additional.items():
        if name in ('img.inf','seb.inf') and name in result:
            result[name]=_merge_rows(result[name],payload)
        elif name not in result:result[name]=payload
        elif result[name]!=payload:
            raise ValueError('Conflicting synthetic resource payload: '+name)
    return result


def create_campaign_fixture(root,font_file=None,quest_time_limit=None):
    """Write shared synthetic data; return the S1 diagnostic control scene text.

    Use a fresh directory. The town index supplies resources0/11/13; the mission
    index retains S1's supported resource set. Both point to the same tables and
    common2 archive. New-game selection is Area0/Quest-1 until menu confirmation;
    a standalone mission/control invocation must explicitly select Quest2.
    """
    root=Path(root);root.mkdir(parents=True,exist_ok=True)
    if (root/'textassets').exists():raise ValueError('campaign fixture requires a fresh data directory')
    font_file=font_file or os.environ.get('SAGA_TEST_FONT')
    if not font_file:raise ValueError('campaign fixture requires a public font or SAGA_TEST_FONT')
    read=_container_reader()
    control=make_data_entry_fixture(root,font_file,quest_time_limit=quest_time_limit)
    assets=root/'textassets'
    mission={name:dict(read((assets/(name+'.bin')).read_bytes()))
             for name in ('tables','common','effect','hero')}
    mission_index=(root/'mission-resources-v2.tsv').read_text()

    # This supplies the existing town layout, residents, intro, resource11/13 and
    # the twelve-field stage text projection, without copying their definitions.
    add_town_background(root,update=True)
    town_tables=dict(read((assets/'tables.bin').read_bytes()))
    tables=dict(mission['tables'])
    for name,payload in town_tables.items():
        if name.endswith(('/Chara.txt','/Equipment.txt','/Cloth.txt','/Hat.txt','/Talk.txt','/Event.txt','/Quest.txt')):
            tables[name]=_merge_rows(tables.get(name,b''),payload)
        else:tables[name]=payload

    # Keep mission Hero2/Sub3 and town residents0/1/4..9. The native total-rescue
    # predicate visits every quest slot, so provide Quest1 as an undiscovered
    # copy of the town's simple Quest0 rather than introducing a null gap.
    quests={int(line.split('\t',1)[0]):line.split('\t')
            for line in tables['English.lproj/Quest.txt'].decode().splitlines()}
    filler=quests[0].copy();filler[0]='1';filler[-2]='0';quests[1]=filler
    # Native best-damage scans every row and requires dense quest records.
    for missing in range(max(quests)+1):
        if missing not in quests:
            quests[missing]=list(quests[2]);quests[missing][0]=str(missing)
    quests[2][-2:] = ['1','1'] # Discover Quest2; native selection costs one stamina.
    tables['English.lproj/Quest.txt']=('\n'.join('\t'.join(quests[i]) for i in sorted(quests))+'\n').encode()
    tables['English.lproj/Area.txt']=b'0\tFirst\t0\t1\t2\t1\n1\tSecond\t0\t0\t0\n'

    tables['English.lproj/Voice.txt']+=b'\n2\t2\t1\tSynthetic royal greeting\n'

    # Stage event212 stays directly executable, with a false automatic term
    # (money<0) so the town selector cannot prematurely open its mission talk.
    events={int(line.split('\t',1)[0]):line for line in tables['English.lproj/Event.txt'].decode().splitlines()}
    events[212]='212\tStage talk\t0\t1\t3\t1\t0\t3\t1\t2\t8\t4'
    tables['English.lproj/Event.txt']=('\n'.join(events[i] for i in sorted(events))+'\n').encode()
    (assets/'tables.bin').write_bytes(jar(list(tables.items())))
    (root/'tables.bin').write_bytes((assets/'tables.bin').read_bytes())

    common=_merge_resources(mission['common'],dict(read((assets/'com.bin').read_bytes())))
    common['seb.inf']=_merge_rows(common['seb.inf'],b'73\ticon.seb\n') # Reached resident gift uses the existing synthetic icon image.
    effect=_merge_resources(mission['effect'],dict(read((assets/'effect.bin').read_bytes())))
    actor=mission['hero']
    for name in ('chara','hat','weapon'):
        actor=_merge_resources(actor,dict(read((assets/(name+'.bin')).read_bytes())))
    for name,members in (('common',common),('effect',effect),('hero',actor)):
        (assets/(name+'.bin')).write_bytes(jar(list(members.items())))
        (root/(name+'.bin')).write_bytes((assets/(name+'.bin')).read_bytes())

    # add_town_background appends its standalone actor index. Replace that index
    # with the complete S1 set; resource IDs1/21/22 share the merged hero archive.
    (root/'mission-resources-v2.tsv').write_text(mission_index)
    (root/'mission-resources.tsv').write_text(mission_index)
    town_index=(root/'town-resources-v2.tsv').read_text().replace('"textassets/com.bin"','"textassets/common.bin"')
    (root/'town-resources-v2.tsv').write_text(town_index)
    seed=root/'new-game-save.tsv';seed.write_text(seed.read_text(encoding='utf-8').replace('new_game_save 1','new_game_save 2')+'town_scale 100\n',encoding='ascii')
    (root/'town-messages.tsv').write_text('town_messages 1\ngift_title "Synthetic gift"\ngift_format "Gift <0>G"\n',encoding='utf-8')
    return control


def add_king_reward_case(root):
    """Two eligible rewards, one retained child and a numeric effect; synthetic only."""
    root=Path(root);archive=root/'textassets/tables.bin'
    tables=dict(_container_reader()(archive.read_bytes()))
    tables['English.lproj/KingReward.txt']=(
        b'0\tFirst\t10\t3\t0\t0\t0\n'
        b'1\tSecond\t20\t7\t123\t0\t2\t2\t8\t9\t2\t1\t11\n')
    tables['English.lproj/Talk.txt']+=b'\n9\t0\t1\tReward <0>/<1>\n'
    archive.write_bytes(jar(list(tables.items())))


def add_construction_case(root):
    """Five enabled plots and two construction stocks; synthetic textures/rates only."""
    from test_sprite_preview import seb,png
    root=Path(root);archive=root/'textassets/tables.bin';read=_container_reader()
    tables=dict(read(archive.read_bytes()))
    rows={int(l.split(b'\t',1)[0]):l for l in tables['English.lproj/Town.txt'].splitlines()}
    for index in (2,3,4):rows[index]=f'{index}\t-1\t0\t-1\t1'.encode()
    tables['English.lproj/Town.txt']=b'\n'.join(rows[i] for i in sorted(rows))+b'\n'
    tables['English.lproj/Building.txt']+=b'\n0\tWorkshop\tselect\t14\t-1\t1\t7\t2\tSynthetic\t1\n1\tHouse\tselect\t47\t-1\t6\t10\t3\tSynthetic\t1\n'
    archive.write_bytes(jar(list(tables.items())))
    common=root/'textassets/common.bin';members=dict(read(common.read_bytes()))
    members['img.inf']=_merge_rows(members['img.inf'],b'99\tbuild-window.png,bin\n')
    members['seb.inf']=_merge_rows(members['seb.inf'],b'68\tbuild-window.seb\n')
    members['build-window.png']=png(240,240,[20,24,36,255]*(240*240))
    members['build-window.seb']=seb([(0,99,0,0,240,240,0,0,0,0)])
    common.write_bytes(jar(list(members.items())))
    (root/'building-defaults.tsv').write_text('building_defaults 1\nresident_rates 9 125 250 375 500 625 750 875 1000 1125\n')


def add_magic_shop_case(root):
    """A real host shop route over synthetic catalog, rates and two-frame sprites."""
    from test_sprite_preview import seb,png
    root=Path(root);add_construction_case(root);read=_container_reader()
    archive=root/'textassets/tables.bin';tables=dict(read(archive.read_bytes()))
    towns=tables['English.lproj/Town.txt'].decode().splitlines()
    cells=towns[0].split('\t');cells[1]='0';towns[0]='\t'.join(cells)
    tables['English.lproj/Town.txt']=('\n'.join(towns)+'\n').encode()
    tables['English.lproj/MagicShop.txt']=('\n'.join(
        f'{i}\tUpgrade{i}\t{i}\t{10*(i+1)}\t{i+2}\tSynthetic\t{1 if i<2 else 0}' for i in range(8))+'\n').encode()
    levels=[l.split('\t') for l in tables['English.lproj/Table.txt'].decode().splitlines()]
    for row in levels:row[3:6]=[str(125*(int(row[0])+1))]*3
    tables['English.lproj/Table.txt']=('\n'.join('\t'.join(l) for l in levels)+'\n').encode()
    archive.write_bytes(jar(list(tables.items())))
    common=root/'textassets/common.bin';members=dict(read(common.read_bytes()))
    members['img.inf']=_merge_rows(members['img.inf'],b'100\tbuy.png,bin\n')
    members['seb.inf']=_merge_rows(members['seb.inf'],b'70\tbuy.seb\n')
    members['buy.png']=png(116,26,[40,160,80,255]*(116*26))
    members['buy.seb']=seb([(0,100,0,0,116,26,-58,0,0,0),(1,100,0,0,116,26,-58,0,0,0)])
    common.write_bytes(jar(list(members.items())))
    shop={'img.inf':b'0\tshop.png,bin\n','seb.inf':b'1\tbackground.seb\n12\trow.seb\n',
          'shop.png':png(240,240,[24,32,64,255]*(240*240)),
          'background.seb':seb([(0,0,0,0,240,240,-120,-120,0,0),(1,0,0,0,240,240,-120,-120,0,0)]),
          'row.seb':seb([(0,0,0,0,120,32,0,0,0,0),(1,0,120,0,120,32,0,0,0,0)])}
    (root/'textassets/shop.bin').write_bytes(jar(list(shop.items())))
    index=root/'town-resources-v2.tsv';index.write_text(index.read_text()+'resource\t14\t"textassets/shop.bin"\t"shop"\n')
