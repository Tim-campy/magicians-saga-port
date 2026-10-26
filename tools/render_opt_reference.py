#!/usr/bin/env python3
"""Research-only OPT occupied-bounds visualization, not original image dimensions.

Copies RGBA samples without compositing and verifies each piece against the
packed source. Overlap, unsupported selectors and invalid crops are rejected.
Requires Pillow from the already installed local external/unitypy-libs directory.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'external/unitypy-libs'))
from PIL import Image, __version__ as PILLOW_VERSION
from inspect_opt import parse


def render_reference(opt, source, strict_grid=False):
    source = source.convert('RGBA')
    pieces = []
    for index,cell in enumerate(opt.cells):
        for selector,dx,dy,u,v,w,h in cell:
            if selector != -1:
                raise ValueError('external image selector unsupported')
            if min(u,v,w,h)<0 or u+w>source.width or v+h>source.height:
                raise ValueError('piece crop outside packed PNG')
            x=(index%opt.columns)*opt.cell_width+dx
            y=(index//opt.columns)*opt.cell_height+dy
            if w and h:
                pieces.append((x,y,w,h,u,v))
    grid=(opt.columns*opt.cell_width,opt.rows*opt.cell_height)
    outside=sum(x<0 or y<0 or x+w>grid[0] or y+h>grid[1] for x,y,w,h,_,_ in pieces)
    if strict_grid and outside:
        raise ValueError('piece exceeds header grid domain')
    for i,(x,y,w,h,_,_) in enumerate(pieces):
        for xx,yy,ww,hh,_,_ in pieces[i+1:]:
            if max(x,xx)<min(x+w,xx+ww) and max(y,yy)<min(y+h,yy+hh):
                raise ValueError('overlapping destinations: no blending rule inferred')
    report={'canvas_kind':'occupied_bounds_not_original_dimensions',
            'grid_domain':[0,0,*grid], 'pieces_outside_grid':outside,
            'piece_count':len(pieces),'overlap_pairs':0}
    if not pieces:
        report.update({'occupied_bounds':None,'assigned_pixels':0,'pixel_roundtrip':True})
        return None,report
    x0=min(p[0] for p in pieces);y0=min(p[1] for p in pieces)
    x1=max(p[0]+p[2] for p in pieces);y1=max(p[1]+p[3] for p in pieces)
    if (x1-x0)*(y1-y0)>64*1024*1024:
        raise ValueError('occupied canvas exceeds research memory budget')
    canvas=Image.new('RGBA',(x1-x0,y1-y0),(0,0,0,0))
    assigned=0
    for x,y,w,h,u,v in pieces:
        crop=source.crop((u,v,u+w,v+h))
        canvas.paste(crop,(x-x0,y-y0))  # No mask: exact RGBA copy, not alpha blend.
        assigned+=w*h
    for x,y,w,h,u,v in pieces:
        expected=source.crop((u,v,u+w,v+h)).tobytes()
        actual=canvas.crop((x-x0,y-y0,x-x0+w,y-y0+h)).tobytes()
        if actual!=expected:
            raise ValueError('piece pixel roundtrip failed')
    histogram=canvas.getchannel('A').histogram()
    nontransparent=sum(histogram[1:])
    report.update({'occupied_bounds':[x0,y0,x1,y1],
        'canvas_size':[canvas.width,canvas.height], 'assigned_pixels':assigned,
        'nontransparent_pixels':nontransparent,
        'transparent_assigned_pixels':assigned-nontransparent,
        'unassigned_padding_pixels':canvas.width*canvas.height-assigned,
        'padding_convention':'RGBA zero for research display only',
        'pixel_roundtrip':True,'rgba_sha256':hashlib.sha256(canvas.tobytes()).hexdigest()})
    return canvas,report


def run(manifest_path,output,select=(),strict_grid=False):
    root=manifest_path.parent.resolve()
    output=output.resolve()
    allowed=(ROOT/'research/exports').resolve()
    if not output.is_relative_to(allowed) or output==allowed:
        raise ValueError('output must be a subdirectory of research/exports')
    if output.is_relative_to(root) or root.is_relative_to(output):
        raise ValueError('output and extracted source trees must be separate')
    manifest=json.loads(manifest_path.read_text(encoding='utf-8'))
    members={(o['container'],o['name']):o for o in manifest['members']}
    if len(members)!=len(manifest['members']):raise ValueError('duplicate member identity')

    def verified(item):
        path=(root/item['output']).resolve()
        if not path.is_relative_to(root):raise ValueError('member escapes root')
        blob=path.read_bytes()
        if len(blob)!=item['size'] or hashlib.sha256(blob).hexdigest()!=item['sha256']:
            raise ValueError('member identity mismatch')
        return path,blob

    rows=[]
    for item in manifest['members']:
        if not item['name'].endswith('.opt') or (select and not any(s in item['output'] for s in select)):
            continue
        opt_path,blob=verified(item)
        png=members.get((item['container'],item['name'][:-4]+'.png'))
        if png is None:raise ValueError('no same-basename source PNG')
        png_path,_=verified(png)
        with Image.open(png_path) as source:
            canvas,row=render_reference(parse(blob),source,strict_grid)
        row.update({'opt_path':item['output'],'opt_sha256':item['sha256'],
                    'packed_png_path':png['output'],'packed_png_sha256':png['sha256']})
        if canvas is not None:
            relative=Path(item['output']).with_suffix('.occupied.png')
            target=(output/relative).resolve()
            if not target.is_relative_to(output):raise ValueError('output escapes directory')
            target.parent.mkdir(parents=True,exist_ok=True)
            canvas.save(target,format='PNG')
            with Image.open(target) as saved:
                if saved.convert('RGBA').tobytes()!=canvas.tobytes():
                    raise ValueError('saved PNG changed RGBA samples')
            row['output']=relative.as_posix()
            row['output_sha256']=hashlib.sha256(target.read_bytes()).hexdigest()
        rows.append(row)
    if not rows:raise ValueError('selection matched no OPT files')
    report={'apk_sha256':manifest['apk_sha256'],'pillow_version':PILLOW_VERSION,
            'strict_grid':strict_grid,'source':'user-supplied extraction; research output only',
            'limitations':['Occupied bounds are not original image dimensions.',
                           'No game renderer, alpha blending or animation timing reconstructed.',
                           'Same-basename source pairing is static; no runtime loading observed.'],
            'files':rows}
    output.mkdir(parents=True,exist_ok=True)
    (output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n',encoding='utf-8')
    return report


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('manifest',type=Path)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--select',action='append',default=[])
    ap.add_argument('--strict-grid',action='store_true')
    args=ap.parse_args()
    report=run(args.manifest,args.output,args.select,args.strict_grid)
    print('Occupied-bounds references:',len(report['files']))
    print('Files exceeding header grid:',sum(r['pieces_outside_grid']>0 for r in report['files']))


if __name__=='__main__':main()
