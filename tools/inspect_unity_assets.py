#!/usr/bin/env python3
"""Catalogue Unity serialized resources inside a user APK; no media export.

Uses UnityPy 1.25.4, installed only under external/unitypy-libs, for object tables
and selected metadata fields. Reads split files in numeric order in memory.
No asset writes, decoder calls, runtime execution, or network fetching.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
import math
from pathlib import Path, PurePosixPath
import re
import struct
import sys
from urllib.parse import urlsplit
import zipfile

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'external/unitypy-libs'))
import UnityPy
from UnityPy.files import SerializedFile
from UnityPy.helpers import TypeTreeHelper

# Keep parsing exceptions in Python instead of relying on the C typetree reader.
TypeTreeHelper.read_typetree_boost = False
SPLIT = re.compile(r'^(.*)\.split(\d+)$')
URL = re.compile(rb'https?://[^\s\x00<>"\x27]+')


def hash_bytes(data):
    return hashlib.sha256(data).hexdigest()


def merge_parts(parts):
    ordered = sorted(parts)
    if [i for i, _ in ordered] != list(range(len(parts))):
        raise ValueError('Split sequence has gaps/duplicates')
    return b''.join(data for _, data in ordered)


def serialized_header(data):
    if len(data) < 48:
        return None
    _, legacy_size, version, legacy_data = struct.unpack_from('>4I', data)
    if version != 22 or data[16] not in (0, 1):
        return None
    metadata_size, size, data_offset, reserved = struct.unpack_from('>IQQQ', data, 20)
    if size != len(data) or not 48 <= data_offset <= size or metadata_size > size:
        raise ValueError('Invalid serialized-file v22 bounds')
    return {'format_version': version, 'declared_size': size,
            'metadata_size': metadata_size, 'data_offset': data_offset}


def domains(data):
    result = set()
    for match in URL.finditer(data):
        try:
            host = urlsplit(match.group().decode('ascii')).hostname
            if host:
                result.add(host.lower())
        except (UnicodeDecodeError, ValueError):
            pass
    return sorted(result)


def text_payload(obj):
    # TextAsset serialization: aligned UTF-8 name followed by length+payload.
    raw = obj.get_raw_data()
    endian = obj.assets_file.reader.endian
    n, = struct.unpack_from(endian+'i', raw)
    pos = (4+n+3) & ~3
    if n < 0 or pos+4 > len(raw):
        raise ValueError('Bad TextAsset name range')
    length, = struct.unpack_from(endian+'i', raw, pos)
    if length < 0 or pos+4+length > len(raw):
        raise ValueError('Bad TextAsset payload range')
    return raw[pos+4:pos+4+length]


def catalogue_links(files, objects, logical):
    """Resolve table references, without claiming runtime load/use."""
    file_index = {f['path']: f for f in files}
    object_index = {(o['file'], o['path_id']): o for o in objects}

    def resolve(owner, ref):
        path = owner
        if ref['m_FileID']:
            ext = file_index[owner]['externals'][ref['m_FileID']-1]['path']
            path = str(PurePosixPath(owner).parent / ext)
        return object_index.get((path, ref['m_PathID']))

    scene_scripts, streams = [], []
    for obj in objects:
        meta = obj.get('metadata', {})
        if obj['file'].endswith('/level0') and obj['type'] == 'MonoBehaviour':
            game = resolve(obj['file'], meta['m_GameObject'])
            script = resolve(obj['file'], meta['m_Script'])
            if game is None or script is None or script['type'] != 'MonoScript':
                raise ValueError('Unresolved level0 script reference')
            scene_scripts.append({'scene_file': obj['file'], 'component_path_id': obj['path_id'],
                                  'game_object_path_id': game['path_id'], 'game_object_name': game['name'],
                                  'script_file': script['file'], 'script_path_id': script['path_id'],
                                  'script': script['metadata']})
        if obj['type'] == 'AudioClip':
            stream = meta.get('m_Resource', {})
            source, offset, size = stream.get('m_Source', ''), stream.get('m_Offset', 0), stream.get('m_Size', 0)
        elif obj['type'] == 'Texture2D':
            stream = meta.get('m_StreamData', {})
            source, offset, size = stream.get('path', ''), stream.get('offset', 0), stream.get('size', 0)
        else:
            continue
        if source:
            path = str(PurePosixPath(obj['file']).parent / source)
            blob = logical.get(path)
            valid = blob is not None and offset >= 0 and size >= 0 and offset+size <= len(blob)
            streams.append({'object_file': obj['file'], 'path_id': obj['path_id'], 'type': obj['type'],
                            'name': obj['name'], 'source': source, 'resolved_path': path,
                            'offset': offset, 'size': size, 'range_present_in_apk': valid})
    return scene_scripts, streams


def inspect(apk):
    if UnityPy.__version__ != '1.25.4':
        raise ValueError('Expected UnityPy 1.25.4')
    with zipfile.ZipFile(apk) as z:
        infos = [i for i in z.infolist() if i.filename.startswith('assets/') and not i.is_dir()]
        if len({i.filename for i in infos}) != len(infos):
            raise ValueError('Duplicate asset paths')
        if sum(i.file_size for i in infos) > 256*1024*1024:
            raise ValueError('Asset read budget exceeded')
        raw_files = {i.filename: z.read(i) for i in infos}
    groups = defaultdict(list)
    logical = {}
    splits = []
    for name, data in raw_files.items():
        match = SPLIT.match(name)
        if match:
            groups[match[1]].append((int(match[2]), data, name))
        else:
            logical[name] = data
    for base, parts in sorted(groups.items()):
        if base in logical:
            raise ValueError('Split and unsplit names collide')
        merged = merge_parts([(i, d) for i, d, _ in parts])
        header = serialized_header(merged)
        if not header:
            raise ValueError('Merged split is not expected serialized v22')
        logical[base] = merged
        splits.append({'logical_path': base, 'size': len(merged), 'sha256': hash_bytes(merged),
                       'parts': [{'index': i, 'path': n, 'size': len(d), 'sha256': hash_bytes(d)}
                                 for i, d, n in sorted(parts)], 'header': header})
    files, objects, errors = [], [], []
    nonserialized = []
    for name, data in sorted(logical.items()):
        header = serialized_header(data)
        if not header:
            nonserialized.append({'path': name, 'size': len(data), 'sha256': hash_bytes(data),
                                  'prefix_hex': data[:16].hex(), 'url_domains': domains(data)})
            continue
        env = UnityPy.load(data)
        asset = next(iter(env.files.values()))
        if not isinstance(asset, SerializedFile):
            raise ValueError('Header/parser format disagreement')
        files.append({'path': name, 'sha256': hash_bytes(data), 'header': header,
                      'unity_version': asset.unity_version, 'object_count': len(asset.objects),
                      'externals': [{'file_id': i+1, 'path': e.path} for i, e in enumerate(asset.externals)]})
        for obj in sorted(asset.objects.values(), key=lambda o:o.path_id):
            if obj.byte_start < header['data_offset'] or obj.byte_size < 0 or obj.byte_start+obj.byte_size > len(data):
                raise ValueError('Object byte range outside serialized data')
            kind = obj.type.name
            item = {'file': name, 'path_id': obj.path_id, 'type': kind,
                    'byte_start': obj.byte_start, 'byte_size': obj.byte_size}
            try:
                item['name'] = obj.peek_name()
                if kind == 'TextAsset':
                    payload = text_payload(obj)
                    item['payload'] = {'size': len(payload), 'sha256': hash_bytes(payload),
                                       'prefix_hex': payload[:16].hex(), 'url_domains': domains(payload)}
                    item['payload']['byte_entropy_bits'] = round(-sum(
                        (count/len(payload))*math.log2(count/len(payload))
                        for count in Counter(payload).values()), 6) if payload else 0.0
                    try:
                        txt = payload.decode('utf-8')
                        item['payload']['utf8'] = True
                        if txt.lstrip().startswith(('{', '[')):
                            parsed = json.loads(txt)
                            item['payload']['json_root'] = type(parsed).__name__
                            if isinstance(parsed, dict):
                                item['payload']['json_keys'] = sorted(parsed)
                    except (UnicodeDecodeError, json.JSONDecodeError):
                        item['payload']['utf8_or_json_unparsed'] = True
                elif kind in ('Texture2D', 'AudioClip', 'Font', 'Sprite', 'MonoScript', 'BuildSettings',
                              'TimeManager', 'AudioManager', 'ResourceManager') or (
                                  name.endswith('/level0') and kind in ('GameObject', 'MonoBehaviour')):
                    partial = kind == 'MonoBehaviour'
                    tree = obj.parse_as_dict(check_read=not partial)
                    keys = {'Texture2D': ['m_Width','m_Height','m_TextureFormat','m_MipCount','m_StreamData'],
                            'AudioClip': ['m_Channels','m_Frequency','m_Length','m_CompressionFormat','m_Resource'],
                            'Font': ['m_FontSize','m_Ascent','m_FontNames'],
                            'Sprite': ['m_Rect','m_PixelsToUnits','m_RenderDataKey'],
                            'MonoScript': ['m_ClassName','m_Namespace','m_AssemblyName','m_ExecutionOrder'],
                            'BuildSettings': ['scenes','m_Version','m_GraphicsAPIs','preloadedPlugins'],
                            'TimeManager': ['Fixed Timestep','Maximum Allowed Timestep','m_TimeScale','Maximum Particle Timestep'],
                            'AudioManager': ['m_Volume','m_SampleRate','m_RealVoiceCount','m_VirtualVoiceCount'],
                            'GameObject': ['m_Component','m_IsActive','m_Layer'],
                            'MonoBehaviour': ['m_GameObject','m_Enabled','m_Script'],
                            'ResourceManager': ['m_Container']}[kind]
                    item['metadata'] = {k: tree[k] for k in keys if k in tree}
                    if partial:
                        item['metadata_scope'] = 'Unity MonoBehaviour header only; custom fields not decoded'
                    if kind == 'Font' and 'm_FontData' in tree:
                        item['font_data_size'] = len(tree['m_FontData'])
            except Exception as exc:
                # Preserve table counts while exposing unsupported content explicitly.
                item['parse_error'] = str(exc)[:300]
                errors.append({'file': name, 'path_id': obj.path_id, 'type': kind, 'error': str(exc)[:300]})
            objects.append(item)
    with apk.open('rb') as f:
        apk_hash = hashlib.file_digest(f, 'sha256').hexdigest()
    scene_scripts, streams = catalogue_links(files, objects, logical)
    return {'apk_sha256': apk_hash, 'unitypy_version': UnityPy.__version__,
            'raw_asset_file_count': len(raw_files), 'logical_file_count': len(logical),
            'serialized_file_count': len(files), 'object_count': len(objects),
            'type_counts': dict(sorted(Counter(o['type'] for o in objects).items())),
            'split_merges': splits, 'files': files, 'objects': objects,
            'scene_script_links': scene_scripts, 'stream_references': streams,
            'nonserialized_files': nonserialized, 'parse_errors': errors,
            'limitations': ['Object metadata only; no media decoded/exported or runtime observed.',
                            'URL domains in TextAssets/opaque files are static strings, not observed requests.',
                            'MonoBehaviour custom fields and asset availability during gameplay are not established.',
                            'Absence of an external resource/download string does not establish offline completeness.']}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('apk', type=Path)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    if args.apk.resolve() == args.output.resolve():
        ap.error('Output must not overwrite APK')
    result = inspect(args.apk)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False, sort_keys=True)+'\n', encoding='utf-8')
    print(json.dumps({k: result[k] for k in ['serialized_file_count','object_count','type_counts']}, indent=2))
    print('parse_errors:', len(result['parse_errors']))


if __name__ == '__main__':
    main()
