# About

Reverse engineering of *Magician's Saga* (Kairosoft, Android, version 1.4.0) towards a **native, behaviourally faithful port for Windows and Linux** in C++17, with a portable core meant for a later Nintendo 3DS version.

The game is a Unity 6 / IL2CPP build: its logic is ARM64 code in `libil2cpp.so`, from the assemblies `Assembly-CSharp.dll` and Kairosoft's in-house engine `KairoLibrary.dll`. Subsystems are documented precisely from that code and then reimplemented as tested C++ modules.

[Roadmap](docs/ROADMAP.md), [subsystems](docs/subsystems), [file formats](docs/formats), [progress rules](docs/PROGRESS.md), [setup](docs/SETUP.md), [working method](AGENTS.md)

## Status

[![functions](https://tim-campy.github.io/magicians-saga-port/badge-functions.svg)](https://tim-campy.github.io/magicians-saga-port/progress.html) [![formats](https://tim-campy.github.io/magicians-saga-port/badge-formats.svg)](https://tim-campy.github.io/magicians-saga-port/progress.html)

[![progress map](https://tim-campy.github.io/magicians-saga-port/progress.svg)](https://tim-campy.github.io/magicians-saga-port/progress.html)

<sub>Percentages count the 10,959 methods with native code in the two game assemblies (`re/functions.csv`) and the formats in `re/formats.csv`, at the `documented` stage. Unity, mscorlib and other third-party assemblies are not counted.</sub>

Milestone **M3** (first playable loop) is current. No playable build exists yet: the C++ modules reproduce documented contracts in isolation and are tested on Windows and, in CI, on Linux.

| Work package | State |
| --- | --- |
| 1. Events and scripts | [Event pipeline](docs/subsystems/events.md) from Event.txt to effects: executor, delayed-event queue, dispatcher of the 8 effect opcodes in use and the progression effects (money, crystals, areas, missions, stamina) are ported; condition predicates and the id-indexed loader are open. |
| 2. Game data and saves | [Data tables](docs/subsystems/data.md) (UTF-8, id-indexed with gaps) and the [save](docs/subsystems/save.md) structure are mapped; the [script payload](docs/formats/script-payload.md) codec is ported. The save envelope (`SecureProperty`, CRC64) is not decoded yet. |
| 3. Combat, RNG and rewards | [Combat](docs/subsystems/combat.md) entry, hit test, damage pipeline and affinity, [rewards](docs/subsystems/rewards.md) from defeat to the quest result and town return; parameter subtraction, affinity and stock grants are ported. The [RNG](docs/subsystems/rng.md) generator is open. |
| 4. ECS world and animation | [ECS](docs/subsystems/ecs.md) entities, component slots, filters, enumeration and lifecycle; [animation](docs/subsystems/animation.md) transitions and payload codecs are ported. |
| 5. Text and fonts | [Text](docs/subsystems/text.md) preparation, split/trim, argument substitution, markup parameters, reveal timing and fitting are ported; [fonts](docs/subsystems/fonts.md) state is ported, the rendering backend is open. |
| 6. Boot, forms and frame loop | [Boot](docs/subsystems/boot.md) path, frame and pause gates, [form](docs/subsystems/ui.md) events are mapped statically. |
| 7. Assets | [Asset pipeline](docs/subsystems/assets.md): the [Jar containers](docs/formats/jar-container.md), [SEB](docs/formats/seb.md) and [OPT](docs/formats/opt.md) are rebuilt byte-exactly; Unity textures, audio and fonts are extracted; C++ readers and a technical viewer exist. |
| 8. Platform | [Java/native boundary](docs/subsystems/platform.md) and Android services the PC host must replace. |

## Tools

Python 3 (standard library unless noted) and C++17 with CMake. Each tool has tests on synthetic data.

| Tool | Purpose |
| --- | --- |
| [`tools/inspect_apk.py`](tools/inspect_apk.py) | Read-only APK inventory. |
| [`tools/inspect_il2cpp_metadata.py`](tools/inspect_il2cpp_metadata.py) | Read the IL2CPP v31 metadata catalogue. |
| [`tools/map_il2cpp_methods.py`](tools/map_il2cpp_methods.py) | Map metadata method tokens to ARM64 code addresses. |
| [`tools/extract_assets.py`](tools/extract_assets.py) | Extract and verify the TextAsset containers and, with `--unity-media`, textures, audio and fonts; `--culture-signs` adds native number-format signs and `--culture-names` adds name lookup records; `--culture-neutral` also derives the neutral mapping. |
| [`tools/inspect_seb.py`](tools/inspect_seb.py), [`tools/inspect_opt.py`](tools/inspect_opt.py) | Read and rebuild SEB sprites/animations and OPT texture grids. |
| [`tools/inspect_unity_assets.py`](tools/inspect_unity_assets.py) | Catalogue the Unity serialized resources. |
| [`tools/inspect_event_table.py`](tools/inspect_event_table.py), [`tools/run_event_scenario.py`](tools/run_event_scenario.py) | Read Event.txt; run the native event kernels on it. |
| [`tools/inspect_presentation_table.py`](tools/inspect_presentation_table.py) | Read the talk and event-message tables. |
| [`tools/inspect_arm64_calls.py`](tools/inspect_arm64_calls.py), [`tools/inspect_unwind_ranges.py`](tools/inspect_unwind_ranges.py), [`tools/inspect_exception_callsites.py`](tools/inspect_exception_callsites.py) | ARM64 call inventory, FDE ranges and LSDA call sites of `libil2cpp.so`. |
| [`tools/registry.py`](tools/registry.py) | Build, update, check and search the method registry. |
| [`tools/progress.py`](tools/progress.py) | Progress numbers, treemap, badges and history from the registries. |
| [`tools/ghidra/`](tools/ghidra) | Ghidra headless scripts (bounded instruction export, JNI table, bootstrap). |
| `src/platform/desktop/` | `asset_inspect`, `event_trace` and the optional SDL3 `asset_viewer`. |

## Building

```powershell
tools\build-desktop.cmd            # Windows (Visual Studio C++ tools): build and run CTest
python -I tools/run_tests.py       # Python tests
```

Native culture signs can be extracted from the same hash-verified Magician's Saga 1.4.0 APK using `uv run tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/culture-enabled --culture-signs`. Check all output sizes and hashes with `uv run tools/extract_assets.py --check --out extracted/culture-enabled --culture-signs`; this also requires the culture inventory to be present. Use `--culture-names` instead to include the name/culture index and extend the pool for name lookup; use the same flag with `--check` to require those files. `--culture-neutral` additionally derives the 130 neutral-name/LCID entries from the supplied binary and metadata. Use a fresh output directory when enabling new options. See [setup](docs/SETUP.md) and [format](docs/formats/culture-number-signs.md).

Linux: `cmake -S . -B build/linux -DBUILD_TESTING=ON && cmake --build build/linux && ctest --test-dir build/linux`. Extraction and the local layout are described in [SETUP](docs/SETUP.md).

## Disclaimer

This project is intended for research, preservation and interoperability. It does not include, distribute or require copyrighted game data: no APK, native libraries, metadata, extracted assets, text dumps, saves or decompiled code. Findings are recorded as knowledge (layouts, offsets, algorithms, names and small illustrative values). Users must obtain their own copy of the game and use it in accordance with applicable laws.

This is an independent project and is not affiliated with, authorized or endorsed by Kairosoft Co., Ltd. "Magician's Saga" and "Kairosoft" are trademarks of their respective owners.

## License

GNU General Public License version 2 only. `tools/progress.py` adapts parts of [AnyPS5](https://github.com/boykopovar/AnyPS5)'s progress tool, which is GPL-2.0.
