#!/usr/bin/env python3
"""Audit shipped INF references; candidate matches are NOT runtime resolution.

Preserves row order, duplicates, missing IDs and options. Reads only manifest-
verified members; never rewrites extracted assets or infers locale precedence.
"""
import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path, PurePosixPath


def parse_catalog(blob):
    rows = []
    for line_number, line in enumerate(blob.decode('utf-8-sig').splitlines(), 1):
        if not line.strip():
            continue
        parts = line.split('\t')
        if len(parts) == 1:
            ident, value = None, parts[0].strip()
        elif len(parts) == 2:
            ident, value = int(parts[0].strip()), parts[1].strip()
        else:
            raise ValueError(f'unsupported catalog columns on line {line_number}')
        name, *options = value.split(',')
        if not name:
            raise ValueError(f'empty resource on line {line_number}')
        rows.append(dict(line=line_number, explicit_id=ident, name=name,
                         options=options))
    return rows


def audit(manifest_path):
    root = manifest_path.resolve().parent
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    index = {}
    for member in manifest['members']:
        key = member['container'], member['name']
        if key in index:
            raise ValueError('duplicate manifest identity')
        path = (root/member['output']).resolve()
        if not path.is_relative_to(root):
            raise ValueError('member escapes extraction root')
        blob = path.read_bytes()
        if len(blob) != member['size'] or hashlib.sha256(blob).hexdigest() != member['sha256']:
            raise ValueError(f'member identity mismatch: {member["output"]}')
        index[key] = member, blob
    catalogs = []
    counts = Counter()
    options = Counter()
    for (container, name), (member, blob) in sorted(index.items()):
        if PurePosixPath(name).name not in ('img.inf', 'seb.inf'):
            continue
        rows = parse_catalog(blob)
        ids = defaultdict(list)
        for row in rows:
            if row['explicit_id'] is not None:
                ids[row['explicit_id']].append(row['line'])
            options.update(row['options'])
            requested = PurePosixPath(row['name'])
            base = PurePosixPath(name).parent
            candidates = []
            # Enumerate evidence separately; never silently choose fallback order.
            lookups = [(str(base/requested), 'catalog_directory_exact'),
                       (str(requested), 'container_root_exact')]
            if PurePosixPath(name).name == 'img.inf':
                lookups += [(str(base/requested.with_suffix('.png')), 'catalog_directory_png_candidate'),
                            (str(requested.with_suffix('.png')), 'container_root_png_candidate')]
            for candidate, relation in lookups:
                found = index.get((container, candidate))
                if found and not any(c['name'] == candidate for c in candidates):
                    target = found[0]
                    candidates.append(dict(name=candidate, relation=relation,
                                           output=target['output'], sha256=target['sha256']))
            row['candidates'] = candidates
            counts['rows'] += 1
            counts['rows_without_explicit_id'] += row['explicit_id'] is None
            counts['rows_without_candidates'] += not candidates
            counts['rows_with_only_png_candidates'] += bool(candidates) and all('png_candidate' in c['relation'] for c in candidates)
        duplicates = {str(k): v for k, v in sorted(ids.items()) if len(v) > 1}
        counts['duplicate_id_groups'] += len(duplicates)
        counts['catalogs'] += 1
        counts['bom_catalogs'] += blob.startswith(b'\xef\xbb\xbf')
        catalogs.append(dict(container=container, name=name, sha256=member['sha256'],
                             duplicate_ids=duplicates, rows=rows))
    return dict(schema=1, apk_sha256=manifest['apk_sha256'],
                scope='static_candidates_not_runtime_resolution',
                counts=dict(sorted(counts.items())), options=dict(sorted(options.items())),
                catalogs=catalogs)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    report = audit(args.manifest)
    destination = args.out.resolve()
    extraction = args.manifest.resolve().parent
    if destination.is_relative_to(extraction):
        parser.error('report must be outside extracted source tree')
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8', newline='\n')
    print(json.dumps(report['counts'], sort_keys=True))
