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

## Evidence

Native record expansion at 0x10367cc copies record +0x14 into scratch +0x28 and record +0x16 into scratch +0x58. CultureData.GetNFIValues at 0x1e1dec4 decodes these into NumberFormatInfo +0x30 and +0x28. Its helper at 0x1e1ddcc scans to NUL and calls UTF8 Encoding.GetString. The 339-entry culture-name index and culture records cover number indices 0..338. Details and local evidence paths: [events](../subsystems/events.md). Confidence high; no runtime comparison yet.

`culture_signs_contract` uses synthetic records to check field identity, endian order, UTF-8 preservation, empty signs, buffer lifetime and malformed bounds. No original table or strings are included in tests. `apply_culture_contract` additionally composes the reader, decoder, integer parser and gift initializer using synthetic direction-mark signs and malformed UTF-8. Its optional local file arguments exercised all 339 extracted records through decoding and signed integer parsing successfully; this is integration evidence, not a runtime comparison against Android.

## Open questions

- (blocking for host integration) Connect the extracted buffers and selected record index to the desktop host.
- Bind culture/neutral selection and audit later culture/sign setters.
