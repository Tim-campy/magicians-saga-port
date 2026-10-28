# ECS (entities, components, systems)

The game runs on Kairosoft's own ECS (`kairo.unity.ecs`, KairoLibrary) specialised by `ecs.*` types in Assembly-CSharp. Statically established: the world/manager construction, the concrete entity factory (one `BaseEntity` type with 35 component slots), slot storage and its notifications, subset (group) maintenance through `IFilter.Matches`, the `HashSet<Entity>` storage and its enumeration order, system registration and Update/Init/Pause dispatch, enable/pause/remove-all/serialize lifecycle, and the two-pass entity load and destroy order. Port: component slot storage, `ContainsAll/ContainsAny` and the `Filter.Matches` predicate are reimplemented and tested; subset membership and observable slot ordering are now ported with live collision/garbage cursors; entity ID selection and creation orchestration are now ported with explicit registry/factory services; entity insertion/component subscriptions are now connected; dynamic group creation/dedup and the complete system schedule remain open. Nothing was compared against a running original.

All addresses are ELF VAs (load bias 0; Ghidra exports add `0x100000`). Numbers like `37083` in the evidence are IL2CPP metadata method indices, not registry ids.

## Overview

### Object graph

- `World` (`kairo.unity.ecs.World`). `World..cctor` (`0x11332b0`) allocates `ecs.MyEntityManager` with slot count **0x23 = 35** and passes it to `World..ctor` (`0x11333ac`), which stores it at `World+0x20`. The `SystemManager` sits at `World+0x28`. `World.byte+0x38` is set to 1 by `World.Deserialize` after a successful load and is tested by `AppData.InitWorld` before creating systems (see Lifecycle).
- `EntityManager` (`MyEntityManager..ctor` is a tail call to `EntityManager..ctor`). Fields: `+0x10` slot count per entity (35), `+0x14` factory/ID counter (zeroed by ctor, persisted), `+0x18` `Dictionary` of registered entities keyed by `entity+0x28`, `+0x20` collection of `Subset`s.
- `BaseEntity` (metadata type 4371) derives from `Entity`. Fields: `+0x10/+0x18/+0x20` delegates OnComponentAdded/Removed/Changed, `+0x28` int32 ID (hash key), `+0x2c` bool flag (persisted, meaning unknown), `+0x38` component array (fixed length = manager slot count).
- Components are reference objects in the slot array; each component type has a fixed slot index (known: **Seb = 2**, **Animation = 10**, **Direction = 12**; see [animation.md](animation.md)). `Entity.get_<x>` / `get_has<x>` / `Add<x>` are generated per component and go through the slot array.
- `Subset` (group): `+0x10` OnEntityAdded delegate, `+0x18` OnEntityRemoved delegate, `+0x20` OnEntityComponentChanged delegate, `+0x28` `IFilter`, `+0x30` `HashSet<Entity>` (metadata field `entities_`), `+0x38` cache field (`entitiesCache_`; cleared on mutations, never read by `get_entities`). Metadata also lists `dirty_`; full offset mapping not validated.
- `Filter` (`IFilter` + `IComponentFilter`): `+0x10` AND list (`andComponentTypes_`), `+0x18` OR list, `+0x20` NOR list, `+0x28` hash cache (-1 = not computed).
- `SystemManager`: `+0x10` OnSystemEnabled delegate, `+0x18` OnSystemDisabled delegate, `+0x20` all systems, `+0x28` IInitSystem list, `+0x30` IUpdateSystem list, `+0x48` IPausable list, `+0x70` byte isPause; further collections at `+0x38,+0x40,+0x50,+0x58,+0x60` (draw/other lists, contracts not reconstructed).
- `BaseSystem`: `+0x10` manager reference (used by `BasePauseSystem` hooks), `+0x18` byte enabled. Vtable slots: 4 OnEnabled, 5 OnDisabled, 6 OnPause, 7 Serialize, 8 Deserialize, 9 SerializeExtra, 10 DeserializeExtra, 11 OnBeforeSerialize, 12 OnDeserialized.

### Contact collision projections

Position uses slot 0 with binary32 x/y/z at +0x10/+0x14/+0x18; Bullet presence uses slot 29. Human slot 14 contributes int32 state +0x24 and invincibleTime +0x70; Enemy slot 24 contributes level +0x10, flags +0x14, state +0x18 and frame +0x1c. Human attackFrame +0x30 and frame +0x68 are distinct counters reset by the state-transition chain. Direction slot 12 has its locked byte at +0x14. These are borrowed portable projections in `contact_collision.hpp` and `combat_records.hpp`, not complete serialization layouts. Evidence: OnCollide at 0x114f604, local `contact-guards/` exports and metadata field offsets; see [combat.md](combat.md#contact-collision-orchestration).

### Projectile hit components

The generated getters confirm Speed slot 1, Projectile slot 27, Collision slot 32 and Damage slot 33. Projectile.parent (+0x38) supplies the attacker; Damage.value/attribute (+0x10/+0x14) supply numeric attack inputs. OnBulletHit removes Projectile and Collision after impact, then stores zero in Speed.vx/vy/vz (+0x10/+0x14/+0x18). Generated removers notify through entity removal before returning the captured object to ComponentFactory stacks. Portable component projections and the handler live in `projectile_hit`; Collision radius/offset fields and pairwise geometry are now implemented in `collision_check`; visited bookkeeping and deferred pair delivery are now ported in collision_pass. See [combat.md](combat.md#damage-callers) for guards and callback-sensitive ordering.

### Combat record links

Combat getters establish four component slots: equipment 8 (`Entity.get_equip`, 0x112e6dc), human 14 (`get_human`, 0x112f19c), parameter 23 (`get_parameter`, 0x113070c), enemy 24 (`get_enemy`, 0x112e45c). Each passes that index to the entity's virtual getter and checks a nonnull result's component type. A missing component returns null there; its subsequent dereference by the stat route faults.

`HumanComponent.get_data` (0x113ffc4, 124 bytes) indexes Data.charaData by signed int32 +0x18. `EnemyComponent.get_data` (0x113efb0, 124 bytes) indexes Data.enemyData by signed int32 +0x20; enemy level is separate at +0x10. Both reject negative/out-of-range indices through unsigned bounds checking, and both return a null record slot unchanged. There is no record creation, clamping or fallback.

EquipComponent holds a nullable int32 ID array at +0x10. Its four getters resolve: index 0 weapon and index 1 shield from the same Data.equipmentData table; index 2 from Data.hatData; index 3 from Data.clothData. Getters at 0x113f374/0x113f400/0x113f490/0x113f520 check array length and table bounds, then return the record pointer (possibly null). `SetWeapon(int)` (0x113f5c8) and `SetShield(int)` (0x113f608) only write the ID array; they do not validate the selected record or emit entity component-change notifications. The record-taking weapon overload at 0x113f5b0 first requires a nonnull record and reads its id at +0x18. The component constructor at 0x113f6fc only forwards to Object; allocation/population of the ID array remains to be located.

Port: `combat_records.hpp/.cpp` supplies typed, non-owning record lookup through the existing `ComponentSlots`. Portable HumanRecordLink/EnemyRecordLink/EquipmentRecordLinks are projections, not the full original components. `CombatRecordTables` borrows decoded catalogs; returned records keep shared identity and subsequent lookups observe component IDs/table edits. Tests cover all six lookup routes, shared weapon/shield table, null records versus invalid IDs, absent/wrong component types, short/null equipment arrays, setter timing and lookup-to-character-growth composition. Parameter slot 23 is identified but no complete parameter-component lifecycle is claimed. Live resolution on each damage call is now connected by EcsDamageHost. Concrete entity construction and persistence remain open. ParamSet is dictionary-backed; sparse keyed lookup in slot 23 is covered without reconstructing .NET collection internals (see combat.md).

Evidence: local `research/exports/combat-record-links/` and `combat-slots/` bounded ARM64 exports (exported addresses add 0x100000 to ELF addresses). Confidence high for read/store/bounds behavior. No public collection/runtime internals were reconstructed.

### Entity creation

`World.CreateEntity` (`0x1133568`) forwards to `EntityManager.CreateEntity` (`0x11375e4`) with second argument 0. That calls the manager's virtual factory (vtable slot 4, via class+0x178): `MyEntityManager.Create` (`0x1141a44`, tail call) → `EntityManager.Create` (`0x11378d0`). `Create` returns **null for any type selector ≠ 0**; for selector 0 it allocates `BaseEntity` and calls `BaseEntity..ctor(id, slotCount=manager+0x10, manager)` (`0x1136824`), which allocates the component array. `CreateEntity` treats a null return as an error, then registers the result via `AddEntity`. ID selection, threshold scanning and failure ordering are now recovered and ported as described below.

Game factories (`World.CreateHuman/Enemy/MapChip/Nature/Effect/Particle`) build on this and add components explicitly; their animation-relevant arguments are in [animation.md](animation.md).

### Registration and group maintenance

`EntityManager.AddEntity(entity, bool)` (`0x1137758`): if the bool is **false**, inserts into `manager+0x18` keyed by `entity+0x28`; then in **both** cases subscribes, in order, OnEntityComponentAdded, OnEntityComponentRemoved, OnEntityComponentChanged to the entity's three delegates. No initial pass syncs existing components into subsets.

- `OnEntityComponentAdded` and `OnEntityComponentRemoved` are identical in body: for each Subset (from `manager+0x20`), call `Subset.Add(entity, index, component)`; if it returns false, call `Subset.Remove(entity, index, component)`. Membership is thus re-evaluated on both add and remove.
- `OnEntityComponentChanged` only invokes each Subset's `+0x20` delegate (entity, index, old, new) when non-null. **No Matches/Add/Remove.** `Subset.ChangeComponent` is the same forward. So `Change(index, null)` empties the slot without updating group membership.

`Subset.Add(entity, index, component)` (`0x1137f00`): calls `IFilter.Matches(entity)` on `+0x28` (entity already mutated). If true: `HashSet.Add` on `+0x30` (**result ignored**), clear `+0x38`, invoke OnEntityAdded (`+0x10`) if present — even when the HashSet already held the entity. Returns the Matches result.
`Subset.Remove` (`0x1138018`): `HashSet.Remove`; invoke `+0x18` only if it returned true; clear `+0x38` **after** the callback (also on the unsuccessful path), so an exception in the callback skips the clear.
`Subset.Add(entity)` overload (`0x11386c0`): Matches → HashSet.Add → clear `+0x38`, **no notification**. Do not substitute one overload for the other.
`Subset.get_entities` (`0x113942c`) returns `+0x30` directly (no snapshot). `get_count` (`0x11393e4`) reads the set's count (`set+0x20`). `get_entity` (`0x1139434`) returns null when count is 0, else forwards to an unresolved generic helper (`0x12af8d4`).
`Subset..ctor` (`0x1138604`) stores the filter at `+0x28` and creates the HashSet at `+0x30` with the static `EqualityComparer` instance.

`World.GetSubSet` is called with a Filter (e.g. AnimationSystem.Init); lookup, population and publication order are established below, with concrete Filter identity now ported.

### Filters and predicates

`BaseEntity.ContainsAll` (`0x1136b9c`, vtable slot 10) and `ContainsAny` (`0x1136c10`, slot 11) walk the index array in given order against the slot array (no bitset):
- ContainsAll: false at the first absent slot; true at end; empty → true.
- ContainsAny: true at the first present slot; false at end; empty → false.
- Each index actually consulted is bounds-checked (unsigned compare, so negatives fail); no upfront validation; duplicates not normalised; null array → exception. Short-circuit order is part of the contract (e.g. slot0 present, slot1 absent: `All([1,-1])` is false without touching -1).

`Filter.Matches` (`0x113a4dc`): null entity → false first. Then, skipping each list for which `IListExtension.IsNullOrEmpty<int>` is true (null → true; else `Count == 0`):
1. AND: `ContainsAll` false → false.
2. OR: `ContainsAny` false → false.
3. NOR: `ContainsAny` true → false.
Otherwise true. An empty OR list is skipped (although `ContainsAny([])` alone is false).

`Filter..ctor` (`0x113a378`) writes -1 to `+0x28`. Static builders `Filter.And/Or/Nor` (`0x113a398/0x113a3fc/0x113a460`) allocate a new Filter, set `+0x28 = -1`, store the argument at `+0x10/+0x18/+0x20`. Explicit `IComponentFilter.And/Or/Nor` (`0x113a4c4/cc/d4`, 8 bytes each) just store the received reference into the same field and return: no concatenation, no copy, **no hash invalidation**. Chaining `And(x).And(y)` replaces, not accumulates.

### Collections and enumeration order

`Subset.entities_` is `System.Collections.Generic.HashSet<Entity>` (resolved via generic method specs: `.ctor` spec 54616, `Add` 54617, `Remove` 54620, `GetEnumerator` 54619, `Enumerator.MoveNext` 54033, `Dispose` 54032; generic instance 1063 = `<kairo.unity.ecs.Entity>`). Comparer: `EqualityComparer` (metadata type 4355, static `instance_`, created by its `.cctor` `0x1132d24`): `Equals` (`0x1132d88`) compares **references**; `GetHashCode` (`0x1132d94`) returns `entity+0x28` (throws on null). Two distinct entities with the same ID can coexist; equality is not by ID.

HashSet internals (mscorlib instantiation, outside the registry): slots array at `set+0x18`, 16-byte slots (`+0` hash, `+4` next, `+8` item), count `+0x20`, used-limit `+0x24`, free-list head `+0x28`, comparer `+0x30`, version `+0x38`. InternalHash (`0x16366c0`): null → 0, else `comparer.GetHashCode & 0x7fffffff`.
- Add (`0x163589c` → AddCore `0x1636328`): on hash+equality hit returns false with no count/version change; else takes the free-list head, or `+0x24` (incremented), stores the item, increments count and version.
- Remove (`0x1634fe8`): false if absent; else unlinks, sets slot hash = -1, clears item, pushes slot on the free-list head, decrements count, increments version; when count reaches 0 resets limit to 0 and free head to -1.
- SetCapacity (`0x1636214`, from IncreaseCapacity `0x163614c`): copies slots `[0, limit)` to the same indices and rebuilds bucket chains; no compaction. Capacity growth policy not recovered.
- Enumerator (ctor `0x157ad38`: set at +0, index 0 at +8, captured version at +0xc, current null at +0x10). MoveNext (`0x157ad64`): version mismatch → exception; then scans slots by increasing index below `+0x24`, skipping hash < 0.

**Visit order = internal slot order**, not bucket order or ID order. Example (derived, not observed): insert A,B,C → slots 0,1,2; remove B; insert D reuses slot 1 → enumeration A,D,C. Multiple removals form a LIFO of reusable slots. A `std::unordered_set` or append-only vector does not reproduce this. Duplicate Add / absent Remove don't bump the version, but Subset callbacks may mutate the set anyway.

### Systems

Creation: `Systems.CreateSystems` (`0x116883c`) allocates an array of **32** systems in fixed order. Known positions: 18 `ModifyAnimatinSystem` (original spelling), **19 `AnimationSystem`** (store at array+0xb8, `0x1169094`, after `set_enabled(true)`), 20 `CellCullingSystem`, 21 `RenderSystem`, 27 `SubMenuSystem` (its ctor call goes to `BasePauseSystem..ctor`; type identity taken from the GOT allocation, not the last ctor called). Positions are array indices, not Update-list indices.

`AppData.InitWorld` (`0x1101c48`): reads the World (static+0x18); if `World.byte+0x38 == 0`, calls `CreateSystems` and `World.AddSystems` (→ `SystemManager.AddSystems`, which calls `SystemManager.Add` for each element in **increasing index**), then creates the map and calls `World.Init`.

`SystemManager.Add` (`0x113bbd4`): appends to the general list `+0x20`; if `IInitSystem` (metadata 4400) appends to `+0x28`; if `IUpdateSystem` (4403) appends to `+0x30`; also fills other interface lists (`+0x48` = `IPausable`, metadata 4401). Append at current size, grow if needed; no sorting. Relative factory order is preserved within each list.

`SystemManager.Init` (`0x113c614`) calls `IInitSystem` method 0 on each of `+0x28` in order. Its versioned traversal, world argument and pause reset are ported below.
`SystemManager.Update(int pass)` (`0x113c7d0`) for each of `+0x30` in increasing index: `NeedsUpdate(pass)` (interface method 0); if bit0 clear skip; else `Update(pass)` (method 1). **It does not test the enabled byte nor isPause.** `World.Update` (`0x1135804`) forwards to it without tests.

Call chain from the form layer (see [ui.md](ui.md)): form event 1 → `QuestForm.Update` → `World.Update(pass)` → `SystemManager.Update`. `QuestForm.Update` runs up to 1 or **3** passes (3 when `Config.static.byte+0x10` and `Canvas.CheckKeyState(0x10)`), passing the pass index 0..n-1; passes after 0 are skipped unless `MyFormBase.Active` and the form is still the top form.

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `kairo.unity.ecs.World..cctor` | game:0x06000be1 | Creates MyEntityManager(35) and the World | high |
| `kairo.unity.ecs.World..ctor` | game:0x06000bfb | Stores entity manager at +0x20 | medium |
| `kairo.unity.ecs.World.CreateEntity` | game:0x06000c03 | Forwards to EntityManager.CreateEntity(…, 0) | high |
| `kairo.unity.ecs.World.AddSystems` | game:0x06000c06 | Forwards to SystemManager.AddSystems | high |
| `kairo.unity.ecs.World.Update` | game:0x06000c0a | Forwards pass index to SystemManager.Update | high |
| `kairo.unity.ecs.World.Destroy` | game:0x06000c02 | Visible-set clear, entity destruction, systems, event handlers | high |
| `kairo.unity.ecs.World.Deserialize` | game:0x06000c19 | Entities, then systems, then byte+0x38=1 | high |
| `ecs.MyEntityManager..ctor` | game:0x06000db5 | Tail call to EntityManager..ctor | high |
| `ecs.MyEntityManager.Create` | game:0x06000db6 | Tail call to EntityManager.Create | high |
| `kairo.unity.ecs.EntityManager..ctor` | game:0x06000c59 | +0x10 slot count, +0x14 = 0, collections | high |
| `kairo.unity.ecs.EntityManager.CreateEntity` | game:0x06000c5d | Virtual factory, null = error, AddEntity | high |
| `kairo.unity.ecs.EntityManager.Create` | game:0x06000c5e | Selector 0 → BaseEntity, else null | high |
| `kairo.unity.ecs.EntityManager.AddEntity` | game:0x06000c62 | Optional dictionary insert + 3 subscriptions | high |
| `kairo.unity.ecs.EntityManager.OnEntityComponentAdded` | game:0x06000c63 | Subset.Add else Subset.Remove per subset | high |
| `kairo.unity.ecs.EntityManager.OnEntityComponentRemoved` | game:0x06000c64 | Same body as Added | high |
| `kairo.unity.ecs.EntityManager.OnEntityComponentChanged` | game:0x06000c65 | Forwards to subset change delegate only | high |
| `kairo.unity.ecs.EntityManager.DestroyEntity` | game:0x06000c5f | RemoveAll → DisposeEvents → optional dict remove | high |
| `kairo.unity.ecs.EntityManager.DestroyAllEntities` | game:0x06000c60 | DestroyEntities(values, null, false) then Clear | high |
| `kairo.unity.ecs.EntityManager.DestroyEntities` | game:0x06000c61 | Entity.Destroy on all but the excluded one | high |
| `kairo.unity.ecs.EntityManager.Serialize` | game:0x06000c69 | Two-pass entity write ([entity-save](../formats/entity-save.md)) | high |
| `kairo.unity.ecs.EntityManager.Deserialize` | game:0x06000c6a | Two-pass entity load | high |
| `kairo.unity.ecs.EntityManager.OnBeforeSerialize` | game:0x06000c6b | RET | high |
| `kairo.unity.ecs.EntityManager.OnAfterDeserialize` | game:0x06000c6c | RET | high |
| `kairo.unity.ecs.BaseEntity..ctor` (id,count,manager) | game:0x06000c45 | Allocates component array of given count | high |
| `kairo.unity.ecs.BaseEntity..ctor` (other overload) | game:0x06000c46 | Delegates to base ctor, no array | medium |
| `kairo.unity.ecs.BaseEntity.get_type` | game:0x06000c47 | Type selector getter (vtable slot 4) | medium |
| `kairo.unity.ecs.BaseEntity.Add` | game:0x06000c48 | Add if empty, notify | high |
| `kairo.unity.ecs.BaseEntity.RemoveAll` | game:0x06000c49 | Ascending Remove over live state | high |
| `kairo.unity.ecs.BaseEntity.Remove` | game:0x06000c4a | Clear then notify | high |
| `kairo.unity.ecs.BaseEntity.Change` | game:0x06000c4b | Swap then notify; Add if empty | high |
| `kairo.unity.ecs.BaseEntity.Contains` | game:0x06000c4c | Non-null slot | high |
| `kairo.unity.ecs.BaseEntity.ContainsAll` | game:0x06000c4d | Ordered all-present predicate | high |
| `kairo.unity.ecs.BaseEntity.ContainsAny` | game:0x06000c4e | Ordered any-present predicate | high |
| `kairo.unity.ecs.BaseEntity.GetComponent` | game:0x06000c4f | Bounds-checked slot read | high |
| `kairo.unity.ecs.BaseEntity.SerializeComponents` | game:0x06000c51 | Slot block write | high |
| `kairo.unity.ecs.BaseEntity.DeserializeComponents` | game:0x06000c52 | Slot block read | high |
| `kairo.unity.ecs.Entity.OnAddComponent` | game:0x06000bc2 | Invoke delegate +0x10 | high |
| `kairo.unity.ecs.Entity.OnRemoveComponent` | game:0x06000bc3 | Invoke delegate +0x18 | high |
| `kairo.unity.ecs.Entity.OnChangeComponent` | game:0x06000bc4 | Invoke delegate +0x20 | high |
| `kairo.unity.ecs.Entity.DisposeEvents` | game:0x06000bb2 | Nulls the three delegates | high |
| `kairo.unity.ecs.Entity.Destroy` | game:0x06000bb1 | Idempotent destroyed guard; manager deletion before flag store | high |
| `kairo.unity.ecs.Entity.Serialize` | game:0x06000bbe | WriteInt ID, WriteBoolean flag | high |
| `kairo.unity.ecs.Entity.Deserialize` | game:0x06000bbf | ReadInt ID, ReadBoolean flag | high |
| `ecs.ComponentFactory.Create` | game:0x06000db8 | Component instance by slot index | medium |
| `kairo.unity.ecs.IComponent.SerializeExtra` | game:0x06000c40 | Default hook, RET | high |
| `kairo.unity.ecs.IComponent.DeserializeExtra` | game:0x06000c41 | Default hook, RET | high |
| `kairo.unity.ecs.IComponent.OnBeforeSerialize` | game:0x06000c42 | Default hook, RET | high |
| `kairo.unity.ecs.IComponent.OnDeserialized` | game:0x06000c43 | Default hook, RET | high |
| `kairo.unity.ecs.Subset..ctor` | game:0x06000c75 | Filter +0x28, HashSet +0x30 with comparer | high |
| `kairo.unity.ecs.Subset.get_count` | game:0x06000c76 | HashSet count | high |
| `kairo.unity.ecs.Subset.get_entities` | game:0x06000c77 | Returns +0x30 | high |
| `kairo.unity.ecs.Subset.get_entity` | game:0x06000c78 | Null if empty, else generic helper | low |
| `kairo.unity.ecs.Subset.Add` (entity) | game:0x06000c79 | Matches → add, no notification | high |
| `kairo.unity.ecs.Subset.Add` (entity,index,component) | game:0x06000c7a | Matches → add, notify regardless of insert | high |
| `kairo.unity.ecs.Subset.Remove` | game:0x06000c7b | Remove, notify only on success | high |
| `kairo.unity.ecs.Subset.ChangeComponent` | game:0x06000c7c | Forward to change delegate | high |
| `kairo.unity.ecs.Filter..ctor` | game:0x06000ca5 | Hash cache = -1 | high |
| `kairo.unity.ecs.Filter.And` / `Or` / `Nor` | game:0x06000ca6 / ca7 / ca8 | New filter with one list | high |
| `Filter.IComponentFilter.And` / `Or` / `Nor` | game:0x06000ca9 / caa / cab | Store list in this, return | high |
| `kairo.unity.ecs.Filter.Matches` | game:0x06000cac | AND/OR/NOR predicate | high |
| `EqualityComparer..cctor` / `Equals` / `GetHashCode` / `..ctor` | game:0x06000bd2 / bd3 / bd4 / bd5 | Entity comparer: reference equality, hash = ID | high |
| `ecs.Systems.CreateSystems` | game:0x06000fea | 32-entry system array | medium |
| `kairo.unity.ecs.SystemManager.Add` | game:0x06000ce4 | Append to interface lists | high |
| `kairo.unity.ecs.SystemManager.AddSystems` | game:0x06000ce5 | Add in increasing index | high |
| `kairo.unity.ecs.SystemManager.Init` | game:0x06000ce7 | IInitSystem.Init in order | high |
| `kairo.unity.ecs.SystemManager.Update` | game:0x06000ce8 | NeedsUpdate/Update per IUpdateSystem | high |
| `kairo.unity.ecs.SystemManager.EnableSystem` | game:0x06000cee | Set enabled + OnEnabled; always notify | high |
| `kairo.unity.ecs.SystemManager.DisableSystem` | game:0x06000cf0 | Clear enabled + OnDisabled; always notify | high |
| `kairo.unity.ecs.SystemManager.get_isPause` / `set_isPause` | game:0x06000ce2 / ce3 | Byte +0x70; setter has no side effects | high |
| `kairo.unity.ecs.SystemManager.PauseSystems` | game:0x06000cf3 | IPausable.Pause all, then OnPause if true | high |
| `kairo.unity.ecs.SystemManager.RemoveAllSystems` | game:0x06000ce6 | Disable each, then clear lists | high |
| `kairo.unity.ecs.SystemManager.Serialize` | game:0x06000cf6 | Slots 11,7,9 per system | high |
| `kairo.unity.ecs.SystemManager.Deserialize` | game:0x06000cf7 | Create+Add systems, slots 8,10,12 | high |
| `kairo.unity.ecs.SystemManager.add_OnSystemEnabled` / `add_OnSystemDisabled` | game:0x06000cdc / cde | Delegate subscription; no direct callers found | medium |
| `kairo.unity.ecs.BaseSystem.set_enabled` | game:0x06000cc6 | Write byte then OnEnabled/OnDisabled | high |
| `kairo.unity.ecs.BaseSystem.OnEnabled` / `OnDisabled` / `OnPause` | game:0x06000cc8 / cc9 / cca | Default hooks (RET) | high |
| `kairo.unity.ecs.BasePauseSystem.OnEnabled` / `OnDisabled` | game:0x06000cc0 / cc1 | Tail call PauseSystems(true/false) | high |
| `kairo.unity.ecs.BasePauseSystem..ctor` | game:0x06000cc2 | Forwards to BaseSystem..ctor | medium |

## Behaviour

### Component slots (BaseEntity)

- `Add(i, c)`: if slot occupied → return, no write, no notification. Else store `c` (**including null**) and call OnAddComponent(entity, i, c). So `Add(null)` notifies while `Contains` stays false.
- `Change(i, c)`: empty slot → delegates to Add. Occupied → capture old, store new, notify Changed(entity, i, old, new). Same pointer still notifies; `Change(i, null)` is a Change, not a Remove (and does not touch subsets).
- `Remove(i)`: empty → nothing. Occupied → capture, clear, notify Removed(entity, i, old).
- `RemoveAll()`: indices ascending, reading live state. A callback filling a later slot gets that slot removed in the same pass; refilling the current or an earlier slot persists.
- Mutation precedes the callback; no rollback if a callback throws (RemoveAll stops with earlier removals kept).
- Index checked against array length (negatives fail); the original also checks array element type compatibility of non-null objects.

### Destruction

`DestroyEntity(entity, bool)` (`0x1137950`, 152 bytes; null entity → exception): (1) vtable slot 6 = `RemoveAll` (callbacks still live, so subsets update), (2) `Entity.DisposeEvents` nulls `+0x10/+0x18/+0x20`, (3) if bool, `Dictionary.Remove(entity+0x28)`. No try/finally: a throwing callback aborts before disposal/removal.
`World.DestroyEntities` (`0x1135794`) forwards collection/except to the manager with registry removal **true**; the null argument supplied by StageSystem.CleanUp is except, not the removal flag. This differs from the manager's DestroyAllEntities path below. The manager's per-entity RemoveAll -> DisposeEvents -> optional registry removal sequence is ported in `stage_cleanup.cpp`, tested with ComponentSlots and callback-driven subset changes; see [stage cleanup](rewards.md#stage-cleanup-and-entity-deletion).

`DestroyAllEntities` (`0x11379e8`): `Dictionary.Values` → `DestroyEntities(values, except=null, false)` → `Dictionary.Clear` (Dictionary generic instance 399: `get_Values` spec (12523,399), `Clear` (12532,399), `Remove` (12545,399)). Removal from the dictionary is deferred to the Clear, avoiding mutation during enumeration. **Does not reset the counter `+0x14`.** `DestroyEntities` (`0x1137a6c`) skips the entity equal to `except` and calls `Entity.Destroy(bool)` on the rest; enumerator Dispose on all paths.

### Load (two passes)

`World.Deserialize` (`0x1135938`, 76 bytes): `EntityManager.Deserialize(stream)` → `SystemManager.Deserialize(stream)` → `World.byte+0x38 = 1`. No Destroy/RemoveAllSystems in the wrapper, and `EntityManager.Deserialize` does not start with DestroyAll/Clear: entry-state preparation belongs to callers (see [save.md](save.md)). Do not add implicit clears in the port.

`EntityManager.Deserialize` (`0x1138ba8`): read counter → `+0x14`; read entity count; for each: signed type byte → `Create` (slot 4, ID 0) → `entity.Deserialize` (slot 15: ID → `+0x28`, flag → `+0x2c`) → `AddEntity(entity, false)` (dictionary + subscriptions) → keep in temp array. Then, after **all** entities are registered, in the same order, `DeserializeComponents` (slot 17), clearing each temp entry; finally `OnAfterDeserialize` (slot 6, RET). The ID is loaded before registration (a concrete writer of the hash key); component adds in pass 2 hit subsets through the live callbacks. Byte layout: [entity-save.md](../formats/entity-save.md).

### System lifecycle

- `BaseSystem.set_enabled(v)` (`0x113b48c`): if `byte+0x18 == (v&1)` return; else write the byte **then** call slot 4 OnEnabled (true) / slot 5 OnDisabled (false). Does not touch manager lists or delegates.
- `EnableSystem(types[])` (`0x113cc18`): for each type in request order, `GetSystem`; null skipped; if not enabled → byte=1, OnEnabled; then, **whether or not it changed**, invoke manager `+0x10` delegate with the system. `DisableSystem` (`0x113c568`) symmetric with OnDisabled and delegate `+0x18`. Duplicates are not filtered. Neither adds/removes from the Update list.
- `set_isPause` writes `+0x70 = v&1` only. `PauseSystems(v)` (`0x113b03c`): always writes `+0x70` (no early-out), calls `IPausable.Pause(v)` on each of `+0x48`, then, **only if v is true**, calls slot 6 `OnPause` on every system in `+0x20`.
- `BasePauseSystem.OnEnabled/OnDisabled` (`0x113b020/0x113b324`): null-check `this+0x10` (manager) then tail-call `PauseSystems(true/false)`. No nesting counter. Order: set_enabled writes byte → hook → PauseSystems writes isPause → IPausable dispatch → OnPause phase.
- `RemoveAllSystems` (`0x113c1bc`): first enumerate `+0x20` and call `DisableSystem([typeof(system)])` for each (hooks and notifications **before** clearing), then reset sizes / bump versions / clear references of `+0x20,+0x28,+0x30,+0x38,+0x40,+0x48,+0x58,+0x60`, and a separate generic call on `+0x50`. Caller: `World.Destroy`.
- `SystemManager.Serialize` (`0x113ce34`): for each system in `+0x20` ascending, slots 11 OnBeforeSerialize, 7 Serialize, 9 SerializeExtra. No count or system ID written by the wrapper. `Deserialize` (`0x113cefc`): `CreateSystems` + `AddSystems`, then for each in `+0x20`: slots 8 Deserialize, 10 DeserializeExtra, 12 OnDeserialized. No RemoveAllSystems. The stream is positional: **system order must not change**.
- Consequence: since Update dispatch ignores the enabled byte, the manager delegates enable handling to each concrete `NeedsUpdate`; all 26 game overrides check it, including AnimationSystem. Do not add a generic `if (!enabled) skip`.

### Movement integration and projectile ordering

MoveSystem.Init (0x115fa0c) selects required slots [0,1]: Position and Speed. MoveSystem.Update (0x115faac, 616 bytes including exception cleanup) traverses that live subset. For each entity it captures Position and Speed, then adds binary32 vx/vy/vz to x/y/z (x/y are stored together in the original). It does not multiply by a delta time or clamp coordinates.

Position additionally contains offsetX/Y/Z +0x1c/+0x20/+0x24 and parent +0x28. After velocity integration, a nonnull parent with Position slot 0 causes each child coordinate to be overwritten with the freshly fetched parent coordinate plus the corresponding offset. A parent without Position leaves the velocity result intact. There is no self-parent or destroyed-parent guard. A self-parent therefore adds offsets to the already integrated coordinates. The captured child's Position object remains the destination; this parent link is independent of Projectile.parent, which identifies the attacker. Parent-before-child versus child-before-parent iteration changes the result, so the production adapter must preserve subset order.

Relative system order is ProjectileSystem -> MoveSystem -> CollisionSystem (factory indices 10/11/12, stores at 0x1168da0/0x1168df4/0x1168e48). Both ProjectileSystem and MoveSystem explicitly implement IUpdateSystem (native type 12847), confirmed from metadata interface arrays for type indices 4486/4484. SystemManager.Add preserves factory order in its update list. Their NeedsUpdate methods (0x1160010 / 0x115faa4) both return the enabled byte +0x18 and ignore pass. Projectile processing, including its deferred event-23/removal pass, completes before MoveSystem integrates all three velocity axes. Thus Move also applies vy after Projectile has written the current curve height; horizontal-only integration is insufficient. The complete membership and gates are listed below.

Evidence: local `research/exports/move-system/Init.asm`, `Update.asm`, `movement-gates/`, existing `ecs-order/factory-order.json`, metadata PositionComponent offsets and raw v31 type interface entries. Confidence high for the per-entity path and this relative order; no runtime comparison.

### Temporary entity lifetime

GarbageSystem.Init (0x11530b8) requests the subset whose And filter contains only slot 22 (Garbage). NeedsUpdate (0x115314c) returns BaseSystem.enabled (+0x18) and ignores the pass argument. Update (0x1153154, 904 bytes) itself does not test enabled. It allocates a pending list, enumerates the live subset, requires each entity and Garbage component, decrements its int32 counter at +0x10 with wrapping, stores it, and queues the entity when the new value is <= 0. The comparison uses the decremented value: 1 expires immediately, 2 after two successful passes, 0 becomes -1 and expires, and INT_MIN wraps to INT_MAX and survives. It does not skip destroyed entities during this counting phase.

Only after the entire subset enumeration and its disposal does it traverse the pending list and call Entity.Destroy(true) in discovery order. It does not re-read the Garbage component/counter before destruction. Thus a deletion callback changing a later queued entity's counter cannot cancel that deletion; the existing Entity.Destroy destroyed guard can still skip an already-destroyed entity. A counting/enumeration failure leaves earlier decrements visible but destroys none of the pending entries. A deletion failure retains previous deletions and all decrements, and prevents later deletions. The temporary queue is not retained for another update.

Entity.AddGarbage (0x112eccc) pops from the Garbage pool if available, otherwise constructs a component, writes the supplied counter and invokes AddComponent at slot 22. No existing-slot guard or clamping precedes acquisition/write. The port uses an explicit pool interface and the existing ComponentSlots add contract: an occupied slot ignores the new component, so AddGarbage does not refresh an existing counter, although acquisition and the new component write still occur. Impact creation now composes this operation with GarbagePass in tests: the source counter 1 expires on the first pass, while neighboring damage areas with counter 2 retain Collision/Damage until the second pass. These are update passes, not a claim of wall-clock duration or collision opportunities.

### Live gameplay subsets

`EntitySubset` ports the two Add overloads, Remove, ChangeComponent and count over the existing filter predicate. The filter descriptor and its lists remain borrowed/live. Notification Add returns Matches even for a duplicate, clears the cache before notification, and notifies on every matching call; silent Add never notifies. Remove only notifies after a successful removal and clears the cache afterwards, so callback failure retains the cache pointer. Change only forwards the notification, including for entities outside the group; it never reevaluates membership. Cache production and get_entity are not implemented.

For lookup the port uses std::unordered_map, keyed by captured masked ID hash and entity reference. A separate vector preserves slot order and LIFO hole reuse; growth does not move logical indices. Duplicate insertion and absent removal do not change the cursor version; successful mutations do. Emptying a group resets its used-slot limit/free list. Cursors scan occupied slots and reject version changes, even after exhaustion. Equal IDs do not collapse distinct entities. Supplying a changed ID changes lookup while the old captured hash remains stored, matching the original mutable-key hazard; actual ID allocation remains a registry responsibility.

This narrow ordering adapter follows the public Microsoft Reference Source HashSet implementation, commit ec9fa9ae770d522a5b5f0607898044b7478574a3, under MIT: [HashSet.cs](https://github.com/microsoft/referencesource/blob/ec9fa9ae770d522a5b5f0607898044b7478574a3/System.Core/System/Collections/Generic/HashSet.cs), [license](../../third_party/licenses/dotnet-referencesource.txt). AddIfNotPresent, Remove and Enumerator.MoveNext match the already-established APK slot/free-list/version contract. This is a behavioral source match, not identification of Unity's exact runtime revision. No generic runtime library, hash buckets, prime-capacity policy or set algebra is reconstructed.

`SubsetMembership` wires a fixed ordered group list to ComponentSlots notifications: added/removed re-run Add, then Remove when Matches is false; changed only forwards. Wiring does not scan existing entities, matching AddEntity's lack of an initial sync. This host adapter does not claim World.GetSubSet deduplication, dynamic manager group enumeration or registry ownership. `open_collision_subset` and `open_garbage_subset` bind actual live group cursors to the existing passes; garbage identities are explicitly supplied by the registry adapter.

The integrated test constructs Collision/Position and Garbage groups, changes membership through component writes, verifies reordered contact delivery after LIFO reinsertion, then runs expiration and checks that component destruction removes entities from both groups. It also confirms stale membership after Change(null) and invalidation of a cursor after membership mutation. It establishes live collision/destruction integration, not a complete frame scheduler or playable world.

## Port

- `src/core/ecs/entity_subset.hpp/.cpp` ports subset membership/notifications and observable ordering; `tests/test_entity_subset.cpp` covers duplicate notifications, reference equality, LIFO reuse, empty reset, live filters, cursor invalidation, cache failure order and changed ID hashes. `src/core/gameplay/subset_cursors.hpp/.cpp` connects collision/garbage to these groups; `test_live_combat_subsets` checks concrete membership transitions and ordered detection/expiration.

- Collision slot 32 now includes the visited byte (+0x20). `collision_pass` resets and marks it while traversing fresh And({32,0}) cursors; pairs are delivered only after detection finishes. See [combat traversal](combat.md#collision-candidate-traversal-and-deferred-delivery). The cursor adapter now uses EntitySubset for live membership/order/version checks; world registration and scheduling remain open.

- `garbage_lifetime.hpp/.cpp` implements AddGarbage, the wrapping countdown and two-phase expiration orchestration, reusing Entity.Destroy and the existing deletion sequence. Tests cover overflow, callback mutations, deferred deletion, enumeration/deletion failure, destroyed guards and reentrant passes. Cursor ownership preserves disposal before deletion and during exceptions; live group cursors are now supplied by EntitySubset, while system registration/scheduling remain separate; GarbageSystem.Update stays documented pending complete world integration. Scratch capacity is reused between successful passes without sharing the active queue across callbacks.

- `src/core/gameplay/movement.hpp/.cpp` implements the complete per-entity MoveSystem body over borrowed Position/Speed components, including parent overrides. `tests/test_movement.cpp` covers three-axis integration, missing parent Position, self-parent, partial failure, binary32 precision and order-sensitive parent/child updates. `test_projectile_motion` now composes Fire, Projectile's motion/completion pass and Move for a full trajectory. `motion_pass.cpp::move_subset` now supplies live EntitySubset traversal; tests preserve parent/child ordering through group removal/reinsertion. Full scheduler registration remains separate.

- `src/core/ecs/component_slots.{hpp,cpp}` (`saga::core::ComponentSlots`, library `component_slots`): fixed slot vector of borrowed `Component*`, size fixed at construction, synchronous `ComponentObserver` callbacks (added/removed/changed), `add/change/remove/remove_all/get/contains/contains_all/contains_any`. Does not own, clone or free components; operations don't allocate. Index/size errors are C++ exceptions, not IL2CPP exception identity. Test: `tests/test_component_slots.cpp` (CTest `component_slots_contract`): duplicate add, null add, same-pointer change, callback args and visibility, ascending RemoveAll with later/current/earlier reinsertion, exception without rollback, invalid indices, empty array, predicate short-circuit order, empty/repeated lists, Change(null).
- `src/core/ecs/component_filter.{hpp,cpp}` (`matches_component_filter(entity, all, any, none)`): Filter.Matches predicate over ComponentSlots with borrowed nullable lists, re-read each call. Test: `tests/test_component_filter.cpp` (`component_filter_contract`).
- Missing: GC/pooling, array type checks, subclasses overriding virtual slot methods, dynamic subset registration/dedup and original registry enumeration, Filter identity/hash/builders, World, concrete system registration lists, enable/pause/remove-all and (de)serialization passes. Ordered Init/Update dispatch and entity event delivery/removal are ported above. No link yet between component_slots and the animation module.
- Header comments in `src/core/ecs/*.hpp` still cite the old `docs/35-component-slots.md` path; should point here.

### Complete update schedule

All 32 factory systems implement IInitSystem. The 26 IUpdateSystem entries below retain factory order; MapSystem, MemberSystem, TownSystem, SubMenuSystem, MenuSystem and StatusRenderSystem do not enter the update list. Every gate reads BaseSystem.enabled at +0x18. Five additionally require pass == 0; the other 21 ignore pass, including negative values. Gates have no side effects and are called immediately before their system body.

| Factory index | System / NeedsUpdate registry id | ELF address | Gate |
| --- | --- | --- | --- |
| 0 | `InputSystem.NeedsUpdate` / game:0x06000f20 | 0x115b4e0 | enabled && pass == 0 |
| 2 | `BattleSystem.NeedsUpdate` / game:0x06000dc6 | 0x1142364 | enabled |
| 3 | `StageSystem.NeedsUpdate` / game:0x06000f9c | 0x11625e0 | enabled |
| 4 | `HumanSystem.NeedsUpdate` / game:0x06000ebc | 0x115510c | enabled |
| 5 | `EnemySystem.NeedsUpdate` / game:0x06000e69 | 0x114c68c | enabled |
| 8 | `CameraSystem.NeedsUpdate` / game:0x06000de3 | 0x1143f54 | enabled && pass == 0 |
| 9 | `ExpSystem.NeedsUpdate` / game:0x06000e91 | 0x1151b38 | enabled |
| 10 | `ProjectileSystem.NeedsUpdate` / game:0x06000f7d | 0x1160010 | enabled |
| 11 | `MoveSystem.NeedsUpdate` / game:0x06000f6e | 0x115faa4 | enabled |
| 12 | `CollisionSystem.NeedsUpdate` / game:0x06000e0b | 0x1145f38 | enabled |
| 13 | `RotateSystem.NeedsUpdate` / game:0x06000f8d | 0x1161ea4 | enabled |
| 14 | `ParameterSystem.NeedsUpdate` / game:0x06000f75 | 0x115fe34 | enabled |
| 15 | `EmitterSystem.NeedsUpdate` / game:0x06000e5e | 0x114ba20 | enabled |
| 16 | `FukidashiSystem.NeedsUpdate` / game:0x06000e9c | 0x1152468 | enabled |
| 17 | `CellSystem.NeedsUpdate` / game:0x06000e05 | 0x1145a10 | enabled |
| 18 | `ModifyAnimatinSystem.NeedsUpdate` / game:0x06000f68 | 0x115f2ac | enabled |
| 19 | `AnimationSystem.NeedsUpdate` / game:0x06000dbb | 0x1141bb8 | enabled |
| 20 | `CellCullingSystem.NeedsUpdate` / game:0x06000df2 | 0x1144df4 | enabled && pass == 0 |
| 21 | `RenderSystem.NeedsUpdate` / game:0x06000f86 | 0x1160d14 | enabled && pass == 0 |
| 22 | `GaugeSystem.NeedsUpdate` / game:0x06000eab | 0x1153690 | enabled |
| 23 | `EffectSystem.NeedsUpdate` / game:0x06000e3f | 0x1149e64 | enabled |
| 24 | `CursorRenderSystem.NeedsUpdate` / game:0x06000e19 | 0x1147474 | enabled |
| 25 | `InfoSystem.NeedsUpdate` / game:0x06000efa | 0x1159ad0 | enabled |
| 26 | `ButtonSystem.NeedsUpdate` / game:0x06000dce | 0x1142884 | enabled && pass == 0 |
| 30 | `DebugVisualizeSystem.NeedsUpdate` / game:0x06000e2b | 0x1147f60 | enabled |
| 31 | `GarbageSystem.NeedsUpdate` / game:0x06000ea4 | 0x115314c | enabled |

SystemManager.Update captures the current list element before calling the diagnostic stopwatch. On pass 0 it reads stopwatch +0x68 separately for Reset, Start and Stop: Reset -> Start -> NeedsUpdate -> optional Update -> Stop. Even a skipped body is timed; an exception does not execute Stop. An empty list does not require a stopwatch. Other passes never access it. This uses an explicit clock adapter, not a reimplementation of public System.Diagnostics.Stopwatch.

Traversal uses an increasing index and rereads the manager's list pointer and size after each body. Appended entries may execute in the same pass; removing an earlier entry may skip a shifted entry. Replacing the list affects the next index. The captured current system still executes if callbacks remove it. There is no collection-version check or manager-wide enabled/pause guard.

IPausable membership, in factory order: Battle, Human, Enemy, Exp, Projectile, Move, Rotate, Parameter, Emitter, Animation, Effect and Menu systems. Membership alone does not specify each Pause body.

Evidence: local `research/exports/ecs-order/update-membership.json` derived from metadata type/interface arrays (including inherited interfaces), the existing factory allocation/order evidence, `research/exports/system-gates/*.asm`, and `ecs-order/M37221_Update.asm`. The gate exports are 8 bytes for enabled-only and 28 bytes for pass-zero gates. Confidence high; no Android comparison.

Port: `src/core/ecs/update_scheduler.hpp/.cpp` implements indexed dispatch, all 26 gates and ordered binding of the 26 required system bodies. Tests cover callback mutations, timer ordering, failure propagation and live enabled changes. This does not implement the missing system bodies, initialization/event subscriptions or a complete playable frame.

### Entity event delivery

World's generic one/two-argument wrappers forward to the EventManager at World+0x30. The combat call sites select shared native bodies: World Add wrappers 0x131a398/0x131a640, instance Add 0x131a3e8/0x131a690, EventManager Add 0x12bde40/0x12be754; World Send wrappers 0x131ab08/0x131aeec, instance Send 0x131ab58/0x131af44, EventManager Send 0x12bf744/0x12bfa3c.

EventManager stores one-argument and two-argument callbacks in separate dictionaries at +0x10/+0x18, keyed by int32 event ID. Add wraps the supplied typed callback, combines the existing delegate followed by the new wrapper via System.Delegate.Combine (0x1ea70b8), and stores the result. Duplicates are retained. It also adds tracking records to separate lists at +0x20/+0x28; those carry event ID, original delegate hash and wrapper reference for removal. Null registration is not harmless: the dictionary write precedes a required original-delegate hash read.

Send looks up the delegate once through IDictionaryExtension.GetValue (0x12cc074, TryGetValue with null default), returns when absent/null, otherwise invokes the captured delegate with the original payload references. It does not queue events, test system enabled/pause flags or revalidate entities. A subscriber exception propagates.

Public contract reused: [Delegate.Combine](https://learn.microsoft.com/en-us/dotnet/api/system.delegate.combine) appends the second invocation list to the first, retaining duplicates. The [C# delegate specification, sections 21.5-21.6](https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/delegates) establishes immutable invocation lists, synchronous ordered calls and exception termination. Therefore an Add during delivery leaves that delivery unchanged; nested or later Send sees the new list. No delegate runtime, dictionary hashing or generic type machinery is reconstructed.

Port: `entity_events.hpp/.cpp` supplies the Entity-pointer payload projection using standard unordered_map, shared_ptr, vector and function. Registration copies the list of shared callable references; Send captures that list without allocating. Stateful callbacks retain their identity across additions. `bind_combat_receivers` registers the seven established Stage/Human/Enemy/Projectile callbacks in factory order. It is a registration slice, not full system Init; valid nonempty callbacks and borrowed receiver lifetimes are required. Hash-based unsubscribe, tracking and RemoveAll are implemented for this payload projection as described below. Null-registration partial failure and other payload types remain outside it. Consequently the original generic Add/Send methods are not promoted in the registry by this adapter.

The contact integration test now delivers event 29 to both real Human and Enemy receivers, proving that Human position/state updates precede Enemy damage/recoil. The event tests cover independent arities, duplicate callbacks, nested death notification, additions during delivery, callable state and exception interruption.

Evidence: local `research/exports/combat-event-bus/` with exact FDE bounds from `il2cpp-unwind.json`; code-map resolution of Delegate.Combine and metadata usage resolution of Dictionary ContainsKey/get_Item/set_Item and IDictionaryExtension.GetValue. Confidence high for the dispatch contract and selected registration order. Removal matching and direct factory subscriptions are established below; Android comparison absent.

### Factory initialization subscription audit

All 32 factory types have their own Init entry. Bounded exports were checked for direct calls to the World registration wrappers and metadata-resolved delegate targets. They contain 23 registration calls across 14 systems:

| Factory index | System | Event -> receiver, in Init call order |
| --- | --- | --- |
| 0 | InputSystem | 9 -> OnTouchEvent (0x115b4d4) |
| 3 | StageSystem | 30 -> OnHumanDead (0x1162540); 9 -> OnTouchEvent (0x1162570); 15 -> OnDamage (0x11625c8) |
| 4 | HumanSystem | 23 -> OnProjectileImpact (0x11550b8); 29 -> OnCollide (0x1155100) |
| 5 | EnemySystem | 23 -> OnProjectileImpact (0x114c4f0); 29 -> OnCollide (0x114c528); 30 -> OnHumanDead (0x114c578) |
| 7 | TownSystem | 31 -> OnReturnFromQuest (0x116961c) |
| 9 | ExpSystem | 21 -> OnGetExp (0x1151b30) |
| 10 | ProjectileSystem | 23 -> OnProjectileImpact (0x1160004) |
| 14 | ParameterSystem | 16 -> OnDropoff (0x115fe2c) |
| 20 | CellCullingSystem | 14 -> OnCellChange (0x1144b9c) |
| 24 | CursorRenderSystem | 2 -> OnCameraTouchDown (0x1147428); 4 -> OnCameraTouchClick (0x1147468) |
| 25 | InfoSystem | 4 -> OnCameraTouchClick (0x1159550); 11 -> OnButtonTouch (0x1159580); 28 -> OnTownAttack (0x11595c4) |
| 26 | ButtonSystem | 9 -> OnTouchEvent (0x1142878) |
| 27 | SubMenuSystem | 11 -> OnButtonTouch (0x11686d4) |
| 28 | MenuSystem | 11 -> OnButtonTouch (0x115e838); 27 -> OnScreenOrientate (0x115e878) |

The remaining 18 Init entries do not directly register World events. CellCulling additionally subscribes to subset entity-added/removed notifications; these are not World events. StatusRenderSystem.Init is a single return. This is a subscription audit, not a claim that all initialization bodies are documented or ported.

For combat, the factory subscription lists are now bounded: event 23 = Human -> Enemy -> Projectile; event 29 = Human -> Enemy; event 30 = Stage -> Enemy. An executable-segment B/BL scan of the reference-payload Add wrappers, instance forwarding bodies and EventManager Add bodies found no additional direct registration caller for these events. The only extra non-Init caller is MenuSystem.OnEnabled at 0x115e9e0, whose registration call 0x115eab8 uses event 11. No absence claim is made for indirect/reflection-driven registration or custom future system sets.

Evidence: local `research/exports/system-initializers/` contains all 32 bounded Init exports, metadata-resolved `subscriptions.json`, whole-executable immediate-call scan `incoming-add.json`, and MenuOnEnabled. Metadata resolution follows each delegate constructor's method usage, not the handler's name. Confidence high for direct registration sites and order; complete execution paths and Android comparison remain separate.

The collision integration now uses GameplayUpdate index 12 -> CollisionPass -> a live And({32,0}) EntitySubset -> EntityEvents event 29 -> the real Human and Enemy contact handlers. A disabled gate produces no movement or callbacks; enabling it causes detection and ordered contact displacement/damage-host/recoil. This remains one integrated system, not the full 26-system frame; actual damage arithmetic is covered separately and this scenario uses an explicit nonlethal damage host.

### Event unsubscription and world teardown

EventManager.RemoveEventHandler's reference-payload specializations are 0x12bebe0 (one argument) and 0x12bf144 (two). Each first checks whether its arity-specific dictionary contains the event ID. If absent it returns. Otherwise it uses Enumerable.FirstOrDefault on the tracking list. The predicate (0x1422ba8 / 0x1422cb8) compares record.eventId +0x10 first, then record.hash +0x14 with the requested original delegate's GetHashCode. It does **not** compare target/method identity or the original delegate by equality. The first matching record wins, including a hash collision between different handlers.

For a match it removes that record's unique wrapper from the current combined delegate using Delegate.Remove, writes the result back without deleting the dictionary key, removes the record from the tracking list, and calls Log.Println with the event ID in the diagnostic text. A logging failure leaves removal committed. A callback removed during Send still runs in that Send's captured invocation list; nested/later Send observes removal. Empty/missing matches do not log.

EventManager.RemoveAllEventHandlers (game:0x06000c95, 0x1139c04, 1764 bytes) is called by World.Destroy at 0x113576c. It enumerates the single-argument tracking list, calling RemoveEventHandler(record.eventId, record.wrapper), then clears that tracking list; repeats for the two-argument list; finally clears both dictionaries. Thus it supplies the **wrapper** delegate hash to a predicate comparing stored **original** hashes. If no hashes match, the final clears still remove everything. If a wrapper hash matches an original hash for that event, individual removal mutates the enumerated List and the next MoveNext throws, even when it would otherwise be the end. Already removed callbacks stay removed; later lists/clears are not reached. Failure in the pair phase may leave the single dictionary callable after its tracking list was cleared. Do not replace this with an unconditional map clear.

Constructor 0x1139aac creates two List tracking collections (generic instances 1517/1518) and two dictionaries. Public behavior is reused: [Enumerable.FirstOrDefault](https://learn.microsoft.com/en-us/dotnet/api/system.linq.enumerable.firstordefault) chooses the first matching entry; [List enumerator documentation](https://github.com/dotnet/dotnet-api-docs/blob/main/xml/System.Collections.Generic/List%601%2BEnumerator.xml) specifies invalidation after modification. This does not justify reconstructing LINQ, collection internals or delegate hashing.

Port: EntityEvents now accepts explicit EventHashes {original,wrapper}, removes the first matching record, preserves Send snapshots and reproduces cleanup ordering/version failures. Unknown hashes remain optional for delivery-only bindings; an operation that needs an unknown hash fails explicitly instead of inventing one. CombatReceivers can bind all seven hash pairs in registration order. Tests cover removal during nested delivery, duplicates/hash collisions, both arities, logging failure, cleanup failure at either phase, and successful clear/rebind. Runtime hash values and host logging remain explicit inputs. This entity-only projection does not promote the generic methods or claim all payload types are supported.

Evidence: local `research/exports/combat-event-bus/{RemoveCandidate1,RemoveCandidate3,RemovePredicate1,RemovePredicate2,RemoveAll,EventManagerCtor}.asm`, `generic-removal-map.json` and `remove-all-callers.json`. The method-spec/function mapping uses the existing [Il2CppDumper schema](https://github.com/Perfare/Il2CppDumper/blob/4741d46ba9cd6159c5d853eb9d6fc48b4bfa2b1a/Il2CppDumper/Il2Cpp/Il2CppClass.cs): metadata registration 0x26b50d0, generic function definitions 0x74068c, code pointers 0x2658b40, method specs 0x6968d4. Confidence high for matching/order, conditional list failure inferred from the confirmed List type and its public contract; whether the hash collision occurs in the original session requires runtime evidence.

### System initialization dispatch

SystemManager.Init (game:0x06000ce7, 0x113c614, 444 bytes) captures the IInitSystem list at manager+0x28 and obtains its List enumerator. It calls each Init with the same borrowed world argument; the dispatcher does not reject a null world, test enabled, or skip paused systems. A null list or current system faults. After successful completion/disposal it clears isPause at +0x70. If a receiver throws, its earlier effects remain, later systems are not initialized and the pause flag retains its current value.

This differs from the indexed Update loop: additions, removals, replacement of a list entry or Clear invalidate the captured Init enumerator, including on the final MoveNext. Replacing the manager's list reference alone does not redirect that already captured enumerator. Version behavior uses the public List contract already cited under event teardown; collection internals are not reconstructed. The ordinary List enumerator has no observable disposal side effects in this projection.

Port: `system_initialization.hpp/.cpp` provides a borrowed InitSystem/InitializationWorld boundary, a versioned ordered InitList and initialize_systems. List mutations are explicit operations so invalidation cannot silently disappear. Existing factory evidence establishes all 32 systems in Init order; this dispatcher does not supply their missing concrete bodies. The test creates the established Stage/Human/Enemy/Projectile registration slice through Init, then exercises nested impact/death delivery. It also checks a preserved subscription after initialization failure, null-world forwarding, pause timing, list replacement and entry mutation. Callback bodies in this registration test are diagnostic receivers, not full gameplay Init implementations.

Evidence: local `research/exports/ecs-order/M37220_Init.asm`: captures list at 0x113c688, forwards world at 0x113c728, loops through MoveNext at 0x113c6c4 and clears pause at 0x113c740 only on the success path. Confidence high for dispatcher ordering and side effects; Android comparison absent.

### Gameplay pass initialization

The four concrete Init bodies now have portable implementations in `pass_initialization.hpp/.cpp`:

| Method / registry id | All filter, in original order | Later effects |
| --- | --- | --- |
| ProjectileSystem.Init / game:0x06000f7c | 27, 0, 1 | Store subset +0x20; create fresh completion list +0x28; subscribe OnProjectileImpact to event 23 |
| MoveSystem.Init / game:0x06000f6d | 0, 1 | Store subset +0x20 |
| CollisionSystem.Init / game:0x06000e0a | 32, 0 | Store subset +0x20; create fresh pending-pair list +0x28 |
| GarbageSystem.Init / game:0x06000ea3 | 22 | Store subset +0x20 |

Each requires the world when invoking GetSubSet. A returned null subset is stored without a new guard. A lookup failure leaves the previous subset/queue untouched; a subscription failure leaves Projectile's newly stored subset and fresh queue in place. Repeated Projectile Init creates another event-23 subscription without removing the previous one. No enabled or pause guard is added. Filter allocation failures are outside the portable host contract.

PassInitializationWorld exposes subset acquisition and event-23 registration. It does not invent dynamic group deduplication, initial population or entity IDs. Projectile's receiver calls the real projectile_impact_cleanup body; the CombatEventHost must outlive its subscription. The persistent completion/pair queues are reset only after a successful subset lookup. Movement and Garbage do not create those queues in Init.

Tests check all four filters, null-world/lookup/subscription failures, null subset storage, duplicate receiver delivery and removal. The trajectory integration now acquires its real live projectile/movement groups through these Init bodies before the scheduled launch/landing passes, and delivers impact to the actual cleanup receiver. The synthetic factory test returns a shared diagnostic group to inspect requests; it makes no claim about World.GetSubSet semantics.

Evidence: local `research/exports/system-initializers/{ProjectileSystem,MoveSystem,CollisionSystem,GarbageSystem}.asm`. Projectile stores its group at 0x115ffac, queue at 0x115ffc4 and registers at 0x1160004; Collision stores its group at 0x1145f04 and queue at 0x1145f1c. Confidence high; runtime comparison absent.

### Entity ID selection and creation orchestration

EntityManager.CreateEntity (game:0x06000c5d, 0x11375e4, 372 bytes) captures the previous int32 counter at manager+0x14 and computes next with wrapping int32 addition. When signed next < 0x7ffffffd it stores next and uses the previous counter as the selected ID: a new manager starts at 0, so ordinary creation initially receives IDs 0, 1, 2. Deleted IDs are not searched on this path. No dictionary lookup checks the selected sequential ID before calling the factory.

Otherwise it stores 0x7ffffffd to the counter and probes Dictionary.ContainsKey from ID 0 upward, selecting the first absent ID. Probes cover 0 through 0x7ffffffe; 0x7fffffff is the exclusive loop bound. If every tested ID is present, control falls through with the old counter still selected. That exhaustive path is statically established but not exercised by the tests (it requires 2147483647 probes). The next allocation repeats scanning from zero. Counter values loaded from saves are not sanitized: e.g. INT_MAX increments to INT_MIN and follows the signed sequential branch.

After selection, temporary=true passes ID -1 to virtual Create(type,id); it still advances/clamps the counter and performs the free-ID scan before overriding the ID. A null factory result faults. A nonnull result is passed to AddEntity with the same temporary flag, then returned. Factory/registration failures leave earlier counter changes and service effects committed. The existing concrete MyEntityManager factory returns null for nonzero type; custom virtual factory behavior remains explicit rather than rejected ahead of that call.

Port: `entity_creation.hpp/.cpp` implements this entire selection/Create/AddEntity orchestration over EntityIdState and EntityCreationHost. The host supplies registered-ID queries, object allocation and registration; it owns returned ComponentSlots and establishes their ID field/registry relationship. The registry and three component-event subscriptions are now implemented by entity_registration, with a composed creation-to-membership test. Tests verify initial IDs, temporary ID gaps, threshold and repeatable probe order, counter visibility during lookup, signed overflow and lookup/factory/null-result/registration failures. HashSet identity remains supplied by the host.

Evidence: local `research/exports/entity-create/M37082_CreateEntity.asm`: previous counter load at 0x1137628, wrapping increment and signed comparison at 0x1137634/0x113763c, clamp at 0x1137648, probe at 0x113766c, selected scan ID at 0x1137688, temporary override at 0x1137694, factory call at 0x11376a4 and AddEntity at 0x11376bc. The 0x1137680 exhaustion branch skips the scan-ID assignment. Confidence high; no Android comparison.

### Entity registration and component event lifetime

EntityRegistry now implements EntityManager.AddEntity's ordering over borrowed EntityRegistration records (ID, ComponentSlots and subscription service). For temporary=false it inserts ID -> entity with duplicate-key failure, then subscribes manager handlers for added, removed and changed components, in that order. For temporary=true it skips insertion and still subscribes all three. No existing component is synchronized into groups at registration. Repeating a temporary registration retains duplicate subscriptions; repeating an ordinary ID fails before adding any subscription. A later subscription failure preserves the dictionary entry and earlier subscriptions.

`ComponentEvents` is the entity's multicast notification adapter. It captures each channel before delivery, preserves duplicate/ordered receivers and stops on subscriber exceptions using the public delegate contract already cited above. Entity.DisposeEvents (game:0x06000bb2, 0x1131adc, 12 bytes) clears the three channel references; an already captured delivery continues. Dispose does not clear component slots, remove registry entries or update groups. The existing destruction chain must remove components before disposing events when group notifications are required.

The registry uses standard unordered_map for ID lookup, plus the ordered slot adapter described below for dictionary Values enumeration. Its find/erase helpers are host primitives, not claims about the original GetEntitySafety or complete deletion methods. EntityRegistration's ID is supplied by the factory/restorer and remains separate from ComponentSlots; ownership and other ID writers remain explicit.

Port: `entity_registration.hpp/.cpp`. Tests compose create_entity -> factory node -> registry -> component subscriptions -> existing SubsetMembership/EntitySubset, covering ordinary and temporary entities, destruction-order notifications and subsequent creation. Additional tests cover duplicate IDs/subscriptions, preexisting components, a failure on the second subscription, disposal during delivery and exceptions after a component store. The fixed observer remains available; DynamicSubsets now supplies directory-backed group notification and GetSubSet population as described below.

Evidence: local `research/exports/entity-registration/M37087_EntityManager_AddEntity.asm`: insert at 0x1137800, added subscription at 0x1137860, removed at 0x113788c and changed at 0x11378c8. `research/exports/entity-load-followup/M36911_DisposeEvents.asm` clears entity+0x10/+0x18/+0x20 directly. The [existing registration research](../archive/reports/37-entity-factory-and-subsets.md) resolves all three callback targets. Confidence high for registration/disposal order; runtime comparison absent.

### Dynamic group lookup and filter identity

World.GetSubSet (game:0x06000c09, 0x113314c, 28 bytes) requires World.entityManager +0x20 and forwards the filter unchanged. EntityManager.GetSubSet (game:0x06000c66, 0x1138408, 508 bytes) looks up its filter-keyed group dictionary at +0x20 with a null default. A nonnull hit returns immediately. Otherwise it constructs Subset(filter), enumerates the current entity dictionary's Values in their original order, and calls the silent Subset.Add(entity) overload on every value. It disposes the enumerator before Dictionary.Add(filter,subset) publishes the group. There is no initial OnEntityAdded notification. Temporary entities absent from the entity dictionary do not participate in this initial population. Subsequent component notifications can still add them to a published group.

Filter.Equals (game:0x06000cad, 0x113a5a8) rejects null or a different concrete runtime type, then compares the other filter's hash before its own hash. Equal hashes are followed by ordered element comparison of AND, OR and NOR lists, short-circuiting in that order. ArrayExtension.EqualElements (game:0x06000001, 0x10ed39c) treats identical references, including null/null, as equal; one null differs from an empty array. Duplicate elements and list order are significant, despite hash collisions.

Filter.GetComponentTypesArrayHashCode (game:0x06000caf, 0x113a774) returns zero for null/empty. For each int32 index it adds uint32(1) << (index & 31), with wrapping sum. Negative and >=32 index values therefore alias other bits for hashing; Matches still applies actual component bounds checks. A list length >=33 emits a diagnostic before hashing; it does not reject the list. Filter.GetHashCode (game:0x06000cae, 0x113a6fc) caches AND_hash XOR OR_hash XOR NOR_hash at +0x28, using -1 as the recomputation sentinel. Each partial AND/OR result is stored before proceeding. List mutation or replacement does not invalidate the cache. A diagnostic failure in a later list can leave a partial non-sentinel cache that future calls return; a final -1 recomputes next time.

Port: `filter_identity.hpp/.cpp` implements the concrete Filter projection over borrowed SubsetFilter lists. Standard vector equality supplies ordered integer comparison; no generic array/runtime utility is reconstructed. Exact runtime type checking is represented by the typed final FilterIdentity API. The caller supplies the oversized-list diagnostic. Tests cover collisions, order, null/empty distinction, masked shifts, wrapping, stale caches, partial failure and the -1 sentinel.

GetSubSet orchestration is now ported in dynamic_subsets, using the original registry enumeration order and a filter-keyed group directory. Sorting IDs or iterating the portable unordered_map would change population order and later gameplay traversal. The identity module and the now implemented ordered registry cursor provide these inputs for directory deduplication and population.

Evidence: local `research/exports/dynamic-subsets/` bounded ARM64 exports. GetSubSet lookup at 0x11384b0, subset construction at 0x11384d4, silent population call at 0x113852c, dictionary publication at 0x1138564. Filter helper shifts at 0x113a844 and adds at 0x113a848; cached stores at 0x113a730/0x113a74c/0x113a764. Confidence high for normal-path contracts; no runtime comparison.

### Ordered registry Values cursor

Registry enumeration needed by GetSubSet follows occupied entry indices in increasing order, not ID or bucket order. Insertion takes the most recently freed slot before appending. Removal preserves the used-slot limit even after removing the last entity; this differs from the Subset HashSet empty-reset behavior. Resize copies entries to the same indices. Successful removal increments the version; absent removal does not. Native TryInsert increments version before duplicate detection, so a failed duplicate Add invalidates a cursor. Clear increments version even when already empty. MoveNext checks version before scanning, including after exhaustion.

Public prior art: Microsoft [reference Dictionary implementation](https://github.com/microsoft/referencesource/blob/ec9fa9ae770d522a5b5f0607898044b7478574a3/mscorlib/system/collections/generic/dictionary.cs) (MIT, existing notice in third_party/licenses/dotnet-referencesource.txt) supplies the entry-slot/free-list design and increasing-index ValueCollection enumeration. The native version timing above differs and is preserved from the shipped artifact; this is not a claim that this public revision exactly matches Unity's runtime. Bucket layout, capacity primes, hashing internals and generic dispatch are unnecessary for this adapter and are not ported.

Port: EntityRegistry now keeps a standard ID -> slot lookup and a slot vector/free stack. Its values() cursor preserves slot reuse and mutation checks; growth preserves indices. Tests cover multiple holes, reuse after emptying, non-ID ordering, duplicate-Add invalidation, absent removal, temporary registration and empty Clear invalidation. This cursor exposes borrowed ComponentSlots and must not outlive the registry. Original Serialize/DestroyAllEntities methods are not promoted merely because this shared ordering input is available.

Evidence: local `research/exports/registry-order/` exports. The game registration calls Add at 0x14be40c, which tail-calls shared TryInsert at 0x14bee68. TryInsert bumps version +0x2c at 0x14beea8; its insertion path uses free head +0x24 and free count +0x28. Remove 0x14bf808 links a freed entry and increments free count/version at 0x14bfafc. Clear 0x14be594 always increments version at 0x14be5ec. Values.MoveNext 0x15918ec compares captured version, scans 24-byte entries and skips negative hashes. Resize 0x14bf6d4 copies [0,count) at 0x14bf774 before rebuilding buckets. Metadata at GetSubSet's call sites identifies Dictionary<int,Entity>/Values; the shared bodies erase the reference value type. Confidence high for these observable operations; Android comparison absent.

### Dynamic group creation and live directory notifications

`dynamic_subsets.hpp/.cpp` now composes the GetSubSet contract with EntityRegistry::values and EntitySubset. DynamicSubsets.get retains its GroupFilter key, silently populates a fresh group from registered entities, then publishes it. Equivalent keys return the existing group; same-hash unequal keys remain distinct. Stored hash values are masked to 31 bits, with newest matching-hash entries compared first, following the public dictionary lookup contract. A cold uninitialized directory skips lookup hashing until publication. Publication advances its version before hashing/duplicate detection, preserving failure invalidation. The concrete ComponentGroupFilter uses the ported Filter identity; the GroupFilter interface keeps other game-defined Matches/hash/equality implementations explicit. The owned-All constructor retains initialization lists; borrowed identity lists must otherwise outlive use.

EntitySubset now accepts a validated predicate provider in addition to its existing borrowed SubsetFilter constructor. This supports original IFilter calls without guessing the bodies of custom game filters. Published groups retain the filter's live matching behavior even when its cached hash becomes stale. Population order is the registry cursor order; notifications are not emitted by initial population. Temporary unregistered entities enter published groups only through later component notifications.

DynamicSubsets is also the manager's component observer: add/remove enumerate published groups in dictionary slot order, try the notifying Add overload, then Remove on a false match; changed only forwards, without membership reevaluation. The directory currently only grows, so slot order is publication order. A callback creating another group invalidates the current directory enumeration on its next advance, including after the last original entry. Existing mutation side effects remain committed. A group is not visible before population: a reentrant same-key creation can publish an inner group, causing the outer publication to fail as a duplicate. Registry mutation during population invalidates its cursor and prevents publication.

Tests cover ordered population after registry hole reuse, equivalent-filter reuse, stale-key behavior, temporary entities, Change(null), cold hash timing, failed population and reentrant publication. The integration invokes all four real Projectile/Move/Collision/Garbage Init functions against a dynamic directory and real registry, then verifies component writes and temporary entities update the returned groups. Components in this initialization test are markers; motion/damage behavior remains covered by the separate gameplay integration scenarios.

Evidence: the bounded GetSubSet/Filter exports above; manager component handlers in local `research/exports/entity-registration/`; the public dictionary entry/lookup contract and native version differences already recorded under ordered registry Values. Port evidence is `src/core/ecs/dynamic_subsets.cpp` and its contract test. Confidence high for normal paths and tested mutation rules; World.Destroy retention and registered-entity teardown are now ported below; arbitrary directory removal and Android comparison remain separate.

### World teardown and retained groups

World.Destroy (game:0x06000c02, 0x11356f4, 128 bytes) performs four operations, in order: clear World.visibleEntities_ at +0x40 (HashSet<Entity>), call entityManager.DestroyAllEntities, call systemManager.RemoveAllSystems, then eventManager.RemoveAllEventHandlers. It rereads each manager field when reached and faults if absent. Failure prevents later operations; prior effects remain. There is no store resetting World.isLoaded_, entity ID counter, map/player references or the manager's group dictionary in this wrapper. An automatic directory reset would change its behavior.

EntityManager.DestroyAllEntities (game:0x06000c60, 0x11379e8, 132 bytes) captures registry Values and sends Entity.Destroy(false) to every nonnull value (the excluded entity argument is null). This uses the already ported Entity.Destroy guard and manager deletion order: remove components -> dispose component events -> skip individual registry removal -> set destroyed flag. After the cursor finishes/disposes, DestroyAll rereads the registry and clears it. The counter is untouched. A callback changing registry membership invalidates the live cursor on its next advance; final Clear and subsequent world teardown operations are skipped. An entity already marked destroyed is skipped by Entity.Destroy even if components remain; registry Clear still removes its entry on success.

The group dictionary is retained. Component removals update existing groups through live callbacks before entity event disposal. Filters that continue to match an empty entity can keep it, and temporary entities absent from the registry are not traversed by DestroyAll. Do not assume every group becomes empty. Repeated GetSubSet can return the same retained group after teardown. The separate requirement to establish the entry state of Continue/deserialization is not solved by adding an implicit clear here.

Port: `world_teardown.hpp/.cpp` implements the four-step world orchestration and the live-registry destruction/clear pass. Explicit host services bind visible-set clearing, system removal and event teardown; no additional runtime collection implementation is needed. Tests compose the actual registry, dynamic groups, component events, Entity.Destroy/deletion helpers and entity-event teardown. They check retained temporary membership, disposed component notifications, already-destroyed entities, removal failure, registry mutation and replacement of the registry reference before final Clear. Diagnostic host hooks model callbacks; rendering/system implementations remain external.

Evidence: local `research/exports/world-destroy/` bounded World.Destroy and DestroyEntities exports; existing `entity-destruction-load/M37085_DestroyAllEntities.asm`; metadata usage at ELF GOT 0x2730d58 resolves HashSet.Clear, instance 1063, and metadata names World+0x40 visibleEntities_. World calls occur at 0x1135734/0x1135744/0x1135754/0x113576c. DestroyAll passes remove=false at 0x1137a40 and calls registry Clear at 0x1137a64. Confidence high for ordering and retention; no runtime comparison.

## Evidence

- Temporary lifetime: local `research/exports/garbage-lifetime/{Init,NeedsUpdate,Update,AddGarbage}.asm`. Init writes filter value 0x16; AddGarbage writes +0x10 and passes slot 0x16; Update stores the decremented counter at 0x11532e0, then queues on signed <= 0, and calls Entity.Destroy(true) at 0x1153384 only after the first enumeration finishes. Confidence high for arithmetic and call order; Android comparison absent.

- [32-ecs-animation-order.md](../archive/reports/32-ecs-animation-order.md) — CreateSystems, Add, Init/Update dispatch, QuestForm passes
- [33-ecs-lifecycle.md](../archive/reports/33-ecs-lifecycle.md) — enable/disable, pause, RemoveAll, system (de)serialize
- [35-component-slots.md](../archive/reports/35-component-slots.md) — BaseEntity slot contract, port
- [36-pause-hooks.md](../archive/reports/36-pause-hooks.md) — IPausable, BasePauseSystem, World.Deserialize
- [37-entity-factory-and-subsets.md](../archive/reports/37-entity-factory-and-subsets.md) — factory, AddEntity, subset mutations
- [38-component-predicates.md](../archive/reports/38-component-predicates.md) — ContainsAll/Any, Filter, Subset reads
- [39-generic-collections.md](../archive/reports/39-generic-collections.md) — generic tables, HashSet<Entity>, filter port
- [40-entity-enumeration.md](../archive/reports/40-entity-enumeration.md) — comparer, enumeration order
- [41-entity-load-and-destruction.md](../archive/reports/41-entity-load-and-destruction.md) — destroy and load passes
- [45-entity-write-layout.md](../archive/reports/45-entity-write-layout.md) — write passes, IComponent hooks
- Local exports under `build/research/`: `entity-components/`, `entity-component-followup/`, `entity-create/`, `entity-factory/`, `entity-registration/`, `subset-mutations/`, `subset-filters/`, `filter-predicates/`, `collection-operations/`, `collection-add-core/`, `entity-enumeration*/`, `entity-set-capacity/`, `entity-destruction-load/`, `entity-load-followup/`, `ecs-order/` (incl. `interface-dispatch.json`, `factory-order.json`), `ecs-lifecycle/`, `pause-hooks/`.

## Open questions

1. (blocking) Entity ID lifetime: identify any writers of `entity+0x28` besides construction and Deserialize; whether IDs stay stable while in groups (they are the HashSet hash). Counter selection and free-ID scanning are now established and ported above.
2. Exact allocation/capacity limits are not reproduced by the narrow ordering adapter. Public-source slot/version behavior is implemented; revisit capacity only if a concrete gameplay case depends on it.
3. Bind original Serialize to the established Dictionary<int,Entity> slot-order/version cursor; DestroyAllEntities now uses it.
4. Full slot-index → component type table (`ComponentFactory.Create`), and EntityManager deletion callbacks/indirect mutations (`Entity.Destroy` wrapper is now ported; see [rewards.md](rewards.md#member-finalization-and-deferred-town-return)).
5. Continue/deserialization entry-state preparation remains unresolved; World.Destroy retains the group directory and does not reset the counter or loaded flag. Do not add implicit resets to cover this gap.
6. `Subset.get_entity` helper (`0x12af8d4`), `entitiesCache_`/`dirty_` usage, and delegate disposal of subsets.
7. Meaning of `World.byte+0x38`; entity+0x2c is the destroyed guard, set after successful manager deletion; who subscribes OnSystemEnabled/Disabled.
8. Bind initialization/event subscriptions and the remaining update bodies to the established complete system schedule.
9. Remaining SystemManager collections (`+0x38,+0x40,+0x50,+0x58,+0x60`) and BaseSystem ctor / SetWorld initial state.
