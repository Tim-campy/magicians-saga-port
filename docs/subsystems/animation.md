# Animation (ECS animation components and AnimationSystem)

Per-entity sprite animation is driven by three ECS components — `SebComponent` (slot 2), `AnimationComponent` (slot 10), `DirectionComponent` (slot 12) — and by `ecs.AnimationSystem` (factory position 19). Statically established: field layout, the three clip transitions (Change / CutIn / Restore), every direct `AddAnimation` producer and its guard, system registration, Init/Update/Pause dispatch and the persisted payloads. Port: the transitions and the component payload codecs are reimplemented and tested; the per-frame `AnimationSystem.Update` loop, spawn factories and integration with the ECS are not. The shared SEB frame advance (`Seb.Frame`, AutoAnimation) is a different mechanism; see [../formats/seb.md](../formats/seb.md) and the SEB timing report.

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
- `SystemManager.Add` appends it to the IInitSystem and IUpdateSystem lists (interface offsets 13 and 14 in its vtable, start 58921) and to the IPausable list (offset 16). Its absolute index in the Update list is not known.
- `AnimationSystem.Init` (`0x1141a58`): builds `Filter.And([2, 10])` (Seb + Animation), `World.GetSubSet` → stored at `this+0x20`; allocates an array of **24** ResourceManagers filled by `AppData.GetResourceManager(0..23)` (`this+0x28`).
- `SystemManager.Update(pass)` → `AnimationSystem.NeedsUpdate` (`0x1141bb8`, always true, ignores pass) → `AnimationSystem.Update` (`0x1141bc0`). Reached from `QuestForm.Update`, which may run up to 3 ECS passes per form event (see [ecs.md](ecs.md)).

### AnimationSystem.Update (per pass)

1. Walk the ResourceManager array (`this+0x28`, inner arrays +0x18) and call `Seb.Frame` on each non-null SEB (`0x1141c24–0x1141cb4`) — the shared manual frame advance.
2. Enumerate the subset's `HashSet<Entity>` (`Subset+0x30`; `GetEnumerator`/`MoveNext`/`Dispose`; slot order, see [ecs.md](ecs.md)). For each entity: get `seb`; select the Seb by `res`/`id` (negative values filtered, bounds/null checked) and read its maxFrame; `frame += animation.rate`; if `backup != -1` and the counter reaches maxFrame, call `RestoreAnimation`; then re-fetch the Seb component and `frame %= maxFrame` using the maxFrame **captured before** the restore (`0x1141e08–0x1141e24`).

The per-entity counter is separate from the SEB object's own frame; do not merge them or assume both step by 1.

### Pause

`AnimationSystem.Pause(paused)` (`0x11420ac`, the IPausable implementation): if `enabled == (~paused & 1)` return; else write enabled, then call slot 5 (OnDisabled) when pausing or slot 4 (OnEnabled) when resuming. For AnimationSystem those slots are the empty `BaseSystem.OnEnabled/OnDisabled`. Because `SystemManager.Update` ignores `enabled` and `NeedsUpdate` is always true, this **does not stop** `AnimationSystem.Update`; whether animation actually freezes depends on whether the form keeps calling `World.Update`.

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `ecs.AnimationSystem..ctor` | game:0x06000dc4 | Constructor (called by CreateSystems) | medium |
| `ecs.AnimationSystem.Init` | game:0x06000dba | Subset And(2,10), 24 ResourceManagers | high |
| `ecs.AnimationSystem.NeedsUpdate` | game:0x06000dbb | Returns true | high |
| `ecs.AnimationSystem.Update` | game:0x06000dbc | SEB frame advance + per-entity counters + restore | medium |
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
| `kairo.unity.ecs.World.CreateHuman` | game:0x06000bef | Human factory (Seb/Animation/Direction part traced) | medium |
| `kairo.unity.ecs.World.CreateMapChip` | game:0x06000beb | Map chip factory | medium |
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

## Port

- Fire/FlyAway projectile and ModifyAnimation payload creation is ported in `projectile_launch`; see [combat.md](combat.md#projectile-launch-payload). ModifyAnimation per-frame execution remains open.

- `src/core/animation/animation_transitions.{hpp,cpp}` (library `animation_transitions`): plain structs `SpriteAnimation {resource, clip, frame}`, `AnimationPlayback {rate, return_clip}`, `EntityAnimation` with `std::optional` sprite/playback/direction and human/monster/enemy flags, no default initialisers. `change_animation`, `cut_in_animation`, `restore_animation` reproduce the three methods including the mutation order before a missing component (`MissingAnimationComponent` exception, not IL2CPP ABI) and 32-bit wrap without UB. Test: `tests/test_animation_transitions.cpp` (CTest `animation_transitions_contract`): plain change with/without reset, human/monster/enemy direction differences, repeated cut-in, single restore, missing components with preserved partial mutations, negative id alignment and wrap.
- `src/core/animation/animation_payload.{hpp,cpp}`: component payload codecs, see [../formats/animation-payload.md](../formats/animation-payload.md). Test: `tests/test_animation_payload.cpp`.
- Missing: `AnimationSystem.Update` loop (needs SEB maxFrame selection via ResourceManagers and Seb.Frame), Init/subset wiring, Pause, all spawn factories, emitter/projectile systems; no link to `component_slots`. Header comments in `src/core/animation/*.hpp` still cite the old `docs/30-…` report path.

## Evidence

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

1. (blocking) Exact `AnimationSystem.Update` Seb selection from `res`/`id` (ResourceManager array, filtering) and the comparison "reaches maxFrame" (`>=` vs `==`), plus modulo semantics for maxFrame ≤ 0.
2. (blocking) Full component sets created by each factory (only Seb/Animation/Direction traced); the enemy resource resolver (record+0x50) and `BaseData.Check(1)` meaning.
3. Who calls Change/CutIn/Restore from gameplay, with which ids; meaning of the id layout (groups of 4 directions, human base 36).
4. EmitterComponent fields (+0x10..+0x20), its queue collection, `emitter+0x14` values 1 and 2; CreateParticle semantics; ProjectileSystem trajectory math; callers that decide `anim`/`rotate`.
5. `ModifyAnimatinSystem` / `ModifyAnimationComponent` and `SyncAnimationComponent` roles (adjacent animation systems, not yet analysed).
6. Effect of AddAnimation on an entity that already has Animation (slot Add ignores occupied slots, but the pooled component fields are written first).
