# Setup

Everything game-derived stays local and gitignored. The maintainer supplies the APK.

## Local layout

| Path | Contents | In Git |
| --- | --- | --- |
| `game/MagiciansSaga-1.4.0.apk` | The maintainer's APK (SHA-256 `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead`). Read-only input. | no |
| `extracted/` | `extract_assets.py` output: `textassets/`, `members/`, `manifest.json`; `extracted/full/` adds Unity media; `extracted/native/lib/arm64-v8a/` holds `libmain.so` and `libil2cpp.so`. | no |
| `research/` | Local evidence: `il2cpp-metadata-inventory.json`, `il2cpp-code-map.json`, `functions-full.csv`, REA/JADX JSON, and `research/exports/` (Ghidra TSV exports, logs, scenario outputs). | no |
| `ghidra/` | Ghidra projects: `wizard_il2cpp_fde` (libil2cpp.so with FDE-defined functions, the one to use), `wizard_il2cpp`, `wizard_bootstrap` (libmain.so). | no |
| `external/` | Local tools: apktool 2.12.1, JADX headless MCP 0.7.1, Android platform tools (ADB), UnityPy 1.25.4 + Pillow (`unitypy-libs/`), pyelftools 0.32 (`python-libs/`). Versions and hashes in `external/installed-tools.json`. | no |
| `build/` | CMake builds (`build/desktop`, `build/viewer`) and downloaded public dependencies/test fixtures (`build/_deps`). Safe to delete; the next configure downloads missing dependencies. | no |

## Host tools (Windows, verified 2026-10-25)

| Tool | Where | Notes |
| --- | --- | --- |
| Python 3.12 | PATH | Run tools with `python -I`. Most tools use only the standard library. |
| uv | `C:/Users/l_abn/.local/bin/uv.exe` | `uv run tools/extract_assets.py ...` |
| Java 21 | PATH | For JADX, apktool and Ghidra. |
| Ghidra 12.1.4 | `C:\Ghidra` | Headless ARM64 analysis works directly. Run **one Ghidra process at a time**. |
| REA | MCP + `rea.cmd` | The Windows REA bridge does not accept ARM ELF; use Ghidra headless for native code and REA/JADX for DEX. Use `tools\rea-local.cmd` (sets `REA_JADX_MCP_JAR`, heap 768 MiB); run JADX queries sequentially (parallel JVMs hit Windows error 1455). |
| Visual Studio Community 18 | MSVC 19.51, CMake 4.3, Ninja 1.13 | Not on PATH; `tools\build-desktop.cmd` sets up the environment for the child process only. |
| FFmpeg | `C:/ffmpeg/bin` | Audio inspection. |
| ADB | `external/` via `tools\with-tools.cmd adb ...` | No device or emulator is available yet. |

`tools\setup-tools.ps1` reinstalls `external/` (official sources, hashes checked). `tools\with-tools.cmd <command>` runs a command with those tools on PATH without changing global settings.

Desktop builds fetch FreeType2.13.3 from its public release archive, pinned by SHA256 in `cmake/FreeType.cmake`; this GPL-2.0 project uses FreeType's GPLv2 license option. Optional compressed-font/shaping dependencies are disabled. Tests download a pinned public NotoSans-Regular font (OFL1.1) into build/_deps and verify its hash; the revision/URL/hash live in tests/CMakeLists.txt. No original font or other game data is required by these tests. A first configure needs network access; subsequent builds reuse the local downloads.

## Regenerating local inputs

```powershell
# Inventory and metadata (read the APK, never execute it)
python -I tools/inspect_apk.py game/MagiciansSaga-1.4.0.apk --output research/apk-inventory.json
python -I tools/inspect_il2cpp_metadata.py game/MagiciansSaga-1.4.0.apk --output research/il2cpp-metadata-inventory.json
python -I tools/extract_research_members.py game/MagiciansSaga-1.4.0.apk --output extracted/native --member lib/arm64-v8a/libil2cpp.so --member lib/arm64-v8a/libmain.so
python -I tools/map_il2cpp_methods.py extracted/native/lib/arm64-v8a/libil2cpp.so research/il2cpp-metadata-inventory.json --output research/il2cpp-code-map.json
python -I tools/registry.py build research/il2cpp-code-map.json

# Assets (38 TextAssets, 1,963 container members; --unity-media adds textures, FSB/WAV audio and fonts)
uv run tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted
uv run tools/extract_assets.py --check --out extracted
# Use a fresh destination: differing manifests are never overwritten.
uv run tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/culture-enabled --culture-signs
uv run tools/extract_assets.py --check --out extracted/culture-enabled --culture-signs
uv run tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/culture-lookup --culture-names
uv run tools/extract_assets.py --check --out extracted/culture-lookup --culture-names
uv run tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/culture-neutral --culture-neutral
uv run tools/extract_assets.py --check --out extracted/culture-neutral --culture-neutral
uv run tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/culture-lower --culture-lower
uv run tools/extract_assets.py --check --out extracted/culture-lower --culture-lower
uv run tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/runtime --runtime-tables
uv run tools/extract_assets.py --check --out extracted/runtime --runtime-tables
uv run tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/collation --collation
uv run tools/extract_assets.py --check --out extracted/collation --collation
uv run tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/collation-indexers --collation-indexers
uv run tools/extract_assets.py --check --out extracted/collation-indexers --collation-indexers
uv run tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/full --unity-media
```

## GPU bitmap adapter

The optional viewer build also compiles `gl_bitmap` and `test_gl_bitmap`; it reuses SDL 3.4.18 and requires an OpenGL 3.3 core driver. GLSL compiles through the driver at runtime, without a separate shader SDK. Existing `asset_viewer` and `queue_capture` commands still use their SDL 2D diagnostic path; the GL adapter currently consumes explicit ready mesh/material bindings.

```powershell
tools\build-viewer.cmd
build\viewer\test_gl_bitmap.exe build\viewer\gl-contract
```

The build runs `gl_bitmap_depth_pixels` through CTest. The standalone invocation repeats that contract and saves four synthetic 32x32 BMPs in the supplied directory. It checks native-Z occlusion, layer clears, alpha discard, depth writes, blend equations, orientation and the concrete AppData/BitmapBackend/Offscreen submission chain. These captures contain synthetic test data. A hidden SDL OpenGL window still requires a graphics driver; failure is reported, not silently skipped. Windows was exercised on Intel UHD Graphics 770 with OpenGL 3.3.0. Matrices, wrapping and target dimensions remain explicit host inputs; see [GPU behavior and limits](subsystems/assets.md#gpu-bitmap-submission).

Linux CI installs Mesa/X11 development packages, Xvfb and xauth, enables `SAGA_BUILD_VIEWER`, and runs CTest with `xvfb-run -a env LIBGL_ALWAYS_SOFTWARE=1`. The adapter's owned framebuffer is RGBA8/D24; it does not request an Android emulator or execute the APK.

## Ghidra headless

```powershell
& C:/Ghidra/support/analyzeHeadless.bat ghidra wizard_il2cpp_fde -process libil2cpp.so -noanalysis -scriptPath tools/ghidra -postScript ExportBoundedInstructions.java research/exports/<name>.tsv research/exports/<name> -max-cpu 1
```

`ExportBoundedInstructions.java` exports the instructions of the method ranges listed in the input TSV (`start, end, label`, ELF VAs); `InspectSelectedMethods.java`, `InspectJniTable.java`, `InspectBootstrap.java` and `DefineUnwindFunctions.java` cover the other cases (see each script's header). Check the export coverage header: the current `wizard_il2cpp_fde` exports report image base `0x100000`. `ExportBoundedInstructions` adds that base to input ELF VAs, so subtract `0x100000` from its displayed instruction addresses when citing ELF VAs. Ghidra rejects project and script paths that contain a `.`-prefixed directory: from a worktree under `.claude/`, copy the scripts to a temporary directory first.

## Building and testing

```powershell
tools\build-desktop.cmd                 # Windows: configure, build, run CTest (build/desktop)
tools\build-viewer.cmd                  # optional SDL3 viewer (downloads pinned SDL3 into build/; stb also serves headless Gauge22)
python -I tools/run_tests.py            # all Python tests
```

Both Windows build scripts select code page 65001, request VSLANG=1033 after Visual Studio setup and use fresh CMake compiler detection. Detection must match the compiler's actual installed language: the old viewer cache stored a garbled Portuguese /showIncludes prefix, so Ninja recorded zero header dependencies and could link stale objects after a layout change. If build/viewer was created with that older script, clean that generated build once before using the corrected script. The repaired build was checked directly with Ninja's deps tool: image_draw.cpp.obj records 33 dependencies, including image_draw.hpp, and the complete viewer tests pass. No compiler/SDK reinstallation is needed.

Linux (also what CI runs):

```sh
cmake -S . -B build/linux -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/linux --parallel && ctest --test-dir build/linux --output-on-failure
```

Desktop executables: `build/desktop/asset_inspect.exe <container.bin>` (JSON index of a decoded container), `build/desktop/event_trace.exe` (event scenario; see `docs/subsystems/events.md`), `build/viewer/asset_viewer.exe <png|opt>` (technical viewer, Escape closes).

The headless animation scenario accepts `build/desktop/animation_trace.exe <selected-seb> <clip-id> <passes> <application-update:0|1> <automatic:0|1>`. It binds the chosen file to the supplied clip slot of effect resource 20, creates the actual effect and emits JSON Lines for both animation clocks. Supply a previously selected SEB path from your own extraction; the tool does not infer catalog paths or language precedence. The gate arguments are scenario inputs, not recovered live flag values. It runs animation only: Garbage expiration and drawing are not executed. For example, use clip ID 7, five passes, application-update 1 and automatic 0 to inspect one selected file's manual-clock behavior. The synthetic file-to-effect integration is registered in CTest. Append `--sprites` to include evaluated twelve-cell sprite arrays (or null) for each layer in every trace row. This uses normalized/packed record data and the native selection/interpolation contract, without producing pixels or guessing Image/OPT resolution.

After the desktop build, exercise the extracted number signs locally (optional; CI uses synthetic data):

```powershell
.\build\desktop\test_apply_culture.exe extracted/culture-enabled/culture/number-records.bin extracted/culture-enabled/culture/sign-pool.bin
```

To include name-to-number lookup in that local integration check:

```powershell
.\build\desktop\test_apply_culture.exe extracted/culture-lookup/culture/number-records.bin extracted/culture-lookup/culture/sign-pool.bin extracted/culture-lookup/culture/name-index.bin extracted/culture-lookup/culture/culture-records.bin
```

The Windows desktop build script uses code page 65001, requests VSLANG=1033 after vcvars, and refreshes CMake configuration with --fresh (CMake 3.24+; supported by the installed Visual Studio CMake). Installed localized compiler resources may still emit Portuguese: UTF-8 keeps CMake's detected /showIncludes prefix consistent with Ninja's input. When migrating an old build with a corrupt localized prefix, remove its generated CMakeFiles/<cmake-version>/CMakeCXXCompiler.cmake and clean Ninja outputs once before rebuilding; reconfiguration alone cannot repair already stale objects or the persisted compiler detection.

After the desktop build and a successful culture manifest check, exercise native loading/selection/parsing with `build/desktop/test_culture_data.exe extracted/culture-lower EN-US +42` (expected LCID/value: `1033 42`). The optional arguments use local data; ordinary CTest uses only synthetic inventories. `CultureDataHost` supplies the base-culture adapter; it does not select the OS locale or install live thread storage.

After a successful collation manifest check, `build/desktop/test_collation_tables.exe extracted/collation/collation/collation.core.bin extracted/collation/collation/collation.tailoring.bin` exercises the portable reader against the extracted resources. Expected inventory: five core tables, 33 tailoring records, 3142 UTF-16 units. This validates decoding, not text-comparison behavior.

`build/desktop/test_collation_index.exe extracted/collation-indexers/collation/indexers.bin extracted/collation-indexers/collation/collation.core.bin extracted/collation-indexers/collation/collation.tailoring.bin` checks the seven original configurations and every UTF-16 unit against category/weight table bounds. This is static integration validation, not an Android comparison.

The runtime Hashtable table is loaded with `load_hashtable_primes(extracted_root)` and shared by `RuntimeCallContextHost`. After its manifest check, run `build/desktop/test_runtime_data.exe extracted/runtime` to exercise growth and full mutable-context copying with local data. The host requires an explicit correlation-value clone callback; scheduler/TLS state is not inferred. See [runtime table format](formats/runtime-hashtable-primes.md).

The selected-sprite technical preview uses the existing SDL viewer:

```powershell
build/viewer/asset_viewer.exe <selected.png> --sprite <selected.seb> <frame> <layer> <texture-slot> --canvas <width> <height> --origin <x> <y> --opt <selected.opt> --headless --capture <new-output.bmp>
```

The archive-backed mode resolves catalogs, the selected clip's TexId, PNG and optional OPT without manual PNG/OPT filenames:

```powershell
build/viewer/asset_viewer.exe <decoded-container.bin> --resource <clip-id> <frame> <layer> --language English.lproj --dpi 1 --canvas 200 160 --origin 100 100 --headless --capture <new-output.bmp>
```

For a technical multi-resource composition, the viewer build also creates `queue_capture`:

```powershell
build/viewer/queue_capture.exe scene.txt 600 220 new-output.bmp
```

The scene description is local diagnostic input, not a game format. Paths resolve relative to it. `world` is required and supplies nine column-major binary32 values; `resource` binds a native AppData ID, quoted decoded-archive path, quoted language folder and signed DPI index. `global` has the same syntax for the independently supplied global AppData used by image overrides. `draw` supplies the nineteen decimal int32 cells in [RenderField order](../src/core/gameplay/render_queue.hpp), with queue type 0. Blank lines and whole lines beginning with # are ignored. For example:

```text
world 1 0 0 0 1 0 0 0 1
resource 1 "local-characters.bin" "English.lproj" 1
draw -1 1 0 0 0 32 64 -1 -1 -1 -1 -1 -1 0 255 -1 0 -1 0
```

The default mode uses the shared bitmap backend, source-specific texture/mask caches, native command sorting and stable material-queue composition on an explicit transparent RGBA canvas. Its SDL 2D path does not infer native depth-buffer/order semantics. A graphics-backed desktop renderer is required. Captures refuse existing destinations. `queue_capture_pixels` checks synthetic multi-image, OPT, transparency, global override and world-transform scenarios through CTest.

For the native-Z GPU path, append --native-depth and supply these additional scene rows:

```text
projection 0.0625 0 0 0 0 0.125 0 0 0 0 0.00000133324445 0 -1 -1 -1 1
gpu_depth 24 1
material 0 65808 2000 10 0
```

This illustrative projection is for a 32x16 target, near0/far1500100 and column-major OpenGL clip coordinates; it is not the mission's inferred live camera. projection requires sixteen binary32 cells. gpu_depth supplies native depth bits17..32 and the depth-step flag0/1; the actual desktop target is RGBA8/D24 with nearest/clamp sampling and an initial color+depth clear. Each material row supplies texture key, decimal shader key, absolute queue, explicit identity ID and uniform-color flag0/1. The example describes one synthetic opaque texture pass; supply the bindings actually reached by your selected assets. Missing bindings report their tuple and fail without writing a capture. Reused identity IDs must have consistent queue/color/shader state. This makes material identity a declared diagnostic input rather than an invented production cache policy.

```powershell
build/viewer/queue_capture.exe scene.txt 32 16 new-native-output.bmp --native-depth
python -I tests/test_mission_combat_pixels.py build/viewer/saga_mission.exe
```

The GPU mode uses BitmapMeshFrame, the recovered group/array/quad kernels and native stream submission. Its pixel tests distinguish same-layer Z occlusion from next-layer depth clearing. test_mission_combat_pixels drives the production saga_mission owner for 240 passes per ordinary/physical category, using synthetic archive/table bytes. It compares captured state against headless execution and checks live RGBA output. Real assets use the same entry. Captures of table-backed missions include a nested state object with the final actor/combat observations; existing image metadata stays at the top level. No Android equivalence is implied.

For table-backed entity scenes, replace manual draw rows with tables, progress and entity rows. The same archive lookup reads Chara/Equipment/Cloth/Hat/MagicShop/Enemy/Bullet.txt, without requiring img.inf/seb.inf in that data archive. Use these additional row shapes (all inputs are explicit):

| Row | Fields |
| --- | --- |
| tables | quoted decoded table archive, quoted language folder, DPI index |
| progress | character ID, saved level, saved EXP |
| purchase | shop ID, saved purchase count; every reached shop needs a supplied value |
| resource_name | quoted record resource name, AppData resource ID |
| global_alias | global target ID, ready application source ID; declare after that resource |
| slots | component-slot count (diagnostic limit1..1024) |
| view | width, height, scale, camera X/Y, VIEW margins X/Y, extra-depth rate |
| geometry | map step X/Z and height; required when a selected bullet reaches the map |
| animate | initial animation-pass count0..10000, automatic flag0/1, application-update flag0/1 |
| mission_events | fresh plus null/quoted UTF8 normal-save string0, or new_game from mission_initialization; followed by IsShortWords0/1, held-advance0/1 and PRINT0/1; requires quest_frame/tick_ms and freshly loaded event counters |
| talk_layout | talk id, page count and that many nonnegative reveal lengths; provisional TextLayout provider outputs, never inferred from raw text; requires mission_events |
| talk_font | quoted font path, requested-font-scale int32, application-ratio float, layout-scale float, localization-disabled0, Japanese0/1, Japanese-style0/1, FreeType load-flags int32, kerning-mode int32 (-1 disabled, 0..1 pixel units); requires mission_events and excludes every talk_layout row |
| hero | literal fresh; replaces a selected human/player declaration; requires system_init0, system_construct, native stage entry, town_save19 and explicit saved progression for the first table main character |
| town_save19 | decoded int32 normal-save town count; required with hero fresh; Town7 raises it to the number of flag1 Town.txt records only when larger |
| system_init | loaded byte0..255; zero requires hero fresh, town_save19, system_construct and native stage entry, and runs original InitWorld construct/add/map/Init; nonzero remains an explicit initialized diagnostic snapshot; saved/configuration state stays explicit |
| system_construct | literal empty_manager; requires system_init and stage_start; runs constructor enable phases before registration/Init, with native Menu/SubMenu/Debug hooks; exclusive with system_enabled/menu_visible overrides; current world/configuration remain explicit |
| quest_frame | effective Config debug-controls bool0/1 and system-save orientation int32 or initial from mission_initialization; requires system_construct/native stage entry; enables QuestForm.Update and gated 1/3 passes |
| menu_visible | bool0/1; optional provisional Init-only visibility snapshot with declared legacy default true; requires system_init and excludes system_construct, whose native constructor supplies false |
| system_enabled | factory index0..31 and bool0/1; optional declared Init-only snapshot flag, once per slot; unspecified flags default true from CreateSystems' selected enabled state; requires system_init; no constructor or hook is implied |
| human | direction, character ID, level, formation X/Z, EXP, position X/Y/Z |
| enemy | direction, enemy ID, level, flags, position X/Y/Z |
| bullet | bullet ID, damage, attribute, parent creation ordinal (-1 means null), position X/Y/Z |
| effect | clip ID, image ID (-1 keeps SEB images), loop flag0/1, position X/Y/Z; uses AppData20 |
| bubble | parent selected-entity ordinal (-1 null), data ID, type0/1, frame maximum (-1 infinite), resource, clip, initial X/Y/Z; provisional CreateZObject + AddFukidashi diagnostic composition; at most100000 rows |
| gauge | creation step0..100000, parent selected-entity ordinal, type0..7, from/to/maximum, image, frame maximum, flags, option, option2; actual CreateGauge factory with explicit provisional call arguments/timing |
| human_static_fps | explicit captured class-initialization FPS1..1000 for Retry invincibility; separate from runtime FPS; required only when diagnostic retry is reached |
| tick_ms | diagnostic animation/render interval1..1000; required for loop modes |
| simulate | initial selected logic passes0..10000; enables optional Human/Enemy/Projectile -> Move -> Collision -> Emitter -> Modifier -> Animation -> Render -> Garbage on every loop step; absent means animation-only |
| mission_presentation | width height initial-form-scale automatic0/1 application-update0/1 near far camera-Z; native identity/slots/margins/depth-rate and no pre-roll; exclusive with world/projection/view/slots/animate/simulate; requires system_init and mission_map |
| mission_resources | quoted language and DPI; requires --data and generated mission-resources.tsv; binds tables, nine mission archives, global aliases and resource names |
| material_policy | literal native; ordinary image/shader/queue material cache; exclusive with explicit material rows |
| runtime | FPS1..1000 or initial, numeric-damage DEBUG0/1 or initial, backend system and seed int32; initial selects MyConfig30/0; must agree with CLI values |
| mission_map | fresh plus four int32 chip IDs (start/middle/end/background); requires human_system, one player, AppData18 chips and geometry312/312/(distance+2); native map/camera/cells/culling; distance0..1022 diagnostic allocation bound |
| input | fresh, initial held uint32, last-active -1..2, move-front0/1, then three attack state/frame int32 pairs; requires human_system attribute0..2 and one player; states/held mask must agree with existing scenario rows |
| human_system | quest distance, minimum/maximum Z rates, attack attribute, attack button state; requires geometry/runtime/simulate |
| player | creation ordinal of the Human marked with native Player slot30; hero lookup retains the original subset/cache contract |
| actor_state | creation ordinal, Human state, finite initial Z velocity; explicit diagnostic state input, not a keyboard mapping |
| gear_levels | equipment/hat/cloth, saved level for every present row in that table; all three are required for Human total stats |
| gear_new_game | no fields; run native Equipment/Hat/Cloth NewGame on fresh records; requires hero fresh, system_init0, system_construct and result providers; excludes gear_levels/result_inventory rows; other providers remain required |
| character_shop_new_game | explicit int32 shop discovery mask (native cctor initial1); run Chara/MagicShop NewGame on fresh records; requires hero fresh/system_init0/system_construct; excludes progress/purchase/sorties/result_recruits rows; other scenario inputs remain required |
| new_game_save | no fields; requires --data/new-game-save.tsv, fresh hero/system_init0/system_construct and stage_notifications completion token new_game; supplies normal-save selection/town/wallet and all quest resets; excludes quest/mission_selection/stage_start/town_save19/result_wallet/result_record snapshots |
| mission_state | new_game, Yugami int32 or initial from mission_initialization, host-held keys uint32; requires new_game_save/fresh hero/system_init0/system_construct/quest_frame; enables native services/formation without snapshots |
| mission_initialization | no fields; read --data/mission-initialization.tsv; requires new_game_save/hero fresh/system_init0/system_construct; supplies initial references for system Yugami/orientation and the new_game event argument seed |
| rewards | initial defeated count, remaining count, uniform saved defeat count (int32); requires human_system, Treasure.txt and AppData7 for reached drops; no collection or permanent grants |
| enemy_system | Yugami int32, quest enemy maximum, total limit, spawn interval, base level; requires enemy_population/enemy_pool/human_system |
| enemy_population | initial created, defeated, remaining, loop counter, boss-defeated0/1, uniform section limit, uniform section-created count; reward counters must agree if supplied |
| enemy_pool | count0..1000 followed by that many enemy ID/weight int32 pairs, in order; count0 explicitly selects an empty pool |
| collisions | no fields; enables persistent native Collision12 after Move11 and event29 dispatch; requires human_system receivers |
| stage_notifications | initial Stage state/frame, retried0/1, quest flags int32, completed0/1 or new_game (requires new_game_save), held keys uint32, retry answer int32; requires human_system; Stage Update requires stage_loop; no automatic answer |
| collection | literal fresh; initialize zero stage rewards/empty collecting list/box-not-picked; run the Stage3 collection prefix (or full Update with stage_loop) before Human4; requires stage_notifications/human_system |
| result_wallet | initial money/crystals int64, rescued int32; requires every result row, collection fresh and stage_loop |
| result_record | seven int32 fields: best EXP, best money, challenges, clears, defeated, rescued, best damage |
| result_inventory | equipment/cloth/hat, initial found int32, is-new0/1, stock int32; one row per kind, applied uniformly to present IDs; gear_new_game instead supplies per-record native initial state |
| result_recruits | initial found int32, is-new0/1, recruited int32, applied uniformly to present character IDs |
| stage_loop | remaining-time int32, boss-started0/1, fade-frame int32, literal hold; native Stage.Update with explicit deferred-transition hold policy |
| stage_events | literal fresh, count0..1000, then event-ID/progress int32 pairs; consumed flags start zero; reached script dispatch fails |
| mission_selection | decoded normal-save area-slot8, global quest-slot9 and saved rescued count; selects full Area/Quest catalogs and binds the same quest id as Stage.Start stage; exclusive with quest/stage_start; --quest overrides effective slot9 |
| stage_start | stage int32 followed by null, or area and a nonnegative Area.txt ID; requires selected quest, loaded-world system_init, stage_loop/results and party_formation; area mode retains the canonical record; stage remains an explicit saved input |
| party_formation | new_game retains the MemberSystem constructor/Init formation; requires hero fresh/system_init0/system_construct and native entry; alternative sixteen saved IDs/-1 supplies a diagnostic snapshot, duplicates retained |
| sorties | unique character ID and saved sortie count int32; required for every started member; native increment wraps |
| stage_boss | optional boss ID int32 and level int32; requires stage_loop and complete enemy inputs; no default boss binding |
| stage_pulse | unique positive logic-step uint64 and key mask uint32; includes initial simulate steps; diagnostic pulses, not --replay |
| drop | type0..3, value, scale, XYZ; explicit initial entity via native factory; requests Treasure.txt; not a quest-placement decoder |

Scene views select all constructed position/sprite entities as their explicit visible group. They run the recovered render producer, animations and optionally movement/expiry; mission_map/input fresh bind native camera/cells/logical controls; the complete combat schedule and quest bootstrap remain separate. Non-looping effects derive their lifetime from the SEB header minus one. Garbage runs after command production, so an expiring entity retains its final frame's commands; the next frame excludes it. Bounded reports mark expired entities destroyed and omit their removed positions. A human's creation level and supplied saved level remain distinct from its table initial_level. Unknown saved values do not default to zero. The native Data.Load bootstrap, original resource-name resolver and save envelope remain separate work.

```powershell
build/viewer/queue_capture.exe scene.txt 600 220 --material-inputs
build/viewer/queue_capture.exe scene.txt 600 220 --interactive
build/viewer/queue_capture.exe scene.txt 600 220 --run-frames 12
```

--material-inputs reports only reached tuples and writes no capture. Assign explicit material identity rows before GPU execution. --interactive is a visible technical window: Space pauses, N advances one selected logic step while paused, Escape closes. --run-frames uses a hidden window and reports render/logic-step counts, movement_steps (including initial simulate passes), and final positions in entity creation order. With simulate 0, the enemy moves using its factory's table-derived velocity; humans start with zero velocity. Move integrates binary32 speed once per logic pass, without a delta-time multiplier. tick_ms controls this selected preview only; it does not establish the original frame timing or establish a complete quest bootstrap. The maintainer's profiles are local at research/exports/mission-data-scene/scene.txt (animation-only) and movement.txt (simulate 0); they require the supplied decoded archives.

### Headless mission host

The scene-free fresh entry uses the existing extracted media and four small local indexes. Generate the missing indexes without extracting the assets again:

```powershell
python -I tools/extract_assets.py --out extracted/full --mission-index
python -I tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/full --new-game-save
python -I tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/full --mission-initialization
python -I tools/extract_assets.py --out extracted/full --mission-font
.\build\viewer\saga_mission.exe --data .\extracted\full --quest 1
# Fixed-pass execution without SDL:
.\build\desktop\saga_mission.exe --data .\extracted\full --quest 1 --frames 600 --headless
```

The viewer command opens an interactive window; add `--frames N` for a bounded run or `--capture new-output.bmp --frames N` for a capture. Build it with `tools\build-viewer.cmd`. Retry/victory/defeat/result notices use the window title: Enter acknowledges a notice; C spends100 crystals, A selects the offline ad-request service and N declines retry. Replay uses `answer <int32>` (0/1/other for retry;0 acknowledges a notice). Pending native forms suspend the parent mission until explicitly closed. `--human-static-fps` (provisional default30) supplies the FPS captured at Human class initialization for the existing invincibility-duration initializer. Native defaults and every unresolved context override are listed by `--help` and reported as provisional in bootstrap_context; FPS/damage DEBUG use MyConfig30/0, while the System.Random seed42 is declared host policy. The default font is selected from the manifest by Unity identity and its size/hash are checked by --mission-font. Automatic font selection covers English/default skin only; other policies require explicit --font. This entry initializes fresh records/world/formation; it does not load an Android save or boot through title/town. See [integration](subsystems/integration.md#scene-free-fresh-mission-s1-a1).

For table-selected quests, add `quest <id> <saved-rescued-count>` to the profile and use the `quest` source forms in [integration](subsystems/integration.md#table-selected-quests-s1-a1a3). Quest.txt comes from the existing tables archive; saved state, actors and resource bindings remain explicit. `stage_loop quest` uses the ported ResetTimer, while a numeric timer supplies an explicit mid-mission snapshot. The diagnostic CLI accepts `--data` and `--quest` alongside `--scene`; the native fresh entry above needs no scene.

Rotate13 follows actor speed after Move/Collision and before animation/rendering. Reports include rotation_steps and each selected actor's direction, direction_locked and animation_clip; no extra scenario row is needed. The local rotate-down.replay/rotate-up.replay demonstrations use the real quest profile above; see [facing integration](subsystems/integration.md#velocity-facing-rotation-s1-a2a3).

The optional `enemy_gauges` row enables native enemy HP drawing. Bind resource23/global_alias23 to decoded `22_gauge.bin` and supply existing rewards saved counters explicitly; the original visibility threshold is saved defeats>=6. Headless mode reports ordered requests, while queue_capture renders their actual resource images. Local quest-gauges.txt and its run command are described in [health drawing](subsystems/integration.md#enemy-health-drawing-s1-a3).

`saga_mission` is always built, including with `SAGA_BUILD_VIEWER=OFF`. It shares MissionProfile's archive/scene loader and selected logic step with queue_capture; no SDL is required. The existing pinned stb_image decoder is also built without the viewer because Gauge22 geometry reads image alpha. Diagnostic profiles must contain entities and an explicit simulate row; the typed fresh entry prepares both directly. Archive paths are relative to the profile directory unless `--data` selects an extractor output root.

```powershell
.\build\desktop\saga_mission.exe --scene .\research\exports\mission-data-scene\effects.txt --frames 120 --headless --fps 25 --debug 0 --rng system --seed 42
```

`python -I tools/extract_assets.py "game/<maintainer.apk>" --out extracted/full --new-game-save` derives local new-game-save.tsv from the hash-checked metadata, without re-extracting media. The new_game_save row replaces the four result_wallet/result_record/mission_selection/town_save19 rows and requires stage_notifications completion token new_game. It resets all quest records and supplies selection/town/wallet; --quest remains an explicit override of the initial global quest slot. Other record reset modes, formation, configuration and presentation inputs remain required. See [normal-save seeds](subsystems/save.md#normal-save-newgame-seeds).

The command runs fixed passes without wall-clock sleeping and prints final positions, destroyed flags, Human state/frame/attack counters, RNG draw count and render-command count. FPS25/DEBUG0/System.Random/seed42 are explicit provisional configuration choices. With human_system enabled, FPS drives actual attack intervals and walking recovery cadence; the seeded RNG drives actual recovery/celebration draws. DEBUG is passed to the actual numeric damage gate; optional diagnostics remain disabled as a logging stub. Headless mode prepares SEB catalogs/clocks and skips PNGs unless explicit Gauge22 entities require the gauge archive. In that case it decodes that archive and binds CPU images for alpha, without GPU submission. Other resources remain undecoded. `--data <root>` resolves each declared archive filename under `<root>/textassets/`, without falling back to the scene path. `--quest <id>` overrides the ID in quest or mission_selection; the latter also uses it as the effective decoded slot9 and native stage; its saved rescued count and other explicit state remain unchanged. A missing file, absent quest row, table gap or conflicting numeric static field fails. Input-enabled scenes accept --replay; physical gameplay input is supplied by the SDL frontend. Unknown flags fail. The viewer build also opens the shared SDL/OpenGL mission window when `--headless` is omitted. It derives dimensions from the explicit `view` row. Without `--frames` the window is interactive; with `--frames N` it runs a bounded hidden-window replay. Desktop-only builds report that window support is unavailable. Original save/configuration/current-world entry remains unbound. Optional `quest_frame <debug-controls0/1> <saved-orientation>` binds QuestForm.Update on a constructed Stage.Start profile with explicit fade0. This Config byte is separate from `--debug`; when true, holding Tab requests three world passes per form frame. Replay uses held-key bit0x10 and addresses pulse rows by form frame. Orientation0 is an explicit scenario choice; -1 fails at the unbound landscape eligibility provider. See [native frame integration](subsystems/integration.md#native-quest-form-frames-s1-a1a3).

To remove manual archive/material rows, generate the local resource index once from the existing extractor output:

```powershell
python -I tools/extract_assets.py --out extracted/full --mission-index
```

Use `mission_resources "English.lproj" 1` and `material_policy native` in the scenario, removing its tables/resource/global_alias/resource_name/material rows. The first row requires `--data`; it uses the validated manifest name mapping and the original AppData resource IDs. The second selects ordinary native material identity/UniColor under the shared renderer. State, projection, depth and timing remain explicit. `quest-auto-resources.txt` is the local native-Start profile using these rows. `queue_capture` also accepts trailing `--data <root>` with its existing `--replay` option.

`mission_presentation 600 640 100 0 1 0 1500100 -1` replaces the remaining presentation matrices and fixed offsets with the native initializer/resize contracts and public projection math. Its last three values explicitly select the serialized Offscreen camera baseline. It requires loaded-world Init and a native map. The local quest-native-presentation.txt profile exercises it. Interactive non-replay windows can be resized; bounded/headless runs keep declared dimensions unless their replay contains `step viewport width height`. Source textures and game time are retained across resize. Degenerate/minimized targets are ignored interactively; explicit invalid replay sizes fail. Captures must match the prepared dimensions; pause retains the old native queue until the next step.

For a reproducible screenshot after native logic has processed a resize, add `--capture new-output.bmp --frames N` to the viewer host command. It steps and renders every frame through the retained mesh/material owner without wall-clock pacing, then saves the final viewport through the shared GL path; the destination must not exist. `--capture` is incompatible with `--headless`. This avoids adding a simulate override to the presentation owner, which deliberately starts with zero pre-roll.


To open an explicit diagnostic mission through the same game host (viewer build):

```powershell
.\build\viewer\saga_mission.exe --data .\extracted\full --quest 1 --scene .\research\exports\mission-data-scene\quest-native-init.txt --fps 25 --debug 0 --rng system --seed 42
```

W/up and S/down move;1/2/3 select/activate attacks, F or left click activates the selected attack, Enter sends the result confirmation, Space freezes diagnostic execution, N advances one frozen step and Escape closes. With loaded Init and stage_loop, P pauses/resumes the native mission, Q while paused requests retreat, and Y/N answers the pending exit choice shown in the window title. `--replay` disables live gameplay input. These controls use the same frontend as `queue_capture`, rather than a second gameplay loop. Initial party, progress, positions and resource/material bindings remain provisional scenario inputs; selecting a different quest alone does not invoke Stage.Start or restore a save. Add the explicit entry rows below to run Stage.Start.

Add `system_init 1` to a fully supplied Human/Input/map scene to run the existing SystemManager.Init dispatcher through all32 native factory indices. The byte is a provisional loaded-world declaration, not a save-file decoder;1..255 take the native loaded branch and0 is rejected. Original input and enemy Init run before their explicit mid-mission snapshots are applied. Map loading follows Init, so Cursor's initial saved-position observation follows native ordering. Reports distinguish `init_schedule_complete` from `update_schedule_complete` and expose `init_order`, `init_world_loaded_byte`, `init_manager_paused`, `init_town_return_registered`, `submenu_button_events` and `menu_button_events`. Profiles without this row retain their selected-constructor initialization. The [loaded-mission example](subsystems/integration.md#loaded-mission-initialization-s1-a3) supplies reproducible real-data commands for100/401 ticks.

`python -I tools/extract_assets.py "game/<maintainer.apk>" --out extracted/full --mission-initialization` derives the three consumed system/event seeds without media re-extraction. Add `mission_initialization` and use `mission_state new_game initial 0`, `quest_frame 0 initial` and `mission_events new_game 0 0 0`. The three event flags remain the selected effective locale/UI context. The local quest-initial-context.txt profile uses `runtime initial initial system 42`; matching CLI --fps initial/--debug initial select30/0. RNG/backend/seed remain explicit. The system and normal-save namespaces are separate; this does not load a binary save. See [context binding](subsystems/integration.md#initial-system-and-event-context-s1).

`mission_state new_game 10000 0` is the local explicit Yugami/input context for the native system-state provider. It replaces eleven rows in quest-native-party.txt: Human, rewards, Enemy/static population, notifications, collection, stage loop/events, input, boss and formation. The new_game_save, character/shop/gear providers, geometry/map, resources/presentation and quest_frame context remain required. Init-created arrays and native Stage.Start/Enemy.OnStart determine gameplay state; values are not copied from a built-in scene preset. The local quest-native-state.txt profile exercises it; see [native state integration](subsystems/integration.md#native-mission-system-state-s1). FPS/DEBUG, tick clock, Yugami, language/font configuration and initial held keys are explicit provisional context, not observed original runtime settings.

`party_formation new_game` preserves the actual MemberSystem constructor/Init formation through Stage.Start, including the slot10 assignment of the first table main character. It replaces the sixteen-number snapshot row and requires hero fresh/system_init0/system_construct. It does not select characters from discovery/recruitment flags. The local quest-native-party.txt profile composes it with new_game_save and the character/shop/gear reset providers. Reports expose native_party_formation and the actual sixteen-slot party_formation array. Original-data reference: hero0 at slot10 starts at(312,0,852), HP28/MP22. Configuration and the other scenario boundaries remain explicit; this is not scene-free entry.

For native stage entry add `stage_start 0 null`, `party_formation` with exactly16 saved slots, and `sorties <character-id> <saved-count>` for each started member. Keep `system_init 1`, selected quest/map, input, enemy, collection and result providers. `actor_state` overrides are rejected in this mode. The owner starts with an empty roster, runs all32 Init calls, then calls native Start: cleanup, timer, one map load, party/enemy creation, drop placement and button/event resets. The initial hero, saved progress/gear, formation, sortie counts, null area, stage index, Yugami and fade remain declared scenario inputs. DEBUG auto-party services fail explicitly. `quest-stage-start.txt` is the local real-data entry profile; replace the scene argument in the window command above to use it.

The local walking.txt profile enables Human Init/Update before Move/Animation/Render/Garbage, sets the real hero to walking state1 with initial Z speed -1, uses explicit gear level1 and inactive attack state0, and supplies geometry/quest bounds. Replace effects.txt with walking.txt in the command above; 120 passes move the hero from z1000 to z852.819824 and consume five recovery RNG draws. For the GPU window, run `build/viewer/queue_capture.exe research/exports/mission-data-scene/walking.txt 600 220 --interactive`. These initial state/geometry/gear choices are provisional scenario inputs, not a decoded Android save or completed quest bootstrap. The shooting.txt profile enables actual attacks, flight, trails, terminal visuals and nonlethal HP damage. It also loads AppData19/20 bullet/effect archives. Reports include shots, total_damage, HP/MP, live projectiles and pending drops. Permanent result grants require stage_loop and the complete result-state rows; collection fresh binds pickup and stage accumulation; stage_notifications binds main death and boss notification, and the collisions row enables pairwise contact/area delivery and enemy_system additionally enables native Enemy5 through the configuration below.

The local rewards.txt profile adds `rewards 0 1 0`, Treasure.txt loading and AppData7 (`28_quest.bin`), and selects hero level10 versus ordinary Enemy2 level1. Run the headless command above with rewards.txt and `--frames 240`, or open `build/viewer/queue_capture.exe research/exports/mission-data-scene/rewards.txt 600 220 --interactive`. Actual lethal damage, saved defeat counting, pending EXP/treasure factories, landing debris and corpse cleanup execute through existing core modules. The headless/window reports agree on one defeated enemy and a pending EXP drop value2; no collection or character/inventory grant is claimed. Without an explicit rewards row, reached lethal damage fails at the defeat boundary; main death and boss notification require stage_notifications; stage-result.txt separately exercises clear/completion and held return. See [integration](subsystems/integration.md#table-backed-defeat-pending-rewards-and-emission-s1-a3) for observations and remaining bindings.

The enemy-active.txt profile adds all three enemy configuration rows and disables attacks: Enemy2 activates and walks during Human4 -> Enemy5 -> Projectile10 -> Move11. Replace rewards.txt with enemy-active.txt and run 120 passes. enemy-population.txt explicitly enables spawning and produces two additional targeted enemies with four RNG draws in 120 passes. enemy-combat.txt enables attacks against the moving enemy and leaves pending EXP value2 after 240 passes. These rows supply provisional quest/save state, not a decoded quest selection or Android startup; selected raw enemies keep null initial targets, while generated enemies use native SearchTarget. Open any profile with the same queue_capture --interactive command. Reports include enemy_system_enabled, enemy_created, enemy_loop_counter and live enemy states/flags/targets. Collision12 is opt-in through the collisions row; retry/recovery remains unsupported; stage_loop/result state separately enables clear/results and held return; collection fresh alone enables the collection prefix. See [integration](subsystems/integration.md#table-backed-enemy-init-frame-and-population-s1-a3).

collision-combat.txt adds the no-field collisions row to the moving-enemy combat profile; run 240 passes to observe real contact damage/recoil, enemy defeat and pending EXP. collision-area.txt uses hero0/character17 and two stationary targets; run 160 passes to observe native range2 impact areas damaging the lateral target. The area diagnostic omits Enemy5; the synthetic regression also covers areas with Enemy5 active. Both profiles run through queue_capture --interactive or --run-frames with matching reported results. collision_events counts successfully delivered pair events; area_collision_events counts area/enemy pairs and includes misses. Native area lifetime2 is preserved after Render/Garbage ordering. No extra radius, hit deduplication or cell-index dependency is supplied. See [integration](subsystems/integration.md#live-collision-and-neighboring-impact-damage-s1-a2a3).

stage-death.txt supplies `stage_notifications 1 0 0 0 0 655360 -1`; run 160 passes to observe lethal contacts, landed humans and the native pending game-over/retry notification. stage-boss.txt marks the target as a boss and supplies `stage_notifications 1 0 0 2 0 655361 -1`; run 240 passes to observe boss-defeated/max-Z stores, victory presentation observations and key releases. Reports include stage_state/frame, boss_defeated, max_z_rate, held_keys, retry_dialog and ordered notification_events. Form objects/stacks, BGM output and missing QuestForm control lookup are documented service stubs; no dialog is drawn, answer selected, currency spent or complete Stage Update executed. Actor passes continue while the notification remains pending. Main death without the row fails at game-over; sub-character death still dispatches native event30/retarget. See [integration](subsystems/integration.md#stage-death-and-boss-notifications-s1-a3a4).

collection-combat.txt adds collection fresh to the live combat profile; run 240 passes to observe stage EXP2, expired pickup sources and unchanged human_experience0. collection-types.txt explicitly places money7 and a prisoner drop and reaches stage totals2/7/1. collection-boss.txt places real Treasure34 and collects it after actual boss landing, setting reward_box_picked_up and one jingle. The same queue_capture loop commands run these profiles. Fresh collection inputs are provisional; native Stage startup/QuestData placement and save restoration remain separate. Numeric type3 effects retain their original components/lifetimes but are excluded from presentation; the common pickup flash renders. Sound/jingle are output stubs. Reports add stage_rewards, collection_steps, collecting_drops, reward_box_picked_up, collection_sounds/jingles and numeric_effects_live. No character EXP, inventory or currency store is permanently credited. See [integration](subsystems/integration.md#drop-collection-and-stage-accumulation-s1-a3a4).

Selected mission steps use the ported SystemManager.Update dispatcher with all26 explicitly bound adapters, including the native inert Battle2 pass. Reports expose bound_update_systems/unbound_update_systems/last_update_order/update_schedule_complete and battle_steps. Update completeness is true; optional scenario providers control the existing gates. The system_init row independently selects complete loaded-world initialization. Timing measurements are excluded from reports. See [selected dispatch](subsystems/integration.md#selected-movement-pass-s1-a3).

Every selected mission step also executes Exp9, Parameter14 and Effect23 at their native relative positions. Fresh parameter queues are empty; no reward or save producer is inferred. Reports expose parameter_experience_steps/parameter_steps/parameter_experience_pending, effect_steps and the live non-depth effect_updates observations. Held return stops these passes with the other post-Stage logic. Specialized effect drawing remains a presentation stub; see [integration](subsystems/integration.md#parameter-and-non-depth-effect-passes-s1-a3).

Input-enabled profiles add `input fresh 0 0 0 0 0 0 0 0 0` for explicit zero held/move-front/button state and last-active0. The current human_system attribute must be0..2 with its selected button state0. `queue_capture --interactive` shares the mission controls above, including P/Q/Y/N for loaded Stage pause/retreat and Enter for result confirmation. Space freezes diagnostic execution, N steps when no exit choice is pending, Esc closes. These controls use native Input/Stage/Human kernels. The fresh logical Canvas provider is unlocked; Surface drawing/native lock selection remains a documented service boundary.

Both `saga_mission` and native `queue_capture` commands accept a trailing `--replay inputs.txt`. Example decimal logical actions:

```text
2 keys 131072
70 keys 0
90 keys 524288
110 keys 0
120 attack 0
130 cursor -88 341
200 pulse 1048576
```

Indices include initial simulate passes; prefer simulate0 for a replay starting at the first visible logical tick. Same-step actions preserve file order; an empty replay disables physical gameplay input. `keys` is an absolute uint32 mask, `attack` is0..2; `pulse` ORs a one-tick uint32 mask into Stage input and requires stage_loop. It is separate from held keys. Replay is limited to8MiB/100000 actions and has no implicit state defaults, recording or complete-state hash. Replay text is ASCII/invariant (UTF-8 without BOM); comments occupy whole lines beginning with #. Input controls beginning at native frame0 preserve the original no-acceleration quirk until another press; beginning at logical tick2 permits the normal FPS-based dash. See [input integration](subsystems/integration.md#native-movement-attack-and-input-replay-s1-a2a3a5).

Loaded `system_init` plus `stage_loop` also accepts `step answer <signed-int32>` for a pending exit dialog. Pause/resume/request-exit pulses are decimal33554432/67108864/134217728. Example: `2 pulse 33554432`, `3 pulse 134217728`, `10 answer 1`, `11 pulse 67108864` pauses, opens exit, waits, cancels and resumes. Answer0 confirms retreat; every other signed value cancels. No response is inferred from the dialog's initial answer field. Early/duplicate answers fail; retry crystal/ad choices remain unbound. The local pause-retreat.replay and quest-pause-retreat.txt profile exercise both cancellation and confirmation with real Quest1 data.

The local spatial-walking.txt profile adds AppData18 from decoded 19_chip.bin, mission_map fresh with selected chip IDs0/1/2/3, and geometry312/312/12 matching distance10. Use the headless command with --frames240 to observe native map tiles, following camera, cell migration and selected visibility. The same queue_capture --interactive/--run-frames commands render that mode. View width/height are explicit effective GameView dimensions; native LoadMap/Camera replace the view row's camera offsets. Fresh camera-save scratch is written at slots17/18 by LoadMap before reading; no original save is restored. Reports add spatial_enabled, camera, mission_map, camera_steps/cell_steps/visibility_steps, cell_change_events, visible_count/visible_entities and selected actor cell_xi/cell_yi. The last visible set can contain entities destroyed afterward; each reported entry includes its destroyed flag. See [spatial integration](subsystems/integration.md#mission-map-following-camera-and-cell-visibility-s1-a1a3).

With `mission_map fresh`, replay `step cursor x y` sends native event2 in **world-isometric** signed-int32 coordinates, not window pixels, before that step's system updates. It selects the current live Human group and updates native focus; it does not alter the camera center. Camera8 and Cursor24 separately produce cell-change event13 from saved camera coordinates. Reports expose `cursor`, `cursor_steps`, `cursor_touches`, `cursor_cell_events` and `cursor_last_cell_change`. Example `1 cursor -88 341` hits a Human whose position is (312,0,400). The selection ping is a presentation stub; physical pointer conversion/gestures remain unbound. Other replay actions retain their existing strict fields and ordering.

Entity-button lifecycle diagnostics use spatial mode and explicit rows `button parent x y resource clip frame group value dynamic autoDestroy scale offsetX offsetY uniqueValue`. Parent is an original selected-entity ordinal or-1; all flags are0/1 and all numbers are int32. Example `button 0 7 9 1 40 0 99 123 0 0 100 8 -9 0` creates a widget tied to selected actor0 with resource1/clip40, group99/value123. Text is null and text offsets0 by declared scenario policy. Button.Init/Update, shared IDs, native projection/hover/deletion and touch routing run; dedicated image/text output and the native quest producer remain unbound. At most100000 button rows are accepted. Replay `1 touch 13 99 123` delivers a normalized event9 before tick1, including to Input/Stage/Button listeners; types/groups/values are signed int32 and receive no invented defaults. Type13 sets the first matching entity button's hover; later unmatched events still send nullable event11. Physical Surface hit testing is separate. Reports expose `button_steps`, `button_events`, `button_removals`, `last_button_target`, `last_button_event_type` and `entity_buttons`.

Information-controller diagnostics add `info_popup id category type priority single height duration` or `info_slide id width height duration` to a table-backed scene. All values are int32; single is0/1, duration-1 means infinite wait. Other dimensions/content are explicit provisional provider inputs, not original quest bar values. Each list accepts at most100000 rows. Fresh bar fields are zero with the supplied ID; popup insertion is forced, so no unknown concrete equality is inferred. Native base clocks, category/priority layout, disposal and removal run, but dedicated bar text/graphics output remains a stub. Reports expose `info_steps`, `info_clicks`, `info_button_events`, `info_popup_disposals`, `info_popups`, `info_slides`, `info_emphasize_frame`, `info_emphasized_id`, `info_offset_bottom` and `info_dialog_pending`.

Speech-bubble diagnostics use `bubble parentOrdinal dataId type frameMax resource clip initialX initialY initialZ`. Parent -1 is null; other parent ordinals select the scene's pre-existing entity list. Integers are int32 and XYZ are finite binary32. Types0/1 select the native shared offsets(-36,0,-30)/(-36,0,-20); other types fail through that table contract. Frame maximum -1 is infinite; expiry uses strict greater-than. The resource/clip and initial geometry are explicit provisional inputs to CreateZObject + AddFukidashi, not recovered original bubble content or cell registration. Text output is stubbed. Reports expose `bubble_steps`, `bubble_destroy_calls`, `bubble_pending`, `bubble_seen`, live `bubbles`, `debug_visualization_steps` and `debug_visualization_flags`. The [integration example](subsystems/integration.md#live-speech-bubble-lifecycle-and-diagnostic-update-s1-a3) compares all122 shared headless/GPU fields at100 frames using real quest data.

Gauge diagnostics use `gauge step parentOrdinal type from to maximum image frameMax flags option option2`, at most100000 rows. Step0 creates before simulation; positive steps create immediately before that step's Update22, after the cell/render passes. Parent ordinals must be nonnegative; other fields are int32. Geometry and global resource23 are required; original factories supply resource23/clip0, default offsets, image arrays and immediate display. Headless mode decodes only the gauge resource through the same pinned stb_image2.30 source (revision003501f6, dual MIT/public-domain) used by the viewer. With flags bits1/2 present, option selects a nullable parameter and type4 selects its EXP ratio; otherwise native timed interpolation is used. Frame maximum -1 suppresses the clock; finite expiry is strict frame>wrap(frameMax+15). Reports expose `gauge_steps`, `gauge_destroy_calls`, `gauge_pending` and live `gauges` with position/cell/UVWH. These rows supply diagnostic call timing and state, not original quest producers; use a positive step when the selected parent's initial cells need the first native Cell17 pass.

Replay `1 click -88 341` sends world-isometric signed-int32 coordinates through event4: Cursor's inert click receiver then Info's first-hit Monster16 selection. It requires spatial mode and existing input fresh, like cursor/touch replay; it is separate from event2 `cursor`. The current selected quest has no Monster16 entities, so click does not fabricate a popup for Human14/Enemy24. Native MonsterInfo/AttackNoticeBar constructors fail explicitly if reached until their providers are bound. Default manager pause=false represents the fresh selected world only; diagnostic execution pause is not a native manager pause. Physical pointer conversion, native content factories and original bar ID allocation remain open.

The local boss-entry.txt profile replaces the pre-created boss with native threshold-based entry: explicit stage_boss2/1, hero at(312,0,400), initial boss-started0 and resource_name quest7 for the native auxiliary record1. Run `build/viewer/queue_capture.exe research/exports/mission-data-scene/boss-entry.txt 600 640 --interactive` for manual controls, or use saga_mission with that scene, --frames450 and --replay research/exports/mission-data-scene/boss-entry.replay plus the existing FPS/DEBUG/RNG/seed arguments. The replay activates attack at1 and confirms at400; native return/cleanup occurs at401. See [boss integration](subsystems/integration.md#native-boss-entry-s1-a3a4).

The local stage-result.txt profile combines actual boss combat/collection with the native Stage.Update clear/result path. Run the headless command with that profile and --frames 240, or `build/viewer/queue_capture.exe research/exports/mission-data-scene/stage-result.txt 600 220 --run-frames 240`. Initial wallet/record/stock/recruitment, timer, already-started boss state, fresh event list and confirmation at step200 are explicit provisional inputs. Table.txt is loaded only when result state is requested. The native grant changes live member EXP/levels; OnFinish writes decoded saved state after cleanup. The hold transition policy stops the old mission before later actor passes and leaves the town callback pending; frames/logic_steps count executed ticks, movement_steps counts Move calls, and frames_requested remains the requested bound. It does not open the town or write a save. See [result integration](subsystems/integration.md#stage-clear-results-and-deferred-return-s1-a4).

Use the extractor's decoded textassets/*.bin output, not the XOR-obfuscated APK member. Language folder and DPI index are explicit preparation inputs. The text profile is ASCII/invariant, Config selector 2, no pack overrides; all shipped member names/catalogs fit it. The selected PNG, OPT and standard resolution come from the catalogs, so --opt/--standard cannot override them. SEB u/v/c and image bin/WxH/q+N/q-N options are supported. q is a relative material queue offset; unknown image tokens such as a1 are retained in JSON and do not alter the image; actual @ atlas and other recognized unbound services fail before capture. --resource now prepares every present image in the selected catalog, so unrelated missing/corrupt files also prevent a full-ready capture. In the maintainer's local copy, enemy container clip6/frame0/layer0 and character container clip41/frame0/layer1 render; enemy clip4 and character clip42 also apply u, and effect clip101 applies q+1 with material queues 2451/3001. JSON reports ready SEB/image counts, reversal flags, render queues and unrecognized image tokens. Native async/cache/bootstrap and mission resource population remain separate work.

`--opt` and `--origin` are optional; `--standard <width> <height>` supplies explicit Image standard dimensions (zero by default). In manual --sprite mode, supply the PNG matching the selected SEB TexId yourself. The --resource mode resolves selected archive catalogs, while mission manager population and effective runtime locale remain separate. Canvas is required and limited to 8,192 pixels/axis and 16,777,216 pixels total. The profile is an identity world matrix with the native local matrix stack, no atlas, zero destination expansion, native texture blend/color passes and no UI overlays; JSON reports the selected sprite, image alpha classification, shader keys and emitted quad count. SEB horizontal/vertical mirrors and their OPT piece translations use native matrix scopes. `--angle <degrees>` supplies the original command rotation; append `--skew` to select its tangent-shear branch. `--alpha <int32>` supplies the original encoded command factor (default 255); `--color <int32>` supplies packed ARGB tint (default -1, disabled). Mode-0 factors dim RGB; mode-1 factors change opacity. `--background <r> <g> <b> <a>` supplies explicit 0..255 clear channels. `--raw-rgba` selects the earlier diagnostic replacement profile and cannot be combined with alpha/color overrides. Split alpha passes use retained masks with nearest sampling; capture reads an explicit RGBA8 canvas. Unbound local-coordinate/forced-material/grayscale/stencil/external-image paths fail before capture. Captures must target new files. `sprite_preview_pixels` is registered in CTest when SAGA_BUILD_VIEWER=ON; its PNG/SEB/OPT fixtures are synthetic and use only Python's standard library. For this mode, `--headless` uses an invisible graphics-backed window plus SDL readback: a desktop video/graphics driver is required. SDL 3.4.18 software geometry truncates source UV texels and is refused here; ordinary PNG/occupied-bounds OPT capture still uses its original software path. No Android comparison or playable mission is implied.
