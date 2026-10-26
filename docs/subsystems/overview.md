# Overview

*Magician's Saga* 1.4.0 (Kairosoft) is a Unity 2022.3 / IL2CPP Android game whose rules live in two managed assemblies compiled to ARM64 native code. Identity, packaging, the Android/JNI bootstrap, the IL2CPP metadata catalogue and a complete method→address map are established statically. The original has never been run under observation; the port has asset readers and isolated rule modules, but no boot path, frame loop or playable build.

## Overview

### APK identity

| Field | Value |
|---|---|
| File | `Magician's+Saga_1.4.0_APKPure.apk` (provenance/signature not verified; "APKPure" in the name is not validation) |
| SHA-256 | `d58943e6ed4137975a554f1e210853f4a7b41573a38dbb462f4f5e159bf6eead` |
| Size | 64,345,815 bytes; 1,399 ZIP entries; 173,261,265 bytes uncompressed; all CRCs valid |
| Package | `net.kairosoft.android.wizardquest`, version `1.4.0`, code `140` |
| SDK min / target / compile | 24 / 35 / 35 |
| Launcher activity | `main.Main` (DEX) |
| DEX | `classes.dex`, `classes2.dex`, `classes3.dex` |
| ABIs | `arm64-v8a` (AArch64), `armeabi-v7a` (ARM32). Only ARM64 has been analysed |
| OBB | none in the ZIP (does not prove the game is complete offline) |

Each ABI ships six libraries: `libmain.so` (Unity bootstrap), `libunity.so` (engine; needs libmain, libandroid, liblog, libz, libEGL, libmediandk), `libil2cpp.so` (all compiled managed code), `libFirebaseCppApp-13_2_0.so`, `libFirebaseCppMessaging.so`, `libdatastore_shared_counter.so`. Hashes of the analysed members:

| Member | Bytes | SHA-256 |
|---|---:|---|
| `lib/arm64-v8a/libil2cpp.so` | 42,888,064 | `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f` |
| `lib/arm64-v8a/libmain.so` | 6,728 | `9348cd29bcdec5b677c7124b111a2ebb285bfd6550282619c8fd178f2982eb5d` |
| `assets/bin/Data/Managed/Metadata/global-metadata.dat` | 7,792,604 | `8f79c53ce22e97269f7ac7dd932cd4d80ed83169a5a61f9b9a85fc26d08a0cbd` |

Assets: 1,184 entries under `assets/` (41,154,362 bytes), 1,177 in `assets/bin/Data`. The ZIP has two case-only name collisions (`res/Iw.png`/`res/iW.png`, `res/Lr.xml`/`res/lR.xml`), so extraction on a case-insensitive filesystem must not write both to the same path.

Manifest: portrait orientation (`1`), `hardwareAccelerated=false`, `resizeableActivity=false`, `debuggable=false`, `extractNativeLibs=true`, `usesCleartextTraffic=true`. It declares 15 permissions (internet/network/wifi state, notifications, C2DM/FCM, wake lock, billing, install referrer, ad id and ad-services, foreground service) and components for FCM (`kairo.android.plugin.notification.FCMMessageService`), Google Mobile Ads, Tapjoy, BillingClient and AndroidX WorkManager. See [platform.md](platform.md).

### Engine and code layout

- Unity: `globalgamemanagers`/BuildSettings record **2022.3.62f2**; `unity default resources` records **2022.3.54f1**. Keep both; do not assume one generator version for every file.
- IL2CPP metadata **v31**: 64 images, 8,619 types, 63,273 methods; 56,955 methods map to distinct executable VAs, 6,318 slots are null. See [../formats/il2cpp-metadata.md](../formats/il2cpp-metadata.md).
- Game code (only these are in the function registry `re/functions.csv`):

| Image | Index | Types | Methods | Type range | Registry prefix |
|---|---:|---:|---:|---|---|
| `KairoLibrary.dll` (Kairosoft framework: forms, canvas, ECS base, IO, Java shims) | 1 | 645 | 6,771 | 1705–2349 | `kairo:` |
| `Assembly-CSharp.dll` (game) | 4 | 440 | 4,704 | 4111–4550 | `game:` |
| `Assembly-CSharp-firstpass.dll` (plugins; not in the registry) | 15 | 75 | 215 | 7747–7821 | — |

- The single build scene `Assets/Scenes/main.unity` (`level0`) holds GameObject `Main` (Transform, Camera, AudioListener, three scripts); MonoBehaviour 11 → MonoScript 883 → **`main.Main` of Assembly-CSharp**. That IL2CPP type (index 4166) is unrelated to the DEX launcher class of the same name. The scene also references MyTapjoy, FCMReceiver and TapjoyUnity components.
- `main.Main` derives from `kairo.unity.ui.IApplication` (the Unity MonoBehaviour); the frame loop is described in [boot.md](boot.md).
- TimeManager: fixed timestep ≈0.02 s, max ≈0.333333 s, time scale 1 — but `IApplication.Start` overrides fixedDeltaTime to 1.0 and `FixedUpdate` is empty; the game does not simulate at 50 Hz.

### Subsystem map

| Doc | Scope |
|---|---|
| [boot.md](boot.md) | Android startup, native bootstrap, `main.Main` callbacks, Unity lifecycle, frame gates, FPS policy |
| [platform.md](platform.md) | Java/JNI boundary, Android services, what the PC port must replace, Android oracle plan |
| [ui.md](ui.md) | Form manager, form events, pass counting, pause interface |
| [assets.md](assets.md) | Unity assets, resource reader/encryption, extraction, images, catalogues |
| [ecs.md](ecs.md) | Entity/component/system framework, lifecycle, save fields |
| [animation.md](animation.md) | SEB/INF animation, transitions, timing |
| [save.md](save.md) | Persistence entry points, save order and streams |
| [data.md](data.md) | Base data, data lists, game tables |
| [combat.md](combat.md) | Combat entry, hit, damage, easing |
| [rewards.md](rewards.md) | Defeat, drops, quest results, treasure, town return |
| [events.md](events.md) | Event tables, opcodes, delays, progression |
| [text.md](text.md) | Presentation text, talk, fonts, layout |

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `main.Main.OnCreate` | `game:0x0600026c` | Game initialisation hook (see boot.md) | high |
| `main.Main.OnUpdate` | `game:0x0600026d` | Per-frame logic hook | high |
| `main.Main.OnDraw` | `game:0x0600026e` | Per-frame draw hook | high |

## Behaviour

- All addresses in these docs are **ELF virtual addresses with load bias 0** (not runtime ASLR addresses, not file offsets). Ghidra projects used base `0x100000`; subtract it.
- Pseudocode from Ghidra is not reliable for large methods (OnUpdate absorbed callee code in three attempts); evidence is bounded assembly per FDE range.
- `main.AppData.ResourceReDownload` (both overloads, `0x1111970`/`0x1111974`) are 4-byte `RET` stubs in this build; no re-download through them (other download paths not excluded).

## Port

Layout: `src/core` (rules; `gameplay`, `animation`, `ecs`, `persistence`), `src/assets` (readers: JAR archive, OPT grid, SEB), `src/platform/desktop` (SDL3 technical viewer/tools, stb_image), `src/platform/n3ds` (empty). C++17/CMake, builds and CTest pass on Windows via `tools/build-desktop.cmd`. No boot, loop or platform layer yet. Windows/Linux and 3DS feasibility are not demonstrated. The final product must not depend on an Android runtime.

## Evidence

- [../archive/reports/01-apk-inventory.md](../archive/reports/01-apk-inventory.md), [02-architecture.md](../archive/reports/02-architecture.md), [03-open-questions.md](../archive/reports/03-open-questions.md), [05-il2cpp-metadata.md](../archive/reports/05-il2cpp-metadata.md), [07-il2cpp-code-map.md](../archive/reports/07-il2cpp-code-map.md), [10-unity-assets.md](../archive/reports/10-unity-assets.md) (scene/TimeManager), [80-project-restructure.md](../archive/reports/80-project-restructure.md)
- Local: `research/local/apk-inventory.json` (`tools/inspect_apk.py`), `research/local/rea-evidence.json`, `build/research/resource-redownload-stubs.json`

## Open questions

1. (blocking) Is all gameplay data in the APK? Absence of OBB/URLs is not proof; needs load/download chains and a controlled run.
2. (blocking) Are network, billing, ads and Firebase required to play? Needs call sites/conditions and authorised tests, without bypassing controls.
3. How does the scene activate the framework and which form is first? Scene and hooks are located; the initial form is not identified (see boot.md).
4. Effective timing: simulation vs presentation vs input (partly answered in boot.md/ui.md).
5. How are UI and animation composited (OnDraw slot resolved; atlas, coordinates and frame durations unverified against the original)?
6. Audio and font reproduction (formats decoded, triggers/volume semantics open).
7. Save and RNG behaviour (paths, schema, seeds; see save.md).
8. 3DS fit: real RAM/CPU/texture/screen/controls benchmark (M6).
9. The DEX class count differs between JADX direct load (19,137 inner-class log entries, duplicate `DebugProbesKt`) and the REA bridge (12,499 classes); unreconciled, nothing depends on it.
10. ARM32 libraries not compared with ARM64.
