# Culture number-format sign fields

The native runtime stores number-format records and UTF-8 strings in libil2cpp.so. This document covers only the two sign fields needed by the recovered integer parser; other record fields are not claimed as ported.

## Layout

| Location | Representation | Meaning |
| --- | --- | --- |
| ELF 0x897cfc | 48-byte records | Number-format table; indices 0..338 are reachable from the culture-name table. |
| Record +0x14 | uint16 little-endian | Negative-sign byte offset in the pool. |
| Record +0x16 | uint16 little-endian | Positive-sign byte offset in the pool. |
| ELF 0x886828 | NUL-terminated UTF-8 strings | Shared culture string pool. |

Record +0x10 is **negative infinity**, not the negative sign. Offsets address bytes, not UTF-16 code units. Direction marks are significant and must be retained.

## Reading it

`src/assets/culture_signs.cpp` accepts caller-owned record and pool byte views plus a record index, returning positive/negative UTF-8 views without allocation or filesystem dependencies. Record input must contain whole 48-byte records. The reader checks index, offset and terminator bounds; these checks define the portable malformed-input boundary rather than reproducing native out-of-bounds behavior. The caller must retain the buffers while using those views. `src/core/gameplay/apply_culture.cpp` now composes this reader with UTF-8-to-UTF-16 conversion and returns owning `ApplyNumberSigns` for `parse_apply_integer`. It preserves every scalar, including direction marks and supplementary characters, without trimming or normalization. The extractor proves valid UTF-8 for every shipped sign; corrupted input is rejected at this adapter boundary. Managed replacement fallback for invalid UTF-8 is not claimed.

`tools/extract_assets.py --culture-signs` now exports `culture/number-records.bin` (339 records) and `culture/sign-pool.bin`. It first verifies the exact APK hash, then maps ELF VAs through unique file-backed PT_LOAD segments. It copies the reachable record range and retains the original pool prefix through the last NUL needed by any sign. This proves a sufficient extraction extent, not the full shared-pool allocation size. Every referenced sign must decode as strict UTF-8. Original offsets and bytes remain unchanged. The ordinary manifest records both file sizes/hashes, and `--check --culture-signs` requires and verifies them. Outputs remain local. On the supplied APK the outputs are 16,272 bytes of records and a 1,745-byte pool prefix; all 678 sign references pass strict UTF-8 decoding. Extraction and the complete manifest hash check succeeded locally.

The optional name lookup reader `find_culture_number_index` accepts a sorted unique table of 4-byte records (uint16 name-pool offset, int16 culture index), 56-byte culture records and a shared pool. A matching culture record supplies int16 number_index at +0x20. Native bases are 0x8818b4 and 0x881e00; the shared pool starts at 0x886828. All 339 shipped name entries were confirmed strictly sorted and unique. The sign-only extracted pool is not sufficient for this lookup. `--culture-names` now exports `culture/name-index.bin` and `culture/culture-records.bin`, includes the number records and extends `culture/sign-pool.bin` through all referenced lookup names. Name keys must be unique, strictly sorted bytes and valid UTF-8; culture/number indices must stay in 0..338. On the supplied APK this produces 1,356 bytes of name index, 18,984 bytes of culture records, 16,272 bytes of number records and a 19,129-byte shared-pool prefix; the complete manifest check passed. Original records and offsets are preserved. A subsequent bounded audit confirms all 339 canonical names (+8) and nonzero territory offsets (+0x14) also fit this prefix; coverage of other string fields is not claimed. `--check --culture-names` requires all four files; the old two-file `--culture-signs` inventory remains supported. UTF-16 query conversion is implemented separately in `encode_culture_lookup_name`, including native NUL and surrogate rules.

`find_culture_record_by_lcid` searches the same culture records by signed int16 LCID, after truncating the native argument to 16 bits. Records are sorted nondecreasingly; two LCIDs have duplicate rows. Midpoint tie selection uses the AOSP policy documented in [events](../subsystems/events.md), pending reference-device comparison. Managed negative-ID rejection and invariant/failure constructor behavior are separate caller responsibilities.

`--culture-neutral` includes the preceding four buffers and adds `culture/neutral-map.json`: schema 1 with an `entries` array of `{name, lcid}` objects. All 130 associations are derived from bounded ARM64 equality leaves and the user's relocated metadata literals. Names remain strings and LCIDs remain integer data; no mapping is compiled into the portable sources. The extractor verifies the equality target, register setup, failed-match branch, successful constructor branch, literal-use kind and bounds, unique keys and exact case count. Its entry list matched the separate local Ghidra/dispatch audit exactly. Unknown keys and missing target LCIDs are not filled in or filtered out. The outer current-culture cache/fallback and its managed-object exception filter are ported; the decoded specific factory consumes these tables and validated typed neutral entries, with invariant data and lowercasing supplied by the host.

The normal APK hash gate applies before this extraction. `--check --culture-neutral` requires the five-file inventory and verifies sizes/hashes; older two/four-file inventories remain valid for their own options. Synthetic tests exercise leaf decoding and reject altered instruction shapes, bad branches, duplicate keys, truncation and invalid ELF. Full extracted mappings stay under the gitignored output directory.

### Invariant lowercase arrays

`--culture-lower` implies neutral/name/sign extraction and adds `culture/lower-0.bin` through `lower-8.bin`. These are the nine little-endian uint16 arrays consumed by TextInfoToLowerData, in static-field order; their input ranges, unit counts and metadata offsets are documented in [events](../subsystems/events.md). They total 4,270 bytes. Offsets are relative to the v31 metadata default_data section (header section index 8), and the extractor validates both that section's file bounds and every initializer extent. The exact original APK SHA-256 gate applies before these version-specific offsets are used.

The arrays are copied byte-for-byte without normalization, deduplication or use of the operating system's Unicode tables. The manifest's existing culture inventory records each size and SHA-256. `--check --culture-lower` requires all 14 culture outputs; ordinary checks accept the old 2/4/5-file inventories and the new 14-file inventory, but reject incomplete or duplicated entries. Synthetic tests cover exact reconstruction, endian preservation, invalid version/header/section/initializer bounds, file modification and inventory compatibility. Full arrays remain local.

The desktop `CultureData::load` reader now owns and validates all 14 files and their cross-table references, including the schema-1 JSON neutral map. It does not replace manifest hash checking. The base host consumes these buffers directly for selection, lowering and integer signs; see [events](../subsystems/events.md) for tests and remaining live-host boundaries.

## Evidence

Native record expansion at 0x10367cc copies record +0x14 into scratch +0x28 and record +0x16 into scratch +0x58. CultureData.GetNFIValues at 0x1e1dec4 decodes these into NumberFormatInfo +0x30 and +0x28. Its helper at 0x1e1ddcc scans to NUL and calls UTF8 Encoding.GetString. The 339-entry culture-name index and culture records cover number indices 0..338. Details and local evidence paths: [events](../subsystems/events.md). Confidence high; no runtime comparison yet.

`culture_signs_contract` uses synthetic records to check field identity, endian order, UTF-8 preservation, empty signs, buffer lifetime and malformed bounds. No original table or strings are included in tests. `apply_culture_contract` additionally composes the reader, decoder, integer parser and gift initializer using synthetic direction-mark signs and malformed UTF-8. Its optional local file arguments exercised all 339 extracted records through decoding and signed integer parsing successfully; the extended local run also resolved all 339 names to the number indices recorded in their culture rows and parsed with their signs. This is integration evidence, not a runtime comparison against Android.

## Open questions

- (blocking for gameplay integration) Connect the desktop CultureDataHost to live thread/notification providers with the recovered exception scopes.
- Audit later culture/sign setters; the neutral-map loader and invariant integer-sign projection are implemented.
