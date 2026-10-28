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

### Indexing selected rows

In the not-yet-loaded path of ResourceManager.LoadStart, parsed IDs initially use -1 when a row has no ID. Start the destination length at the row count, then increase it for each signed wrap32(id+1) that is larger. In row order, replace each -1 with the smallest nonnegative ID absent from the complete current ID list, including IDs assigned to earlier missing rows. Allocate the destination with that captured length and place rows by their resolved IDs in source order: duplicate IDs retain the last row; gaps and trailing unused slots stay null. Negative IDs other than -1 fault at placement. INT_MAX+1 wraps and does not expand the length. This is the indexing stage after LoadReady has selected/merged rows; it does not establish language precedence or reload policy.

Examples: IDs [2,-1,2,-1,8,-1] become [2,0,2,1,8,3], with length 9 and row 2 at slot 2. IDs [0,0,0] produce length 3, slot 0 = last row, slots 1/2 null. Missing IDs are not assigned by simply counting lines.

**optimize_*.inf**: `key = value` lines (`size`, `merge`, `compare`, `external_merge`, `external_compare`, `level`, `color_range`, `width`, `output`); e.g. `optimize_624x640.inf` has `size = 624,640`. Referenced by OPTINFO; no runtime consumer found.

## Reading it

- `tools/audit_resource_catalogs.py`: verifies manifest size/SHA-256 of all 1,963 members, preserves order, duplicates, null IDs, BOM, options; keeps folder vs root candidates and containers separate; never converts a missing `.seb` to `.png` or searches other containers. `tests/test_resource_catalogs.py` (3). Output stable (SHA-256 `3bde780a…38e85`), identical from schema-2 and schema-3 manifests.
- `src/core/gameplay/animation_resources.cpp::index_resource_catalog` ports the row-indexing stage over host-parsed int32 IDs, returning resolved IDs and optional source-row indices. Standard unordered_set accelerates membership checks without reconstructing library containers. A configurable slot budget bounds untrusted preparation inputs; it is not an original game limit. `tests/test_animation_resources.cpp` covers missing/duplicate IDs, gaps, retained row-count length, invalid negative IDs, wrapping and the memory policy. C++ text parsing, locale selection and full LoadReady/LoadStart orchestration remain external.

## Evidence

- [13-animation-formats](../archive/reports/13-animation-formats.md), [25-resource-catalog-audit](../archive/reports/25-resource-catalog-audit.md), [26-resource-aliases](../archive/reports/26-resource-aliases.md), [21-image-resource-mapping](../archive/reports/21-image-resource-mapping.md)
- Local: `build/research/resource-catalogs.json`
- Indexing evidence: local `research/exports/image-resource-bounded/ResourceManager_LoadStart_18865.asm`, 0x1ba72d8..0x1ba7430: row-count baseline at 0x1ba72d8, wrapped length scan at 0x1ba72f8..0x1ba7304, first-free assignment at 0x1ba731c..0x1ba7370, new array at 0x1ba7394 and ordered indexed store at 0x1ba741c. The SEB object array is reordered by the same IDs at 0x1ba7590..0x1ba7648. Confidence high for this slice; no runtime comparison.

## Open questions

1. (blocking) Language catalog precedence and selection/merge behavior in LoadReady; the subsequent missing/duplicate-ID indexing stage is established above.
2. Meaning of options `u`, `q±n`, `a1`.
3. Whether `.inf` is looked up as `.txt` (JarInflater `.inf`->`.txt` rule applies only with a path prefix).
