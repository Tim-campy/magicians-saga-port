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

Irregularities: 3 lines without ID (`03_lineup_layout/seb.inf` `bg.seb`; `34_resident/seb.inf` `talk.seb`, `bustup.seb`) — kept null; duplicate ID 2 in `30_chara/img.inf` (two identical `m_chara02.png,bin` lines); 21 lines miss the literal name but have a `.png` candidate (explained by `Config.ConvertExtension` `.gif`->`.png`); 3 lines in `07_room` have no local candidate (`bg_long.png`, `equip_frame01.seb`, `equip_frame02.seb`), names present in `09_shop`.

**optimize_*.inf**: `key = value` lines (`size`, `merge`, `compare`, `external_merge`, `external_compare`, `level`, `color_range`, `width`, `output`); e.g. `optimize_624x640.inf` has `size = 624,640`. Referenced by OPTINFO; no runtime consumer found.

## Reading it

- `tools/audit_resource_catalogs.py`: verifies manifest size/SHA-256 of all 1,963 members, preserves order, duplicates, null IDs, BOM, options; keeps folder vs root candidates and containers separate; never converts a missing `.seb` to `.png` or searches other containers. `tests/test_resource_catalogs.py` (3). Output stable (SHA-256 `3bde780a…38e85`), identical from schema-2 and schema-3 manifests.
- No C++ reader yet.

## Evidence

- [13-animation-formats](../archive/reports/13-animation-formats.md), [25-resource-catalog-audit](../archive/reports/25-resource-catalog-audit.md), [26-resource-aliases](../archive/reports/26-resource-aliases.md), [21-image-resource-mapping](../archive/reports/21-image-resource-mapping.md)
- Local: `build/research/resource-catalogs.json`

## Open questions

1. (blocking) Handling of lines without ID and duplicate IDs; language catalog precedence.
2. Meaning of options `u`, `q±n`, `a1`.
3. Whether `.inf` is looked up as `.txt` (JarInflater `.inf`->`.txt` rule applies only with a path prefix).
