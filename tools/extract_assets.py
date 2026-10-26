# /// script
# requires-python = ">=3.12"
# dependencies = ["UnityPy==1.25.4", "tpk_ar==0.2.4", "fmod_toolkit==0.1.3"]
# ///
"""Extract TextAsset payloads from the exact user-supplied Magician's Saga 1.4.0 APK.

AppData.Init (ARM64 VA 0x1100298) initializes eleven 32-bit words and serializes
them little-endian into 44 key bytes. Their metadata initializer is at 4557992.
Encrypter.Encode (0x1bf3c68) XORs each byte with key[i % key_length]; Decode
delegates to it. Thirty payloads share an encrypted prefix and decode to an
outer container with big-endian a,b and len == a+b+4. The index stores count,
length-prefixed names, offset array, size array and byte flags. Data contains
length-prefixed members. All 1963 observed flags are zero (uncompressed).
Eight other TextAssets are preserved byte-for-byte. No proprietary key is
embedded in this script: it is read from the hash-verified user's metadata.
--culture-signs also exports 339 native number-format records and the smallest
shared-pool prefix containing all referenced signs, validated within ELF load bounds.
--culture-names also includes the sorted name index/culture records and extends the
pool prefix through all lookup names; it does not extract other culture strings.
--culture-neutral also derives the 130 neutral-name/LCID leaves from bounded ARM64
code and relocated metadata literals; the mapping is local data, not source code.
Outputs and manifest stay local under extracted/; --check verifies all payloads and
members. Names are retained in the manifest, output names have numeric prefixes
to avoid Windows case collisions. No game content is embedded in the script.
"""
import argparse
from collections import defaultdict
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import sys
import zipfile
import wave

APK_SHA = 'd58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead'
KEY_SHA = 'cb79ca3dff35eca5c3307e5321dbf86d68aefb366d138b0b72ff2fb47173a24c'
METADATA = 'assets/bin/Data/Managed/Metadata/global-metadata.dat'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def xor(data, key):
    if not key:
        raise ValueError('Empty key')
    return bytes(v ^ key[i % len(key)] for i, v in enumerate(data))


def bounded(root, relative):
    path = (root / relative).resolve()
    if not path.is_relative_to(root.resolve()) or path == root.resolve():
        raise ValueError('Output escapes destination')
    return path


def unpack_container(data):
    """Parse the layout observed in JarInflater._load, VA 0x1bfb5d0."""
    a, b, count = struct.unpack_from('>III', data)
    if a+b+4 != len(data) or count > 100000:
        raise ValueError('Invalid container header')
    pos = 12
    names = []
    for _ in range(count):
        size, = struct.unpack_from('>I', data, pos)
        pos += 4
        if pos+size > a+4:
            raise ValueError('Name exceeds index')
        names.append(data[pos:pos+size])
        pos += size
    offsets = struct.unpack_from(f'>{count}I', data, pos); pos += 4*count
    sizes = struct.unpack_from(f'>{count}I', data, pos); pos += 4*count
    flags = data[pos:pos+count]; pos += count
    if pos != a+4 or any(flags):
        raise ValueError('Unsupported index or compressed member')
    members, chunks = [], []
    cursor = 0
    for name, offset, size in zip(names, offsets, sizes):
        if offset != cursor or pos+offset+4+size > len(data):
            raise ValueError('Noncontiguous/out-of-range member')
        stored_size, = struct.unpack_from('>I', data, pos+offset)
        if stored_size != size:
            raise ValueError('Member size mismatch')
        payload = data[pos+offset+4:pos+offset+4+size]
        members.append((name.decode('utf8'), payload))
        chunks.append(struct.pack('>I', size)+payload)
        cursor += 4+size
    if cursor != b:
        raise ValueError('Unconsumed container bytes')
    rebuilt = (struct.pack('>III', a, b, count) +
               b''.join(struct.pack('>I', len(n))+n for n in names) +
               struct.pack(f'>{count}I', *offsets) +
               struct.pack(f'>{count}I', *sizes) + flags + b''.join(chunks))
    if rebuilt != data:
        raise ValueError('Container round trip differs')
    return members


def decode_neutral_cases(code, base, literal, equality, join, failure):
    """Decode the audited ARM64 equality leaves, not a general disassembler."""
    if len(code) % 4:
        raise ValueError('Unaligned neutral dispatch')
    words = list(struct.unpack('<'+'I'*(len(code)//4), code))
    def signed(value, bits):
        return value-(1 << bits) if value & (1 << (bits-1)) else value
    def branch(word, pc):
        return pc+4*signed(word & 0x3ffffff, 26)
    result = []
    for i, word in enumerate(words):
        pc = base+4*i
        if word >> 26 != 0x25 or branch(word, pc) != equality:
            continue
        if i < 5 or i+2 >= len(words):
            raise ValueError('Truncated neutral equality leaf')
        page, load, arg0, arg2, arg1 = words[i-5:i]
        guard, move = words[i+1:i+3]
        if (page & 0x9f00001f != 0x90000008 or load & 0xffc003ff != 0xf9400108
                or (arg0, arg2, arg1) != (0xaa1403e0, 0xaa1f03e2, 0xf9400101)
                or guard & 0xfff8001f != 0x36000000 or move & 0xffe0001f != 0x52800013):
            raise ValueError('Unsupported neutral equality leaf')
        if pc+4+4*signed((guard >> 5) & 0x3fff, 14) != failure:
            raise ValueError('Unexpected neutral failure branch')
        after = pc+12
        if after != join and (i+3 >= len(words) or words[i+3] >> 26 != 5 or branch(words[i+3], after) != join):
            raise ValueError('Unexpected neutral constructor branch')
        immediate = ((page >> 29) & 3) | (((page >> 5) & 0x7ffff) << 2)
        got = ((pc-20) & ~0xfff)+4096*signed(immediate, 21)+8*((load >> 10) & 0xfff)
        name = literal(got)
        if any(row['name'] == name for row in result):
            raise ValueError('Duplicate neutral culture key')
        result.append({'name': name, 'lcid': (move >> 5) & 0xffff})
    return result


def neutral_culture_payload(elf, metadata):
    """Derive local neutral-name/LCID data from the hash-verified shipped artifact."""
    def unpack(fmt, offset):
        if offset < 0 or offset+struct.calcsize(fmt) > len(elf):
            raise ValueError('Neutral ELF read outside file')
        return struct.unpack_from(fmt, elf, offset)
    header = unpack('<16sHHIQQQIHHHHHH', 0)
    if header[0][:7] != b'\x7fELF\x02\x01\x01' or header[1:3] != (3,183) or header[9] != 56 or header[11] != 64:
        raise ValueError('Unsupported neutral ELF layout')
    segments = [unpack('<IIQQQQQQ', header[5]+56*i) for i in range(header[10])]
    def read(va, size):
        matches = [p[2]+va-p[3] for p in segments if p[0] == 1 and p[3] <= va and va+size <= p[3]+p[5]]
        if len(matches) != 1 or matches[0]+size > len(elf):
            raise ValueError('Neutral address not uniquely file-backed')
        return elf[matches[0]:matches[0]+size]
    relocations = {}
    for i in range(header[12]):
        section = unpack('<IIQQQQIIQQ', header[6]+64*i)
        if section[1] != 4:
            continue
        if section[9] != 24 or section[5] % 24:
            raise ValueError('Invalid neutral RELA layout')
        for pos in range(section[4], section[4]+section[5], 24):
            target, info, addend = unpack('<QQq', pos)
            if info == 1027:
                if target in relocations:
                    raise ValueError('Duplicate relative relocation')
                relocations[target] = addend
    if len(metadata) < 24 or struct.unpack_from('<II', metadata) != (0xfab11baf,31):
        raise ValueError('Expected v31 metadata for neutral literals')
    table, table_size, data, data_size = struct.unpack_from('<IIII', metadata, 8)
    if table_size % 8 or table+table_size > len(metadata) or data+data_size > len(metadata):
        raise ValueError('Neutral literal sections outside metadata')
    def literal(got):
        if got not in relocations:
            raise ValueError('Neutral literal GOT lacks relative relocation')
        usage, = struct.unpack('<Q', read(relocations[got],8))
        if usage >> 29 != 5:
            raise ValueError('Expected string-literal metadata usage')
        index = (usage & 0x1fffffff) >> 1
        if index >= table_size//8:
            raise ValueError('Neutral literal index outside table')
        length, offset = struct.unpack_from('<II', metadata, table+8*index)
        if offset+length > data_size:
            raise ValueError('Neutral literal outside data section')
        return metadata[data+offset:data+offset+length].decode('utf8')
    entries = decode_neutral_cases(read(0x1e27fc4,0x1e2a70c-0x1e27fc4), 0x1e27fc4,
                                   literal, 0x1ce2124, 0x1e2a680, 0x1e2a6b4)
    if len(entries) != 130:
        raise ValueError('Unexpected neutral mapping coverage')
    return (json.dumps({'schema':1, 'entries':entries},ensure_ascii=True,indent=2)+'\n').encode('utf8')


def culture_sign_payloads(elf, include_names=False):
    """Copy 339 reached 48-byte records and the pool prefix needed by their signs.

    Layout from native 0x10367cc/GetNFIValues. Preserve offsets and UTF-8 bytes;
    pool allocation size is unknown, so scan only within its file-backed segment.
    """
    if len(elf) < 64 or elf[:6] != b'\x7fELF\x02\x01' or struct.unpack_from('<H', elf, 18)[0] != 183:
        raise ValueError('Expected little-endian ARM64 ELF')
    phoff, = struct.unpack_from('<Q', elf, 32)
    entsize, count = struct.unpack_from('<HH', elf, 54)
    if entsize != 56 or phoff+count*entsize > len(elf):
        raise ValueError('Invalid ELF program headers')
    segments = []
    for i in range(count):
        kind, _, offset, va, _, size, _, _ = struct.unpack_from('<IIQQQQQQ', elf, phoff+i*entsize)
        if kind == 1:
            if offset+size > len(elf):
                raise ValueError('Truncated ELF load segment')
            segments.append((va, offset, size))
    def tail(va):
        matches = [(offset+va-base, offset+size) for base, offset, size in segments if base <= va < base+size]
        if len(matches) != 1:
            raise ValueError('Culture address is not uniquely file-backed')
        begin, end = matches[0]
        return elf[begin:end]
    records = tail(0x897cfc)[:339*48]
    if len(records) != 339*48:
        raise ValueError('Truncated culture number records')
    pool = tail(0x886828)
    extent = 0
    for i in range(339):
        for field in (0x14, 0x16):
            offset, = struct.unpack_from('<H', records, i*48+field)
            end = pool.find(b'\0', offset)
            if end < 0:
                raise ValueError('Culture sign is outside the pool or unterminated')
            pool[offset:end].decode('utf8', errors='strict')
            extent = max(extent, end+1)
    outputs = {'culture/number-records.bin': records}
    if include_names:
        names, cultures = tail(0x8818b4)[:339*4], tail(0x881e00)[:339*56]
        if len(names) != 339*4 or len(cultures) != 339*56:
            raise ValueError('Truncated culture name/record tables')
        previous = None
        for i in range(339):
            offset, index = struct.unpack_from('<Hh', names, i*4)
            if not 0 <= index < 339:
                raise ValueError('Invalid culture record index')
            number, = struct.unpack_from('<h', cultures, index*56+0x20)
            if not 0 <= number < 339:
                raise ValueError('Invalid culture number index')
            end = pool.find(b'\0', offset)
            if end < 0:
                raise ValueError('Unterminated culture name')
            key = pool[offset:end]
            key.decode('utf8', errors='strict')
            if previous is not None and previous >= key:
                raise ValueError('Culture name table must be strictly sorted and unique')
            previous = key
            extent = max(extent, end+1)
        outputs.update({'culture/name-index.bin': names, 'culture/culture-records.bin': cultures})
    outputs['culture/sign-pool.bin'] = pool[:extent]
    return outputs


def check(root, require_culture=False, require_names=False, require_neutral=False):
    manifest = json.loads((root / 'manifest.json').read_text(encoding='utf8'))
    if (manifest['apk_sha256'] != APK_SHA or len(manifest['files']) != 38
            or len(manifest['members']) != 1963):
        raise ValueError('Unexpected manifest version/count')
    failed = False
    if 'unity_media' in manifest and (len(manifest['unity_media']) != 187 or len(manifest['empty_textures']) != 5):
        raise ValueError('Unexpected Unity media inventory')
    if require_culture and 'culture_signs' not in manifest:
        raise ValueError('Culture sign outputs are required; extract with --culture-signs')
    basic = {'culture/number-records.bin', 'culture/sign-pool.bin'}
    full = basic | {'culture/name-index.bin', 'culture/culture-records.bin'}
    neutral = full | {'culture/neutral-map.json'}
    rows = manifest.get('culture_signs', [])
    inventory = {r['output'] for r in rows}
    if require_names and inventory not in (full, neutral):
        raise ValueError('Culture name outputs are required; extract with --culture-names')
    if require_neutral and inventory != neutral:
        raise ValueError('Neutral culture outputs are required; extract with --culture-neutral')
    if 'culture_signs' in manifest and (inventory not in (basic, full, neutral) or len(rows) != len(inventory)):
        raise ValueError('Unexpected culture sign inventory')
    for row in manifest['files'] + manifest['members'] + manifest.get('unity_media', []) + manifest.get('culture_signs', []):
        path = bounded(root, row['output'])
        if not path.is_file():
            print('missing:', row['output']); failed = True
        elif path.stat().st_size != row['size'] or digest(path.read_bytes()) != row['sha256']:
            print('modified:', row['output']); failed = True
    return not failed


def unity_media(obj, files, filename):
    """Return local outputs plus facts; preserve originals alongside conversions."""
    from PIL import Image, ImageFont
    tree = obj.read_typetree()
    name = tree['m_Name']
    facts = {'source': filename, 'path_id': obj.path_id, 'name': name, 'type': obj.type.name}
    if obj.type.name == 'Texture2D':
        tex = obj.read()
        facts.update(width=tex.m_Width, height=tex.m_Height, format=tex.m_TextureFormat,
                     mip_count=tex.m_MipCount)
        if not tex.image_data and (not tex.m_StreamData or not tex.m_StreamData.size):
            if tex.m_Width or tex.m_Height:
                raise ValueError('Nonempty texture lacks image data')
            return [], facts
        if not tex.image_data:
            raise ValueError('Streamed texture not supported by this version profile')
        image = tex.image.convert('RGBA')
        buffer = io.BytesIO(); image.save(buffer, format='PNG')
        encoded = buffer.getvalue()
        with Image.open(io.BytesIO(encoded)) as restored:
            if restored.convert('RGBA').tobytes() != image.tobytes():
                raise ValueError('PNG pixel round trip differs')
        facts['conversion'] = 'UnityPy base mip to RGBA PNG; original encoded mip data preserved'
        return [('.png', encoded), ('.texturedata.bin', bytes(tex.image_data))], facts
    if obj.type.name == 'Font':
        content = bytes(tree['m_FontData'])
        if content[:4] not in (b'\0\1\0\0', b'OTTO'):
            raise ValueError('Unsupported font header')
        font = ImageFont.truetype(io.BytesIO(content), 20)
        facts['freetype_name'] = list(font.getname())
        facts['conversion'] = 'none; original font including any checksum anomalies preserved'
        return [('.otf' if content[:4] == b'OTTO' else '.ttf', content)], facts
    import fmod_toolkit
    if fmod_toolkit.__version__ != '0.1.3':
        raise ValueError('Requires fmod_toolkit 0.1.3')
    resource = tree['m_Resource']
    source = str(Path(filename).parent / resource['m_Source']).replace('\\', '/')
    if source not in files:
        raise ValueError('Audio stream absent from APK: '+source)
    start, size = resource['m_Offset'], resource['m_Size']
    if start < 0 or size < 60 or start+size > len(files[source]):
        raise ValueError('Invalid audio stream range')
    content = files[source][start:start+size]
    version, count, headers, names, body, mode = struct.unpack_from('<6I', content, 4)
    if content[:4] != b'FSB5' or version != 1 or count != 1 or 60+headers+names+body != size:
        raise ValueError('Unexpected FSB5 structure')
    packed, = struct.unpack_from('<Q', content, 60)
    expected_frames = packed >> 34
    decoded = fmod_toolkit.raw_to_wav(content, name, tree['m_Channels'], tree['m_Frequency'],
                                    convert_pcm_float=False)
    if len(decoded) != 1:
        raise ValueError('Unexpected audio sample count')
    wav = next(iter(decoded.values()))
    with wave.open(io.BytesIO(wav)) as stream:
        channels, rate, frames = stream.getnchannels(), stream.getframerate(), stream.getnframes()
        width = stream.getsampwidth(); pcm = stream.readframes(frames)
    if ((channels, rate, frames) != (tree['m_Channels'], tree['m_Frequency'], expected_frames)
            or len(pcm) != channels*width*frames):
        raise ValueError('WAV/FSB metadata mismatch')
    facts.update(stream=source, stream_offset=start, stream_size=size, fsb_mode=mode,
                 frames=frames, channels=channels, rate=rate, sample_width=width,
                 pcm_sha256=digest(pcm), conversion='FMOD decode to PCM; original FSB retained for loop metadata')
    return [('.fsb', content), ('.wav', wav)], facts


def extract(apk, root, include_unity=False, include_culture=False, include_names=False, include_neutral=False):
    include_names = include_names or include_neutral
    include_culture = include_culture or include_names
    if digest(apk.read_bytes()) != APK_SHA:
        raise ValueError('Expected original Magician\'s Saga 1.4.0 APK, SHA-256 ' + APK_SHA)
    # Optional existing isolated installation; uv provides the same pinned dependency.
    local = Path(__file__).resolve().parents[1] / 'external/unitypy-libs'
    if local.is_dir():
        sys.path.insert(0, str(local))
    import UnityPy
    from UnityPy.helpers import TypeTreeHelper
    if UnityPy.__version__ != '1.25.4':
        raise ValueError('Requires UnityPy 1.25.4')
    TypeTreeHelper.read_typetree_boost = False
    with zipfile.ZipFile(apk) as archive:
        culture = culture_sign_payloads(archive.read('lib/arm64-v8a/libil2cpp.so'), include_names) if include_culture else {}
        metadata = archive.read(METADATA)
        if include_neutral:
            culture['culture/neutral-map.json'] = neutral_culture_payload(archive.read('lib/arm64-v8a/libil2cpp.so'), metadata)
        key = metadata[4557992:4558036]
        if len(key) != 44 or digest(key) != KEY_SHA:
            raise ValueError('Metadata key initializer identity mismatch')
        files, groups = {}, defaultdict(list)
        for name in archive.namelist():
            if not name.startswith('assets/bin/Data/') or name.endswith('/'):
                continue
            match = re.fullmatch(r'(.*)\.split(\d+)', name)
            if match:
                groups[match[1]].append((int(match[2]), archive.read(name)))
            else:
                files[name] = archive.read(name)
        for name, parts in groups.items():
            parts.sort()
            if name in files or [i for i, _ in parts] != list(range(len(parts))):
                raise ValueError('Invalid split sequence')
            files[name] = b''.join(data for _, data in parts)
    outputs, rows, member_rows, media_rows, empty = {}, [], [], [], []
    for filename, data in sorted(files.items()):
        if len(data) < 48 or struct.unpack_from('>I', data, 8)[0] != 22:
            continue
        if struct.unpack_from('>Q', data, 24)[0] != len(data):
            raise ValueError('Serialized file size mismatch')
        env = UnityPy.load(data)
        for obj in sorted(env.objects, key=lambda x: x.path_id):
            if include_unity and obj.type.name in ('Texture2D', 'AudioClip', 'Font'):
                media, facts = unity_media(obj, files, filename)
                if not media:
                    empty.append(facts)
                safe = re.sub(r'[^A-Za-z0-9_.-]', '_', facts['name'])[:80] or 'unnamed'
                for extension, content in media:
                    relative = f"unity/{obj.type.name}/{len(media_rows):03d}_{safe}{extension}"
                    outputs[relative] = content
                    media_rows.append({**facts, 'output': relative, 'size': len(content), 'sha256': digest(content)})
            if obj.type.name != 'TextAsset':
                continue
            raw = obj.get_raw_data()
            endian = obj.assets_file.reader.endian
            name_len, = struct.unpack_from(endian+'i', raw)
            if name_len < 0 or name_len > len(raw)-4:
                raise ValueError('Invalid name length')
            name = raw[4:4+name_len].decode('utf8')
            pos = (4+name_len+3) & ~3
            size, = struct.unpack_from(endian+'i', raw, pos)
            if size < 0 or pos+4+size > len(raw):
                raise ValueError('Invalid payload range')
            payload = raw[pos+4:pos+4+size]
            decoded = payload.startswith(bytes.fromhex('65b6'))
            result = xor(payload, key) if decoded else payload
            envelope = None
            if decoded:
                a, b = struct.unpack_from('>II', result)
                if a+b+4 != len(result) or a < 4 or xor(result, key) != payload:
                    raise ValueError('Envelope/round-trip mismatch: '+name)
                envelope = {'first_size': a, 'second_size': b}
            safe_name = re.sub(r'[^A-Za-z0-9_.-]', '_', name)[:80] or 'unnamed'
            output = f'textassets/{len(rows):02d}_{safe_name}.bin'
            outputs[output] = result
            if decoded:
                for member_index, (member_name, content) in enumerate(unpack_container(result)):
                    safe_member = re.sub(r'[^A-Za-z0-9_.-]', '_', member_name)[:140] or 'unnamed'
                    member_output = f'members/{len(rows):02d}_{safe_name}/{member_index:04d}_{safe_member}'
                    outputs[member_output] = content
                    member_rows.append({'container': output, 'name': member_name,
                                        'output': member_output, 'size': len(content),
                                        'sha256': digest(content)})
            rows.append({'source': filename, 'path_id': obj.path_id, 'name': name,
                         'input_sha256': digest(payload), 'output': output,
                         'size': len(result), 'sha256': digest(result),
                         'transform': 'xor44' if decoded else 'identity', 'envelope': envelope})
    if len(rows) != 38 or sum(r['transform'] == 'xor44' for r in rows) != 30:
        raise ValueError('Unexpected TextAsset inventory')
    if len(member_rows) != 1963:
        raise ValueError('Unexpected container member count')
    manifest = {'schema': 2, 'apk_sha256': APK_SHA, 'files': rows, 'members': member_rows,
                'limitations': ['Unity Texture2D/Font/AudioClip export not included.',
                                'Custom SEB/INF formats preserved; not interpreted.']}
    if include_unity:
        if len(media_rows) != 187 or len(empty) != 5:
            raise ValueError('Unexpected Unity media coverage')
        manifest.update(schema=3, unity_media=media_rows, empty_textures=empty,
                        limitations=['Texture PNGs contain base mip only; original encoded mip data retained.',
                                     'FSB loop metadata retained in source; runtime mixer not reconstructed.',
                                     'Font bytes preserved; shaping and glyph coverage not verified.',
                                     'SEB/INF semantics and presentation still under investigation.'])
    if include_culture:
        outputs.update(culture)
        manifest['culture_signs'] = [{'output': name, 'size': len(data), 'sha256': digest(data)} for name, data in culture.items()]
    outputs['manifest.json'] = (json.dumps(manifest, ensure_ascii=False, indent=2)+'\n').encode('utf8')
    # Preflight all collisions before writes. Never replace differing user files.
    for relative, data in outputs.items():
        path = bounded(root, relative)
        if path.exists() and (not path.is_file() or path.read_bytes() != data):
            raise ValueError('Refusing to replace differing output: '+str(path))
    for relative, data in outputs.items():
        path = bounded(root, relative)
        path.parent.mkdir(parents=True, exist_ok=True)
        if not path.exists():
            path.write_bytes(data)
    print('Extracted 38 TextAssets and 1963 members; 30 containers passed exact round trips.')
    if include_unity:
        print('Unity media: 68 PNGs + encoded textures, 23 FSB/WAV pairs, 5 fonts; 5 empty texture records.')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('apk', type=Path, nargs='?')
    p.add_argument('--out', type=Path, default=Path('extracted'))
    p.add_argument('--check', action='store_true')
    p.add_argument('--culture-neutral', action='store_true', help='Also derive the neutral-name to LCID mapping from native code')
    p.add_argument('--culture-names', action='store_true', help='Include name index and culture records with native number signs')
    p.add_argument('--culture-signs', action='store_true', help='Also export native number-format records and bounded sign pool')
    p.add_argument('--unity-media', action='store_true', help='Also export Unity textures, fonts and FSB/WAV audio')
    args = p.parse_args()
    try:
        if args.check:
            if not check(args.out, args.culture_signs or args.culture_names or args.culture_neutral, args.culture_names or args.culture_neutral, args.culture_neutral):
                raise SystemExit(1)
            print('All manifest files verified (TextAssets, members and optional media/culture data).')
        elif args.apk:
            extract(args.apk, args.out, args.unity_media, args.culture_signs, args.culture_names, args.culture_neutral)
        else:
            p.error('Supply APK or --check')
    except (ValueError, OSError, KeyError, struct.error, zipfile.BadZipFile) as error:
        p.exit(1, str(error)+'\n')


if __name__ == '__main__':
    main()
