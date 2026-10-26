# IL2CPP assembly resources

The APK stores mscorlib resources in `assets/bin/Data/Managed/Resources/mscorlib.dll-resources.dat`. This is a directory and contiguous payload container, not a managed DLL. Its seven entries include the collation data needed by notification prefix matching.

## Layout

All integers below are little-endian int32. Offset 0 contains directory length excluding that first word; offset 4 contains entry count. Starting at offset 8, each directory entry contains payload size, name byte count, then exactly that many name bytes with no terminator or alignment padding. Payloads follow the directory in entry order. The first payload starts at directory length + 4; subsequent offsets are cumulative payload sizes. Names need not be unique for parsing; the native resource lookup chooses the first matching descriptor.

In this APK the directory ends at byte 205 and the container ends at byte 337563. Core collation occupies offset 205, length 118901; tailoring occupies offset 119106, length 6724. Five CJK resources follow. Payloads and resource names remain local inputs rather than embedded port data.

The core collation payload starts with version byte 3 followed by five length-prefixed byte tables. Their observed lengths are 16384, 42160, 42160, 8704 and 9472. The tailoring payload starts with version byte 3, an int32 record count, then 13 bytes per record: three int32 values and one boolean byte. After the records are two skipped bytes, an int32 UTF-16 code-unit count and that many little-endian uint16 units. The shipped count is 33 records and 3142 code units. The initializer ignores the two skipped bytes; their observed value is not a parser requirement.

## Reading it

The main extractor supports `--collation` and writes the seven unchanged payloads under `collation/` in the selected output root. The existing APK hash check, all-output collision preflight and no-overwrite policy apply. Manifest `collation` rows record output names, fixed-version sizes and SHA-256. `--check --collation` requires the complete seven-file inventory and checks its bytes along with all other outputs; old manifests remain valid without the requirement. See [setup](../SETUP.md) for commands. No key or decryption is required for this container.

Run `python -I tools/inspect_assembly_resources.py game/MagiciansSaga-1.4.0.apk`. The inspector reads the fixed ZIP member, validates directory/payload extents and the collation layout, and prints names, sizes, offsets and hashes, never payload contents. It requires exact directory/payload consumption as a portable validation boundary; the native trusted-data reader does not provide these bounds checks. Synthetic tests in `tests/test_assembly_resources.py` cover truncation, negative extents, exact ends, duplicate names, empty payloads, version gates and directory round trips.

## Evidence

Native descriptor production `0x100e5d4` reads count at +4, starts directory traversal at +8, and initializes payload offset from the first word +4. Its descriptor has assembly identity, name, payload offset and size; lookup at `0x100e900` compares name bytes and returns base + payload offset. The loader `0x100f018` combines the data path with `Resources` and the assembly name plus `-resources.dat`; literals are at ELF `0x64107c` and `0x62dbe9`. Bounded local exports: `research/exports/collation-descriptors/` and `collation-resource-native/`.

MSCompatUnicodeTable..cctor (`0x1cd5748`) supplies the version checks and inner layouts. Its readiness flag is set after table installation, not merely after the version check. The inspector validates both actual resources against these extents; rebuilding the directory and concatenating the original payloads reproduces the entire container byte for byte. Confidence high for layouts; no runtime comparison is claimed.

## Open questions

- (blocking) Port the collation table readers and comparison algorithm before replacing notification prefix providers.
- Confirm the runtime environment choice and successful resource loading against Android; static presence does not prove live initialization.
