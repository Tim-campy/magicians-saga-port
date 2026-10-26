# UI forms

The game's screens are forms (`kairo.unity.form.FormBase` → `LibraryFormBase` → `form.MyFormBase` → concrete forms such as `QuestForm`) run by a singleton `form.FormManager : FormManagerBase`. The manager dispatches six virtual form events through `DoMethod` (0 Init, 1 Update, 2 Draw, 3 Resume, 4 Suspend, 5 Finish), runs a per-frame number of Update passes derived from the form's requested rate and the manager's target FPS, and supplies the target FPS that `main.Main` hands to Unity. These contracts are established from bounded assembly; concrete form overrides, the form stack and the first form are not reconstructed, and no port exists.

## Overview

- `main.Main.OnUpdate` → `FormManagerBase.Execute` → `_execute` (logic passes). `main.Main.OnDraw` → `FormManager.GetInstance` → vtable slot 4 = `FormManagerBase.OnDraw` (method 19677) for `form.FormManager`. `main.Main.OnCreate` → `FormManagerBase.ChangeCurrentForm` (initial form unknown).
- Virtual dispatch: vtable origin `class+0x138`, stride 16. `FormManagerBase` (type 2316, vtableStart 24492) and `form.FormManager` (type 4274, vtableStart 57317) both have slot 12 = `0x60009a37` → method 19739 **DoMethod**.
- One gameplay chain is known: `DoMethod(1)` → `QuestForm.Update` → `World.Update` → `SystemManager.Update` (see [ecs.md](ecs.md); QuestForm.Update has its own pass multiplication).
- Pause/resume from the application reaches forms as codes 4/3 ([boot.md](boot.md)). In-world pause goes through the ECS `IPausable` interface (below).

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `FormManagerBase.Execute` | `kairo:0x06001a15` | Called by Main.OnUpdate (link to `_execute` not traced) | medium |
| `FormManagerBase._execute` | `kairo:0x06001a16` | Pass count and Update dispatch loop | medium |
| `FormManagerBase.DoMethod` | `kairo:0x06001a56` | Dispatch event code 0..5 to form virtual slot | high |
| `FormManagerBase.GetTargetFps` | `kairo:0x06001a50` | 120 while changing; top form rate; else `+0xb0` | high |
| `FormManagerBase.SetTargetFps` | `kairo:0x06001a4f` | `manager+0xb0 = arg` | high |
| `FormManagerBase.GetFps` | `kairo:0x06001a4c` | Returns `manager+0xb4` (not `+0xb0`) | high |
| `FormManagerBase.IsFormChanging` | `kairo:0x06001a54` | `+0xd8 != null` (JThread.op_Inequality) or `byte+0xd1 != 0` | high |
| `FormManagerBase.GetTopForm` | `kairo:0x06001a28` | Top form, used by GetTargetFps/_execute | medium |
| `FormManagerBase.GetFormsNum` | `kairo:0x06001a25` | Form count, used by TerminateCheck | low |
| `FormManagerBase.ChangeCurrentForm` | `kairo:0x06001a0c` | Form switch; called by Main.OnCreate and continue-game prep | low |
| `FormManagerBase.OnDraw` | `kairo:0x06001a18` | Draw entry via Main.OnDraw slot 4 | medium |
| `FormBase..ctor` | `kairo:0x060019d5` | Initialises `+0x4c`,`+0x50` to -1 | high |
| `FormBase.SetFrameRate(int,int,bool)` | `kairo:0x060019f0` | `+0x4c`, `+0x50` (−1 → copy first), `byte+0x54` | high |
| `FormBase.SetFrameRate(bool)` | `kairo:0x060019f1` | `byte+0x54 = arg & 1` only | high |
| `FormBase.Init` / `Update` / `Draw` / `Resume` / `Suspend` / `Finish` | `kairo:0x060019d6` / `0x060019d8` / `0x060019d7` / `0x060019d9` / `0x060019de` / `0x060019df` | Base targets of event codes 0/1/2/3/4/5 (slots 17/19/18/20/25/26) | high |
| `form.FormManager..ctor` | `game:0x0600072e` | `manager+0xb0 = MyConfig.static+0x50` (30) | high |
| `form.FormManager.Updated` | `game:0x06000731` | Alternative 120-FPS key rule, writes `+0xb0` if changed | medium |
| `form.FormManager.GetInstance` | `game:0x0600072d` | Singleton accessor used by Main.OnDraw/OnUpdate | medium |

## Behaviour

### DoMethod (19739, `0x1be48e4`)

Codes 0..5; byte table at ELF `0x68dd53` = `00 3c 0c 37 05 67`, target = `0x1be4958 + 4*byte`.

| Code | Target | Form slot | Base method |
|---|---|---:|---|
| 0 | `0x1be4958` | 17 | Init 19611 |
| 1 | `0x1be4a48` | 19 | Update 19613 |
| 2 | `0x1be4988` | 18 | Draw 19612 |
| 3 | `0x1be4a34` | 20 | Resume 19614 |
| 4 | `0x1be496c` | 25 | Suspend 19619 |
| 5 | `0x1be4af4` | 26 | Finish 19620 |

Code 1: if `form.byte+0x78 == 0` sets it (`0x1be4b6c`); calls slot 19 (`class+0x268/+0x270`), returns its bool after cleanup.

### Target FPS

`GetTargetFps` (60 bytes): `IsFormChanging()` bit 0 → **120**; else top form with `form+0x4c != -1` → that value; else `manager+0xb0` (default 30, see [boot.md](boot.md) for the OnUpdate overrides 120/400/2). Direct BL/B references (scan of executable segments): 7, all to GetTargetFps — Main.OnUpdate, FormManager.Updated, IApplication.OnGUI, 4 in `_execute`. None to SetFrameRate×2/SetTargetFps; inline stores/indirect calls are not excluded (Main.OnUpdate stores `+0xb0` directly).

`FormManager.Updated` (`0x11d6a14`): reads MyConfig+0x50 and Config+0xc5; with the selector and `CheckKeyState(0x10)` **or** `(0x20)` chooses 120 (`0x11d6aec–0x11d6b40`); writes `+0xb0` only if the choice differs from GetTargetFps (`0x11d6b44–0x11d6b58`). Not identical to Main.OnUpdate's rule; relative order unresolved.

### Pass count in `_execute` (19675, `0x1be01d8`, 4624 bytes)

Path with `form+0x38 == 2` (or after 1→2). If `manager+0x90 != current form`, zero accumulator `manager+0x98` (`0x1be0de4`). If `form+0x50 == -1`, `N = 1`. Else (`0x1be0e00–0x1be0e50`), all int32, `SDIV` truncating toward zero:

```text
Ra = form+0x50 (first read)   Fa = GetTargetFps() (first)
Rb = form+0x50 (second read)  Fb = GetTargetFps() (second)   A = manager+0x98
T = 1000 / Rb
D = 1000 / Fb - T + A
B = max(Ra / Fa, 1)          ; only B is clamped (CMP/CSINC 0x1be0e34)
Q = D / T
N = Q + B
manager+0x98 = D - Q*T
```

N < 1 skips the loop (no general clamp). Divide-by-zero/overflow are not specified; do not port with C++ UB. Worked example (algebra only): Ra=Rb=60, Fa=Fb=30, A=0 → T=16, D=17, B=2, Q=1, N=3, remainder 1. Keep this literal formula; do not replace it with a conventional R/F accumulator.

Time correction (`0x1be0f64`): only if `manager+0x90 == current form` and `form.byte+0x54`. `elapsed = now - manager+0x88`; if `elapsed > 999` keep N; else `elapsed = min(elapsed, 2000 / GetTargetFps())` (signed; no lower clamp). `rate = form+0x50`, or GetTargetFps if −1. `cand = (int)trunc(float(elapsed) / (1000.0f / float(rate)) + 0.1f)` (single precision; 0.1f from ELF `0x6278b8` = `cd cc cc 3d`, loaded `0x1be0d00`, added `0x1be0fd4`; explicit +inf branch). `N = max(N, cand)`.

Loop (`0x1be0e64–0x1be0f30`): stores the form at `manager+0x28`, sets `byte+0xd0` after the first pass. A static object checked at `0x1be0ea0–0x1be0eb4` suppresses the DoMethod call (identity unknown). Otherwise `DoMethod(form, 1, null)` (indirect call `0x1be0ed0`); if it returns true and state is still 2, state = 3. If `GetTopForm()` changed, break. Another pass with state 3 restores 2. State 4 sets an accumulated flag used by a later handler call. On exit clears `byte+0xd0` and `+0x28`. **`form+0x48` is incremented once after the loop** (`0x1be103c–0x1be1048`), even when N < 1 — it is not an Update call count. States 1..4 are not named.

### FormBase rate fields

| Offset | Init | Writer | Meaning |
|---|---|---|---|
| `+0x38` | ? | `_execute` | form state (1..4 observed) |
| `+0x48` | ? | `_execute` | per-frame counter (+1 per execute) |
| `+0x4c` | −1 | SetFrameRate arg1 | form target FPS (overrides manager in GetTargetFps) |
| `+0x50` | −1 | SetFrameRate arg2 (−1 → arg1) | logic rate for pass count |
| `+0x54` | ? | SetFrameRate bool | enables time correction |
| `+0x78` | ? | DoMethod code 1 | set on first Update |

No range validation in SetFrameRate (24 bytes). FormManagerBase: `+0x28` current dispatched form, `+0x88` timestamp, `+0x90` last form (writers unknown), `+0x98` remainder, `+0xb0` default target FPS, `+0xb4` GetFps value, `+0xd0` in-loop flag, `+0xd1`/`+0xd8` form-changing state.

### Pause interface (ECS side)

- `SystemManager.PauseSystems` iterates the list at `manager+0x48` through GOT `0x2731050` → `0x27c7408`, encoded `0x200062f9`, native type ref 12668 = type 4401 **`kairo.unity.ecs.IPausable`** (single method `Pause` 37206, one bool). The call at `0x113b1a4` is `IPausable.Pause(bool)`; the list is not a second Update list.
- `ecs.AnimationSystem` (type 4450) has IPausable at interface offset 16; vtable 58921+16 = entry 58937 = `0x60012477` → `AnimationSystem.Pause` (37435, `0x11420ac`): if `enabled (byte+0x18) == (~arg & 1)` return; else write it, then call slot 5 (pause) or slot 4 (resume), which are empty `BaseSystem.OnEnabled/OnDisabled`. It does not remove the system from the Update list (NeedsUpdate stays true).
- `BasePauseSystem.OnEnabled` (37181, `0x113b020`): reads manager at `this+0x10`, null → exception path, tail-calls `PauseSystems(true)`. `OnDisabled` (37182, `0x113b324`): same with false. No nesting counter in the hooks — do not add one. `BasePauseSystem..ctor` only forwards to `BaseSystem..ctor` (37185).
- Order: `BaseSystem.set_enabled` writes the byte, then the hook; PauseSystems records `isPause` before dispatching `IPausable`; the extra `OnPause` phase of general systems runs only for true. Do not collapse into one boolean assignment.
- Which BasePauseSystem subclasses are enabled by menu transitions is open.

### World load preparation

`World.Deserialize` (37014, `0x1135938`, 76 bytes): `EntityManager.Deserialize(World+0x20, stream)` at `0x1135954`, then `SystemManager.Deserialize(World+0x28, stream)` at `0x1135968`, then `World.byte+0x38 = 1` (`0x1135970`) — the flag `AppData.InitWorld` tests before creating systems. No clear/RemoveAllSystems in the wrapper; do not add one without following the callers. Details belong to [ecs.md](ecs.md)/[save.md](save.md).

## Port

None. A port needs: `FormManager` with DoMethod codes 0..5, GetTargetFps priority, the literal `_execute` pass formula with int32/float32 semantics and defined behaviour for zero divisors, and FormBase rate fields. ECS pause dispatch belongs to the ECS module.

## Evidence

- [../archive/reports/09-native-callbacks.md](../archive/reports/09-native-callbacks.md), [24-frame-conditions.md](../archive/reports/24-frame-conditions.md), [27-frame-policy.md](../archive/reports/27-frame-policy.md), [28-form-events.md](../archive/reports/28-form-events.md), [32-ecs-animation-order.md](../archive/reports/32-ecs-animation-order.md) (QuestForm chain), [36-pause-hooks.md](../archive/reports/36-pause-hooks.md), [49-continue-game-preparation.md](../archive/reports/49-continue-game-preparation.md) (ChangeCurrentForm caller)
- Local: `build/research/frame-policy-followup/M19675__execute.asm`, `build/research/form-events/M19739_DoMethod.asm`, `build/research/frame-policy/direct-references.json`, `build/research/pause-hooks/` (+`coverage.tsv`)

## Open questions

1. (blocking) First form passed by Main.OnCreate and the form stack/transition protocol (`ChangeCurrentForm`, states 1..4, `+0xd8` thread object).
2. (blocking) Writers of `manager+0x88/+0x90/+0x98`, `form+0x48/+0x50`; identity of the static object that suppresses Update.
3. Concrete slot-19 (Update) overrides for boot/menu forms and the first gameplay form.
4. How `+0xb4` (GetFps) is computed.
5. Body of `FormManagerBase.OnDraw` and how DoMethod code 2 (Draw) is used.
6. Which BasePauseSystem-derived systems are toggled by menus; `BaseSystem..ctor`/`SetWorld` initial state.
