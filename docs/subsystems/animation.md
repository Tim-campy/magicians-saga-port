# Animation (ECS animation components and AnimationSystem)

Per-entity sprite animation is driven by three ECS components — `SebComponent` (slot 2), `AnimationComponent` (slot 10), `DirectionComponent` (slot 12) — and by `ecs.AnimationSystem` (factory position 19). Statically established: component layouts, clip transitions (Change / CutIn / Restore), direct `AddAnimation` producers, system registration, Init/Update/Pause dispatch and persisted payloads. Port: transitions, component payload codecs, pooled playback insertion and AnimationSystem Init/Update are tested with live ECS fields. Effect creation now feeds the per-pass animation loop. Resource catalog loading, effective global flags, rendering and other spawn factories remain incomplete. Shared SEB clocks and per-entity counters stay separate; see [../formats/seb.md](../formats/seb.md).

Addresses are ELF VAs (Ghidra exports add `0x100000`).

## Overview

### Components

| Component | Slot | Field (metadata name) | Offset | Width in memory | Meaning |
|---|---:|---|---|---|---|
| SebComponent | 2 | `res` | +0x10 | int32 | Resource (ResourceManager index) |
| | | `id` | +0x14 | int32 | Clip / SEB id ("clip" in the port) |
| | | `frame` | +0x18 | int32 | Per-entity frame counter (not `Seb` instance +0x18) |
| | | `layers` | +0x20 | int[] | Layer array; `[-1]` is one element, not empty |
| AnimationComponent | 10 | `rate` | +0x10 | int32 | Added to `frame` each update |
| | | `backup` | +0x14 | int32 | Pending return clip; **-1 = none** (`get_isPlayingCutInAnimation` = `backup != -1`) |
| DirectionComponent | 12 | `dir` | +0x10 | int32 | Direction, added to clip ids; not normalised to 0..3 by AddDirection |
| | | `locked` | +0x14 | byte | `locked & 1` |

Slot indices come from the index passed by each `Entity.Add<x>` to the entity's virtual slot-5 add (`AddSeb` 2, `AddAnimation` 10, `AddDirection` 12) and from the filter in `AnimationSystem.Init` (2 and 10). Component constructors only forward to the base ctor: there are **no defaults**; every producer writes explicit values. Do not initialise `backup = -1` globally.

Kind predicates used by the transitions: `Entity.get_hasHuman`, `get_hasMonster`, `get_hasEnemy` (Enemy and Monster are distinct components and the code distinguishes them).

### Add methods

- `Entity.AddAnimation(rate, backup)` (`0x112c488`): reuse a pooled component or allocate; `STP` writes **both** args at +0x10/+0x14 (also when reusing); add at slot 10. No `hasAnimation` check in the callers.
- `Entity.AddDirection(dir, locked)` (`0x112db20`): +0x10 = dir, byte +0x14 = `locked & 1`; slot 12.
- `Entity.AddSeb(res, id, frame, layers[])` (`0x1131148`): +0x10/+0x14/+0x18/+0x20; slot 2. Overload `AddSeb(res, id, frame, layer)` (`0x11310b0`) allocates a 1-element array `[layer]` and forwards; `layer = -1` yields `[-1]`.

Component writes happen before the slot add, which notifies subsets (see [ecs.md](ecs.md)).

### Producers (all known direct `AddAnimation` sites)

All seven direct BL sites pass **rate = 1, backup = -1**; they differ in guards. This is a fact about these sites, not a component default.

| Producer | AddSeb arguments | AddAnimation guard | AddDirection |
|---|---|---|---|
| `World.CreateHuman` (`0x1133a14`) | res 1, id `ADD32(dir, 36)`, frame 0, layer -1 (`0x1133d74`) | unconditional (`0x1133d98`) | `(dir, false)` (`0x1133da8`) |
| `World.CreateMapChip` (`0x113368c`) | explicit res, sebId, frame, sebLayer | bool arg `animation` bit0 (`0x113377c`) | none |
| `World.CreateNature` (`0x11337a0`) | res, sebId, frame 0, layer -1 | bool arg `animation` bit0 (`0x1133880`) | none |
| `World.CreateEnemy` (`0x1133fc0`) | res = resolver(record+0x50); id = record+0x5c, or `record+0x30 << 2` if -1; frame = record+0x60, or 0 if -1; layer -1 (`0x1134290`) | only if `BaseData.Check(1)` on the enemy data record is **false** (`0x1134374–0x1134398`) | only if `dir != -1`, `(dir, false)` |
| `World.CreateEffect` (`0x1134a70`) | res/id/frame from args, layer -1 (`0x1134bb8`) | bool arg `anim` bit0 (`0x1134c8c`); `loop` is unrelated | — |
| `ProjectileSystem.Fire` (`0x1157230`) | — (receiver is the `projectile` argument) | bool arg `anim` bit0 (`0x11573c8–0x11573dc`); independent of `rotate`, which gates `AddModifyAnimation` | — |
| `EmitterSystem.Update` (`0x114ba28`) | particle from `World.CreateParticle(resId 19, sebId 1, imgId = BulletComponent.data+0x34, …)` | unconditional on the non-null particle (`0x114be38`), inside the guarded path below | — |

Enemy: frame is **not** always 0; direction may be absent. Enemy record fields +0x30/+0x50/+0x5c/+0x60 are not named; the meaning of `Check(1)` is unknown.

EmitterSystem path to the particle (per enumerated entity with `Entity.get_emitter`): (1) `q, r = divmod(emitter+0x1c + 1, emitter+0x18)` via SDIV/MSUB, the incremented counter stored back to +0x1c; `r != 0` → next entity; (2) if `emitter+0x20 != -1` and `q > emitter+0x20`, queue the entity into another system collection and skip; (3) require `emitter+0x10 == 0`; (4) the particle branch is `emitter+0x14 == 0` (1 and 2 take other paths, others skip); (5) require `hasBullet`; (6) read `ModifyAnimationComponent+0x28` if present else 0 (used later on the particle, does not gate AddAnimation); (7) position/speed from the emitter entity, speeds divided by 10.0f; then CreateParticle and AddAnimation(1,-1) on the **particle**, never on the emitter.

### System registration and dispatch

- `Systems.CreateSystems` puts `AnimationSystem` at array position **19** (after `ModifyAnimatinSystem` at 18, before `CellCullingSystem` 20 and `RenderSystem` 21), constructed then `set_enabled(true)`.
- `SystemManager.Add` appends it to the IInitSystem and IUpdateSystem lists (interface offsets 13 and 14 in its vtable, start 58921) and to the IPausable list (offset 16). It is entry 16 (zero-based) of the established 26-system update list; see [ecs](ecs.md#complete-update-schedule).
- `AnimationSystem.Init` (`0x1141a58`): builds `Filter.And([2, 10])` (Seb + Animation), `World.GetSubSet` → stored at `this+0x20`; allocates an array of **24** ResourceManagers filled by `AppData.GetResourceManager(0..23)` (`this+0x28`).
- `SystemManager.Update(pass)` → `AnimationSystem.NeedsUpdate` (`0x1141bb8`, returns enabled byte +0x18, ignores pass) → `AnimationSystem.Update` (`0x1141bc0`). Reached from `QuestForm.Update`, which may run up to 3 ECS passes per form event (see [ecs.md](ecs.md)).

### AnimationSystem.Update (per pass)

1. Walk the ResourceManager array (`this+0x28`, inner arrays +0x18) and call `Seb.Frame` on each non-null SEB (`0x1141c24–0x1141cb4`) — the shared manual frame advance.
2. Enumerate the subset's `HashSet<Entity>` (`Subset+0x30`; `GetEnumerator`/`MoveNext`/`Dispose`; slot order, see [ecs.md](ecs.md)). For each entity: require Seb; select by `res`/`id` as below and capture maxFrame; add animation.rate to the component frame with int32 wrap. When backup != -1 and the new frame is signed **>= maxFrame**, restore. Re-fetch the component and take ARM signed remainder using the maximum captured **before** restoration (`0x1141e08–0x1141e24`). No positive-maximum guard or lower-frame clamp is present.

Selection is direct indexing of AnimationSystem.resourceManagers_ (+0x28), then ResourceManager.seb (+0x18), not a GetSeb callback or a search. Its asymmetry matters:

| Condition | Outcome before entity-frame mutation |
| --- | --- |
| Negative resource ID | Skip entity |
| Negative clip ID or resource ID >= manager-array length | Skip entity |
| Null selected manager | Skip entity |
| Null manager array or selected manager's SEB array | Error |
| Clip ID >= selected SEB-array length | Bounds error |
| Null selected SEB | Error (shared phase skips this null entry) |

The shared phase captures the manager array, skips null managers and null SEBs, and requires every nonnull manager's SEB array, even when its manager is not selected by an entity. It captures each inner array's initial length and re-reads that manager's array for successive entries. Shared references are not deduplicated: a repeated SEB pointer is advanced per occurrence. The entity phase re-reads the manager-array field. Shared advancement finishes before the entity cursor opens; earlier shared/individual frame writes survive a later failure.

Seb.Frame (kairo:0x0600176d, entry 0x1bb16c4) leaves its shared frame unchanged when AutoAnimation is true or IApplication.static+0x1c is false. Otherwise add 1 with int32 wrap and apply SDIV/MSUB remainder. These gates do not suppress the separate entity loop. ARM divisor zero has quotient zero, retaining the numerator; INT_MIN/-1 yields remainder zero; other remainders follow division toward zero and can be negative. The port computes the nonzero-divisor remainder in int64 to avoid C++ undefined behavior.

Initialization stores the And(2,10) result first (null is retained), publishes a fresh 24-element null array, then requests AppData.GetResourceManager(0..23) in order. Returned null entries are allowed. A lookup failure keeps the new subset, array and prior filled entries. The current destination array is captured before each lookup; replacement during that lookup redirects the next iteration, not the current store. Reinitialization replaces the array without destroying prior referenced storage.

The per-entity counter is separate from the SEB object's own frame; do not merge them or assume both step by 1.

### Pause

`AnimationSystem.Pause(paused)` (`0x11420ac`, the IPausable implementation): if `enabled == (~paused & 1)` return; else write enabled, then call slot 5 (OnDisabled) when pausing or slot 4 (OnEnabled) when resuming. For AnimationSystem those slots are the empty `BaseSystem.OnEnabled/OnDisabled`. Although `SystemManager.Update` has no enabled guard, `AnimationSystem.NeedsUpdate` reads enabled, so pausing prevents its Update body from running. This corrects the earlier always-true interpretation: the bounded export at 0x1141bb8 loads byte +0x18 and returns it (local `research/exports/system-gates/AnimationSystem.asm`).

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `ecs.AnimationSystem..ctor` | game:0x06000dc4 | Constructor (called by CreateSystems) | medium |
| `ecs.AnimationSystem.Init` | game:0x06000dba | Subset And(2,10), 24 ResourceManagers | high |
| `ecs.AnimationSystem.NeedsUpdate` | game:0x06000dbb | Returns enabled byte +0x18; ignores pass | high |
| `ecs.AnimationSystem.Update` | game:0x06000dbc | Ordered shared clocks, direct selection, frame/restore/remainder | high |
| `main.AppData.GetResourceManager(int32)` | game:0x0600023f | Thirteen selected fields; other IDs return null | high |
| `ecs.AnimationSystem.Pause` | game:0x06000dbe | IPausable.Pause: toggles enabled + hook | high |
| `ecs.AnimationSystem.ChangeAnimation` | game:0x06000dbf | Set clip (+direction), optional frame reset | high |
| `ecs.AnimationSystem.CutInAnimation` | game:0x06000dc0 | Set return clip and clip, reset frame | high |
| `ecs.AnimationSystem.RestoreAnimation` | game:0x06000dc1 | Consume return clip, re-align direction | high |
| `ecs.AnimationSystem.Serialize` / `Deserialize` | game:0x06000dc2 / dc3 | RET (system has no own state in stream) | high |
| `kairo.unity.ecs.Entity.get_animation` / `get_hasAnimation` | game:0x06000b1e / b1f | Slot 10 accessors | high |
| `kairo.unity.ecs.Entity.get_direction` / `get_hasDirection` | game:0x06000b43 / b44 | Slot 12 accessors | high |
| `kairo.unity.ecs.Entity.get_seb` | game:0x06000b99 | Slot 2 accessor | high |
| `kairo.unity.ecs.Entity.get_hasHuman` / `get_hasMonster` / `get_hasEnemy` | game:0x06000b68 / b86 / b54 | Kind predicates | high |
| `kairo.unity.ecs.Entity.AddAnimation` | game:0x06000b20 | Write rate/backup, add slot 10 | high |
| `kairo.unity.ecs.Entity.AddDirection` | game:0x06000b45 | Write dir/locked, add slot 12 | high |
| `kairo.unity.ecs.Entity.AddSeb` (array) | game:0x06000b9c | Write res/id/frame/layers, add slot 2 | high |
| `kairo.unity.ecs.Entity.AddSeb` (single layer) | game:0x06000b9b | Wrap layer in 1-element array | high |
| `ecs.AnimationComponent.get_isPlayingCutInAnimation` | game:0x06000d0f | `backup != -1` | high |
| `ecs.AnimationComponent.Serialize` / `Deserialize` | game:0x06000d10 / d11 | Payload ([animation-payload](../formats/animation-payload.md)) | high |
| `ecs.DirectionComponent.Serialize` / `Deserialize` | game:0x06000d33 / d34 | Payload | high |
| `ecs.SebComponent.Serialize` / `Deserialize` | game:0x06000dab / dac | Payload with layers | high |
| `ecs.AnimationComponent..ctor` / `ecs.DirectionComponent..ctor` / `ecs.SebComponent..ctor` | game:0x06000d12 / d35 / dad | Forward to base; no defaults | high |
| `kairo.unity.ecs.World.CreateHuman` | game:0x06000bef | Complete human component factory, composed with party entry | high |
| `kairo.unity.ecs.World.CreateMapChip` | game:0x06000beb | Complete map-chip factory, optional playback and mission-load integration ([details](rewards.md#mission-map-construction)) | high |
| `kairo.unity.ecs.World.CreateNature` | game:0x06000bec | Nature object factory | medium |
| `kairo.unity.ecs.World.CreateEnemy` | game:0x06000bf0 | Enemy factory | medium |
| `kairo.unity.ecs.World.CreateEffect` | game:0x06000bf5 | Effect factory | medium |
| `kairo.unity.ecs.World.CreateParticle` | game:0x06000bf7 | Particle factory (args from emitter path) | low |
| `ecs.EmitterSystem.Update` | game:0x06000e5f | Emits particles, adds Animation to them | medium |
| `ecs.ProjectileSystem.Fire` | game:0x06000f81 | Configures projectile; Animation if `anim` | medium |

## Behaviour

All additions are 32-bit wrapping (`ADD32`); never rely on C++ signed overflow.

**ChangeAnimation(entity, id, reset)** (`0x11420e0`):
1. If hasHuman **or** hasMonster: `id = ADD32(direction.dir, id)`. No hasDirection guard: a missing Direction faults before Seb is touched.
2. `seb.id = id`.
3. If `reset`: `seb.frame = 0`, else keep frame. `res` and `backup` untouched.

**CutInAnimation(entity, id, returnId)** (`0x1142168`):
1. If not hasAnimation → return (Seb not required).
2. `animation.backup = returnId` — the **explicit argument**, not the current clip; no stack. Repeated cut-ins overwrite.
3. Only if hasDirection and (hasHuman **or hasEnemy**): `id = ADD32(dir, id)`. (Enemy here, Monster in Change/Restore.)
4. `seb.id = id; seb.frame = 0`. A missing Seb faults **after** step 2 (backup already written).

**RestoreAnimation(entity)** (`0x1141f40`):
1. Requires Animation; if `backup == -1` return (Seb not required).
2. `seb.id = backup; seb.frame = 0`.
3. If hasHuman or hasMonster: `adjusted = id < 0 ? ADD32(id, 3) : id; group = adjusted & 0xfffffffc; seb.id = ADD32(dir, group)` (i.e. truncating division by 4 toward zero, times 4, plus current direction; `0x1142004–0x114201c`). Missing Direction faults here, leaving the copied id and frame 0, with backup **not** consumed.
4. `backup = -1` last (`0x1142034`).

Examples (port tests, direction 3): human Change(12, reset) → 15, frame 0; monster CutIn(20, 11) → 20 (no direction), then Restore → 11 (group 8 + 3); enemy CutIn(20, 10) → 23, Restore → literal 10; human Restore of -5 → -1 and of -3 → 3 (truncation toward zero, not a mask); `INT32_MIN` → `INT32_MIN + 3`; additions wrap (dir `INT32_MAX` + restore `INT32_MAX` → -5).

Spawn values never come from component defaults: Human/MapChip/Nature/Effect/Projectile/Particle use rate 1 / backup -1 only when the guarded call is reached; enemies may start at a non-zero frame and may lack Direction; static objects may lack Animation.

### ECS playback insertion

Entity.AddAnimation (game:0x06000b20, 0x112c488, 312 bytes) acquires a component and writes rate and backup together at +0x10/+0x14 before inserting slot 10. If a nonempty pool returns null, the native helper allocates a replacement; the portable PlaybackPool boundary must return a valid acquired component or throw. Neither field is clamped and no existing-slot guard precedes acquisition/writes. An occupied slot ignores the incoming object but its payload is already written; returning the installed pooled object changes that payload without a new notification. A notification error retains both stores and slot insertion. The known producers' (1,-1) values remain caller arguments, not defaults.

Port: `visual_components.hpp/.cpp` defines EntityPlayback containing the existing AnimationPlayback state and implements add_animation_component. EffectFactoryHost uses it by default; the effect test checks actual slot 10, rate 1/backup -1 and live Image+Animation group membership. The dedicated visual_components test checks negative/full-width arguments, reuse, occupied-slot objects and pool/notification failures. The animation_pass integration below supplies live transition references and the per-frame loop.

Evidence: local `research/exports/animation-producers/M36765_AddAnimation.asm` and `research/exports/visual-components/metadata-contract.json`; combined field store at 0x112c594 and slot selection at 0x112c5b0. Confidence high; no runtime comparison.

### Resource preparation and effect durations

AppData.GetResourceManager(int32) (game:0x0600023f, 0x1103e90, 152 bytes) selects only the fields below. The jump table accepts IDs 1..23 but sends the unlisted cases to null; ID 0 and values outside that range also return null. It performs no allocation, loading, fallback or null replacement. Animation Init therefore does not imply 24 nonnull managers.

| ID | Field | AppData offset |
| --- | --- | --- |
| 1 | resChara_ | +0xd8 |
| 2 | resCom_ | +0xe0 |
| 3 | resEnemy_ | +0xe8 |
| 4 | resEquipment_ | +0xf0 |
| 7 | resQuest_ | +0x100 |
| 8 | resResident_ | +0x108 |
| 11 | resSelect_ | +0x118 |
| 18 | resChip_ | +0x140 |
| 19 | resBullet_ | +0x148 |
| 20 | resEffect_ | +0x150 |
| 21 | resHat_ | +0x158 |
| 22 | resWeapon_ | +0x160 |
| 23 | resGauge_ | +0x168 |

The frame limit is the signed second SEB header short, for both disk modes; [SEB](../formats/seb.md#reading-it) records the load/reset evidence. No scan of FrameNo records is needed. The effect factory's pre-allocation duration query uses this same selected manager's SEB array and clip +0x1c; a null manager/array/clip or invalid index faults before creating the effect. A successful query gives lifetime wrap32(maxFrame-1), so a zero header gives -1.

`animation_resources.hpp/.cpp` adds the exact AppData selector and clip maximum query, plus PreparedAnimationResource for desktop preparation from already selected/parsed files. It owns contiguous optional clocks and publishes a stable pointer array, retaining null ID gaps without one heap allocation per clock. Each supplied entry is a separate prepared instance; sharing an original loaded instance remains an explicit host binding. The [catalog indexing slice](../formats/inf.md#indexing-selected-rows) now resolves missing/duplicate IDs separately from file/locale selection. These adapters do not implement asynchronous LoadReady/LoadStart or choose a language/path policy.

`src/platform/desktop/animation_trace.cpp` exercises selected SEB -> prepared effect resource 20 at a supplied clip ID -> actual Init/GetSubSet -> CreateEffect -> ordered entity-component notifications -> live animation passes. It emits JSON Lines for the maximum, initial effect lifetime and independent clocks. Position is a diagnostic origin, image override -1, and both global gates are explicit arguments. The scenario does not execute the other systems, expire Garbage, render or run the original game. `tests/test_animation_scenario.py` drives it with synthetic files in CTest; the local corpus audit runs all 303 supplied SEBs for a full period plus two passes with no differences (maxFrame range 1..101). This validates the file-to-rule integration rather than actual catalog selection.

Evidence: local `research/exports/animation-resources/{GetResourceManager,SebDispose}.asm`, `resource-map.json` (23 jump-table bytes at ELF 0x627ac4 resolved to field-offset metadata), `corpus.json`, and existing `seb-readers/Seb_load_18983.asm`. Selector dispatch is 0x1103e90..0x1103eb4; field load 0x1103f1c, default null return 0x1103f20. The effect factory reads the selected SEB array at 0x1134b14, clip maximum at 0x1134b34 and subtracts one at 0x1134b38. Confidence high for these static contracts; Android comparison absent.

## Port

- Fire/FlyAway projectile and ModifyAnimation payload creation is ported in `projectile_launch`; see [combat.md](combat.md#projectile-launch-payload). ModifyAnimation per-frame execution remains open.

- `src/core/animation/animation_transitions.{hpp,cpp}` (library `animation_transitions`): plain structs `SpriteAnimation {resource, clip, frame}`, `AnimationPlayback {rate, return_clip}`, `EntityAnimation` with `std::optional` sprite/playback/direction and human/monster/enemy flags, no default initialisers. `change_animation`, `cut_in_animation`, `restore_animation` reproduce the three methods including the mutation order before a missing component (`MissingAnimationComponent` exception, not IL2CPP ABI) and 32-bit wrap without UB. Test: `tests/test_animation_transitions.cpp` (CTest `animation_transitions_contract`): plain change with/without reset, human/monster/enemy direction differences, repeated cut-in, single restore, missing components with preserved partial mutations, negative id alignment and wrap.
- `src/core/animation/animation_payload.{hpp,cpp}`: component payload codecs, see [../formats/animation-payload.md](../formats/animation-payload.md). Test: `tests/test_animation_payload.cpp`.
- `src/core/gameplay/visual_components.{hpp,cpp}` links the slot-10 playback payload to `component_slots` and the effect factory. The effect/drop entity factories and projectile trajectory/launch kernels are now ported; their per-frame visual consumers remain incomplete.
- `src/core/gameplay/animation_pass.{hpp,cpp}` implements initialize_animation, ordered update_animations and the shared manual clock step over explicit loaded resource/clip arrays and effective flag inputs. live_animation projects the actual Sprite/Playback/ContactDirection fields and Human/Monster/Enemy slot predicates (14/16/24) into a borrowed EntityAnimationView. The existing transition algorithms now operate on that view; the optional-value API delegates to the same implementation, preserving partial mutations without copying a failed operation back.
- `tests/test_animation_pass.cpp` covers Init filter/24 lookups/null managers, reinitialization and partial failures, array replacement, pass/enable gates through GameplayUpdate, direct selection skips/errors, shared/individual ordering, duplicate shared clocks, overshoot/zero-rate restoration, live kind/direction differences, missing-direction partial restoration, int32 wrap and signed/zero/overflow divisors. The effect factory test composes actual ID registration/dynamic And(2,10) group -> effect components -> five animation passes and checks independent shared/entity clocks. These tests provide no Android or pixel comparison.
- Missing: complete catalog/file/locale selection and production population of the manager fields, effective flag binding, concrete Pause adapter, other spawn factories, emitter execution and image/rendering consumers. Selected-file clock preparation, header maxima and AppData selection are now implemented.

## Evidence

- Local `research/exports/seb-timing-followup/M37433_Update.asm`, `animation-producers/M37431_Init.asm`, `seb-timing/M18994_Frame.asm`, `animation-transitions/M37438_RestoreAnimation.asm` and `animation-pass-contract.json`. Update shared calls are at 0x1141c8c; resource/clip checks at 0x1141d3c..0x1141d8c, max capture at 0x1141d90, wrapped store at 0x1141dc4, signed >= decision at 0x1141df8/0x1141dfc, restore at 0x1141e04 and remainder at 0x1141e1c..0x1141e24. Init publishes the manager array at 0x1141b1c before lookup 0x1141b48 and stores through the captured array at 0x1141b80. Registered field offsets corroborate resourceManagers_ +0x28 and ResourceManager.seb +0x18; hasMonster's word at 0x1130548 loads slot 16. Confidence high for these static contracts; no runtime comparison.

- [29-seb-animation-timing.md](../archive/reports/29-seb-animation-timing.md) — AnimationSystem.Update body (owned by the SEB/timing docs; used here for the per-entity loop)
- [30-animation-transitions.md](../archive/reports/30-animation-transitions.md) — components, Change/CutIn/Restore, port
- [31-animation-producers.md](../archive/reports/31-animation-producers.md) — Add methods, factory arguments
- [32-ecs-animation-order.md](../archive/reports/32-ecs-animation-order.md) — registration, Init, dispatch chain
- [33-ecs-lifecycle.md](../archive/reports/33-ecs-lifecycle.md) — empty hooks, enabled not filtered
- [34-animation-emitter-projectile.md](../archive/reports/34-animation-emitter-projectile.md) — Emitter/Projectile producers
- [36-pause-hooks.md](../archive/reports/36-pause-hooks.md) — AnimationSystem as IPausable
- [42-component-save-fields.md](../archive/reports/42-component-save-fields.md), [43-animation-payload-codec.md](../archive/reports/43-animation-payload-codec.md), [44-sprite-payload-codec.md](../archive/reports/44-sprite-payload-codec.md), [45-entity-write-layout.md](../archive/reports/45-entity-write-layout.md) — persistence
- Local exports under `build/research/`: `animation-transitions/`, `animation-components/`, `animation-producers/` (incl. `references.json`), `animation-spawn/`, `animation-remaining-producers/`, `ecs-order/`, `seb-timing-followup/`.

## Open questions

1. (blocking) Complete catalog/file/locale selection to populate all required AppData manager fields; bind effective AutoAnimation/application-update flags. ID placement, header-derived clocks, AppData selection and the per-pass loop are implemented.
2. (blocking) Remaining factories' full component sets (CreateEffect/CreateDrop now ported); the enemy resource resolver (record+0x50) and `BaseData.Check(1)` meaning.
3. Who calls Change/CutIn/Restore from gameplay, with which ids; meaning of the id layout (groups of 4 directions, human base 36).
4. EmitterComponent fields (+0x10..+0x20), its queue collection, `emitter+0x14` values 1 and 2; CreateParticle semantics; callers that decide `anim`/`rotate`. Projectile trajectory math is now ported in gameplay.
5. `ModifyAnimatinSystem` and `SyncAnimationComponent` execution roles; ModifyAnimation creation/payload and the shared Sync marker are ported, per-frame execution remains open.
