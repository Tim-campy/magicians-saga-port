# ECS lifecycle

The fixed32-system mission factory constructs every system before adding the array to SystemManager. Base construction captures the current static world's manager and enables the concrete type, including virtual enable hooks. The desktop host binds that lifecycle as an explicit empty-manager entry mode; save restoration and original world/entry selection remain external.

## Overview

System identities, factory indices, complete Init/Update dispatch and component contracts are described in [ECS](ecs.md). `SystemManager+0x20` owns all systems, `+0x48` owns IPausable entries and `+0x70` is its pause byte. BaseSystem captures the manager at `+0x10`; its enabled byte is `+0x18`. A system's captured manager and a later global-world query are distinct dependencies, even when this host supplies the same world.

## Key methods

| Method | Registry id | ELF address | Contract |
| --- | --- | --- | --- |
| BaseSystem..ctor | game:0x06000cc4 | 0x113af4c | Require current static world, capture its manager, enable before virtual OnEnabled |
| BaseSystem.set_enabled | game:0x06000cc6 | 0x113b48c | Changed-value store before enable/disable hook |
| BasePauseSystem.OnEnabled | game:0x06000cc0 | 0x113b020 | Require captured manager, PauseSystems(true) |
| BasePauseSystem.OnDisabled | game:0x06000cc1 | 0x113b324 | Require captured manager, PauseSystems(false) |
| SystemManager.PauseSystems | game:0x06000cf3 | 0x113b03c | Store pause, dispatch IPausable, then all OnPause only for true |
| MenuSystem.OnEnabled | game:0x06000f5d | 0x115e9e0 | Base RET, fresh world query, add event11 |
| MenuSystem.OnDisabled | game:0x06000f5e | 0x115eac0 | Base RET, fresh world query, remove event11 |
| MenuSystem.Pause | game:0x06000f62 | 0x115eec8 | Set enabled to !pause |

## Behaviour

### Construction before registration

BaseSystem..ctor requires the current static world before reading its manager. It stores the manager even when enabled is already1; that case returns without OnEnabled. Otherwise it stores1 before invoking virtual slot4. A nonnull world may contain a null manager: the base stores it and the default RET hook succeeds, while BasePauseSystem's override faults after the enabled store. A missing world leaves the old manager/enabled state intact. Object/CLR class initialization is an external runtime contract; `SystemConstructionWorld` supplies the established current-world dependency rather than recreating runtime internals.

Systems.CreateSystems (0x116883c) creates its32-element array in factory order. Each constructor is followed by set_enabled(true) and the array store. Base construction already enabled the system, so this second set is normally a no-op. Manager.Deserialize calls CreateSystems before AddSystems at0x113cf64/0x113cf70. SystemManager..ctor (0x113b8dc) initializes its lists empty. Therefore SubMenu27's inherited BasePauseSystem enable hook can set this explicitly empty manager's pause flag without pausing any of the not-yet-registered systems or delivering Info.OnPause. Init later clears the manager flag directly; it does not issue PauseSystems(false).

Twenty-six concrete constructors only forward to their base after class initialization. The other six initialize these owned payloads; their allocation order relative to BaseSystem is established by bounded instructions:

| Factory / type | Constructor ELF | Payload relative to base |
| --- | --- | --- |
| 6 Member | 0x115c1ec | After base: two fresh entity lists, then sixteen formation IDs all -1 |
| 7 Town | 0x11694a0 | Before base: fresh list at +0x20; its fresh-town consumer remains outside S1 |
| 8 Camera | 0x1144860 | Before base: fresh int32[2] at +0x48 |
| 9 Exp | 0x115194c | After base: dictionary with thirteen distinct empty lists keyed0..12 |
| 16 Fukidashi | 0x1152318 | After base: fresh lists at +0x28/+0x30 |
| 25 Info | 0x1159348 | After base: fresh popup/slide lists at +0x20/+0x28 |

Menu28's constructor (0x115eee8) forwards to BaseSystem with its concrete virtual table. OnEnabled registers event11 before Info25.Init registers its receiver. Menu.Init registers the same target/method again. The constructor's visible flag is CLR-zero/false; Init validates row shapes and subscribes without setting it. The resulting normal receiver order is Menu28 -> Info25 -> SubMenu27 -> Menu28. Debug30's enable hook calls the already-ported path visualization setter: visualize=true, wait milliseconds=0, independent of the application DEBUG update flag.

### Enable, pause and removal

set_enabled(v) returns when the enabled byte already equals v&1. Otherwise it stores first, then invokes OnEnabled or OnDisabled; failures retain that store. Manager EnableSystem/DisableSystem instead also notify their respective manager delegates for every requested nonnull system, even if its flag was unchanged. Duplicate requested types are not removed. Neither operation changes the Update list.

PauseSystems always stores its pause flag, including repeated values. The fixed game's IPausable order is2/4/5/9/10/11/13/14/15/19/23/28. Each concrete Pause writes enabled=!pause; resume enables the whole cohort rather than restoring a snapshot. A true request then calls OnPause on all32 systems, including disabled entries. Info25 starts category0 popup exit; ordinary Info updates dispose it after eight exit ticks. Update has no manager-wide pause gate. Stage3 therefore pauses later Human/Enemy/Projectile/Move gates in the same tick; Battle2 has already run. On resume Battle2 stays skipped for that tick, while Human4 resumes immediately.

Menu.OnDisabled removes one event11 registration and leaves orientation27 registered. EventManager removes the first matching tracking record. MissionScene's ordered known-method identities preserve this normal path without reproducing native delegate-hash collisions; delivery retains the whole receiver snapshot across mutations. With constructor plus Init there are two Menu registrations: pause removes the first and leaves the second active, then resume appends another. The post-pause order is Info25 -> SubMenu27 -> Menu28; after resume it is Info25 -> SubMenu27 -> Menu28 -> Menu28. Init-only scenarios explicitly retain one initial registration instead.

BasePauseSystem has no nesting counter: enable/disable forwards true/false to its captured manager. RemoveAllSystems (0x113c1bc), called by World.Destroy, enumerates all registered systems and calls DisableSystem for each before clearing lists. It resets sizes/versions/references at+0x20/+0x28/+0x30/+0x38/+0x40/+0x48/+0x58/+0x60 and makes a separate generic call on+0x50. Hooks and manager notifications precede these clears. The operation is documented, not bound to this desktop entry.

### Positional serialization

SystemManager.Serialize (0x113ce34) enumerates all systems in order and calls slots11/7/9, OnBeforeSerialize -> Serialize -> SerializeExtra, without writing an ID/count wrapper. Deserialize (0x113cefc) constructs/adds systems, then calls slots8/10/12, Deserialize -> DeserializeExtra -> OnDeserialized, in that same order. It does not call RemoveAllSystems first. World.Deserialize loads entities before systems, then writes loaded byte1 only after both succeed. Existing-manager restoration must not be substituted for the explicit empty-manager entry without establishing its caller.

## Port

`update_scheduler.hpp/.cpp` supplies construct_base_system, captured-manager BasePause hooks, changed-value enable hooks and the fixed gameplay pause projection. Its generic Update dispatcher remains separate from pause. The general manager's mutable/versioned pause lists, removal delegates and positional serialization remain documented only.

`system_construct empty_manager` requires system_init and stage_start, rejecting system_enabled/menu_visible snapshot overrides. MissionProfile supplies the current world and an explicitly empty manager, runs the32 constructor enable phases before Init, calls the existing Member/Camera/Exp/Fukidashi/Info state owners, and binds Menu/SubMenu/Debug hooks. C++ storage owners are allocated before that phase; managed allocation-failure identity/order and the unused Town list are not claimed as reconstructed. No runtime, collection or pathfinding implementation is recreated. Scenario configuration and saved data remain explicit; fresh Town creation is still rejected. Without construction, menu_visible0/1 exposes the provisional Init-only visibility snapshot, whose declared legacy default is true; the constructor mode derives false from the native zeroed field instead.

Tests cover manager rebinding while enabled, missing world, null manager, virtual-hook failure prefixes, second-enable no-op, constructor pause before registration, all32 enable flags, native member/start state, Menu duplicate delivery/removal/restoration and continued gameplay after Init. Host commands and real-data observations belong to [integration](integration.md).

## Evidence

Local research/exports/mission-construction.tsv and mission-construction/ contain all32 selected constructor bodies plus BaseSystem and SystemManager constructors, with full syntactic coverage. Factory order comes from ecs-order/M37991_CreateSystems.asm and factory-order.json; ecs-lifecycle/M37236_Deserialize.asm establishes construction before AddSystems. pause-hooks/ and mission-pause/ supply BasePause and Menu hook bodies. The analyzed ELF SHA256 is17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f; Ghidra's image base0x100000 is subtracted from instruction addresses. Confidence high for these static call/field contracts; no Android comparison.

## Open questions

- (blocking) Bind original world/SetWorld/entry selection and decoded system state. The constructor mode explicitly starts from an empty manager and does not establish the original Continue/new-game caller.
- Bind the fresh Town list and its construction consumers when S2 reaches them; do not infer town state from a loaded mission snapshot.
- Bind generic manager-list mutation, delegate-hash collision inputs and native FormManager timing only when the mission/town host requires them.
