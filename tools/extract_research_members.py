"""Extract only explicitly named APK research members; never executes them."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('apk', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--member', action='append', required=True)
    args = parser.parse_args()
    root = args.output.resolve()
    records = []
    with args.apk.open('rb') as source:
        apk_hash = hashlib.file_digest(source, 'sha256').hexdigest()
    with zipfile.ZipFile(args.apk) as apk:
        planned = []
        for name in args.member:
            parts = PurePosixPath(name)
            if parts.is_absolute() or '..' in parts.parts or '\\' in name or ':' in name:
                raise ValueError('Unsafe member path')
            target = root.joinpath(*parts.parts).resolve()
            if not target.is_relative_to(root) or target == args.apk.resolve():
                raise ValueError('Output outside research directory or overwrites APK')
            if sum(i.filename == name for i in apk.infolist()) != 1:
                raise ValueError('Missing or ambiguous ZIP member')
            info = apk.getinfo(name)
            if info.is_dir() or info.file_size > 256 * 1024 * 1024:
                raise ValueError('Unsupported member size/kind')
            planned.append((name, target))
        for name, target in planned:
            data = apk.read(name)
            if target.exists() and target.read_bytes() != data:
                raise ValueError('Refusing to replace different existing output')
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            records.append({'member': name, 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()})
    manifest = {'apk_sha256': apk_hash, 'members': records}
    root.mkdir(parents=True, exist_ok=True)
    (root / 'extraction-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf8')
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
