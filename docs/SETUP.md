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
| `build/` | CMake builds only (`build/desktop`, `build/viewer`). Safe to delete. | no |

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
tools\build-viewer.cmd                  # optional SDL3 viewer (downloads pinned SDL3/stb into build/)
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
build/viewer/test_combat_pixels.exe build/viewer/combat-contract
```

The GPU mode uses BitmapMeshFrame, the recovered group/array/quad kernels and native stream submission. Its pixel tests distinguish same-layer Z occlusion from next-layer depth clearing. test_combat_pixels exercises 240 frames of the shared combat scenario with synthetic catalogs; selected real archives can be supplied to queue_capture, but neither tool establishes original boot, complete mission resource selection or Android equivalence.

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
| human | direction, character ID, level, formation X/Z, EXP, position X/Y/Z |
| enemy | direction, enemy ID, level, flags, position X/Y/Z |
| bullet | bullet ID, damage, attribute, parent creation ordinal (-1 means null), position X/Y/Z |
| tick_ms | diagnostic animation/render interval1..1000; required for loop modes |
| simulate | initial movement + animation passes0..10000; enables MoveSystem before animation/render on every subsequent loop step; absent means animation-only |

Scene views select all constructed position/sprite entities as their explicit visible group. They run the recovered render producer and animations; actual mission visibility/camera/input and the complete combat schedule are not supplied by this diagnostic profile. A human's creation level and supplied saved level remain distinct from its table initial_level. Unknown saved values do not default to zero. The native Data.Load bootstrap, original resource-name resolver and save envelope remain separate work.

```powershell
build/viewer/queue_capture.exe scene.txt 600 220 --material-inputs
build/viewer/queue_capture.exe scene.txt 600 220 --interactive
build/viewer/queue_capture.exe scene.txt 600 220 --run-frames 12
```

--material-inputs reports only reached tuples and writes no capture. Assign explicit material identity rows before GPU execution. --interactive is a visible technical window: Space pauses, N advances one selected logic step while paused, Escape closes. --run-frames uses a hidden window and reports render/logic-step counts, movement_steps (including initial simulate passes), and final positions in entity creation order. With simulate 0, the enemy moves using its factory's table-derived velocity; humans start with zero velocity. Move integrates binary32 speed once per logic pass, without a delta-time multiplier. tick_ms controls this selected preview only; it does not establish the original frame timing or make the scene playable. The maintainer's profiles are local at research/exports/mission-data-scene/scene.txt (animation-only) and movement.txt (simulate 0); they require the supplied decoded archives.

Use the extractor's decoded textassets/*.bin output, not the XOR-obfuscated APK member. Language folder and DPI index are explicit preparation inputs. The text profile is ASCII/invariant, Config selector 2, no pack overrides; all shipped member names/catalogs fit it. The selected PNG, OPT and standard resolution come from the catalogs, so --opt/--standard cannot override them. SEB u/v/c and image bin/WxH/q+N/q-N options are supported. q is a relative material queue offset; unknown image tokens such as a1 are retained in JSON and do not alter the image; actual @ atlas and other recognized unbound services fail before capture. --resource now prepares every present image in the selected catalog, so unrelated missing/corrupt files also prevent a full-ready capture. In the maintainer's local copy, enemy container clip6/frame0/layer0 and character container clip41/frame0/layer1 render; enemy clip4 and character clip42 also apply u, and effect clip101 applies q+1 with material queues 2451/3001. JSON reports ready SEB/image counts, reversal flags, render queues and unrecognized image tokens. Native async/cache/bootstrap and mission resource population remain separate work.

`--opt` and `--origin` are optional; `--standard <width> <height>` supplies explicit Image standard dimensions (zero by default). In manual --sprite mode, supply the PNG matching the selected SEB TexId yourself. The --resource mode resolves selected archive catalogs, while mission manager population and effective runtime locale remain separate. Canvas is required and limited to 8,192 pixels/axis and 16,777,216 pixels total. The profile is an identity world matrix with the native local matrix stack, no atlas, zero destination expansion, native texture blend/color passes and no UI overlays; JSON reports the selected sprite, image alpha classification, shader keys and emitted quad count. SEB horizontal/vertical mirrors and their OPT piece translations use native matrix scopes. `--angle <degrees>` supplies the original command rotation; append `--skew` to select its tangent-shear branch. `--alpha <int32>` supplies the original encoded command factor (default 255); `--color <int32>` supplies packed ARGB tint (default -1, disabled). Mode-0 factors dim RGB; mode-1 factors change opacity. `--background <r> <g> <b> <a>` supplies explicit 0..255 clear channels. `--raw-rgba` selects the earlier diagnostic replacement profile and cannot be combined with alpha/color overrides. Split alpha passes use retained masks with nearest sampling; capture reads an explicit RGBA8 canvas. Unbound local-coordinate/forced-material/grayscale/stencil/external-image paths fail before capture. Captures must target new files. `sprite_preview_pixels` is registered in CTest when SAGA_BUILD_VIEWER=ON; its PNG/SEB/OPT fixtures are synthetic and use only Python's standard library. For this mode, `--headless` uses an invisible graphics-backed window plus SDL readback: a desktop video/graphics driver is required. SDL 3.4.18 software geometry truncates source UV texels and is refused here; ordinary PNG/occupied-bounds OPT capture still uses its original software path. No Android comparison or playable mission is implied.
