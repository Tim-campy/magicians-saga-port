# INF resource catalogs and optimize configs

63 UTF-8 `.inf` members in the JarInflater containers: 51 resource catalogs (24 `img.inf`, 24 `seb.inf`, 2 `English.lproj/img.inf`, 1 `zh-CN/img.inf`) mapping IDs to member names, and 12 `optimize_*.inf` key=value production configs. Catalog lines are parsed by `ResourceManager.LoadReady`/`LoadStart`; an audit tool enumerates them without claiming runtime resolution.

## Layout

**Catalog line** (`img.inf`, `seb.inf`): `<decimal id>` TAB `<name>[,<option>...]`. Strip outer whitespace before parsing the ID (one line starts with a space before `0`). Two catalogs carry a UTF-8 BOM. 1,298 non-empty lines.

| Part | Meaning |
|---|---|
| id | explicit index; `LoadStart` reorders entries by it (not file order) |
| name | member name; may start with `../` or `..\` (stripped by `ResourceManager.GetFileName`) |
| options | `bin` (837), `u` (54), `q+1` (53), `a1` (7), `40x40` (2), `50x12` (2), `q-1` (2), `q-2`/`q-4`/`q-5` (1 each); counts include language variants |

`bin` selects the direct byte path in the load closure; `WxH` feeds `Image.SetStandardResolution`; `,bin` is an option, not a `.bin` file. The `.opt` sidecar name is derived from the first comma field (`chip_00_0.png,bin,q+1` -> `chip_00_0.opt`). `LoadReady` searches TAB 0x09 (0x1ba6804–0x1ba6838); `LoadStart` splits via `StringUtil.Split` (0x1ba6f68), `JInteger.ParseInt` (0x1ba6f98), ID store 0x1ba6fb0, name 0x1ba6fec.

Irregularities: 3 lines without ID (`03_lineup_layout/seb.inf` `bg.seb`; `34_resident/seb.inf` `talk.seb`, `bustup.seb`) — the audit retains null explicit IDs; the later runtime indexing step assigns them as described below. Duplicate ID 2 occurs in `30_chara/img.inf` (two identical `m_chara02.png,bin` lines); 21 lines miss the literal name but have a `.png` candidate (explained by `Config.ConvertExtension` `.gif`->`.png`); 3 lines in `07_room` have no local candidate (`bg_long.png`, `equip_frame01.seb`, `equip_frame02.seb`), names present in `09_shop`.

### Selected catalog and base supplementation

ResourceManager.LoadReady (0x1ba64ec, 1,900 bytes) first loads the catalog with the current resource-language policy and trims every selected row in place; a null selected row faults at Trim. If that load returned nonnull and Language's static pointer at +0xb8 is nonnull, it sets UseResourceLanguage=false, loads the same catalog name again, then sets UseResourceLanguage=true. The setters are sequential calls, not a finally restoration; loading failure can leave the flag false. A null second result skips supplementation. The meaning/population of that Language pointer and the upstream file candidate order remain external; this is not proof that a particular locale file was selected.

The supplementation kernel starts a 256-byte false mask. Walk the trimmed selected array with an int32 cursor initially 0; null/empty rows neither mark nor advance it. For each nonempty row, a TAB replaces the cursor with System.Int32.Parse of the prefix; otherwise retain it. Grow the mask to cursor+1 when needed, mark that index and increment the cursor with int32 wrapping. Negative indices fault. This **ordinal cursor is not** LoadStart's smallest-free-ID assignment below.

Walk the second array with a separate cursor initially 0. Skip null rows; Trim once and skip empty results. Search TAB on a second Trim result and parse the prefix of the first trimmed result when present; otherwise use the cursor. If the selected mask has that ID marked, skip the row; otherwise append the first trimmed string to the selected array. Every accepted row allocates/copies/publishes a new array, preserving aliases of its previous version. Increment the cursor after each nonempty row, including suppressed ones. Crucially, **appends do not update the mask**: duplicates within the base array all survive if the original selected array did not claim their ID. Later indexed placement keeps the last duplicate. A later malformed ID leaves already-published appended rows in place.

Example: selected IDs [2,3], base IDs [0,2,4,4] retain selected rows then append base rows [0,4,4]. The final ID-4 slot belongs to the last base row. Missing IDs follow the ordinal cursor while testing supplementation, but remain missing in the text and receive smallest-free IDs later; these two steps must not be collapsed into a map merge.

### Indexing selected rows

In the not-yet-loaded path of ResourceManager.LoadStart, parsed IDs initially use -1 when a row has no ID. Start the destination length at the row count, then increase it for each signed wrap32(id+1) that is larger. In row order, replace each -1 with the smallest nonnegative ID absent from the complete current ID list, including IDs assigned to earlier missing rows. Allocate the destination with that captured length and place rows by their resolved IDs in source order: duplicate IDs retain the last row; gaps and trailing unused slots stay null. Negative IDs other than -1 fault at placement. INT_MAX+1 wraps and does not expand the length. This is the indexing stage after LoadReady has selected/merged rows; it does not establish language precedence or reload policy.

Examples: IDs [2,-1,2,-1,8,-1] become [2,0,2,1,8,3], with length 9 and row 2 at slot 2. IDs [0,0,0] produce length 3, slot 0 = last row, slots 1/2 null. Missing IDs are not assigned by simply counting lines.

**optimize_*.inf**: `key = value` lines (`size`, `merge`, `compare`, `external_merge`, `external_compare`, `level`, `color_range`, `width`, `output`); e.g. `optimize_624x640.inf` has `size = 624,640`. Referenced by OPTINFO; no runtime consumer found.

## Reading it

- `tools/audit_resource_catalogs.py`: verifies manifest size/SHA-256 of all 1,963 members, preserves order, duplicates, null IDs, BOM, options; keeps folder vs root candidates and containers separate; never converts a missing `.seb` to `.png` or searches other containers. `tests/test_resource_catalogs.py` (3). Output stable (SHA-256 `3bde780a…38e85`), identical from schema-2 and schema-3 manifests.
- `src/core/gameplay/resource_catalog.{hpp,cpp}` ports selected-row trimming and base supplementation over UTF-16 immutable strings/shared arrays. Public Trim/Int32.Parse behavior comes through a narrow CatalogTextHost; no Unicode/culture/runtime code is reconstructed. A configurable mask budget is a desktop preparation policy, not an original limit. `tests/test_resource_catalog.cpp` covers explicit/implicit cursors, null/blank rows, suppression, retained base duplicates, original aliases, mask growth, malformed-ID prefixes and composition with the indexer. The test text service is limited to synthetic ASCII fixtures; it is not a production text backend.
- `src/core/gameplay/animation_resources.cpp::index_resource_catalog` ports the row-indexing stage over host-parsed int32 IDs, returning resolved IDs and optional source-row indices. Standard unordered_set accelerates membership checks without reconstructing library containers. A configurable slot budget bounds untrusted preparation inputs; it is not an original game limit. `tests/test_animation_resources.cpp` covers missing/duplicate IDs, gaps, retained row-count length, invalid negative IDs, wrapping and the memory policy. File decoding, effective locale/density and runtime text services, and full LoadReady/LoadStart orchestration remain external; candidate generation and archive lookup are ported in resource_lookup.

## Evidence

- [13-animation-formats](../archive/reports/13-animation-formats.md), [25-resource-catalog-audit](../archive/reports/25-resource-catalog-audit.md), [26-resource-aliases](../archive/reports/26-resource-aliases.md), [21-image-resource-mapping](../archive/reports/21-image-resource-mapping.md)
- Local: `build/research/resource-catalogs.json`
- Indexing evidence: local `research/exports/image-resource-bounded/ResourceManager_LoadStart_18865.asm`, 0x1ba72d8..0x1ba7430: row-count baseline at 0x1ba72d8, wrapped length scan at 0x1ba72f8..0x1ba7304, first-free assignment at 0x1ba731c..0x1ba7370, new array at 0x1ba7394 and ordered indexed store at 0x1ba741c. The SEB object array is reordered by the same IDs at 0x1ba7590..0x1ba7648. Confidence high for this slice; no runtime comparison.
- Supplementation evidence: `research/exports/image-resource-bounded/ResourceManager_LoadReady_18863.asm`, full syntactic coverage for the same ELF; exports display ELF VA+0x100000. Selected load/store 0x1ba6650/0x1ba6674, selected Trim 0x1ba66c0, Language-pointer gate 0x1ba6734..0x1ba673c, false/load/true 0x1ba6754/0x1ba677c/0x1ba678c. The 256-byte mask is allocated at 0x1ba67a0..0x1ba67a4; selected ordinal/explicit ID scan and mask writes 0x1ba67d8..0x1ba68c4; base Trim/ID parsing 0x1ba68ec..0x1ba6950; mask test 0x1ba6954..0x1ba6970; append/copy/publication 0x1ba6974..0x1ba6a14; base cursor advance 0x1ba6a18. GOT 0x272f550 -> slot0x27c7f08 -> encoded type0x20006993 -> native type13513 -> metadata definition1829 establishes Language; calls resolve to String.Trim, String.IndexOf, Int32.Parse, Array.Copy and Language.set_UseResourceLanguage. No library body was reverse engineered. These exported bytes were already available and reused; no new Ghidra process was needed.

## Open questions

1. (blocking) Bind effective Language/density state and file/text providers to catalog loading. [Candidate order and first-match lookup](../subsystems/assets.md#resource-file-candidate-order), selected-over-base supplementation and subsequent ID indexing are established and ported; production loading is not yet composed.
2. Meaning of options `u`, `q±n`, `a1`.
3. Whether `.inf` is looked up as `.txt` (JarInflater `.inf`->`.txt` rule applies only with a path prefix).
