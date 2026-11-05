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
    quests[2][-2:] = ['1','1'] # Discover Quest2; native selection costs one stamina.
    tables['English.lproj/Quest.txt']=('\n'.join('\t'.join(quests[i]) for i in sorted(quests))+'\n').encode()
    tables['English.lproj/Area.txt']=b'0\tFirst\t0\t1\t2\t1\n1\tSecond\t0\t0\t0\n'

    # Stage event212 stays directly executable, with a false automatic term
    # (money<0) so the town selector cannot prematurely open its mission talk.
    events={int(line.split('\t',1)[0]):line for line in tables['English.lproj/Event.txt'].decode().splitlines()}
    events[212]='212\tStage talk\t0\t1\t3\t1\t0\t3\t1\t2\t8\t4'
    tables['English.lproj/Event.txt']=('\n'.join(events[i] for i in sorted(events))+'\n').encode()
    (assets/'tables.bin').write_bytes(jar(list(tables.items())))
    (root/'tables.bin').write_bytes((assets/'tables.bin').read_bytes())

    common=_merge_resources(mission['common'],dict(read((assets/'com.bin').read_bytes())))
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
    return control
