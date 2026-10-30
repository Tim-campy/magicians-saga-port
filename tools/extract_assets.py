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
--culture-lower includes the preceding culture outputs and nine invariant UTF-16
lowercase arrays from metadata default-data initializers.
--runtime-tables exports the 288-byte Hashtable prime initializer from metadata.
--collation exports seven version-3 resources from the mscorlib resource container.
--collation-indexers adds the seven interval configurations derived from metadata/native initializers.
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
# AppData RESOURCE names row1 (cctor 0x11183c0..0x1118630), indexed by GetResourceId 0x11155dc.
# Only resources reached by the mission host; the full application inventory stays local.
MISSION_RESOURCES = ((1, 'chara'), (3, 'enemy'), (7, 'quest'), (18, 'chip'),
                     (19, 'bullet'), (20, 'effect'), (21, 'hat'), (22, 'weapon'), (23, 'gauge'))


def new_game_save_payload(metadata):
    """AppData.cctor normal-save seeds, consumed by NewGame at ELF0x1101e30.

    Field handles at ELF0x272ff50/0x272fef0 resolve to metadata fields24359/24245.
    Reuse the public v31 field-default layout; export only the five reached values.
    This is decoded initialization data, not a SecureProperty save-file decoder.
    """
    if len(metadata) < 256 or struct.unpack_from('<II', metadata) != (0xfab11baf, 31):
        raise ValueError('Expected v31 metadata for new-game save defaults')
    fields, field_size, data, data_size = struct.unpack_from('<IIII', metadata, 64)
    if (fields < 256 or data < 256 or field_size % 12 or
            fields+field_size > len(metadata) or data+data_size > len(metadata) or
            max(fields, data) < min(fields+field_size, data+data_size)):
        raise ValueError('Invalid new-game default sections')
    defaults = {}
    for field, _, offset in struct.iter_unpack('<iii', metadata[fields:fields+field_size]):
        if field not in (24359, 24245):
            continue
        size = 116 if field == 24359 else 24
        if field in defaults or offset < 0 or offset+size > data_size:
            raise ValueError('Invalid or duplicate new-game initializer')
        defaults[field] = struct.unpack_from('<29i' if size == 116 else '<3q', metadata, data+offset)
    if len(defaults) != 2:
        raise ValueError('Missing new-game initializer')
    integers, longs = defaults[24359], defaults[24245]
    return (f'new_game_save\t1\nselection\t{integers[8]}\t{integers[9]}\n'
            f'town_capacity\t{integers[19]}\nwallet\t{longs[0]}\t{longs[1]}\n').encode('ascii')


def write_new_game_save(apk, root):
    """Derive the mission seed without re-extracting Unity media or changing the manifest."""
    manifest = json.loads((root / 'manifest.json').read_text(encoding='utf-8'))
    if manifest.get('schema') not in (2, 3) or manifest.get('apk_sha256') != APK_SHA:
        raise ValueError('New-game defaults require the supported extraction manifest')
    with apk.open('rb') as source:
        if hashlib.file_digest(source, 'sha256').hexdigest() != APK_SHA:
            raise ValueError('New-game defaults require the supported APK')
    with zipfile.ZipFile(apk) as archive:
        payload = new_game_save_payload(archive.read(METADATA))
    output = bounded(root, 'new-game-save.tsv')
    if output.exists() and output.read_bytes() != payload:
        raise ValueError('Refusing to replace differing new-game defaults')
    if not output.exists():
        output.write_bytes(payload)
    return output


def write_mission_index(root):
    """Publish a small native-host index from already extracted, hash-checked files."""
    manifest = json.loads((root / 'manifest.json').read_text(encoding='utf-8'))
    if manifest.get('schema') not in (2, 3) or manifest.get('apk_sha256') != APK_SHA:
        raise ValueError('Mission index requires the supported APK schema-2/3 manifest')
    rows = ['mission_resources\t1']
    for resource, name in ((-1, 'xls'), *MISSION_RESOURCES):
        entries = [entry for entry in manifest['files'] if entry['name'] == name]
        if len(entries) != 1:
            raise ValueError('Mission archive missing or ambiguous: ' + name)
        entry = entries[0];relative = entry['output']
        if Path(relative).parent != Path('textassets'):
            raise ValueError('Mission archives must be direct textassets children')
        path = bounded(root, relative)
        if hashlib.sha256(path.read_bytes()).hexdigest() != entry['sha256']:
            raise ValueError('Mission archive hash mismatch: ' + name)
        kind = 'tables' if resource == -1 else 'resource'
        rows.append(f'{kind}\t{resource}\t{json.dumps(relative, ensure_ascii=False)}\t{json.dumps(name)}')
    output = bounded(root, 'mission-resources.tsv');payload = ('\n'.join(rows) + '\n').encode('utf-8')
    if output.exists() and output.read_bytes() != payload:
        raise ValueError('Refusing to replace differing mission resource index')
    if not output.exists():
        output.write_bytes(payload)
    return output
KEY_SHA = 'cb79ca3dff35eca5c3307e5321dbf86d68aefb366d138b0b72ff2fb47173a24c'
METADATA = 'assets/bin/Data/Managed/Metadata/global-metadata.dat'
# TextInfoToLowerData .cctor 0x1e2460c; offsets within v31 default_data.
# Version-specific layout knowledge only; table contents come from the verified APK.
LOWER_LAYOUT = ((98432,1175),(6824,38),(105288,509),(106360,16),(31832,26),
                (43136,47),(62816,131),(29032,87),(47744,106))


COLLATION_MEMBER = 'assets/bin/Data/Managed/Resources/mscorlib.dll-resources.dat'
COLLATION_SIZES = dict(zip(('core', 'tailoring', 'cjkCHS', 'cjkCHT', 'cjkJA', 'cjkKO', 'cjkKOlv2'),
                           (118901, 6724, 55813, 44549, 44549, 44549, 22273)))


def collation_payloads(container):
    """Native 0x100e5d4 directory: int32 length/count, then size/name-length/name."""
    if len(container) < 8:
        raise ValueError('Truncated assembly resource header')
    directory, count = struct.unpack_from('<ii', container)
    body = directory + 4
    if body < 8 or body > len(container) or count != 7:
        raise ValueError('Unexpected assembly resource directory')
    cursor, outputs = 8, {}
    for _ in range(count):
        if cursor + 8 > directory + 4:
            raise ValueError('Truncated assembly resource entry')
        size, length = struct.unpack_from('<ii', container, cursor)
        cursor += 8
        if length < 0 or cursor + length > directory + 4 or size < 1 or body + size > len(container):
            raise ValueError('Assembly resource outside container')
        name = container[cursor:cursor + length].decode('ascii')
        cursor += length
        expected = {'collation.' + key + '.bin': value for key, value in COLLATION_SIZES.items()}
        if name not in expected or size != expected[name] or 'collation/' + name in outputs:
            raise ValueError('Unexpected collation resource inventory')
        payload = container[body:body + size]
        if payload[0] != 3:
            raise ValueError('Expected collation version 3')
        outputs['collation/' + name] = payload
        body += size
    if cursor != directory + 4 or body != len(container):
        raise ValueError('Assembly resource end mismatch')
    return outputs


def collation_indexer_payload(metadata, instructions):
    """MSCompatUnicodeTableUtil..cctor 0x1cd61e4; emit SCI1 plus seven indexers."""
    if len(metadata) < 256 or struct.unpack_from('<II', metadata) != (0xfab11baf, 31):
        raise ValueError('Expected v31 metadata for collation indexers')
    base, size = struct.unpack_from('<II', metadata, 72)
    if base < 256 or base+size > len(metadata) or len(instructions) != 8:
        raise ValueError('Invalid collation initializer extent')
    first, second = struct.unpack('<II', instructions)
    if first & 0xffe0001f != 0x52800009 or second & 0xffe0001f != 0x52800008:
        raise ValueError('Expected MOVZ W9/W8 collation range initializers')
    immediate_starts = struct.pack('<iii', 0, (first >> 5) & 65535, (second >> 5) & 65535)
    def values(offset, count):
        if offset+4*count > size:
            raise ValueError('Collation range initializer outside metadata')
        return metadata[base+offset:base+offset+4*count]
    layouts = [(None,6160,3,-1),(113376,105184,6,0),(113376,105184,6,0),
               (22232,90032,4,0),(22232,106664,4,0),(6808,24720,3,-1),(12096,105168,3,-1)]
    output = bytearray(b'SCI1'+struct.pack('<I', 7))
    for start,end,count,default in layouts:
        output += struct.pack('<iii',count,default,default)
        output += immediate_starts if start is None else values(start,count)
        output += values(end,count)
    return bytes(output)


def culture_lower_payloads(metadata):
    if len(metadata) < 256 or struct.unpack_from('<II', metadata) != (0xfab11baf,31):
        raise ValueError('Expected v31 metadata for lowercase tables')
    # Ninth header section: field/parameter default-value data, index 8.
    base, size = struct.unpack_from('<II', metadata, 8+8*8)
    if base < 256 or base+size > len(metadata):
        raise ValueError('Lowercase default-data section outside metadata')
    outputs = {}
    for index, (offset,count) in enumerate(LOWER_LAYOUT):
        length = 2*count
        if offset+length > size:
            raise ValueError('Lowercase initializer outside default-data section')
        outputs[f'culture/lower-{index}.bin'] = metadata[base+offset:base+offset+length]
    return outputs


def runtime_payloads(metadata):
    """HashHelpers .cctor 0x1e2fe44: 72 int32 entries at default_data +50056."""
    if len(metadata) < 256 or struct.unpack_from('<II', metadata) != (0xfab11baf,31):
        raise ValueError('Expected v31 metadata for runtime tables')
    base, size = struct.unpack_from('<II', metadata, 72)
    if base < 256 or base+size > len(metadata) or size < 50056+288:
        raise ValueError('Runtime initializer outside default-data section')
    return {'runtime/hashtable-primes.bin': metadata[base+50056:base+50056+288]}


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


def check(root, require_culture=False, require_names=False, require_neutral=False, require_lower=False, require_runtime=False, require_collation=False, require_indexers=False):
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
    lower = neutral | {f'culture/lower-{i}.bin' for i in range(9)}
    rows = manifest.get('culture_signs', [])
    inventory = {r['output'] for r in rows}
    if require_names and inventory not in (full, neutral, lower):
        raise ValueError('Culture name outputs are required; extract with --culture-names')
    if require_neutral and inventory not in (neutral, lower):
        raise ValueError('Neutral culture outputs are required; extract with --culture-neutral')
    if require_lower and inventory != lower:
        raise ValueError('Lowercase culture outputs are required; extract with --culture-lower')
    if 'culture_signs' in manifest and (inventory not in (basic, full, neutral, lower) or len(rows) != len(inventory)):
        raise ValueError('Unexpected culture sign inventory')
    runtime = manifest.get('runtime_tables', [])
    if require_runtime and not runtime:
        raise ValueError('Runtime tables are required; extract with --runtime-tables')
    if 'runtime_tables' in manifest and (len(runtime) != 1 or runtime[0]['output'] != 'runtime/hashtable-primes.bin' or runtime[0]['size'] != 288):
        raise ValueError('Unexpected runtime table inventory')
    collation = manifest.get('collation', [])
    expected_collation = {'collation/collation.' + key + '.bin': size for key, size in COLLATION_SIZES.items()}
    if require_collation and not collation:
        raise ValueError('Collation outputs are required; extract with --collation')
    if 'collation' in manifest and (len(collation) != 7 or {r['output']: r['size'] for r in collation} != expected_collation):
        raise ValueError('Unexpected collation inventory')
    indexers = manifest.get('collation_indexers', [])
    if require_indexers and not indexers:
        raise ValueError('Collation indexers are required; extract with --collation-indexers')
    if 'collation_indexers' in manifest and (len(indexers) != 1 or indexers[0]['output'] != 'collation/indexers.bin' or indexers[0]['size'] != 324):
        raise ValueError('Unexpected collation indexer inventory')
    for row in manifest['files'] + manifest['members'] + manifest.get('unity_media', []) + manifest.get('culture_signs', []) + runtime + collation + indexers:
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


def extract(apk, root, include_unity=False, include_culture=False, include_names=False, include_neutral=False, include_lower=False, include_runtime=False, include_collation=False, include_indexers=False):
    include_collation = include_collation or include_indexers
    include_neutral = include_neutral or include_lower
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
        collation = collation_payloads(archive.read(COLLATION_MEMBER)) if include_collation else {}
        metadata = archive.read(METADATA)
        indexers = {}
        if include_indexers:
            native = archive.read('lib/arm64-v8a/libil2cpp.so')
            # Fixed-profile file offsets for ELF VAs 0x1cd62d8 and 0x1cd630c.
            instructions = native[0x1cd22d8:0x1cd22dc]+native[0x1cd230c:0x1cd2310]
            indexers['collation/indexers.bin'] = collation_indexer_payload(metadata, instructions)
        runtime = runtime_payloads(metadata) if include_runtime else {}
        if include_lower:
            culture.update(culture_lower_payloads(metadata))
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
    if include_runtime:
        outputs.update(runtime)
        manifest['runtime_tables'] = [{'output': name, 'size': len(data), 'sha256': digest(data)} for name, data in runtime.items()]
    if include_collation:
        outputs.update(collation)
        manifest['collation'] = [{'output': name, 'size': len(data), 'sha256': digest(data)} for name, data in collation.items()]
    if include_indexers:
        outputs.update(indexers)
        manifest['collation_indexers'] = [{'output': name, 'size': len(data), 'sha256': digest(data)} for name, data in indexers.items()]
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
    p.add_argument('--mission-index', action='store_true', help='Index existing decoded mission archives for the native host; no APK re-extraction')
    p.add_argument('--new-game-save', action='store_true', help='Export the reached normal-save NewGame seeds from the supported APK metadata only')
    p.add_argument('--collation-indexers', action='store_true', help='Include native indexer configurations and collation resources')
    p.add_argument('--collation', action='store_true', help='Include seven original version-3 collation resources')
    p.add_argument('--runtime-tables', action='store_true', help='Include the 72-entry runtime Hashtable prime table')
    p.add_argument('--culture-lower', action='store_true', help='Include nine invariant lowercase tables and all culture outputs')
    p.add_argument('--culture-neutral', action='store_true', help='Also derive the neutral-name to LCID mapping from native code')
    p.add_argument('--culture-names', action='store_true', help='Include name index and culture records with native number signs')
    p.add_argument('--culture-signs', action='store_true', help='Also export native number-format records and bounded sign pool')
    p.add_argument('--unity-media', action='store_true', help='Also export Unity textures, fonts and FSB/WAV audio')
    args = p.parse_args()
    try:
        if args.new_game_save:
            if not args.apk or args.check or args.mission_index or any((args.unity_media, args.culture_signs, args.culture_names, args.culture_neutral, args.culture_lower, args.runtime_tables, args.collation, args.collation_indexers)):
                raise ValueError('--new-game-save requires APK and --out only')
            print('New-game defaults: ' + str(write_new_game_save(args.apk, args.out)))
        elif args.mission_index:
            if args.apk or args.check:
                raise ValueError('--mission-index cannot combine with APK or --check')
            print('Mission resource index: ' + str(write_mission_index(args.out)))
        elif args.check:
            if not check(args.out, args.culture_signs or args.culture_names or args.culture_neutral or args.culture_lower,
                         args.culture_names or args.culture_neutral or args.culture_lower,
                         args.culture_neutral or args.culture_lower, args.culture_lower, args.runtime_tables, args.collation or args.collation_indexers, args.collation_indexers):
                raise SystemExit(1)
            print('All manifest files verified (TextAssets, members and optional media/culture/runtime/collation data).')
        elif args.apk:
            extract(args.apk, args.out, args.unity_media, args.culture_signs, args.culture_names, args.culture_neutral, args.culture_lower, args.runtime_tables, args.collation, args.collation_indexers)
        else:
            p.error('Supply APK or --check')
    except (ValueError, OSError, KeyError, struct.error, zipfile.BadZipFile) as error:
        p.exit(1, str(error)+'\n')


if __name__ == '__main__':
    main()
