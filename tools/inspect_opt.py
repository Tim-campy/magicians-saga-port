#!/usr/bin/env python3
"""Read/rebuild the regular OPT grid layout proven by Image.LoadOptimize.

Supports the non-FF variant observed in all 286 supplied OPT files. The native
FF variant is rejected explicitly. Seven record shorts remain neutrally named;
no crop, atlas replacement, renderer, or inferred animation is performed.
"""
import argparse
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import struct


@dataclass(frozen=True)
class Opt:
    cell_width: int
    cell_height: int
    columns: int
    rows: int
    cells: tuple[tuple[tuple[int, ...], ...], ...]


def parse(data):
    if not data:
        raise ValueError('missing OPT header')
    if data[0] == 255:
        raise ValueError('unsupported FF OPT variant')
    if len(data) < 4:
        raise ValueError('truncated OPT header')
    width, height, columns, rows = struct.unpack_from('>BBbb', data)
    if columns < 0 or rows < 0:
        raise ValueError('negative OPT grid dimensions')
    pos = 4
    cells = []
    for _ in range(rows*columns):
        if pos >= len(data):
            raise ValueError(f'missing OPT cell count at {pos}')
        count = struct.unpack_from('b', data, pos)[0]
        pos += 1
        if count < 0:
            raise ValueError('negative OPT piece count')
        end = pos+14*count
        if end > len(data):
            raise ValueError(f'truncated OPT pieces at {pos}')
        cells.append(tuple(struct.iter_unpack('>7h', data[pos:end])))
        pos = end
    if pos != len(data):
        raise ValueError(f'unconsumed OPT bytes: {len(data)-pos}')
    return Opt(width, height, columns, rows, tuple(cells))


def encode(opt):
    if not 0 <= opt.cell_width < 255 or not 0 <= opt.cell_height <= 255:
        raise ValueError('invalid regular OPT cell extent')
    if not 0 <= opt.columns <= 127 or not 0 <= opt.rows <= 127:
        raise ValueError('invalid signed OPT grid count')
    if len(opt.cells) != opt.rows*opt.columns:
        raise ValueError('OPT cell count/grid mismatch')
    out = bytearray(struct.pack('>BBbb', opt.cell_width,opt.cell_height,opt.columns,opt.rows))
    for cell in opt.cells:
        if len(cell) > 127:
            raise ValueError('OPT piece count outside signed byte range')
        out.extend(struct.pack('b',len(cell)))
        for values in cell:
            if len(values) != 7:
                raise ValueError('OPT piece must contain seven shorts')
            try:
                out.extend(struct.pack('>7h',*values))
            except (struct.error,TypeError) as exc:
                raise ValueError('OPT short outside encoded range') from exc
    return bytes(out)


def inspect_manifest(path):
    root = path.parent.resolve()
    manifest = json.loads(path.read_text(encoding='utf-8'))
    members = {(o['container'],o['name']):o for o in manifest['members']}
    if len(members) != len(manifest['members']):
        raise ValueError('duplicate container/member identity')

    def member_bytes(item):
        source = (root/item['output']).resolve()
        if not source.is_relative_to(root) or source == root:
            raise ValueError('manifest path escapes root')
        data = source.read_bytes()
        if len(data) != item['size'] or hashlib.sha256(data).hexdigest() != item['sha256']:
            raise ValueError('manifest identity mismatch: '+item['output'])
        return data

    records, infos = [], []
    for item in manifest['members']:
        ext = Path(item['name']).suffix
        if ext not in ('.opt','.optinfo'):
            continue
        data = member_bytes(item)
        digest = hashlib.sha256(data).hexdigest()
        row = {'path':item['output'],'sha256':digest,'size':len(data)}
        if ext == '.optinfo':
            # Text catalogue only: no runtime meaning assigned to authoring fields.
            lines = data.decode('utf-8').splitlines()
            row.update({'line_count':len(lines),'format_tag':lines[0] if lines else '',
                        'configuration_name':lines[1] if len(lines)>1 else '',
                        'runtime_use':'unknown'})
            infos.append(row)
            continue
        opt = parse(data)
        if encode(opt) != data:
            raise ValueError('OPT reconstruction mismatch')
        row.update({'cell_width':opt.cell_width,'cell_height':opt.cell_height,
                    'columns':opt.columns,'rows':opt.rows,
                    'piece_counts':[len(c) for c in opt.cells], 'roundtrip':True})
        png = members.get((item['container'],item['name'][:-4]+'.png'))
        if png:
            blob = member_bytes(png)
            if len(blob)<33 or blob[:8]!=b'\x89PNG\r\n\x1a\n' or blob[12:16]!=b'IHDR':
                raise ValueError('same-name PNG lacks IHDR')
            width,height = struct.unpack_from('>II',blob,16)
            own = [v for cell in opt.cells for v in cell if v[0] == -1]
            inside = sum(0<=v[3] and 0<=v[4] and v[5]>=0 and v[6]>=0
                         and v[3]+v[5]<=width and v[4]+v[6]<=height for v in own)
            row['same_basename_png'] = {'path':png['output'],'sha256':png['sha256'],
                'width':width,'height':height,'self_image_pieces':len(own),
                'self_image_crops_inside':inside,
                'scope':'static basename pairing and bounds; runtime load not observed'}
        records.append(row)
    return {'apk_sha256':manifest['apk_sha256'],
            'manifest_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
            'scope':'regular OPT disk structure; no reconstructed pixels',
            'files':records,'optinfo_catalogue':infos}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('manifest',type=Path)
    ap.add_argument('--output',type=Path)
    args = ap.parse_args()
    report = inspect_manifest(args.manifest)
    if args.output:
        if args.output.resolve().is_relative_to(args.manifest.parent.resolve()):
            ap.error('report must remain outside extraction tree')
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n',encoding='utf-8')
    print('OPT structural roundtrips:',len(report['files']))
    print('OPTINFO text catalogues:',len(report['optinfo_catalogue']))


if __name__ == '__main__':
    main()
