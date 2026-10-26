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
uv run tools/extract_assets.py game/MagiciansSaga-1.4.0.apk --out extracted/full --unity-media
```

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

Linux (also what CI runs):

```sh
cmake -S . -B build/linux -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/linux --parallel && ctest --test-dir build/linux --output-on-failure
```

Desktop executables: `build/desktop/asset_inspect.exe <container.bin>` (JSON index of a decoded container), `build/desktop/event_trace.exe` (event scenario; see `docs/subsystems/events.md`), `build/viewer/asset_viewer.exe <png|opt>` (technical viewer, Escape closes).

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

The runtime Hashtable table is loaded with `load_hashtable_primes(extracted_root)` and shared by `RuntimeCallContextHost`. After its manifest check, run `build/desktop/test_runtime_data.exe extracted/runtime` to exercise growth and full mutable-context copying with local data. The host requires an explicit correlation-value clone callback; scheduler/TLS state is not inferred. See [runtime table format](formats/runtime-hashtable-primes.md).
