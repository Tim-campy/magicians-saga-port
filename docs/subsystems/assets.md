# Assets and resource pipeline

All shipped resources of the 1.4.0 APK (SHA-256 `d58943e6…6eead`) are catalogued and extracted: 38 TextAssets (30 XOR-obfuscated JarInflater containers holding 1,963 members) plus Unity textures, FSB5 audio and TrueType fonts. The on-disk JarInflater, SEB (modes 0/1) and regular OPT formats are proven by assembly and byte-exact round trips, and each has a C++17 reader in `src/assets/` with contract tests. Visible-entity draw-command production and the game's depth-sort queues are ported and composed with the combat/camera/visibility test. The runtime resolution chain (INF catalog -> ResourceManager ID -> Image/OPT -> Seb sprite) is not closed end to end; scene pixels, audio playback and comparison with Android remain open.

## Overview

Layers, outermost first:

1. **APK / Unity serialized files.** 1,184 `assets/` members form 1,182 logical files (1,149 Unity serialized v22, 33 other). The only split group, `globalgamemanagers.assets.split0..2`, is concatenated in numeric order in memory (2,953,096 bytes). One build scene `Assets/Scenes/main.unity` (`level0`) holds GameObject `Main` with `main.Main`, `system.tapjoy.MyTapjoy`, `system.notification.FCMReceiver`. Unity 2022.3.62f2. See [unity-media](../formats/unity-media.md).
2. **TextAssets.** 38 objects; 30 start with `65 b6` and are XOR-obfuscated containers, 8 are kept verbatim (6 readable UTF-8: `language_pack_template_ja/en`, `language_pack_format`, `BillingMode`, `snd`, `filelist`). Read in game by `kairo.unity.io.AssetReader`; `GetData` reaches `Encrypter.Decode`. See [jar-container](../formats/jar-container.md).
3. **JarInflater containers.** Decoded payloads are a name-indexed archive parsed by `JarInflater._load`; lookups go through `JarInflater.ConvertExtension`/`_search` (name aliases, language candidates).
4. **Catalogs.** Per-container `img.inf` / `seb.inf` map explicit decimal IDs to member names with options (`bin`, `u`, `q+1`, `WxH`, ...). `ResourceManager.LoadReady`/`LoadStart` parse them and store images by ID in `ResourceManager+0x10`. See [inf](../formats/inf.md).
5. **Images.** `kairo.unity.ui.Image` wraps a Unity Texture2D (`DecodeByteArray` -> `UnityUtil.LoadImage`). An optional `<name>.opt` grid (path from `Image.GetOptionFile`) remaps a packed PNG into logical cells. See [opt](../formats/opt.md).
6. **SEB sprites/animations.** `Seb._load` parses groups of records into compact runtime buffers; `GetSpriteLocal` selects a record by FrameNo; `ConvBufferToSprite` fills a 12-slot `Sprite` int array (FrameNo, TexId, U, V, W, H, X, Y, ReverseU, ReverseV, Blend, Color); `Seb.GetImage` resolves TexId through `ResourceManager.GetImage`. Frame advance is in `Seb.Frame` / `ThisFrameRenderAnimation`. See [seb](../formats/seb.md).

Extracted corpus (local, never distributed, `data/extracted/manifest.json` schema 2 / `data/full` schema 3): 973 PNG, 303 SEB, 286 OPT, 286 OPTINFO, 63 INF, 47 TXT, 3 CSV, 1 BIN, 1 MAB. All 973 PNGs pass Pillow `verify()` + `load()`; none exceeds 1,024 px on either axis (RGBA8 expansion of all would be 53,709,576 bytes; not a VRAM/3DS measurement).

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `AssetReader.GetData` | kairo:0x06000f39 | Calls `_init`, dispatches via `IApplication.RunOnUiThread` (0x1b7b428), tests name with `String.Equals/EndsWith` and static flags; one path BLs to `Encrypter.Decode` at 0x1c8fcf8 | medium |
| `AssetReader._getInputStream` / `_getData` / `_getTextAsset` | kairo:0x06000f34 / 0x06000f37 / 0x06000f38 | Build a `UIMethod` and run it on the UI thread; delegate body unresolved | low |
| `AssetReader.GetAccessFilesList` | kairo:0x06000f35 | Builds candidate path list; calls `Config.ConvertExtension`, `StringUtil.Split`, `Language.get_UseResourceLanguage` (0x1c90124), `Language.GetFolder` (0x1c90224) | medium |
| `Encrypter.SetEncryptionKey` | kairo:0x0600030e | Stores key array in Encrypter static storage | high |
| `Encrypter..cctor` | kairo:0x0600031b | Initial key is a 1-byte array (real key set later) | high |
| `Encrypter.Decode` | kairo:0x06000317 | Loads static key, forwards to overload 0x06000318 | high |
| `Encrypter.Decode` (overload) | kairo:0x06000318 | Forwards to `Encode` 0x06000316 | high |
| `Encrypter.Encode` | kairo:0x06000316 | In-place XOR `data[i] ^= key[i % key.Length]` | high |
| `JarInflater._load` | kairo:0x060003c6 | Parses container index + data | high |
| `JarInflater.ConvertExtension` | kairo:0x060003d0 | `Config.ConvertExtension`, then `.inf`/`.ini` -> `.txt` when prefix `+0x20` is set | high |
| `JarInflater._search` | kairo:0x060003cd | Trim, ConvertExtension, ToLower, try `GetAccessFilesList` candidates in order against dictionary `+0x10` | high |
| `JarInflater.Exist` / `GetData` / `GetInputStream` | kairo:0x060003ca / 0x060003cb / 0x060003c8 | Prefixed path first, then `_search`; `GetData` tries `Storage.Read` first | medium |
| `JarInflater.GetResourceAsStream` / `CheckResourceAsStream` | kairo:0x060003ce / 0x060003cf | Try name, then name + `.bytes` | high |
| `Config.ConvertExtension` | kairo:0x06000269 | If selector in {2,3,4}: replace all `.gif`->`.png`, then `.mld`->`.ogg` | high |
| `ResourceManager.LoadReady` | kairo:0x060016ea | Loads string array; finds TAB (0x09), `Substring`, `Int32.Parse` | medium |
| `ResourceManager.LoadStart` | kairo:0x060016ec | Splits catalog line, ID via `JInteger.ParseInt`, reorders by ID, stores images at `+0x10`, loads `.opt` and calls `Image.LoadOptimize` | medium |
| `<>c__DisplayClass60_0.<LoadStart>b__0` | kairo:0x0600171d | Per-image load closure: option `bin`, `WxH`, palette, constructs `Image` | medium |
| `ResourceManager.GetFileName` | kairo:0x060016ee | Strips `..\` / `../` prefix, then Trim | high |
| `ResourceManager.GetImage` | kairo:0x0600170c | Returns image array `+0x10`[id] (bounds checked) unless collection `+0x60` overrides | high |
| `Image.GetOptionFile` | kairo:0x0600155c | `name,opts` -> first part, drop extension, append `.opt` | high |
| `Image.GetAreaSize` | kairo:0x0600152e | Reads encoded image dimensions (GIF/PNG/JPEG headers) | high |
| `Image..ctor` (7 overloads) | kairo:0x060014ff..0x06001505 | Initialise width/height `+0x28/+0x2c` | medium |
| `Image.DecodeByteArray` | kairo:0x0600152f | Creates Texture2D, calls `UnityUtil.LoadImage` at 0x1b8ff90 | high |
| `Image.<Use>b__125_0` | kairo:0x06001566 | Decode, optional `LoadOptimize` (0x1b92f00), then width/height from `Texture.get_width/height` | medium |
| `Image.OverwritePaletteData` | kairo:0x0600152b | Applies optional palette; returns input unchanged when none | medium |
| `Image.GetWidth` / `GetHeight` | kairo:0x06001517 / 0x06001516 | Return `+0x28` / `+0x2c` | high |
| `Image.SetStandardResolution` | kairo:0x06001558 | Stores `WxH` option into `+0x70/+0x74` | high |
| `UnityUtil.LoadImage` | kairo:0x06000679 | Version switch (`2018.1.0`); reflective `ImageConversion.LoadImage(tex, bytes)` or `Texture.LoadImage(bytes)` | medium |

OPT- and SEB-specific methods (Image.LoadOptimize/GetOptimize, Seb._load, ConvBufferToSprite, GetSpriteLocal, Frame, PushThisFrameRender, Sprite getters, ...) are described in [opt](../formats/opt.md) and [seb](../formats/seb.md); their ids and statuses are in `re/status/assets.tsv`.

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `ecs.RenderSystem.Init` | game:0x06000f85 | Store supplied world's map and allocate 32 layer indices | high |
| `ecs.RenderSystem.Update` | game:0x06000f87 | Enumerate visible entities, project coordinates, produce commands, catch loop faults and sort retained commands | high |
| `ecs.RenderSystem..cctor` | game:0x06000f8b | Set static extra-depth rate to 100; exposed as a host input in the port | high |
| `AppData.TransormIsometricViewX/Y` (binary32 overloads) | game:0x06000244 / game:0x06000245 | `x-z` and `(x+z)*0.5`, respectively | high |
| `AppData.ClearZSortObjects` | game:0x06000246 | Clear counts without releasing row buffers | high |
| `AppData.InsertZSortObject` | game:0x06000247 | Write 19 int32 fields; grow selected row buffer by 128 | high |
| `AppData.Zsort` (both overloads) | game:0x06000248 / game:0x06000249 | Sort category 0 then category 1, or one supplied category | high |
| `AppData.QuickSort/Pivot/Partition` | game:0x0600024a / game:0x0600024b / game:0x0600024c | Ascending depth with observable game-specific tie order | high |
| `Graphics.GetColorOfRGB` (three arguments) | kairo:0x06001355 | Clamp each channel to 0..255, return packed `0xffRRGGBB` | high |

## Behaviour

**Key derivation (method, no key bytes).** `main.AppData.Init` (0x1100298) allocates an 11-element int array (0x110062c), fills it with `RuntimeHelpers.InitializeArray` (0x1100640), expands the eleven ints into 44 little-endian bytes (loop to 0x11006c0) and calls `SetEncryptionKey` at 0x11006e4. Other `SetEncryptionKey` call sites exist in `ApplicationManager`; the key is not proven global/immutable. The initializer is metadata field 24349 (`<PrivateImplementationDetails>`, offset 4557992, length 44, default-data index 188696), resolved via code ref 0x272fa50 -> reloc 0x27ea1c0 (value 0x800000f5, usage 4 / fieldRef 122, type index 8809); its SHA-256 matches its generated name. The extractor reads it from the hash-verified user metadata and embeds no key.

**Name aliasing.** `Config.ConvertExtension` reads the first int of Config static storage (`class+0xb8`, offset 0). `Config..cctor` initialises that block from ELF 0x625fb0 to `(2,0,100,30)`, so the selector starts at 2 (later writes not excluded). `StringUtil.Replace` (0x1c19534) replaces **every occurrence** (IndexOf from 0, mode 4, loop) — not a suffix swap; do not use `Path.ChangeExtension` semantics there. `JarInflater.ConvertExtension` then uses `Path.GetExtension` + `ToLower` + `Path.ChangeExtension` for `.inf`/`.ini` -> `.txt` (true final-extension rule), only when `this+0x20` (path prefix) is non-null. Literal indices: `.gif` 1037, `.png` 1050, `.mld` 1044, `.ogg` 1046, `.inf` 1039, `.ini` 1040, `.txt` 1055, `.bytes` 1020, `.opt` 1047, `bin` 10179, `gif/png/jpg` 11283/12529/11765.

**Lookup order.** `Exist`: ConvertExtension, then prefixed `CheckResourceAsStream` if `+0x20` set, else `_search != null`. `_search`: null if name or dictionary null; first candidate (lower-cased) present in the dictionary wins. `GetInputStream`: prefixed stream, else `_search`; a found record may be served by `AssetReader.GetCustomResource` (field `+0x28`). `GetData`: `Storage.Read(prefix+name)`, then `GetSize`/`GetInputStream`; size -1 returns null. No direct cross-container fallback (e.g. `07_room` -> `09_shop`) exists in these bodies.

**Image dimensions.** `Image+0x28/+0x2c` are the encoded image size, not an OPT logical size. ctor 0x060014ff writes 0,0; 0x06001500 two virtual calls (slots 0x178/0x198 = slots 4/6 = `Texture.get_width/get_height`, methods 52930/52932); 0x06001502 uses `GetAreaSize`; 0x06001503..05 write 240,240 (provisional). `GetAreaSize`: GIF LE u16 at data offsets 6/8, PNG BE u32 at 16/20, JPEG scans to marker `0xffc0`. After `LoadOptimize`, `<Use>b__125_0` refreshes the fields from the Texture (no piece offsets added). `LoadOptimize` never writes them; no `0 => 256` rule; `optimize_624x640.inf` is not read for dimensions.

**`bin` load path.** In the closure, option `bin` sets a flag (0x1baa63c–0x1baa654); the uncached branch (0x1bab0f0) reads bytes via `JarInflater.GetData` (0x1bab124), optional palette + `OverwritePaletteData` (0x1bab358), constructs Image with ctor 0x06001502 (0x1bab390). `WxH` options are split on `x` (0x1baa924–0x1baa974) into `SetStandardResolution`.

**Unity media.** 23 `AudioClip` stream ranges into `.resource` files, all FSB5 Vorbis; 5 TTF fonts; 73 Texture2D (68 exportable, 5 empty font textures). TimeManager: fixed timestep ~0.02 s, max 0.333333 s, time scale 1 (engine settings, not proof of a 50 Hz game tick).

### Visible-entity commands

`RenderSystem.Init` (0x1160ca8, 108 bytes) requires the supplied world, stores its nullable map at +0x28, then stores a fresh int32[32] scratch at +0x30. It does not clear the draw queue or register a subset. `Update` (0x1160d30, 4,124 bytes) obtains current QuestForm camera offsets, calls the current width and a freshly obtained form's height, and discards both dimensions. These calls are outside its entity-loop catch. Scheduler gating is already established in [ecs](ecs.md); the renderer itself does not clear the queues.

The loop uses World's visible HashSet (+0x40), preserving slot order and version invalidation through the existing public-HashSet adapter. It skips Invisible slot 21 or absent Position slot 0. Screen coordinates are native int32 conversions of binary32 `x-z` and `(x+z)*0.5-y`. Each conversion preserves the original positive-infinity result of INT32_MIN. For Cell slot 5 it requires Cell twice, then updates Depth slot 4 with binary32 `y+x+z+extra+overlay`: `extra` is signed `wrap32(rate*depth.extra)/100`, rate initially 100; overlay is 9,999,999.0F for Fukidashi slot 18 or Gauge slot 19, otherwise zero. Without Cell, existing Depth.value is retained. The map stored by Init is unused by this body.

Resource selection captures current AppData and fresh Seb slot 2 resource ID, obtains the selected ResourceManager's clip array, and indexes a freshly read Seb clip ID. SyncAnimation slot 13 selects the shared clip clock; otherwise it captures the entity's Seb frame. A single layer value -1 requests all layers through `Seb.GetLayersNum`, filling the persistent 32-cell scratch without growth; count 33 fails after storing indices 0..31. Other layer arrays keep their original reference and captured length; later callbacks may replace the component's array without replacing the captured array. Elements remain live, and -1 entries are skipped. Image slot 7 is captured once before layer processing.

Each layer calls `Seb.GetSprite(frame,layer)` and reads U/V/W/H at result indices 2..5. This result is a runtime sprite, not a direct crop of the PNG or a disk SEB record. Image overrides apply in this order: texture ID, image-resource ID, resource -2 early skip, U/V/W/H arrays, texture -2 late skip. Singleton arrays apply to every layer; crop -1 preserves the sprite value; shorter arrays fall back. Required null arrays and negative indexed layers fault.

Base alpha comes from Color slot 6 or 255. ModifyAnimation slot 15 scales width/height with binary32 multiplication and native conversion, adds modifier x to screen x, and adds modifier y plus the original-minus-scaled height to screen y. Anchor bit 2 additionally adds half the width difference; bit 32 subtracts half the height difference; signed division truncates toward zero. **These x/y changes accumulate across layers**, rather than resetting for each sprite. Modifier alpha <=254 becomes `wrap32(alpha+1000)`; larger values retain base alpha. Type 9 sends angle as skew; other types send zero. Modifier z is unused here.

Immediately before insertion the body captures current AppData again, then reads fresh Seb resource, Depth.value, Seb clip and Color RGB. Missing Color produces color -1. Camera offsets and frame retain their earlier captured values. All arithmetic on int32 positions/depth wraps.

`InsertZSortObject` (0x111586c, 800 bytes) takes 20 arguments: the following 19 cells plus `type`, which selects an outer queue. RenderSystem always selects type 0.

| Cell | Meaning |
|---|---|
| 0 / 1 | Texture override / Seb resource ID |
| 2 / 3 / 4 | Depth plus layer / Seb clip ID / captured frame |
| 5 / 6 | Screen x minus camera x / screen y minus camera y |
| 7 / 8 | Scaled width / scaled height |
| 9 / 10 | U / V |
| 11 / 12 | Source width / source height after Image overrides |
| 13 / 14 / 15 | Layer / alpha / packed RGB or -1 |
| 16 / 17 / 18 | Angle / image-resource override / skew |

AppData +0x3d0 holds counts and +0x3d8 holds row buffers. Insertion grows the selected buffer by 128 rows, preserving old row references and allocating new int32[19] rows; it publishes growth before accessing the count-selected row. Each field writes sequentially, and the count increments only after all writes. A malformed short row retains its written prefix. Clear resets counts only; subsequent commands reuse rows. This is application behavior, not a reason to reconstruct a managed allocator.

`Zsort()` (0x1115b8c) sorts category 0 before 1. The typed overload (0x1115bac) computes `wrap32(count-1)`, skips a negative end, and otherwise calls QuickSort (0x1115c10). Pivot (0x1115cc4) scans from left+1 for the first depth unequal to the left depth, choosing the larger depth's index; equal ranges return -1 unchanged. Partition (0x1115d50) scans left while depth is less than pivot and right while depth is greater than or equal, swapping row references. QuickSort recurses left and iterates right. Ties therefore are **not stable**: depths `[2(A),1(B),1(C)]` become `[1(C),1(B),2(A)]`. These small private game helpers are retained because `std::sort` and `std::stable_sort` promise neither this exact order nor its row reuse; storage and swaps use the standard C++ library.

The native LSDA resolves the loop catch to `System.Exception`: visible-cursor acquisition, enumeration, component reads, sprite queries and insertion are inside; viewport queries and final `Zsort()` are outside. A caught failure stops the entire loop, logs the current entity when nonnull, retains already queued commands, then sorts that prefix. Logging failure propagates before sorting. A version change at the next cursor step still names the last entity. A failure before selecting an entity is not logged. No queue rollback or implicit clear occurs.

## Port

- `src/assets/jar_archive.{hpp,cpp}` — `parse_jar`, decoded container index, BE32, contiguous members, flags must be 0; test `tests/test_jar_archive.cpp` (`jar_archive_contract`). Differential: 30 containers / 1,963 members identical to the Python extractor.
- `src/assets/opt_grid.{hpp,cpp}` — `parse_opt`, regular OPT; test `tests/test_opt_grid.cpp`. Differential: 286 files / 2,559 cells / 2,481 pieces.
- `src/assets/seb.{hpp,cpp}` — `parse_seb`, disk modes 0/1; test `tests/test_seb.cpp`. Differential: 303 files / 452 groups / 2,686 records.
- `src/core/gameplay/render_frame.{hpp,cpp}` and `render_queue.{hpp,cpp}` — visible draw-command selection, binary32 projection, image/modifier overrides, retained-failure prefix and native queue growth/sorting. Contracts: `tests/test_render_frame.cpp` and `tests/test_render_queue.cpp`. `tests/test_emitter_frame.cpp` composes Render factory index 21 after CellCulling 20 and before Garbage 31 in both 120-pass shot/landing/cleanup scenarios. Explicit host inputs provide current application, shared clips, sprite bounds, extra-depth rate and viewport. The 120-pass scenarios retain synthetic sprite bounds. A separate RenderSystem contract binds PreparedAnimationResource and the real SEB evaluator from synthetic disk records; the desktop animation trace evaluates all 303 shipped SEBs. Command consumption to pixels remains open. `EntitySprite.layers` now retains shared managed-array identity, with factories allocating fresh singleton arrays.
- `src/platform/desktop/asset_inspect.cpp` — JSON dumper: `asset_inspect <decoded-container>`, `--opt <file>`, `--seb <file>`; exit 1 invalid input, 2 usage; Windows `wmain` UTF-16 paths. Build `tools\build-desktop.cmd` (MSVC + CMake/Ninja + CTest, no external deps, `/W4 /permissive-` or `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion`). Linux commands documented but never run.
- `src/platform/desktop/asset_viewer.cpp` — optional technical viewer (`SAGA_BUILD_VIEWER=OFF` default; `tools\build-viewer.cmd`). SDL3 release-3.4.18 and stb_image commit 003501f6 fetched into `build/_deps` with SHA-256 checked by CMake. Loads a static PNG (<= 64 MiB, <= 8,192 px/axis, <= 16,777,216 px, 16-bit rejected) to RGBA8; window mode (nearest, letterbox, Esc) compiled but never run interactively; `--headless --capture out.bmp` uses `SDL_CreateSoftwareRenderer`, `SDL_BLENDMODE_NONE`, refuses existing targets. `--opt file.opt` composes pieces on an occupied-bounds canvas (selector -1 only, overlap rejected via occupancy mask, int64 arithmetic checked to int32, budget 65,536 pieces); prints JSON (`canvas_kind`, `occupied_bounds`, `canvas_size`, `grid_domain`, `piece_count`, `pieces_outside_grid`, `assigned_pixels`, `padding_convention`). Validated: synthetic 7x5 RGBA and `title.png` 600x320 byte-identical; 286/286 OPT compositions identical to the Python harness.
- Tools (Python): `tools/inspect_unity_assets.py` (catalogue, UnityPy 1.25.4 pinned under `.tools/unitypy-libs`, no media export); `tools/extract_assets.py` (PEP 723, hash-locked to 1.4.0; `--out`, `--check`, `--unity-media`; refuses to overwrite differing outputs; manifest with origin, path ID, original name, transform, sizes, hashes; numeric output prefixes against Windows case collisions); `tools/inspect_seb.py`, `tools/inspect_opt.py` (strict parse + encoder); `tools/render_opt_reference.py` (research-only occupied-bounds PNGs, outputs only under `build/research`, `--select`, `--strict-grid`); `tools/audit_resource_catalogs.py` (INF audit, candidates only). Tests: `tests/test_extract_assets.py` (6), `test_seb.py` (6), `test_opt.py` (5 + 3 harness), `test_resource_catalogs.py` (3).
- Missing: XOR decode and Unity media extraction in C++ (desktop relies on the Python extractor output); complete INF text/locale loading and file selection (ID placement and AppData selection are ported in animation_resources); name alias rules (`Config`/`JarInflater.ConvertExtension`, language candidates); Image dimension logic; native loader bounding-box/extension-block handling and automatic drawing-clock advancement (record normalization/packing, sprite conversion and manual Seb.Frame are ported); OPT FF variant; scene pixel rendering, audio or font path; demonstrated Linux/3DS execution.

## Evidence

- [10-unity-assets](../archive/reports/10-unity-assets.md), [11-resource-reader](../archive/reports/11-resource-reader.md), [12-asset-extraction](../archive/reports/12-asset-extraction.md), [13-animation-formats](../archive/reports/13-animation-formats.md), [14-unity-media](../archive/reports/14-unity-media.md), [15-native-assets](../archive/reports/15-native-assets.md), [16-sprite-layout](../archive/reports/16-sprite-layout.md), [18-optimized-textures](../archive/reports/18-optimized-textures.md), [19-asset-viewer](../archive/reports/19-asset-viewer.md), [21-image-resource-mapping](../archive/reports/21-image-resource-mapping.md), [25-resource-catalog-audit](../archive/reports/25-resource-catalog-audit.md), [26-resource-aliases](../archive/reports/26-resource-aliases.md), [29-seb-animation-timing](../archive/reports/29-seb-animation-timing.md)
- Local (gitignored): `research/local/unity-assets.json`, `research/local/extracted-image-budgets.json`; `build/research/asset-reader*`, `jar-load/`, `image-resource-{bounded,followup,final}/`, `image-unityload/`, `resource-aliases/`, `resource-alias-followup/`, `frame-policy*/`, `native-differential.json`, `viewer-validation/`, `viewer-opt/report.json`, `resource-catalogs.json`.
- Render evidence (local, gitignored): `research/exports/render-system/` (Init/Update/cctor), `render-bindings/` (InsertZSortObject, binary32 projection, unported Seb.GetSprite), `render-queue/` (clear/sort/RGB), `render-sort/QueueSort.asm` and `render-order/` (Pivot/Partition). The Update LSDA report is `render-system/exception-callsites.json`: width 0x1160e10 and height 0x1160e30 are outside, visible enumeration 0x1160e6c/0x1160ea4 and insertion 0x1161838 are inside, final sort 0x1161908 is outside. Catch type resolves through 0x272f548 to System.Exception. Rate 100 is stored by cctor 0x1161db4; overlay binary32 is at 0x6279c8. Confidence high for static contracts; no Android comparison or playable scene is claimed.

## Open questions

The next rendering blocker is the `AppData.DrawZSortObjects` consumer and its image selection: bind evaluated sprites to actual Image/OPT resources and the existing SDL backend before claiming mission pixels. `Seb.GetSprite` selection/interpolation/cache is now implemented; see [SEB](../formats/seb.md). The 120-pass mission fixture still supplies synthetic bounds.

1. (blocking) Full ResourceManager resolution: which ResourceManager/catalog serves each Seb, TexId -> `img.inf` ID -> Image, and the delegate bodies dispatched by `LoadStart`/`AssetReader` via `RunOnUiThread`.
2. (blocking) Language precedence and candidate order in `AssetReader.GetAccessFilesList`; how `English.lproj/` and `zh-CN/` catalogs override the root.
3. Runtime value of the `Config` extension selector (initially 2) and whether it ever leaves {2,3,4}.
4. Three `07_room` names (`bg_long.png`, `equip_frame01/02.seb`) exist only in `09_shop`; no fallback proven. Lines without explicit ID (`bg.seb`, `talk.seb`, `bustup.seb`) and the duplicate ID 2 in `30_chara/img.inf`: runtime handling unknown.
5. Consumer of standard resolution `+0x70/+0x74`; true logical size of the 18 `19_chip` OPT images (PNG sizes known, 624x640 config not proven consumed).
6. Meaning of options `u`, `q±n`, `a1`; palette source for `OverwritePaletteData`.
7. Which `AssetReader.GetData` flags/name tests select decoding; whether all `65 b6` TextAssets take that path at runtime (extraction proves the transform, not the branch).
8. Audio loop/volume/streaming policy (no FSB loop chunks) and font selection/fallback/shaping; no audiovisual comparison exists.
