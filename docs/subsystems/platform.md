# Platform boundary

The Java/native boundary is the standard Unity Android player: `libmain.so` registers `NativeLoader.load/unload`, which `dlopen` `libunity.so` and `libil2cpp.so`; the UnityMain looper drives `nativeRender`, touch goes through `nativeInjectEvent`, focus through `nativeFocusChanged`. The game reaches Android services (notifications/FCM, Tapjoy, ads, billing, license check, account/server time) through `native.AppPlugin`, `KairoPlugin` and related wrappers. The `libmain` bootstrap is fully mapped statically; `initJni`, `libunity` internals and the managed plugin bridges are not. The port has a desktop technical viewer only; no platform layer replaces these services yet, and the Android original has not been run.

## Overview

### Native bootstrap (`libmain.so`, ARM64, 6,728 bytes)

Nine non-thunk functions, 11 unresolved externals, no DWARF (`libmain.dbg.so` absent). Ghidra base `0x100000`; table gives ELF VA.

| Function / table | ELF VA | File offset |
|---|---|---|
| `JNI_OnLoad` (exported) | `0x4a70` | `0x0a70` |
| `NativeLoader.load` impl | `0x4b00` | `0x0b00` |
| `NativeLoader.unload` impl | `0x4bdc` | `0x0bdc` |
| dynamic-load helper | `0x4cd8` | `0x0cd8` |
| `JNINativeMethod[2]` | `0xd138` | `0x1138` |

- `JNI_OnLoad`: GetEnv, `FindClass("com/unity3d/player/NativeLoader")`, `RegisterNatives` with count 2 (`0x4ab8..0x4ac4`), returns `0x10006` (JNI 1.6) on success. Entries: `load (Ljava/lang/String;)Z` → `0x4b00`; `unload ()Z` → `0x4bdc`. Both are `native` in smali.
- `load`: copies the directory string, calls the helper for `libunity.so` then `libil2cpp.so`, releases the copy, returns **whether the Unity handle is set** (does not test both handles).
- helper: returns if a handle exists; else GetJavaVM, `dlopen("<dir>/<lib>", 1)` (RTLD_LAZY), retries with the bare name on failure; `dlsym("JNI_OnLoad")` and calls it if present; signed compare of its result with `0x10007` (`0x4d8c..0x4d98`). Selected failures call `FatalError` (fatal, not a Java exception).
- `unload`: for the Unity and IL2CPP handles, optional `JNI_OnUnload`, then `dlclose`.
- JNI slots checked against JDK 21 `jni.h`: VM `AttachCurrentThread 0x20`; Env `FindClass 0x30`, `FatalError 0x90`, `GetStringUTFLength/Chars/Release 0x540/0x548/0x550`, `RegisterNatives 0x6b8`, `GetJavaVM 0x6d8`.

### Java → native entry points

| Java (DEX) | Native | Notes |
|---|---|---|
| `NativeLoader.load(String)` / `unload()` | libmain | above |
| `UnityPlayer.initJni(Context)` | libunity | native; implementation unknown |
| `T0.handleMessage` → `nativeRender()` | libunity | false return → `finish()` unless finishing; see [boot.md](boot.md) |
| `nativeFocusChanged(bool)` | libunity | from T0 focus branches |
| `UnityPlayerActivity.onTouchEvent` → `UnityPlayer.onTouchEvent` → `injectEvent(ev)` → `injectEvent(ev, 0)` → `nativeInjectEvent` | libunity | `onTouchEvent` returns false if `mGlView.c()`; `injectEvent` calls native only when `W0.d()`; semantics of both checks unknown |

### Managed → Android/services (static call sites only)

- `main.Main.OnCreate` → `native.AppPlugin.Init`.
- `main.Main.OnUpdate` → `kairo.unity.system.KairoBase.IsIllegalPackage` (package check; condition/effect not recovered).
- `IApplication.OnApplicationPause`: `NotifyManager.DoNotify/DoCancel`, `KairoPlugin.SetNotificationBackground(bool)`, `KairoAccount.Resume`, `StartAsyncConnectThread`, `ClockTime.UpdateServerTime`, `GooglePlayLicenseCheck`, `AudioSettings.Reset`.
- `IApplication.Awake`: `Application.persistentDataPath/productName/platform/temporaryCachePath/dataPath`, `CopyDirectory` (file-system roots to map on PC).
- `IApplication.TerminateCheck` reads `Config.IsXbox` (multi-platform framework flags exist).
- Scene components: MyTapjoy, FCMReceiver, TapjoyUnity. Manifest: FCM service, Firebase Messaging, Google Mobile Ads, Tapjoy, BillingClient, WorkManager, 15 permissions (see [overview.md](overview.md)).
- DEX `main.Main.onCreate` → `plugin.Utility.CreateNotificationChannelIfNeeded()`.
- `main.AppData.ResourceReDownload` overloads are empty stubs.

None of these proves a service is required for play.

### What the PC port must replace

| Android piece | Port replacement |
|---|---|
| Activity/UnityPlayer/UnityMain looper, `nativeRender` | `src/platform` host loop calling the reconstructed Update/OnGUI sequence ([boot.md](boot.md)) |
| Unity `Application.targetFrameRate`, `vSyncCount`, `Time` | host frame pacing honouring `GetTargetFps` |
| `nativeInjectEvent`, `Canvas.OnInputUpdate`, `CheckKeyState` masks | SDL input mapped to the Canvas key/touch model (key masks still numeric) |
| `nativeFocusChanged`, OnApplicationPause/Focus | window focus/minimise events → pause flag and form codes 4/3 |
| `persistentDataPath` etc., `RecordStore` | user data directory; save format per [save.md](save.md) |
| Notifications, FCM, Tapjoy, ads, billing, license, account, server time | stubs (no-op/offline) unless shown required |
| `SoundPlayer`/`AudioSettings` | SDL audio backend ([assets.md](assets.md) for streams) |

Current desktop code: `src/platform/desktop` (`asset_viewer.cpp` SDL3 + stb_image, `asset_inspect.cpp`, `event_trace.cpp`); `src/platform/n3ds` empty. Core stays free of platform libraries.

### Android oracle plan

Status (2026-10-25): `adb devices -l` empty; no SDK/emulator found; host ≈8 GB RAM, 95 GB free on C:. `HypervisorPresent=true` but Windows Hypervisor Platform `InstallState=2` (disabled); processor virtualization flags read false (not proof of BIOS disabled). WHPX is the recommended accelerator and may need a reboot. The APK is ARM-only; a candidate is an Android 11 Google APIs image with ARM translation (documented, untested here). The user deferred dynamic tests; the device-vs-emulator choice is needed only before running the comparison, not for static work. No Windows feature changes or reboots without the user.

First scenario:
1. Record device/serial, Android version, screen, package `net.kairosoft.android.wizardquest` 1.4.0/140; note existing saves, never clear them.
2. Observe boot from activity `main.Main`; record inputs, conditions and real prompts.
3. Capture first stable state and initial navigation; relate screenshots to PNG/SEB/OPT and ResourceManager references.
4. Compare scale, offsets, flips, alpha, font, audio triggers; separate presentation from rule differences.
5. Then controlled pause/resume, network and persistence scenarios, preserving original state and not changing a remote session's network.

Exit criteria: reproducible boot/first-form reference, one comparable composition, list of observed dependencies/prompts.

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `native.AppPlugin.Init` | `game:0x06000127` | Plugin init called from Main.OnCreate | medium |
| `kairo.unity.system.KairoBase.IsIllegalPackage` | `kairo:0x06000dd0` | Package check called from Main.OnUpdate | low |

Java/libmain functions are not in the registry (which covers only the two managed game assemblies).

## Behaviour

- libmain `load` return = Unity handle present; do not substitute "both libraries loaded".
- Helper JNI version check is a signed compare against `0x10007`; do not tighten it.
- `nativeRender` false → activity `finish()` unless already finishing.
- Rescheduling of render message 2269 happens only when `b && e <= 0`.

## Port

None (desktop viewer only). See replacement table above.

## Evidence

- [../archive/reports/01-apk-inventory.md](../archive/reports/01-apk-inventory.md), [04-android-startup.md](../archive/reports/04-android-startup.md), [06-native-bootstrap.md](../archive/reports/06-native-bootstrap.md), [08-smali-validation.md](../archive/reports/08-smali-validation.md), [09-native-callbacks.md](../archive/reports/09-native-callbacks.md), [22-unity-lifecycle.md](../archive/reports/22-unity-lifecycle.md), [23-android-oracle.md](../archive/reports/23-android-oracle.md), [24-frame-conditions.md](../archive/reports/24-frame-conditions.md)
- Local: `build/research/native/extraction-manifest.json`, `build/research/libmain-arm64/functions.tsv`, `jni-table.tsv`, `research/local/unity-*.json` (REA/JADX envelopes, evidence ids in report 04), `build/research/smali/`
- Tools: `tools/extract_research_members.py`, `tools/ghidra/InspectBootstrap.java`, `InspectJniTable.java`, `tools/ExportSmali.java`, `tools/rea-local.cmd` (run JADX queries sequentially with 768 MiB heap; parallel JVMs failed with Win32 error 1455)

## Open questions

1. (blocking) Are network, billing, ads, license check or Firebase required to start or play? Needs the conditions around `IsIllegalPackage`, `GooglePlayLicenseCheck`, `StartAsyncConnectThread` and an authorised run.
2. `UnityPlayer.initJni` implementation and libunity registration of `nativeRender`/`nativeInjectEvent`/`nativeFocusChanged`.
3. Meaning of `mGlView.c()`, `W0.d()`/`W0.e()` and T0 flags `b/c/d/e`.
4. Touch coordinate transform from MotionEvent to Canvas (scale/offset relative to the 240×280 viewport).
5. `native.AppPlugin` / `KairoPlugin` bridge mechanism (AndroidJavaObject calls?) and which plugin methods have effects on gameplay.
6. Oracle: device or emulator (user decision; WHPX would need enabling and possibly a reboot).
