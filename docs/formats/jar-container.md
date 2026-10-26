# JarInflater container (obfuscated TextAsset)

30 of the 38 Unity TextAssets are XOR-obfuscated archives read by `kairo.unity.util.JarInflater._load` (kairo:0x060003c6, VA 0x1bfb5d0). After the XOR layer they are a flat, name-indexed, uncompressed archive. All 30 decode, parse and re-serialize byte-exactly (1,963 members); a C++ reader exists.

## Layout

**Obfuscation layer.** Payload bytes are XORed in place with a 44-byte key, periodic: `out[i] = in[i] ^ key[i % 44]` (`Encrypter.Encode`, kairo:0x06000316, reached from `AssetReader.GetData` -> `Encrypter.Decode` 0x06000317 -> 0x06000318). The key is eleven 32-bit ints from a metadata array initializer (field 24349, metadata offset 4557992), serialized little-endian by `main.AppData.Init` and passed to `Encrypter.SetEncryptionKey`. Key bytes are not reproduced here; the extractor derives them from the user's hash-verified `global-metadata.dat`. Encrypted payloads share the prefix `65 b6`; the prefix is an artefact of XOR over a container header, not a magic number. XOR is its own inverse (Decode delegates to Encode).

**Container** (all integers unsigned **big-endian**):

| Offset | Size | Type | Meaning |
|---|---|---|---|
| 0 | 4 | u32 | A = index size; index ends at A+4 |
| 4 | 4 | u32 | B = data section size |
| 8 | 4 | u32 | N = member count |
| 12 | var | N x (u32 len + UTF-8 bytes) | member names |
| .. | 4N | u32[N] | offsets, relative to data start, pointing at each member's length prefix |
| .. | 4N | u32[N] | sizes |
| .. | N | u8[N] | flags; all 0 in 1.4.0 (uncompressed) |
| A+4 | B | data | each member = u32 length + payload |

Total size = A + B + 4. Observed members are contiguous with no gaps, each internal length equals the table size. Names may contain directories (`English.lproj/img.inf`, `zh-CN/img.inf`); duplicates/empty/control-char names are preserved, never used as raw disk paths.

## Reading it

- `tools/extract_assets.py`: requires APK SHA-256 `d58943e6…6eead`; reverses XOR (re-encoding compared to input), parses, re-serializes and compares byte-for-byte; rejects non-zero flags, bad bounds, length mismatch, empty key, output outside the target. 8 non-container TextAssets are preserved verbatim. `--check` re-verifies all hashes. Tests: `tests/test_extract_assets.py` (6 synthetic).
- `src/assets/jar_archive.cpp` (`parse_jar`): operates on an already-decoded buffer; returns `string_view` names into the caller's buffer, relative `offset`, absolute `payload_offset`, `size`. Validates BE32 widths, total size, index, UTF-8 (rejects truncated, overlong, surrogate, >U+10FFFF), flags 0, prefixes, ranges, contiguity, full consumption. Default `JarLimits`: 128 MiB, 100,000 members, 65,536 bytes per name (tool limits, not game limits). Errors throw `JarError`. Test `tests/test_jar_archive.cpp`. Differential vs Python: 30 containers / 1,963 members, identical order, names, sizes, offsets, payload hashes.
- CLI: `build\desktop\asset_inspect.exe data\extracted\textassets\00_hat.bin` -> JSON `data_offset`, `members[]{name,offset,payload_offset,size}`.

## Evidence

- [11-resource-reader](../archive/reports/11-resource-reader.md), [12-asset-extraction](../archive/reports/12-asset-extraction.md), [15-native-assets](../archive/reports/15-native-assets.md), [10-unity-assets](../archive/reports/10-unity-assets.md) (entropy 5.14–8.00 bits/byte over `65 b6` assets)
- Local: `build/research/jar-load/`, `build/research/native-differential.json`

## Open questions

1. Behaviour of non-zero member flags (never observed); the reader rejects them.
2. Whether the key changes at runtime (other `SetEncryptionKey`/`PushEncryptionKey` call sites in `ApplicationManager`) for any resource class.
3. The extractor is version-specific; other APK versions are unsupported.
