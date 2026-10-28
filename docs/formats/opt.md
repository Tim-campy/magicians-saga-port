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

Placement: `x = column*cell_width + v1`, `y = row*cell_height + v2`. The whole-image `Graphics.DrawImage` overload (0x1b63574) iterates rows/columns with these steps (0x1b6367c..0x1b636dc). The reached rectangle overloads (0x1b64c90 / 0x1b64dc0) instead call `_drawImage` once; its regular OPT path selects **one cell** by signed truncating division of U/V by the cell steps and processes that cell's passes. It does not automatically traverse every cell crossed by a rectangle. Zero steps use ARM SDIV's zero result. GetOptimize call 0x1b64080; selector 0x1b64094..0x1b640cc; offsets 0x1b6412c..0x1b64180; limits 0x1b64174..0x1b64200; crop add 0x1b64290..0x1b642a0. This reached path is now ported through bitmap-primitive emission; rasterization remains separate.

**Rectangle drawing.** Destination and source extents are binary32; source U/V remain int32 until the native conversion points. For regular OPT with standardWidth/Height >=1, multiply the corresponding source coordinate/extent by `binary32(columns*step)/binary32(standardWidth)` (rows/height for Y), truncate to int32 with the established positive-infinity override, and convert the extent back to binary32. Destination/source extent division gives the per-axis scale. For each selected cell piece, subtract cell origin and piece offset from source U/V. A negative relative coordinate moves destination left/top forward and reduces source extent; source extent extending beyond the piece reduces destination right/bottom. The final crop adds the piece's source U/V. No positive-area check is inserted at this stage: even negative or unordered extents reach the bitmap service as in the original.

The step/grid does **not** clip the piece to a cell-sized canvas. For the representative 112x128, 1x1 chip piece `(-1,226,127,0,0,398,393)`, a request of 624x640 from source (0,0) yields a bitmap at (226,127), size 398x393 and source crop (0,0,398,393), without changing header steps. This answers the consumer arithmetic; the game's effective requests and standard dimensions still require production catalog binding.

**Graphics state.** Image clipping enables each axis only when both its position and extent are not NaN. Clipping adjusts destination and source proportionally, truncates source U/V, then reflects the clipped source coordinate when the matching flip bit is set. Nonpositive extents return before bitmap emission; native unordered comparisons permit NaN to continue. A flip pushes a temporary scale about half the **source** extents; modes 4 and 5 both request -90-degree temporary rotation. When local matrices/coordinates are active, each piece additionally pushes its destination-offset translation, emits at the original requested destination, then pops. Shared rectangle and seven-cell piece work remain live; accounting after bitmap callbacks rereads piece dimensions. Required null external sources can fail during mark/accounting even when bitmap emission was skipped. Inner piece, outer flip and profiler cleanup scopes are preserved on exceptions.

**Optional frame lookup.** `_drawImage` also has an exact U/V/width/height lookup path through Image.optimizeSeb_ and GetOptimizeSeb. The checked loader clears optimizeSeb_ (+0xa8) at 0x1b8d3c0 and allocates it only in the first-byte-FF branch at 0x1b8d994..0x1b8d9f4. Consequently the 286 regular files use the direct cell path. The optional path accepts caller-supplied decoded boxes/pieces in the port and has synthetic contracts; it does not establish an FF wire decoder. It skips image clipping with a shared one-time debug warning and emits nothing when no exact frame box matches. No unused FF decoder is being reconstructed without a sample.

**Grid is not image size.** `header x grid` is the draw iteration domain only. 268/286 files fit inside it; 18 in `19_chip` (`chip_00_0..3`, `chip_01_1/3`..`chip_07_1/3`) do not, e.g. header 112x128, grid 1x1, piece `(-1,226,127,0,0,398,393)`. Their OPTINFO names `optimize_624x640.inf` (624/640 low bytes = 112/128) but no rule adds 512 or reads that config; do not "fix" headers.

**OPTINFO** (286 files, UTF-8, 5 lines): tag `img1`, config name (e.g. `optimize_30x30.inf`), `palette=off`, `1`, a PNG name/date line. Dates are production timestamps.

Corpus: 38,437 bytes, 2,559 cells, 2,481 pieces; every piece uses selector -1; 2,481/2,481 crops lie inside the paired PNG; no destination overlaps. Example `hat_000`: PNG 39x15, cell 30x30, grid 1x2, pieces `(-1,4,4,20,0,19,15)` and `(-1,4,5,0,0,20,15)`; occupied bounds `[4,4,24,50]` (20x46).

## Reading it

- `tools/inspect_opt.py` (`parse`/`encode`): manifest hash check, full consumption, exact rebuild from scalars; rejects `ff`, negative counts, truncation, trailing bytes. `tests/test_opt.py` (5). Inventory JSON stable (SHA-256 `d35c1bf5…aff97`).
- `src/assets/opt_grid.cpp` (`parse_opt`): two-pass (validate, then exact allocation); contiguous `cells{first_piece,piece_count}` + `pieces{int16[7]}`; `OptLimits` 32 MiB / 16,129 cells / 2,048,383 pieces; throws `OptError`. Test `tests/test_opt_grid.cpp`. `asset_inspect --opt` JSON matched Python on 286 files.
- `tools/render_opt_reference.py`: research harness, occupied-bounds canvas (origin may be negative), raw RGBA copy without compositing (RGB under alpha 0 preserved), unassigned pixels RGBA 0, overlap/external selector/bad crop rejected, reverse-crop and reload checks; `--strict-grid` rejects the 18 out-of-grid files. Results: 286 references, 5,272,514 assigned pixels (3,170,722 alpha>0), 2,120,364 padding pixels. `asset_viewer --opt` reproduces all 286 byte-exactly.
- `src/core/gameplay/image_draw.{hpp,cpp}`: the two reached rectangle wrappers and `_drawImage` selection/clipping/scale/transform/accounting/cleanup orchestration, with bitmap/profiler/culling/matrix services explicit. PreparedOptImage retains the existing reader's compact contiguous cells/seven-short pieces, without a padded per-pass matrix or per-draw allocation; it retains header steps, uses max(1,piece_count) passes and preserves empty slots and shared result identity. `tests/test_image_draw.cpp` covers the two hat cells, partial/scaled crops, large chip offsets, standard dimensions, int32 U above 2^24, NaN clipping, nested cleanup, live accounting, external-null sources and optional decoded frame lookup. The RenderSystem integration in `tests/test_render_frame.cpp` now runs selected SEB records -> sorted AppData commands -> regular OPT selection -> exact bitmap rectangle. The whole-image overload and `_drawBitmap` backend remain separate.

## Evidence

- [18-optimized-textures](../archive/reports/18-optimized-textures.md), [16-sprite-layout](../archive/reports/16-sprite-layout.md), [15-native-assets](../archive/reports/15-native-assets.md), [19-asset-viewer](../archive/reports/19-asset-viewer.md), [21-image-resource-mapping](../archive/reports/21-image-resource-mapping.md)
- Local: `build/research/opt-{readers,geometry,consumer,endian,dimensions}/`, `opt-reference/report.json`, `native-opt/differential.json`, `research/local/opt-inventory.json`
- Current `research/exports/image-draw/coverage.tsv` proves full syntactic coverage of 0x1b637bc..0x1b64c90 (5,332 bytes / 1,333 instructions), plus the whole/rectangle/scaled wrappers. The earlier opt-consumer export contained only 1,215 instruction lines and is insufficient for full control flow. `exception-callsites.json` records 113 ranges at LSDA 0x9311c8: initial CheckUse faults reach profiler cleanup; bitmap failures inside temporary per-piece translation reach inner pop, then outer flip/profiler cleanup. Flip creation/push faults occur before outer flip cleanup becomes active. `research/exports/opt-readers/Image_LoadOptimize_18384.asm` establishes regular versus FF table allocation; `opt-geometry/Image_GetOptimize_18387.asm` and Image_GetOptimize_18390.asm establish slot selection and shared seven-cell expansion. Confidence high for these static contracts; no original runtime/pixel comparison.

## Open questions

1. Layout of the `ff` variant (no samples).
2. (blocking) Production crop and standard-dimension bindings for the 18 out-of-grid chip images. Consumer arithmetic is established above; do not repair headers or infer runtime dimensions from OPTINFO names.
3. Complete `_drawBitmap` primitive/graphics-state contract and bind it to the existing SDL backend; derive the implementation from its reached game contract and public APIs instead of reconstructing Unity's renderer.
4. Blending/overwrite between overlapping pieces (no overlaps in corpus).
