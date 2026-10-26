# Boot and frame loop

Android starts `main.Main` (DEX) → UnityPlayerActivity → UnityPlayer, which loads `libmain.so`/`libunity.so`/`libil2cpp.so` and runs a `UnityMain` looper thread that calls `nativeRender`. Inside Unity the scene object `Main` is the IL2CPP `main.Main : kairo.unity.ui.IApplication`; Unity's Awake/Update/OnGUI call its virtual hooks OnCreate/OnUpdate/OnDraw behind several gates, and the frame rate is requested per form (default 30). All of this is static evidence (bounded ARM64 assembly, smali); nothing has been observed at runtime and the port has no boot path or frame loop yet.

## Overview

### Android side (DEX, confirmed in smali)

1. `main.Main.onCreate(Bundle)` (DEX) calls super, then `plugin.Utility.CreateNotificationChannelIfNeeded()`.
2. `UnityPlayerActivity.onCreate` requests window feature 1, calls `Activity.onCreate`, updates Intent arguments, constructs `UnityPlayer(this, this)`, `setContentView`, requests focus.
3. `UnityPlayer(Context, IUnityPlayerLifecycleEvents)` creates state, an event queue and thread `U0`; captures activity, orientation and URI; prepares splash and Java plugins; calls `loadNative`; if `W0.d()` holds, calls `initJni(context)`, creates view `i0`, configures Android services and starts the thread.
4. `loadNative(String)`: `System.load("<dir>/libmain.so")`; on `UnsatisfiedLinkError` falls back to `System.loadLibrary("main")`; `SecurityException` and a failed fallback both return the `logLoadLibMainError` message. On success calls `NativeLoader.load(dir)` (native, see [platform.md](platform.md)); if it returns true, calls `W0.e()`.
5. `U0.run()` names the thread `UnityMain`, prepares a Looper, installs `T0` as Handler callback, enters `Looper.loop()`.
6. `T0.handleMessage` handles only `what == 2269` (`0x8dd`):
   - `S0.h`: runs GL jobs and, under state/window conditions, calls `nativeRender()`; if it returns false and the activity is not finishing, calls `finish()`.
   - `S0.e`: sets flag `c`; if `d == 3`, jumps to the shared focus-gain block. `S0.g`: sets `d = 3`; jumps to the same block if `c`. Shared block: `nativeFocusChanged(true)`, `d = 1`.
   - `S0.f`: `nativeFocusChanged(false)` only if `d == 1`, then `d = 2`.
   - Rescheduling: enqueues another 2269/`S0.h` only when `b` is true and `e <= 0`, then increments `e`. Early returns in the render branch skip it. No frame duration follows from this.
   - Obfuscated field names (`b`,`c`,`d`,`e`) are kept as is; their semantics are not named.

### Managed side (IL2CPP)

`kairo.unity.ui.IApplication` is the MonoBehaviour. `main.Main` (type 4166, vtableStart 55788) overrides slots 5/11/12 = OnCreate/OnUpdate/OnDraw (methods 34537/34538/34539). Virtual call offsets use vtable origin `class+0x138`, stride 16 (slot n at `0x138 + 16n`).

| Unity entry | Calls hook | Call site |
|---|---|---|
| `Awake` | slot 5 `OnCreate` (receiver `this` in x26, pair at `class+0x188`) | `0x1b7f138..0x1b7f144` (a second slot-5 call at `0x1b7f250` is on another object, x24) |
| `Update` | slot 11 `OnUpdate` (`class+0x1e8`) | `0x1b821c4..0x1b821d0` (the only call site) |
| `OnGUI` | slot 12 `OnDraw` (`class+0x1f8`) | `0x1b841fc..0x1b84208` |

`FixedUpdate` is a single `RET` (`0xd65f03c0`): no gameplay in the framework's fixed step. `Start+0x108` (`0x1b80d78`) calls `Time.set_fixedDeltaTime` with s0 = **1.0f** (`0x3f800000`, loaded at `0x1b80d70`), overriding the serialized 0.02.

### main.Main hooks

- **OnCreate** (`0x117c78c`, FDE 1596 bytes; 77 BL): `IApplication.OnCreate`, `native.AppPlugin.Init`, `IApplication.SetViewport(this, 240, 280)` (`w1=0xf0`, `w2=0x118` at call `0x117c918`), `Canvas.SetBackground`, `Canvas.SetDisplayCurrent`, `FormManagerBase.ChangeCurrentForm`, `RecordStore.ReadRecord`, `JarInflater..ctor`, `StringUtil.LoadStringArray`, `Language.SetTextTable`. The initial form is not identified.
- **OnUpdate** (`0x117cdc8`, 3128 bytes; 165 BL): `FormManagerBase.Execute`, `main.Main.SetJingle`, `Canvas.CheckKeyState`, the FPS policy below, `SaveRecorder.SaveSystem`, `AppData.AddErrorLog`, `KairoBase.IsIllegalPackage`. Conditions and effects of the last three not recovered.
- **OnDraw** (`0x117db20`, 192 bytes; 13 BL): `form.FormManager.GetInstance` (`0x117db28`), then virtual slot 4 (`class+0x178`, `blr x9` at `0x117db38`). For `form.FormManager` (type 4274, vtableStart 57317) slot 4 = `0x600099bb` → method 19677 **FormManagerBase.OnDraw**. The concrete runtime type is not captured.
- `SetViewport` (8 bytes) only stores w1/w2 at instance `+0x5c/+0x60`; it configures no surface. 240×280 is a logical value, not proven physical resolution or scale.
- `SetJingle` (288 bytes) gets `SoundPlayer.GetInstance` and tail-calls `SoundPlayer.SetVolume` with either 0 or `SecureInt.op_Implicit(...) * 51`. Unit/range not inferred.

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `main.Main.OnCreate` | `game:0x0600026c` | Init hook: framework, plugin, viewport, first form, records, string tables | high |
| `main.Main.OnUpdate` | `game:0x0600026d` | Logic hook: form execute, jingle, keys, FPS policy, system save | high |
| `main.Main.OnDraw` | `game:0x0600026e` | Draw hook: `FormManager.GetInstance` → `FormManagerBase.OnDraw` | high |
| `main.Main.SetJingle` | `game:0x06000272` | Jingle volume: 0 or setting×51 → `SoundPlayer.SetVolume` | medium |
| `IApplication.Awake` | `kairo:0x0600144e` | Paths (persistentDataPath, temporaryCachePath, dataPath…), CopyDirectory, Screen/Canvas; calls OnCreate | high |
| `IApplication.Start` | `kairo:0x06001450` | fixedDeltaTime=1.0, SetResolutionScaleRatio, Screen.sleepTimeout, OnApplicationFocus | high |
| `IApplication.FixedUpdate` | `kairo:0x06001451` | Empty (`RET`) | high |
| `IApplication.Update` | `kairo:0x06001452` | Frame gates, TotalFrames++, jingle/UI/update methods, calls OnUpdate | high |
| `IApplication.LateUpdate` | `kairo:0x06001453` | TerminateCheck, UpdateResolution, ExecuteUIMethods | medium |
| `IApplication.OnGUI` | `kairo:0x06001455` | Paint begin/end, input, splash, calls OnDraw, optional frame wait | high |
| `IApplication.get_TotalFrames` | `kairo:0x06001446` | Returns `this+0x58` of the singleton | high |
| `IApplication.OnApplicationPause` | `kairo:0x06001458` | Stores pause flag `+0x35`; suspend/resume side effects | medium |
| `IApplication.OnApplicationFocus` | `kairo:0x0600145b` | Stores focus `+0x34`; suspend on focus loss; may forward to pause | medium |
| `IApplication.TerminateCheck` | `kairo:0x0600145d` | Decides termination, arms quit countdown `+0x140` | medium |
| `IApplication.Terminate` | `kairo:0x0600146a` | Called by TerminateCheck before quit | low |
| `IApplication.ExecuteUpdateMethods` | `kairo:0x0600148d` | Called in Update before OnUpdate | low |
| `IApplication.ExecuteUIMethods` | `kairo:0x0600148c` | Called in Update/LateUpdate/OnGUI and in the frame wait; return ≥1 resets wait timer | low |
| `IApplication.UpdateResolution` | `kairo:0x06001499` | Called in LateUpdate/OnGUI before the `+0x138` gate | low |
| `IApplication.SetViewport` | `kairo:0x06001493` | Stores (w,h) at `+0x5c/+0x60` | high |
| `IApplication.OnCreate` | `kairo:0x06001460` | Base hook called first by Main.OnCreate (4 bytes) | medium |
| `cfg.MyConfig..cctor` | `game:0x0600125a` | Static init; default FPS **30** at static `+0x50` | high |
| `kairo.common.cfg.Config..cctor` | `kairo:0x06000271` | Static init of Config flags used by the loop | high |
| `main.AppData.ResourceReDownload` (2 overloads) | `game:0x06000214`, `game:0x06000215` | Empty `RET` stubs | high |

Form-manager methods (GetTargetFps, SetTargetFps, Execute, _execute, DoMethod…) are in [ui.md](ui.md).

## Behaviour

Notation: `I.static+N` = static block of the class at GOT `0x272f528` (via `class+0xb8`), the IApplication statics. `C.static+N` = static block via GOT `0x272f5c0` → metadata slot `0x27c5898`, encoded `0x20004fef`, native index 10231 = type 1778 **`kairo.common.cfg.Config`**. `MyConfig` = GOT `0x27312d8` → slot `0x27c8788`, encoded `0x20006f19`, native index 14220 = type 4527 **`cfg.MyConfig`**. Field names are not assigned from offsets.

### Update gates (in order)

| Condition | Effect |
|---|---|
| `I.static+0x40 != null` (`0x1b81744–0x1b817a0`) | No hook. Initialises a timestamp at `this+0xc8`; before 3,001 ms returns; afterwards calls `TerminateCheck(this, true)`. |
| `this+0x140` (`0x1b81df8–0x1b81e28`) | 32-bit `x-1`: negative → continue normally; zero → `Application.Quit`; positive → store and skip the hook. Not a per-frame decrement in all states. |
| `TerminateCheck(this,false)` (`0x1b81e80`) bit 0 set | Skip main update. |
| `C.static+0x30 != 0 && this+0x35 != 0` (`0x1b81f58`) | Skip main update (`+0x35` = pause flag). |
| `this+0x58 += 1` (`0x1b82080`) | TotalFrames; advances even when the hook is skipped below. |
| `I.static+0x1c = 1` (`0x1b820a8`) | Enables SEB render-animation in OnGUI. |
| `I.static+0x48 != null` (splash, `0x1b8214c`) | Skip UpdateJingle, ExecuteUIMethods, ExecuteUpdateMethods and the hook; go to `Updated` delegate. |

Normal order: `SoundPlayer.UpdateJingle` (`0x1b821b0`) → `ExecuteUIMethods` (`0x1b821b8`) → `ExecuteUpdateMethods` (`0x1b821c0`) → **Main.OnUpdate** (`0x1b821d0`) → optional delegate `Updated` at `this+0xe8` (`0x1b82210`). `this+0x148` = `CurrentTimeMillis` written at `0x1b81d38`; `this+0x110` = measured duration (`0x1b82258`). `this+0xf9` is set before and cleared after (also on exception paths); it is not a clock.

### OnGUI gates

- `I.static+0x40 != null`: draws a message with Canvas/Graphics and returns (no hook).
- Otherwise: `TerminateCheck(false)` true → exit; `UpdateResolution`, then `this+0x138 - 1` (`0x1b83eb8`): ≥0 → store and exit, negative → continue without storing; `this+0x128 != 0` → exit.
- Sets `this+0xf8`; if no splash, `ExecuteUIMethods` (`0x1b83f50`). If `this+0x1f8 != 0`, `UpdateGUI`; result bit 0 **clear** jumps straight to draw preparation.
- Else reads `Event.current.type`: types 0, 3, 1 → `Canvas.OnInputUpdate`. Only **type 7 with `this+0x20 == 0`** proceeds; others clear `+0xf8` and return.
- `Canvas.BeginPaint`/`EndPaint` calls exist at `0x1b83d08`/`0x1b83e74` (before the gates above in address order; their path conditions are not recovered).
- Draw prep (Graphics, SEB): `Seb.ThisFrameRenderAnimation` (`0x1b84090`) only if `I.static+0x1c`; `Seb.ResetThisFrameRender` (`0x1b840b0`) unconditionally on this path.
- If `this+0x80` and `this+0x88 != null`: `GUI.DrawTexture` of that image's Texture2D, centred; no OnDraw. Else if splash: `SplashScreen.Draw` (`0x1b841c0`); if splash byte `+0x20` set, clears `I.static+0x48` but still skips OnDraw this pass. Else **Main.OnDraw** (`0x1b84208`).

### Frame wait at end of OnGUI

Enabled by `C.static+0x63` (`0x1b84af8`), **initially 0**. Gets the form manager via app slot 15, calls `GetTargetFps` (`0x1b84b34`), `budget = trunc(1000.0f / fps)` to int64 (explicit +inf branch; zero/negative/overflow not modelled). If `now - this+0x148 >= budget` return; else loop `JThread.Sleep(1)` + `ExecuteUIMethods`, exiting on the total budget since `+0x148` or half the budget since a local timestamp; `ExecuteUIMethods` returning ≥1 resets the local timestamp only.

### FPS policy (Main.OnUpdate, normal path)

| # | Site | Effect |
|---|---|---|
| 1 | `0x117ce98`, `0x117ceb8–0x117cec4` | `manager+0xb0 = MyConfig.static+0x50` |
| 2 | `0x117cf64` | if `Config+0xc2`: `+0xb0 = 120` |
| 3 | `0x117cf90` | if `Config+0xc5` and `CheckKeyState(0x10)`: `+0xb0 = 400` |
| 4 | `0x117cfcc` | if `Config+0xc4` and `CheckKeyState(0x400)`: `+0xb0 = 2` (last store wins) |
| 5 | `0x117d000` | `QualitySettings.set_vSyncCount(0)` |
| 6 | `0x117d00c`, `0x117d034` | `Application.set_targetFrameRate(GetTargetFps())` |

Key masks are kept numeric. 400/2 are code possibilities (debug-like), not observed modes. GetTargetFps priority (120 while changing form, then top form's rate, then `+0xb0`) can override the stored default; see [ui.md](ui.md). `form.FormManager.Updated` has a separate rule (120 on `CheckKeyState(0x10)` or `(0x20)` when `Config+0xc5`).

Initial values: `MyConfig..cctor` stores **30** at static `+0x50` (`0x1174390`/`0x11743f4`); `FormManager..ctor` copies it to `manager+0xb0`. `Config..cctor`: `+0x30 = 1`, `+0x63 = 0` (32-bit zero at `+0x60`), `+0xc2 = 0`, `+0xc4 = +0xc5 = 1` (4-byte store of `0x01010101` at `+0xc3`); static `+0..+0xf` copied from ELF `0x625fb0` = int32 LE `(2, 0, 100, 30)`. These are post-initialiser values; writers elsewhere are not inventoried.

### Pause, focus, termination

- **OnApplicationPause** (`0x1b8570c`): returns early if `I.static+0x40`; else `this+0x35 = arg & 1` (`0x1b857bc`). Pause: `NotifyManager.DoNotify`, `KairoPlugin.SetNotificationBackground(true)`, sends form code **4** (Suspend) to forms back-to-front, may deactivate FormManagerBase. Resume: `DoCancel`, `SetNotificationBackground(false)`, code **3** (Resume) to forms, paths to `KairoAccount.Resume`, `StartAsyncConnectThread`, `ClockTime.UpdateServerTime`, `GooglePlayLicenseCheck`, `AudioSettings.Reset`. Presence of calls ≠ network required.
- **OnApplicationFocus** (`0x1b81390`): filtered by `C.static+0x30` and `this+0x178`; stores arg at `this+0x34` (`0x1b81470`). Focus loss on the accepted path deactivates FormManagerBase, calls a virtual hook and `SoundPlayer.Suspend` (`0x1b814dc`). One branch forwards `!focus` to OnApplicationPause (`0x1b81428`). Focus and pause are not interchangeable.
- **TerminateCheck** (`0x1b82744`): enabled by `I.static+0x60`; with arg false, splash present and `Config.IsXbox` → returns false early. No manager or `GetFormsNum()==0`, or arg true, allows termination; requires `this+0x140 <= 0`. Calls `IApplication.Terminate` (`0x1b828f8`), then if `Canvas.static+0x50 == this+0x58` stores **3** in `+0x140` (Update quits when it reaches 0), else `Application.Quit` immediately. Returns true.

### Do not

- Implement a fixed 50/60/120 Hz simulation; TotalFrames, presented frames, form updates and Unity fixed steps are distinct.
- Use Ghidra pseudocode of OnUpdate (three attempts contaminated by callees). Bounded assembly (`ExportBoundedInstructions.java`) covers all words of the 9 lifecycle FDEs: BL counts 77/77, 165/165, 13/13.

## Port

None. Needed: a `src/platform` host loop reproducing Update/OnGUI gate order, a TotalFrames counter separate from logic passes, the FPS request (default 30) and optional frame wait, pause/focus → form codes 4/3. Logic pass counting is in [ui.md](ui.md).

### Replay RNG gate in Awake

At `0x1b7ebe0..0x1b7ec80`, Awake sets JRandom.fix_ when Config.UNITY_EDITOR is true and either ActionReplay.CheckReplayFile succeeds or PlayerPrefs DEVCMD_REPLAY_RECORD equals "on" (default "off"). It does not reset the flag on the false path. The gate is ported/tested with explicit host inputs; [rng.md](rng.md#initialization-and-public-implementation-reuse) records the evidence and remaining live-configuration limits. This does not complete Awake's other startup responsibilities.

## Evidence

- [../archive/reports/04-android-startup.md](../archive/reports/04-android-startup.md), [06-native-bootstrap.md](../archive/reports/06-native-bootstrap.md), [07-il2cpp-code-map.md](../archive/reports/07-il2cpp-code-map.md), [08-smali-validation.md](../archive/reports/08-smali-validation.md), [09-native-callbacks.md](../archive/reports/09-native-callbacks.md), [10-unity-assets.md](../archive/reports/10-unity-assets.md), [22-unity-lifecycle.md](../archive/reports/22-unity-lifecycle.md), [24-frame-conditions.md](../archive/reports/24-frame-conditions.md), [27-frame-policy.md](../archive/reports/27-frame-policy.md), [03-open-questions.md](../archive/reports/03-open-questions.md)
- Local: `build/research/smali/*.smali`, `build/research/lifecycle-asm/` (+`coverage.tsv`), `build/research/frame-gates/`, `build/research/frame-policy*/` (incl. `direct-references.json`), `research/local/main-callback-transfers.json`, `research/local/unity-lifecycle-calls.json`, `build/research/il2cpp-unwind.json`, `build/research/small-methods/`

## Open questions

1. (blocking) Which form does OnCreate pass to `ChangeCurrentForm`, and what does OnCreate read (`RecordStore.ReadRecord`, string arrays)?
2. (blocking) Writers of `Config+0x63`, `+0x30`, `MyConfig+0x50` and `I.static+0x1c/+0x40/+0x48/+0x60`: does the frame wait run on Android; what are `+0x40` (message mode) and `+0x60`?
3. Where is `I.static+0x1c` cleared (SEB render-animation policy)?
4. Argument and timing of Main.OnResume/OnSuspend (4/8-byte bodies, not yet read) and their vtable wiring to pause/resume.
5. Conditions of `SaveRecorder.SaveSystem`, `AppData.AddErrorLog` and `KairoBase.IsIllegalPackage` in OnUpdate.
6. Effective frame rate and order on device (needs the Android oracle, see platform.md).
7. Native side: `UnityPlayer.initJni`, `nativeRender`/`nativeInjectEvent` registration, libunity internals to IL2CPP entry and first scene load.
