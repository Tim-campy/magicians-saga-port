# OPT optimized-texture grid (and OPTINFO)

A `.opt` file remaps a packed PNG of the same basename into a grid of logical cells: each cell lists pieces cropped from a source image and placed at an offset in the cell. Layout proven by `kairo.unity.ui.Image.LoadOptimize` and its consumer `Graphics._drawImage`. All 286 shipped OPT files (regular variant) parse and round-trip byte-exactly; C++ reader and a geometry harness exist. `.optinfo` is a production-side text sidecar with no proven runtime consumer.

## Layout

Integers big-endian where multi-byte.

| Offset | Size | Type | Meaning |
|---|---|---|---|
| 0 | 1 | u8 | cell_width (horizontal step), stored at `Image+0xc0`; value `ff` selects an unsupported variant |
| 1 | 1 | u8 | cell_height (vertical step), `Image+0xc2` |
| 2 | 1 | s8 (>= 0) | columns, `Image+0x98` |
| 3 | 1 | s8 (>= 0) | rows, `Image+0x9c` |
| 4.. | per cell, row-major | s8 (>= 0) | piece count, followed by count x 7 x s16 BE (14 bytes per piece) |

Steps are first sign-extended then reloaded as unsigned bytes (0x1b8d470..0x1b8d488); **0 stays 0, not 256**. Shorts are read through `StreamUtil.ReadShort` (0x1c0c42c) -> `EncodeUtil.ByteAryToShort(…,1)` -> `ConvertEndian` (byte swap) -> `BitConverter.ToInt16`: big-endian. Runtime index (`GetOptimize`, 0x1b8e4cc..0x1b8e4dc): `((columns*row)+column)*maxPieces + pieceIndex`; empty slots hold a sentinel and the getter returns null.

Piece values (roles from `Graphics._drawImage`, kairo:0x0600130f):

| Index | Meaning |
|---|---|
| 0 | source selector: -1 = this image; otherwise index into `Image+0xb8` array |
| 1, 2 | x / y offset of the piece in the logical cell |
| 3, 4 | crop origin x / y in the source |
| 5, 6 | width / height |

Placement: `x = column*cell_width + v1`, `y = row*cell_height + v2`. `Graphics.DrawImage` (0x1b63574) iterates rows/columns with these steps (0x1b6367c..0x1b636dc); `_drawImage` also clips and applies scale/graphics state (not reproduced). Evidence addresses: GetOptimize call 0x1b64080, selector 0x1b64094..0x1b640cc, offsets 0x1b6412c..0x1b64180, limits 0x1b64174..0x1b64200, crop add 0x1b64290..0x1b642a0.

**Grid is not image size.** `header x grid` is the draw iteration domain only. 268/286 files fit inside it; 18 in `19_chip` (`chip_00_0..3`, `chip_01_1/3`..`chip_07_1/3`) do not, e.g. header 112x128, grid 1x1, piece `(-1,226,127,0,0,398,393)`. Their OPTINFO names `optimize_624x640.inf` (624/640 low bytes = 112/128) but no rule adds 512 or reads that config; do not "fix" headers.

**OPTINFO** (286 files, UTF-8, 5 lines): tag `img1`, config name (e.g. `optimize_30x30.inf`), `palette=off`, `1`, a PNG name/date line. Dates are production timestamps.

Corpus: 38,437 bytes, 2,559 cells, 2,481 pieces; every piece uses selector -1; 2,481/2,481 crops lie inside the paired PNG; no destination overlaps. Example `hat_000`: PNG 39x15, cell 30x30, grid 1x2, pieces `(-1,4,4,20,0,19,15)` and `(-1,4,5,0,0,20,15)`; occupied bounds `[4,4,24,50]` (20x46).

## Reading it

- `tools/inspect_opt.py` (`parse`/`encode`): manifest hash check, full consumption, exact rebuild from scalars; rejects `ff`, negative counts, truncation, trailing bytes. `tests/test_opt.py` (5). Inventory JSON stable (SHA-256 `d35c1bf5…aff97`).
- `src/assets/opt_grid.cpp` (`parse_opt`): two-pass (validate, then exact allocation); contiguous `cells{first_piece,piece_count}` + `pieces{int16[7]}`; `OptLimits` 32 MiB / 16,129 cells / 2,048,383 pieces; throws `OptError`. Test `tests/test_opt_grid.cpp`. `asset_inspect --opt` JSON matched Python on 286 files.
- `tools/render_opt_reference.py`: research harness, occupied-bounds canvas (origin may be negative), raw RGBA copy without compositing (RGB under alpha 0 preserved), unassigned pixels RGBA 0, overlap/external selector/bad crop rejected, reverse-crop and reload checks; `--strict-grid` rejects the 18 out-of-grid files. Results: 286 references, 5,272,514 assigned pixels (3,170,722 alpha>0), 2,120,364 padding pixels. `asset_viewer --opt` reproduces all 286 byte-exactly.

## Evidence

- [18-optimized-textures](../archive/reports/18-optimized-textures.md), [16-sprite-layout](../archive/reports/16-sprite-layout.md), [15-native-assets](../archive/reports/15-native-assets.md), [19-asset-viewer](../archive/reports/19-asset-viewer.md), [21-image-resource-mapping](../archive/reports/21-image-resource-mapping.md)
- Local: `build/research/opt-{readers,geometry,consumer,endian,dimensions}/`, `opt-reference/report.json`, `native-opt/differential.json`, `research/local/opt-inventory.json`

## Open questions

1. Layout of the `ff` variant (no samples).
2. (blocking) Logical size of the 18 out-of-grid chip images and how `_drawImage` clipping/scale treats them.
3. `GetOptimizeSeb` (kairo:0x06001512) role: how SEB crops address OPT cells.
4. Blending/overwrite between overlapping pieces (no overlaps in corpus).
