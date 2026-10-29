# About

Reverse engineering of *Magician's Saga* (Kairosoft, Android, version 1.4.0) towards a **native, behaviourally faithful port for Windows and Linux** in C++17, with a portable core meant for a later Nintendo 3DS version.

The shipped BuildSettings identify Unity 2022.3.62f2 / IL2CPP: its logic is ARM64 code in `libil2cpp.so`, from the assemblies `Assembly-CSharp.dll` and Kairosoft's in-house engine `KairoLibrary.dll`. Subsystems are documented precisely from that code and then reimplemented as tested C++ modules.

[Roadmap](docs/ROADMAP.md), [subsystems](docs/subsystems), [file formats](docs/formats), [progress rules](docs/PROGRESS.md), [setup](docs/SETUP.md), [working method](AGENTS.md)

## Status

[![functions](https://tim-campy.github.io/magicians-saga-port/badge-functions.svg)](https://tim-campy.github.io/magicians-saga-port/progress.html) [![formats](https://tim-campy.github.io/magicians-saga-port/badge-formats.svg)](https://tim-campy.github.io/magicians-saga-port/progress.html)

[![progress map](https://tim-campy.github.io/magicians-saga-port/progress.svg)](https://tim-campy.github.io/magicians-saga-port/progress.html)

<sub>Percentages count the 10,959 methods with native code in the two game assemblies (`re/functions.csv`) and the formats in `re/formats.csv`, at the `documented` stage. Unity, mscorlib and other third-party assemblies are not counted.</sub>

Milestone **M3** (first playable loop) is current. The desktop scene host runs selected native gameplay systems with real extracted tables, sprites and map tiles. Keyboard/mouse controls and ordered replays reach the actual movement, dash, attack, combat, collection and clear-result rules. All26 Update adapters and optional32-system loaded-world Init reach the host; loaded profiles also bind native pause/resume and explicit retreat confirmation through cleanup and held return. Complete quest selection/bootstrap, retry/modal form flow, town return and save-file loading remain open. C++ contract and integration tests run on Windows and, in CI, on Linux; no behavioural comparison with the running Android game is claimed.

A table-backed scene preview creates actual character/enemy entities from decoded catalogs and renders their clothing, hats, weapons and animations. `saga_mission` (viewer build) and `queue_capture --interactive` share the SDL/OpenGL window and pause/frame-step controls. A `simulate` profile runs native movement, emission and effect expiry around animation/rendering; `human_system` additionally runs the real Human frame with table-backed walking, recovery and explicit FPS/RNG. Shooting adds MP consumption, flight, terminal damage and impact/shake; explicit `rewards` inputs enable enemy defeat, pending EXP/treasure drops, landing debris and corpse cleanup. Explicit enemy profiles run native Init/Update, population, targeting, activation and recoil. The collisions row adds native pair detection, ordered contact/recoil and neighboring impact-area delivery. Explicit stage_notifications inputs connect main-character game-over and boss defeat/key release, with presentation service stubs. collection fresh adds native pickup, stage totals and boss-box gating. stage_loop/result inputs bind clear confirmation, permanent grants, equipment upgrades, quest completion, cleanup and decoded character writeback. stage_boss supplies explicit quest fields for native boss/marker/fence creation at the progress threshold. mission_map fresh adds native map tiles, following camera, live cell migration and visibility selection. input fresh adds native forward/reverse and attack controls through SDL or logical --replay. The opt-in stage_start scenario runs native map/party/enemy/drop entry; mission_resources/material_policy select verified extracted archives and ordinary native materials automatically; Retry handling and automatic quest/save selection remain unbound; `saga_mission --data/--quest` selects archives and the quest table row while retaining explicit `--scene` state; system_init profiles run the32 native initializers for an explicitly declared loaded world; town transition is held. The always-built `saga_mission --headless` runs the same selected scene steps without SDL, decoding only gauge PNGs needed by Gauge22 geometry; quest bootstrap and complete-state hashing remain open. See [setup](docs/SETUP.md#headless-mission-host) and [result integration](docs/subsystems/integration.md#stage-clear-results-and-deferred-return-s1-a4).

| Work package | State |
| --- | --- |
| 1. Events and scripts | [Event pipeline](docs/subsystems/events.md) from Event.txt to effects: executor, delayed-event queue, dispatcher of the 8 effect opcodes in use and the progression effects (money, crystals, areas, missions, stamina) are ported; condition predicates and the id-indexed loader are open. |
| 2. Game data and saves | [Data tables](docs/subsystems/data.md) (UTF-8, id-indexed with gaps) and the [save](docs/subsystems/save.md) structure are mapped; the [script payload](docs/formats/script-payload.md) codec is ported. The save envelope (`SecureProperty`, CRC64) is not decoded yet. |
| 3. Combat, RNG and rewards | [Combat](docs/subsystems/combat.md) damage, flight, collisions, impacts and temporary areas have tested ports; [rewards](docs/subsystems/rewards.md), progression and town-return modules are available. Live groups compose with hit/expiration tests; hero bootstrap, stored/debug mission-entry party assembly, roster/formation edits and member reset use the actual human factory and 13 base parameters, with explicit data/RNG/ranking services; stage Start now composes entry/reset, party creation, placed items and collection credit through explicit map/enemy/audio services; actual reward drops pass through party/front/player selection, 31-step collection, stage credit, effect creation and Garbage removal. A public seeded [RNG](docs/subsystems/rng.md) backend is ported; the original live generator selection/seed remains open. |
| 4. ECS world and animation | [ECS](docs/subsystems/ecs.md) slots, filters, dynamic groups, registration/IDs and world teardown are ported; all26 Update adapters are bound and tested, with ordered32-system Init available for a declared loaded world; fresh-world bootstrap remains open. Effect/drop factories use concrete Cell/Image payloads and effects insert playback; the [animation](docs/subsystems/animation.md) Init/Update loop runs on live fields and explicit clip clocks. Header-derived SEB clocks feed effect durations and an offline animation scenario; complete catalog/file selection, global frame flags and rendering remain open. |
| 5. Text and fonts | [Text](docs/subsystems/text.md) preparation, split/trim, argument substitution, markup parameters, reveal timing and fitting are ported; [fonts](docs/subsystems/fonts.md) state is ported, the rendering backend is open. |
| 6. Boot, forms and frame loop | [Boot](docs/subsystems/boot.md) path, frame and pause gates, [form](docs/subsystems/ui.md) events are mapped statically. |
| 7. Assets | [Asset pipeline](docs/subsystems/assets.md): the [Jar containers](docs/formats/jar-container.md), [SEB](docs/formats/seb.md) and [OPT](docs/formats/opt.md) are rebuilt byte-exactly; Unity textures, audio and fonts are extracted; C++ readers and a technical viewer exist. |
| 8. Platform | [Java/native boundary](docs/subsystems/platform.md) and Android services the PC host must replace. |

## Tools

The [GPU bitmap adapter](docs/subsystems/assets.md#gpu-bitmap-submission) now draws native-Z sprite commands through recovered quad insertion, mesh ordering and layer depth clears. The shared combat scenario feeds it for 240 frames (synthetic catalogs), and queue_capture --native-depth also accepts actual decoded archives with explicit projection/material inputs. Its OpenGL 3.3 tests cover overlap, alpha discard, blending and viewport state. Production mission ownership and camera/resource bindings remain open.

Python 3 (standard library unless noted) and C++17 with CMake. Each tool has tests on synthetic data.

| Tool | Purpose |
| --- | --- |
| [`tools/inspect_apk.py`](tools/inspect_apk.py) | Read-only APK inventory. |
| [`tools/inspect_il2cpp_metadata.py`](tools/inspect_il2cpp_metadata.py) | Read the IL2CPP v31 metadata catalogue. |
| [`tools/map_il2cpp_methods.py`](tools/map_il2cpp_methods.py) | Map metadata method tokens to ARM64 code addresses. |
| [`tools/extract_assets.py`](tools/extract_assets.py) | Extract and verify the TextAsset containers and, with `--unity-media`, textures, audio and fonts; `--culture-signs` adds native number-format signs and `--culture-names` adds name lookup records; `--culture-neutral` also derives the neutral mapping; --culture-lower includes nine UTF-16 lowercase arrays. |
| [`tools/inspect_seb.py`](tools/inspect_seb.py), [`tools/inspect_opt.py`](tools/inspect_opt.py) | Read and rebuild SEB sprites/animations and OPT texture grids. |
| [`tools/inspect_unity_assets.py`](tools/inspect_unity_assets.py) | Catalogue the Unity serialized resources. |
| [`tools/inspect_event_table.py`](tools/inspect_event_table.py), [`tools/run_event_scenario.py`](tools/run_event_scenario.py) | Read Event.txt; run the native event kernels on it. |
| [`tools/inspect_presentation_table.py`](tools/inspect_presentation_table.py) | Read the talk and event-message tables. |
| [`tools/inspect_arm64_calls.py`](tools/inspect_arm64_calls.py), [`tools/inspect_unwind_ranges.py`](tools/inspect_unwind_ranges.py), [`tools/inspect_exception_callsites.py`](tools/inspect_exception_callsites.py) | ARM64 call inventory, FDE ranges and LSDA call sites of `libil2cpp.so`. |
| [`tools/registry.py`](tools/registry.py) | Build, update, check and search the method registry. |
| [`tools/progress.py`](tools/progress.py) | Progress numbers, treemap, badges and history from the registries. |
| [`tools/ghidra/`](tools/ghidra) | Ghidra headless scripts (bounded instruction export, JNI table, bootstrap). |
| `src/platform/desktop/` | `asset_inspect`, `event_trace`, `animation_trace` (selected SEB -> effect -> live frame counters) and the optional SDL3 `asset_viewer`. |

## Building

```powershell
tools\build-desktop.cmd            # Windows (Visual Studio C++ tools): build and run CTest
python -I tools/run_tests.py       # Python tests
```

Native culture signs can be extracted from the same hash-verified Magician's Saga 1.4.0 APK using `uv run tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/culture-enabled --culture-signs`. Check all output sizes and hashes with `uv run tools/extract_assets.py --check --out extracted/culture-enabled --culture-signs`; this also requires the culture inventory to be present. Use `--culture-names` instead to include the name/culture index and extend the pool for name lookup; use the same flag with `--check` to require those files. `--culture-neutral` additionally derives the 130 neutral-name/LCID entries from the supplied binary and metadata. `--culture-lower` includes all preceding culture outputs and nine invariant UTF-16 lowercase arrays (4,270 bytes); the same flag with `--check` requires the complete 14-file culture inventory. Use a fresh output directory when enabling new options. See [setup](docs/SETUP.md) and [format](docs/formats/culture-number-signs.md).

Linux: `cmake -S . -B build/linux -DBUILD_TESTING=ON && cmake --build build/linux && ctest --test-dir build/linux`. Extraction and the local layout are described in [SETUP](docs/SETUP.md).

## Disclaimer

This project is intended for research, preservation and interoperability. It does not include, distribute or require copyrighted game data: no APK, native libraries, metadata, extracted assets, text dumps, saves or decompiled code. Findings are recorded as knowledge (layouts, offsets, algorithms, names and small illustrative values). Users must obtain their own copy of the game and use it in accordance with applicable laws.

This is an independent project and is not affiliated with, authorized or endorsed by Kairosoft Co., Ltd. "Magician's Saga" and "Kairosoft" are trademarks of their respective owners.

## License

GNU General Public License version 2 only. `tools/progress.py` adapts parts of [AnyPS5](https://github.com/boykopovar/AnyPS5)'s progress tool, which is GPL-2.0.
