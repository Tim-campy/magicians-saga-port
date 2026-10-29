# UI forms

The game's screens are forms (`kairo.unity.form.FormBase` → `LibraryFormBase` → `form.MyFormBase` → concrete forms such as `QuestForm`) run by a singleton `form.FormManager : FormManagerBase`. The manager dispatches six virtual form events through `DoMethod` (0 Init, 1 Update, 2 Draw, 3 Resume, 4 Suspend, 5 Finish), runs a per-frame number of Update passes derived from the form's requested rate and the manager's target FPS, and supplies the target FPS that `main.Main` hands to Unity. These contracts are established from bounded assembly; concrete form overrides, the form stack and the first form are not reconstructed, and the portable port currently covers form-push wrappers and the SelectForm event block, not the manager/frame loop.

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
| `ecs.CameraSystem.Init / Update` | `game:0x06000de2` / `game:0x06000de4` | Camera creation/reset, then formation-derived cursor on pass zero | high |
| `ecs.EnemySystem.DrawDynamic` | `game:0x06000e7a` | Saved defeat threshold, palette and projected HP request | high |
| `ecs.GaugeSystem.DrawParamGauge / DrawGauge` | `game:0x06000eb4` / `game:0x06000eaf` | Parameter/viewport gates and two-half image drawing | high |
| `ecs.GaugeSystem.GetImageOffsetX / AnchorPos` | `game:0x06000eb0` / `game:0x06000eb5` | Retained alpha scratch scan and wrapped anchor offsets | high |
| `ecs.CameraSystem.SetCursor / SetCameraPosition` | `game:0x06000dea` / `game:0x06000deb` | Fresh QuestForm, native float-to-int conversion and distinct cursor/position paths | high |
| `ecs.CameraSystem.SetScale / GetScaleMin / GetScaleMax / GetScaleRange` | `game:0x06000de6` / `game:0x06000de8` / `game:0x06000de9` / `game:0x06000de7` | Component scale clamp/recenter; viewport-dependent minimum and shared range scratch | high |
| `ecs.CameraSystem.OnTouchCamera` | `game:0x06000de5` | Ordered validation, flags and bounded reusable delta history | high |
| `ecs.CameraSystem.GetGameWidth / GetGameHeight` | `game:0x06000dec` / `game:0x06000ded` | Fresh QuestForm viewport forwarding | high |
| `form.QuestForm.GetGameWidth / GetGameHeight` | `game:0x060007c0` / `game:0x060007c1` | GameView dimension minus VIEW_X/Y, wrapping times 100, signed division by form scale | high |
| `form.QuestForm.SetCursor / SetCameraPosition` | `game:0x060007b9` / `game:0x060007ba` | Normal-save center slots 17/18 and live QuestForm camX/camY, with distinct ordering | high |
| `form.QuestForm.DynamicRender / StaticRender` | `game:0x0600079a` / `game:0x0600079b` | Offscreen callback gates, temporary logical viewport, sprite/world draw and cleanup; static callback is a gate only | high |
| `form.QuestForm.DrawOffscreen / UpdateStaticRender` | `game:0x06000796` / `game:0x06000797` | Composite offscreen image into VIEW rectangle; mark static content dirty | high |
| `kairo.unity.ecs.World.CreateCamera / Entity.AddCamera / Entity.get_camera` | `game:0x06000be9` / `game:0x06000b2f` / `game:0x06000b2e` | Registered position-only camera factory and fresh checked slot-3 component | high |
| `ecs.MemberSystem.GetFormationX(int) / GetFormationZ(int)` | `game:0x06000f49` / `game:0x06000f4a` | Fresh hero position minus formation offset plus signed indexed 36-unit grid | high |
| `kairo.unity.math.Coordinate.TopViewToIsometricX / TopViewToIsometricY` | `game:0x06000a8a` / `game:0x06000a8b` | Wrapping x-z and trunc(wrap(x+z)/2), used by CameraSystem.Update | high |
| `ecs.InputSystem.Init / Update / OnTouchEvent` | `game:0x06000f1f` / `game:0x06000f21` / `game:0x06000f22` | Captured movement controls, edge delivery, dash cadence and hero state/velocity | high |
| `ecs.StageSystem.OnTouchEvent / InactiveAttackButton` | `game:0x06000fba` / `game:0x06000fa2` | Three attack selectors, movement suppression and release restoration | high |
| `ecs.HumanSystem.AccelerateSpeed / ResetSpeed` | `game:0x06000eee` / `game:0x06000eef` | All-Human dash accumulation/reset, independent of Input subset | high |

## Behaviour

### Enemy parameter gauges

`EnemySystem.DrawDynamic` (`game:0x06000e7a`, ELF `0x114e1d4`,732 bytes) traverses its live And24 group. It captures EnemyData, skips `Check(1)` records, then requires the **saved defeat count** at EnemyData `+0x20` to be at least6; this is not the actor level. It projects the current nullable Position through AppData.ToScreenPos(position,true), then indexes the captured record's attribute `+0x4c` into the current enemy palette. The request is `(entity,screenX,wrap32(screenY+3),image,parameter0,anchor2,force=false)`. There is no caller-side state, HP or destroyed test. Palette bounds faults occur after projection; earlier drawings remain after later failures. The palette is `[1,5,6,1,1]`, a five-int32 initializer in EnemySystem..cctor (`0x11517e8`); saved counts remain separate from the static EnemyData table.

`GaugeSystem.DrawParamGauge` (`game:0x06000eb4`, `0x1154a48`) returns for absent Parameter23 or a missing parameter id. It captures the parameter and, unless force is true, returns when wrapped value+extra is below1. Resolve the gauge image from AppData.resGauge (`+0x168`), capture its unscaled width/half-height, query the current QuestForm game width then height, and apply AnchorPos. A wrapped integer rectangle check includes touching edges. Only visible requests read the captured parameter's wrapped maximum+extraMaximum, use the existing `Graph.Easing(50,150,1000,maximum,0)` for horizontal scale and query its existing clamped percentage. DrawGauge resolves resource23/image afresh.

`AnchorPos` (`game:0x06000eb5`, `0x1154d00`) subtracts width/2 for mask2, otherwise full width for mask4; independently it subtracts height/2 for mask32, otherwise full height for mask64. Halves truncate toward zero, mask2/32 take precedence, and coordinate subtraction wraps int32. The enemy caller centers horizontally and keeps Y.

`GetImageOffsetX` (`game:0x06000eb0`, `0x1154210`) reads a crop at `(0,trunc(height/2)+trunc(height/4),min(width,30),2)` through Image.GetPixels. The reused scratch starts as128 zero Color entries (`GaugeSystem..cctor`, `0x1153544`); the port retains alpha only. The crop overwrites its prefix, preserving the suffix. Scan the **whole returned buffer**, stopping at the first alpha!=0, including NaN, or return its length. The wrapper flips Unity's crop rows back to top-down order; the desktop adapter reuses decoded PNG data and does not implement Unity texture internals. The [Unity2022.3 API](https://docs.unity3d.com/2022.3/Documentation/ScriptReference/Texture2D.GetPixels.html) supplies the public pixel-order contract.

`DrawGauge` (`game:0x06000eaf`, `0x1154350`) uses wrapped products before signed division. Inset is `wrap32(offset*scale)/100`. Fill source width is the native int32 conversion of `binary32(clamp(rate,0,100)/100.0F) * binary32(width-2*inset)`; positive infinity converts to INT32_MIN. The background uses the image's upper half, destination width `wrap32(width*scale)/100` and unscaled half-height. Fill uses the lower half, **source U=inset**, destination X+inset and that computed source width. Its destination width is `wrap32(fillWidth*scale)/100`, except a positive clamped rate with product<=99 forces1 pixel; rate0 keeps zero. Graphics is required only after image/crop/math work, immediately before the first draw. Scratch and the background drawing survive a failing fill draw.

Port: [gauge_drawing.cpp](../../src/core/gameplay/gauge_drawing.cpp) implements this complete selected enemy draw chain with explicit image/alpha/viewport/output interfaces. The bitmap backend supplies resource23, existing ImageDraw/mesh submission and decoded alpha; MissionScene streams requests as the native cursor traverses the live world. [Tests](../../tests/test_gauge_drawing.cpp) cover the5/6 threshold, palette faults, draw order, parameter extras, force/early exits, anchors, clipping, zero/minimum pixels, scaled source inset, reused scratch/suffix and partial failures. HumanSystem.DrawDynamic also calls DrawParamGauge but has a preceding attack-icon path; that entire method and Stage.DrawPlayerMpGauge remain unbound.

### Form push and dialog markers

`FormBase.Push` (`0x1bda598`, `kairo:0x060019e6`) reads the form's manager at +0x40. If non-null it calls `FormManagerBase.Push` (`0x1bdf670`) with parent=this, child=argument and option=false. It ignores the manager return and returns true. An absent manager also returns true. It does not reject a null child. Confidence high.

`SelectForm.Push` (`0x11eec34`, `game:0x06000837`) writes halfword 0x0101 to static +0x30 before calling FormBase.Push. This sets both autoSave_ (+0x30, metadata field `0x04000b41`) and evenCheck_ (+0x31). Both changes survive an exception from the manager; there is no rollback. The two flags share storage with the previously recovered periodic event flag.

The three MyFormBase.Push wrappers differ:

| Method / registry id | Factory | Marker behavior |
| --- | --- | --- |
| `0x11d7498`, `game:0x06000758` | SubForm.MakeDialog(string), `0x11f609c` | Does not change dialog_; directly invokes FormBase.Push. |
| `0x11d750c`, `game:0x06000759` | SubForm.MakeDialog(string,string), `0x11f5fb4` | Stores the factory result in dialog_ (+0xc0) before FormBase.Push. |
| `0x11d758c`, `game:0x0600075a` | SubForm.MakeDialog(string,string,int), `0x11f6108` | Same marker write after successful factory return. |

The three-argument factory first calls the two-string factory, requires a non-null result, and writes the int argument at +0x13c. These factory bodies are delegated to the UI host in the wrapper port; their dialog content/layout is not reconstructed here. The one-string factory obtains its second argument from literal GOT 0x272ee88. No default text is invented in the portable interface.

A factory exception leaves the previous dialog marker untouched; a manager exception leaves the newly assigned marker in place. In SelectForm.Update (`0x11ebde8`), a non-null marker is cleared and causes an early true return after the earlier common updates. This is a one-shot update suppression, not a port of form dismissal. A scoped immediate-store scan found the two MyFormBase writers and the Update clear. The apparent +0xc0 store in SelectForm._Draw (`0x11f103c`) is stack-relative and is not a dialog_ write.


SelectForm event scheduling is documented in [events.md](events.md): dialog/state/balance gates, loopCounter_ arithmetic, evenCheck_ initialization and periodic requests, and the portable reached queue/selection block. The complete form Update and Push behavior remain unported.

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

### Mission movement and attack controls

InputSystem.Init (0x115b3cc,276 bytes) stores supplied-world And({28,0,1}), writes lastDownFrame_=-1 and subscribes event9. It preserves oldUp_ (+0x28), oldDown_ (+0x29) and frame_ (+0x2c); lastDownFrame_ is +0x30. A fresh managed object supplies zero flags/frame. Update does not iterate the stored subset: controls act on the globally selected hero through MemberSystem.

InputSystem.Update (0x115b4fc,976 bytes), factory index0/pass0, queries resolved Canvas key states0x20000 then0x80000. It looks up surface controls(group35,value0x10011) then(group35,value0x10013), capturing both before event delivery. For each nonnull control, up first then down, a rising edge sends event9/TouchEvent type0 and afterward stores the current frame into lastDownFrame; a falling edge sends type1. It publishes both old flags after both branches, including when controls were missing. Adding a missing control while its key remains held therefore produces no down event. Earlier mutations survive delivery failure.

It then resolves HumanSystem, retaining the result before testing dash gates. When up is held, lastDownFrame must be>=1 and captured frame>=wrap32(lastDownFrame+live FPS) to call AccelerateSpeed. A press starting at frame0 never accelerates until another transition updates that timestamp. Without up, set lastDownFrame=-1 and call ResetSpeed every pass, even when down is held. A successful pass increments frame using wrap32 then signed remainder2147483647. Unused null HumanSystem results do not fault until the selected helper requires them.

TouchComponent.id (+0x18) and value (+0x2c) are projected as InputControl.group/value, not an ECS entity ID. InputSystem.OnTouchEvent (0x115b8cc,804 bytes) requires event/component, then accepts only group35, high word1, low value17 or19. It captures that low value and event type before querying the required MemberSystem/hero/Human. Dead states4/5 return before event-type filtering. Releases1/3/6/10 ignore states0/3/6; otherwise zero Speed.z then resolve HumanSystem and ChangeState0. Press/enter/on types0/2/13 skip attack suppression/state change for states1/3/6; other states call InactiveAttackButton then ChangeState1. Only final state1 obtains fresh HumanSystem.GetMoveSpeed and writes its negative for17 or positive for19. It does not directly change Position or allow free lateral movement.

StageSystem.Init creates three distinct AttackButton objects. OnTouchEvent (0x1166564,404 bytes) accepts group10/type2 with value0x230000|index for attack selection. Only state0 activates: ChangeState1, write attackAttribute_/lastActiveButton_, then capture the current array and deactivate other state1 objects by identity. Other states are retained. InactiveAttackButton (0x115bbf0,112 bytes) captures that array and changes only state1 objects to0; it preserves both selected indices. Group35/high-word1/value17 sets moveFrontPushed_ before any event-type filter; value19 does not. Releases1/3/6/10 restore lastActiveButton when it is not-1 and that required button is state0, then update attackAttribute_. No Stage-state or HP gate is added to this handler.

HumanSystem.AccelerateSpeed (0x1158eec,464 bytes) traverses the whole Human subset, skips dead states4/5, and stores min-ordered(dashSpeed+binary32(0.1),2.0). The increment literal is0x3dcccccd at ELF0x6278b8; the positive-cap comparison preserves NaN. ResetSpeed (0x11590bc,356 bytes) clears dashSpeed on every Human, including dead actors; neither helper iterates Input28 or modifies ordinary Speed directly. Human4 applies that dash to walking afterward.

The desktop MissionInput adapter binds these exact kernels before Stage3/Human4, uses stable native control identities and dispatches event9 in Input -> Stage order. Boss movement release resolves those same objects. Three button states/frames, last-active index, initial held mask and move-front flag are explicit scenario inputs; no Stage.Start initialization is synthesized. SDL supplies logical held keys from its public keyboard-state API, with focus loss releasing them. The original Canvas device remapping/lock implementation is external; the fresh scenario selects an unlocked logical-input provider. No Canvas/Joystick/Keypad implementation is reconstructed. Exact public-source searches for Kairo Canvas/CheckKeyState did not locate a matching implementation; only its call contract was inspected.

QuestForm.OnTouchEvent (0x11dfda0,244 bytes) captures type/control group before forwarding event9, then invokes OnTouchCamera only for captured group17 and returns false in that branch. Other controls return true outside the DEBUG-specific suffix. This full form/cursor path is unbound; SDL attack controls use the narrower normalized event9 projection. Enter and replay pulse0x100000 feed the existing Stage key-pulse interface for native result confirmation; no retry/crystal/ad decision is selected. QuestForm.OnTouchCamera's drag path updates the form offsets, whereas CameraSystem.OnTouchCamera only records flags/history. Do not substitute one for the other.

Port: input_frame.hpp/.cpp and the owning Human frame module have synthetic ordering/failure/boundary tests and are called by the scene host; see [integration](integration.md#native-movement-attack-and-input-replay-s1-a2a3a5). Evidence: local research/exports/mission-input.tsv, mission-input/, mission-input-helpers.tsv and mission-input-helpers/, all full instruction coverage, Ghidra image base0x100000 subtracted. TouchComponent field order/type metadata agrees with the consumed offsets. A timed-out decompiler run was replaced by bounded instructions; no decompiled-source completeness is claimed.

### Quest camera, cursor and viewport

World.CreateCamera (0x1133520, 72 bytes) uses the supplied world's EntityManager to create/register a type-0 non-temporary entity, adds a zero Position (including offsets and null parent), then AddCamera(0,100). AddCamera (0x112cec8, 132 bytes) allocates a **fresh** CameraComponent, writes angle/scale and inserts slot 3; it is not a pooled payload. The nullable get_camera (0x112ce40) requires a compatible stored component type. CameraSystem.Init (0x1143ea4, 176 bytes) stores the created entity, requires its Camera, sets angle=45, clears dirty/drag/cursor-revision flags, writes pinchScaleValue=120 and replaces touchCameraPoint_ with an empty java.util.Vector<int32[]>. It does not destroy an older entity or reset getScaleRange_tmp. The derived constructor creates that two-entry range; the complete BaseSystem constructor remains outside this projection.

CameraSystem.Update (0x1143f70, 328 bytes), factory index 8, is gated to pass zero. Require its stored entity, invoke get_camera and deliberately discard its nullable result. Resolve MemberSystem from the current global world, then obtain GetFormationX(0) and GetFormationZ(0). Each indexed overload resolves the hero twice: capture Position.x/z, then subtract the fresh hero HumanComponent formation_x/z as binary32; add wrap32((index%4)*36) for X or wrap32(trunc(index/4)*36) for Z, converted to binary32. Preserve the subtraction and addition separately. This differs from the already ported Entity overloads used by human movement.

Truncate each formation coordinate with the native FCVTZS/positive-infinity quirk, add 72 with int32 wrapping, project X=wrap32(x-z), Y=trunc(wrap32(x+z)/2), then add 20 to X and subtract 60 from Y. Convert those integers to binary32 and call CameraSystem.SetCursor, which resolves the current QuestForm and converts back to int32 using the same native conversion. The intermediate float round trip matters at large coordinates. This path updates the cursor and its saved center; it does not copy the camera entity's Position into the form.

QuestForm.GetGameWidth/GetGameHeight (0x11df354/0x11df3e0, 140 bytes each) require the form's GameView, query its corresponding game dimension, subtract QuestForm static VIEW_X/VIEW_Y, multiply by 100 with int32 wrapping, then SDIV by the current **QuestForm.scale_** (+0x1dc). SDIV by zero returns 0 and int32-min/-1 returns int32-min. The port reuses the existing GameView binary32 size conversion and SelectForm signed scaling helper. VIEW_X/VIEW_Y initially equal 0/0: QuestForm..cctor copies int32 values (0,0,240,240) from ELF 0x625de0 into statics +0x18..+0x24. They remain explicit mutable inputs rather than inferred desktop sizes.

QuestForm.SetCursor (0x11e1344, 484 bytes) performs five ordered world.map queries: the first result is discarded and the next four are required. It then queries/discards game width and height, writes normal-save SecureInt slots 17=x and 18=y through fresh array selections, rereads slot 17 and sets camX_=wrap32(centerX-trunc(freshGameWidth/2)), then rereads slot 18 and sets camY_ similarly. It does not set isUpdateStaticRender_. Partial save writes and camX survive a later failure. Camera offsets are **QuestForm +0x110/+0x114**, not AppData offsets; AppData.frmQuest_ supplies the form.

QuestForm.SetCameraPosition (0x11e00a0, 360 bytes) instead requires four world.map results, queries/discards width and height, writes camX_/camY_ directly, captures the save array, obtains a fresh width and stores slot 17=wrap32(suppliedX+trunc(width/2)). It then selects the array again, captures the **current** camY_, obtains a fresh height and stores slot 18=wrap32(currentY+trunc(height/2)). Set isUpdateStaticRender_ (+0x1fc) only after that final store. There is no coordinate clamp in either method. The decoded save-value adapter is explicit; SecureInt allocation/encryption and a real save envelope are not implemented by this work.

SetScale (0x1144498) obtains a minimum of 10 when the current global GameView game width <=240, otherwise 75; maximum is 200. Reuse the game's upper-then-lower clamp. If unchanged, return before reading Position/form; otherwise store CameraComponent.scale first, then read the camera entity's current Position.x/y and call the distinct position path. **CameraComponent.scale and QuestForm.scale_ are separate fields.** Do not synchronize them by assumption. GetScaleRange (0x11446f4) captures its old scratch array before obtaining the minimum, writes minimum to that captured entry 0, rereads the scratch field and writes 200 to entry 1, returning the fresh array. A host replacement or second-entry fault preserves the first write.

OnTouchCamera (0x1144158, 832 bytes) validates camera entity/Position, event and Camera in native order. Raw event 0 clears history and enables dragging; 1 probes FirstElement when nonempty/dragging and clears dragging; 5 marks cursor revision only when history is nonempty; 12 sets dirty. Event 8 reads both scale values before querying platform touch count: count>1 returns before flag/history changes. Otherwise set dirty, clear revision if dragging and revision were both set, and compute each delta as SDIV(wrap32(delta*100),capturedScale), converted to binary32 and back to int32. At size<=4 create a new pair; at size>4 overwrite the last pair, remove the last entry, then insert it at index 0. Thus ordinary history reuses five arrays, retaining aliases and partial writes to malformed arrays. Other event types only validate inputs. **This body neither pans the form nor calls SetScale**; no missing gesture behavior is invented.

Public dependency boundary: the metadata-selected operations are [Java 8 Vector](https://docs.oracle.com/javase/8/docs/api/java/util/Vector.html) constructor, Size, RemoveAllElements, FirstElement, LastElement, RemoveElementAt, InsertElementAt and IsEmpty. Their bounded sequence/alias contract uses standard C++ vectors/shared pointers; no Java Vector implementation, synchronization machinery or runtime was reconstructed or copied.

The concrete QuestCameraCellView bridge resolves the current QuestForm separately for width, height and origin; the origin uses its camX_/camY_ and the existing AppData shared projection scratch. It implements [cell visibility inputs](ecs.md#cell-visibility-selection), with the same StageQuestForm scale reference used by stage startup. The 120-pass ordinary/physical shot scenario now runs actual Camera index 8 and CellCulling index 20 on pass zero around the existing flight/collision/death/cleanup chain, including the required stopwatch interface. This proves composed portable behavior on supplied synthetic catalogs/view/save values; the desktop mission_map mode binds the same camera/cell chain and real archive renderer with explicit effective viewport/chip inputs. Live window resizing/input and protected save restoration remain open; see [integration](integration.md#mission-map-following-camera-and-cell-visibility-s1-a1a3).

### Quest mission render callbacks

`QuestForm.DynamicRender` (0x11df6e4, 824 bytes) first requires the stored FormManager. Capturing (`+0x5c`) bypasses pause/activity checks. Otherwise pause (`+0x32`) returns false; when unpaused, draw only if `shouldDrawInBack_` (`+0xd4`), the supplied Offscreen's `isEmpty_` (`+0x40`), or `MyFormBase.Active` is true, in that short-circuit order. `StaticRender` (0x11dfa1c, 56 bytes) implements only the latter three-way predicate: it does not draw or consult manager pause/capture. Background drawing can bypass a null surface at the gate, but DynamicRender still requires that surface later.

On an accepted dynamic callback, require Graphics and disable clipping. If the current AppData.resQuest_ is nonnull, PushOrigin(0,0,false), reread resQuest_, obtain image 90 and capture its width/height. Let W=QuestForm.GetGameWidth; compute H with **int32 wrapping multiplication and signed division at every step**: `((wrap32(W*100) / imageWidth) * imageHeight wrapped) / 100`. ARM64 division by zero returns zero; int32-min/-1 retains int32-min. DrawScaledImage uses the complete image at `(0, wrap32(GetGameHeight-H))`, destination `(W,H)`, then PopOrigin. This is not equivalent to a binary32 aspect-ratio calculation. No origin cleanup protects a failure in this prefix.

PopBlendMode next; its matching push belongs to the enclosing Offscreen renderer. Set the supplied surface's static transform to `(x=0,y=0,depth=0,scaleX=1,scaleY=1)`. The third argument is binary32 depth, not rotation; the [surface transform port](assets.md#offscreen-transforms-and-projection) now supplies that contract and the serialized camera provenance. If the shared QuestForm static-render marker is set, clear it **before** RenderStatics. A static-render failure retains that cleared marker and does not clear sprite queues. `UpdateStaticRender` (0x11df46c, 12 bytes) only sets the same marker.

Capture the old VIEW_W/H, then set VIEW_W from GetGameWidth and VIEW_H from GetGameHeight. In that temporary logical viewport call the current AppData.DrawZSortObjects(Graphics,0), followed by the stored World.DynamicDraw(Graphics). On success or a failure in either viewport query or either draw, restore only VIEW_W/H and call ClearZSortObjects on the **fresh** AppData field. Thus both queue counts are cleared while allocated rows remain reusable; callbacks can replace the application before cleanup. VIEW_X/Y and graphics state are not restored here. Cleanup failure supersedes a pending draw failure. LSDA confirms the protected viewport-query scope; background, blend and static-render setup are outside it.

`DrawOffscreen` (0x11df004, 208 bytes) captures `this.offscreen_.image_` and all four VIEW fields **before** querying GetGameWidth/Height. It passes source rectangle `(VIEW_X,VIEW_Y,gameWidth,gameHeight)` and binary32 destination `(VIEW_X,VIEW_Y,VIEW_W,VIEW_H)` to DrawScaledImage. It does not subtract the camera again, modify a matrix, or test pause/activity. For supplied GameView 480x720, VIEW_X/Y=20/30 and scale150, source size is 306x460 and the captured destination can be 460x690. These are contract-test inputs, not recovered production defaults.

Port: [quest_render.hpp](../../src/core/gameplay/quest_render.hpp) and its implementation compose the existing AppData sprite consumer and queue clearing with the real camera/viewport kernels. [test_quest_render.cpp](../../tests/test_quest_render.cpp) covers all 32 capture/pause/background/empty/activity combinations, draw order/depth, integer aspect-ratio overflow/zero, captured compositing fields, reusable queues, reentrant AppData replacement and scoped failures. Offscreen targets, enclosing blend/matrix scopes, world rendering, manager activity and graphics remain explicit host services; these tests record draw calls and do not establish a desktop mission or Android equivalence.

## Port

`src/core/gameplay/form_push.{hpp,cpp}` implements FormBase.Push, SelectForm.Push and the three MyFormBase dialog-push wrappers. `FormStackHost` owns manager behavior; `DialogFactory` owns dialog construction. Nullable shared text and form objects preserve null values and identity. `consume_dialog_update_gate` ports only the marker consumption block. `tests/test_form_push.cpp` covers absent managers, null children, factory argument forwarding, overload asymmetry, marker ordering, flag ordering and exceptions. The manager stack, dialog rendering and full form update remain open.

[quest_camera.hpp](../../src/core/gameplay/quest_camera.hpp) and its implementation cover camera factory/init/update, nullable getter, cursor/position and viewport paths, zoom limits/range, delta history and the cell-visibility bridge. Indexed formation overloads extend the existing stage_recovery module. tests/test_quest_camera.cpp covers ordering, replacement, partial failure, separate scales, history reuse, native grid coordinates and viewport selection; tests/test_emitter_frame.cpp composes 120 pass-zero updates. Remaining form-loop work includes FormManager/DoMethod, FPS/execution scheduling and complete form/UI ownership; normalized SDL input and archive-backed camera/cell rendering are bound in the selected scene. ECS pause dispatch belongs to the ECS module.

## Evidence

- Enemy gauges: complete bounded exports in ignored `research/exports/mission-hud/`, `mission-actor-hud/`, `mission-gauge-helpers/` and `mission-gauge-helpers2/`. Library SHA256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`; printed addresses include Ghidra base `0x100000`. `palettes.json` resolves GOT `0x27315c0` → encoded FieldInfo `0x80000115` → FieldRef138 → native8809/metadata4550 → field24365 → default-data189432. Image.GetPixels crop rows/aliasing come from its bounded wrapper and `<GetPixels>b__0`, while the public Unity API is reused. Confidence high for static contracts; Android graphics order remains unverified.

- Mission drawing: local `research/exports/quest-draw/` bounded Draw, DrawOffscreen, DynamicRender, StaticRender and UpdateStaticRender exports, all full syntactic coverage, for ELF SHA-256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. Dynamic gates at 0x11df724..0x11df748 and 0x11df93c..0x11df980; image-90 sizing at 0x11df798..0x11df824; static transform/dirty handling at 0x11df840..0x11df878; viewport capture/updates at 0x11df894..0x11df8d0; sprite/world calls at 0x11df8e8/0x11df8fc; normal cleanup at 0x11df904..0x11df92c. `dynamic-exceptions.json` records FDE size824 and LSDA 0x91144c: queries at 0x11df898..0x11df8b4 and 0x11df8c0..0x11df8c8 enter the same cleanup chain as the draw calls. Registered field-offset table 0x277ece8 identifies FormManagerBase pause/capturing, QuestForm VIEW fields and Offscreen image/isEmpty. Confidence high; no runtime comparison.
- Camera: local research/exports/camera-system/ and camera-bindings/, all bounded by metadata method ranges and reporting full syntactic coverage. Native image base is 0x100000; addresses above subtract it. Field names/offsets come from registered field-offset table 0x277ece8 and v31 metadata: CameraComponent angle/scale at +0x10/+0x14; CameraSystem camera/dirty/history/drag/revision/pinch/range at +0x28/+0x30/+0x38/+0x40/+0x41/+0x44/+0x48; QuestForm camX/camY/world/scale/static-render marker at +0x110/+0x114/+0x120/+0x1dc/+0x1fc. Confidence high for these static paths; no Android comparison.
- Native viewport: research/exports/cell-viewport/{GameWidth,GameHeight}.asm; static initial values in quest-viewport-init/QuestFormStaticInit.asm and the small int32 literal block at ELF 0x625de0. GOT 0x272fa18 resolves to QuestForm, confirming VIEW_X/Y are its statics. Cursor/position save slot stores at 0x11e1418/0x11e1468 and 0x11e0178/0x11e01e0; offsets at 0x11e14b8/0x11e1500 and 0x11e0118/0x11e011c. Calls 0x1c06880/0x1c068a8 map to SecureInt implicit conversions, retained behind decoded values.
- Camera history usages at GOT 0x272f828/0x272f850/0x272f848 and 0x2730428/0x2730430/0x2730438/0x2730440/0x2731388 resolve to the named Vector operations, not a guessed linked-list implementation. Public Java API contracts are reused only at the reached methods.

- Push-chain evidence: local bounded exports `research/exports/select-form-push/` and `research/exports/select-dialog/`; metadata parameter native kinds confirm string/string/int32 for the MyFormBase wrappers. SelectForm static fields use the native field-offset table at ELF 0x277ece8. Build evidence: `research/exports/form-push-build.log`.

- [../archive/reports/09-native-callbacks.md](../archive/reports/09-native-callbacks.md), [24-frame-conditions.md](../archive/reports/24-frame-conditions.md), [27-frame-policy.md](../archive/reports/27-frame-policy.md), [28-form-events.md](../archive/reports/28-form-events.md), [32-ecs-animation-order.md](../archive/reports/32-ecs-animation-order.md) (QuestForm chain), [36-pause-hooks.md](../archive/reports/36-pause-hooks.md), [49-continue-game-preparation.md](../archive/reports/49-continue-game-preparation.md) (ChangeCurrentForm caller)
- Local: `build/research/frame-policy-followup/M19675__execute.asm`, `build/research/form-events/M19739_DoMethod.asm`, `build/research/frame-policy/direct-references.json`, `build/research/pause-hooks/` (+`coverage.tsv`)

## Open questions

- Bind HumanSystem.DrawDynamic's attack-icon prefix and HP/MP suffix, Stage HUD, and complete World.DynamicDraw gates/order. Enemy HP drawing is an explicit selected route.
- Establish the creator/restoration path for Gauge19 entities before binding the full GaugeSystem lifecycle. The observed actor HP route calls static drawing helpers; a scan found no direct external B/BL caller of World.CreateGauge, which does not exclude indirect/restored paths.

- Bind actual QuestForm control construction/hit testing and Canvas lock/device selection to the normalized input adapter; native logical movement/attack controls are host-reachable.
- Bind remaining stage dialog answers and pause/resume without selecting a retry payment/ad decision automatically.
- Bind desktop GameView dimensions/scale, current form/world lifetime, normalized touch delivery, visible-set storage and decoded normal-save providers to the tested camera/viewport kernels. The mission render callbacks now consume the real sprite queue in tests; production archive managers, offscreen targets and full world draw remain open. OnTouchCamera's flags/history do not establish another camera-motion or pinch path; follow actual callers/writers before adding one.

1. (blocking) First form passed by Main.OnCreate and the form stack/transition protocol (`ChangeCurrentForm`, states 1..4, `+0xd8` thread object).
2. (blocking) Writers of `manager+0x88/+0x90/+0x98`, `form+0x48/+0x50`; identity of the static object that suppresses Update.
3. Concrete slot-19 (Update) overrides for boot/menu forms and the first gameplay form.
4. How `+0xb4` (GetFps) is computed.
5. Body of `FormManagerBase.OnDraw` and how DoMethod code 2 (Draw) is used.
6. Which BasePauseSystem-derived systems are toggled by menus; `BaseSystem..ctor`/`SetWorld` initial state.
