# ECS (entities, components, systems)

The game runs on Kairosoft's own ECS (`kairo.unity.ecs`, KairoLibrary) specialised by `ecs.*` types in Assembly-CSharp. Statically established: the world/manager construction, the concrete entity factory (one `BaseEntity` type with 35 component slots), slot storage and its notifications, subset (group) maintenance through `IFilter.Matches`, the `HashSet<Entity>` storage and its enumeration order, system registration and Update/Init/Pause dispatch, enable/pause/remove-all/serialize lifecycle, and the two-pass entity load and destroy order. Port: component slot storage, `ContainsAll/ContainsAny` and the `Filter.Matches` predicate are reimplemented and tested; managers, subsets, the HashSet ordering, ID allocation and system scheduling are not ported. Nothing was compared against a running original.

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

The generated getters confirm Speed slot 1, Projectile slot 27, Collision slot 32 and Damage slot 33. Projectile.parent (+0x38) supplies the attacker; Damage.value/attribute (+0x10/+0x14) supply numeric attack inputs. OnBulletHit removes Projectile and Collision after impact, then stores zero in Speed.vx/vy/vz (+0x10/+0x14/+0x18). Generated removers notify through entity removal before returning the captured object to ComponentFactory stacks. Portable component projections and the handler live in `projectile_hit`; Collision radius/offset fields and pairwise geometry are now implemented in `collision_check`; visited bookkeeping remains unported. See [combat.md](combat.md#damage-callers) for guards and callback-sensitive ordering.

### Combat record links

Combat getters establish four component slots: equipment 8 (`Entity.get_equip`, 0x112e6dc), human 14 (`get_human`, 0x112f19c), parameter 23 (`get_parameter`, 0x113070c), enemy 24 (`get_enemy`, 0x112e45c). Each passes that index to the entity's virtual getter and checks a nonnull result's component type. A missing component returns null there; its subsequent dereference by the stat route faults.

`HumanComponent.get_data` (0x113ffc4, 124 bytes) indexes Data.charaData by signed int32 +0x18. `EnemyComponent.get_data` (0x113efb0, 124 bytes) indexes Data.enemyData by signed int32 +0x20; enemy level is separate at +0x10. Both reject negative/out-of-range indices through unsigned bounds checking, and both return a null record slot unchanged. There is no record creation, clamping or fallback.

EquipComponent holds a nullable int32 ID array at +0x10. Its four getters resolve: index 0 weapon and index 1 shield from the same Data.equipmentData table; index 2 from Data.hatData; index 3 from Data.clothData. Getters at 0x113f374/0x113f400/0x113f490/0x113f520 check array length and table bounds, then return the record pointer (possibly null). `SetWeapon(int)` (0x113f5c8) and `SetShield(int)` (0x113f608) only write the ID array; they do not validate the selected record or emit entity component-change notifications. The record-taking weapon overload at 0x113f5b0 first requires a nonnull record and reads its id at +0x18. The component constructor at 0x113f6fc only forwards to Object; allocation/population of the ID array remains to be located.

Port: `combat_records.hpp/.cpp` supplies typed, non-owning record lookup through the existing `ComponentSlots`. Portable HumanRecordLink/EnemyRecordLink/EquipmentRecordLinks are projections, not the full original components. `CombatRecordTables` borrows decoded catalogs; returned records keep shared identity and subsequent lookups observe component IDs/table edits. Tests cover all six lookup routes, shared weapon/shield table, null records versus invalid IDs, absent/wrong component types, short/null equipment arrays, setter timing and lookup-to-character-growth composition. Parameter slot 23 is identified but no complete parameter-component lifecycle is claimed. Live resolution on each damage call is now connected by EcsDamageHost. Concrete entity construction and persistence remain open. ParamSet is dictionary-backed; sparse keyed lookup in slot 23 is covered without reconstructing .NET collection internals (see combat.md).

Evidence: local `research/exports/combat-record-links/` and `combat-slots/` bounded ARM64 exports (exported addresses add 0x100000 to ELF addresses). Confidence high for read/store/bounds behavior. No public collection/runtime internals were reconstructed.

### Entity creation

`World.CreateEntity` (`0x1133568`) forwards to `EntityManager.CreateEntity` (`0x11375e4`) with second argument 0. That calls the manager's virtual factory (vtable slot 4, via class+0x178): `MyEntityManager.Create` (`0x1141a44`, tail call) → `EntityManager.Create` (`0x11378d0`). `Create` returns **null for any type selector ≠ 0**; for selector 0 it allocates `BaseEntity` and calls `BaseEntity..ctor(id, slotCount=manager+0x10, manager)` (`0x1136824`), which allocates the component array. `CreateEntity` treats a null return as an error, then registers the result via `AddEntity`. ID allocation/reuse policy (limit, free-ID search) is not recovered.

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

`World.GetSubSet` is called with a `Filter` (e.g. `AnimationSystem.Init`); the full GetSubSet/dedup contract was not exported.

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

`SystemManager.Init` (`0x113c614`) calls `IInitSystem` method 0 on each of `+0x28` in order.
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
| `kairo.unity.ecs.World.Destroy` | game:0x06000c02 | Calls RemoveAllSystems | medium |
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
- Consequence: since Update dispatch ignores the enabled byte, toggling `enabled` alone does not stop a system whose `NeedsUpdate` returns true (e.g. AnimationSystem). Do not add a generic `if (!enabled) skip`.

### Movement integration and projectile ordering

MoveSystem.Init (0x115fa0c) selects required slots [0,1]: Position and Speed. MoveSystem.Update (0x115faac, 616 bytes including exception cleanup) traverses that live subset. For each entity it captures Position and Speed, then adds binary32 vx/vy/vz to x/y/z (x/y are stored together in the original). It does not multiply by a delta time or clamp coordinates.

Position additionally contains offsetX/Y/Z +0x1c/+0x20/+0x24 and parent +0x28. After velocity integration, a nonnull parent with Position slot 0 causes each child coordinate to be overwritten with the freshly fetched parent coordinate plus the corresponding offset. A parent without Position leaves the velocity result intact. There is no self-parent or destroyed-parent guard. A self-parent therefore adds offsets to the already integrated coordinates. The captured child's Position object remains the destination; this parent link is independent of Projectile.parent, which identifies the attacker. Parent-before-child versus child-before-parent iteration changes the result, so the production adapter must preserve subset order.

Relative system order is ProjectileSystem -> MoveSystem -> CollisionSystem (factory indices 10/11/12, stores at 0x1168da0/0x1168df4/0x1168e48). Both ProjectileSystem and MoveSystem explicitly implement IUpdateSystem (native type 12847), confirmed from metadata interface arrays for type indices 4486/4484. SystemManager.Add preserves factory order in its update list. Their NeedsUpdate methods (0x1160010 / 0x115faa4) both return the enabled byte +0x18 and ignore pass. Projectile processing, including its deferred event-23/removal pass, completes before MoveSystem integrates all three velocity axes. Thus Move also applies vy after Projectile has written the current curve height; horizontal-only integration is insufficient. Other systems and the complete update-list membership remain separately scoped.

Evidence: local `research/exports/move-system/Init.asm`, `Update.asm`, `movement-gates/`, existing `ecs-order/factory-order.json`, metadata PositionComponent offsets and raw v31 type interface entries. Confidence high for the per-entity path and this relative order; no runtime comparison.

### Temporary entity lifetime

GarbageSystem.Init (0x11530b8) requests the subset whose And filter contains only slot 22 (Garbage). NeedsUpdate (0x115314c) returns BaseSystem.enabled (+0x18) and ignores the pass argument. Update (0x1153154, 904 bytes) itself does not test enabled. It allocates a pending list, enumerates the live subset, requires each entity and Garbage component, decrements its int32 counter at +0x10 with wrapping, stores it, and queues the entity when the new value is <= 0. The comparison uses the decremented value: 1 expires immediately, 2 after two successful passes, 0 becomes -1 and expires, and INT_MIN wraps to INT_MAX and survives. It does not skip destroyed entities during this counting phase.

Only after the entire subset enumeration and its disposal does it traverse the pending list and call Entity.Destroy(true) in discovery order. It does not re-read the Garbage component/counter before destruction. Thus a deletion callback changing a later queued entity's counter cannot cancel that deletion; the existing Entity.Destroy destroyed guard can still skip an already-destroyed entity. A counting/enumeration failure leaves earlier decrements visible but destroys none of the pending entries. A deletion failure retains previous deletions and all decrements, and prevents later deletions. The temporary queue is not retained for another update.

Entity.AddGarbage (0x112eccc) pops from the Garbage pool if available, otherwise constructs a component, writes the supplied counter and invokes AddComponent at slot 22. No existing-slot guard or clamping precedes acquisition/write. The port uses an explicit pool interface and the existing ComponentSlots add contract: an occupied slot ignores the new component, so AddGarbage does not refresh an existing counter, although acquisition and the new component write still occur. Impact creation now composes this operation with GarbagePass in tests: the source counter 1 expires on the first pass, while neighboring damage areas with counter 2 retain Collision/Damage until the second pass. These are update passes, not a claim of wall-clock duration or collision opportunities.

## Port

- Collision slot 32 now includes the visited byte (+0x20). `collision_pass` resets and marks it while traversing fresh And({32,0}) cursors; pairs are delivered only after detection finishes. See [combat traversal](combat.md#collision-candidate-traversal-and-deferred-delivery). The cursor adapter still owns live HashSet storage/order/version checks.

- `garbage_lifetime.hpp/.cpp` implements AddGarbage, the wrapping countdown and two-phase expiration orchestration, reusing Entity.Destroy and the existing deletion sequence. Tests cover overflow, callback mutations, deferred deletion, enumeration/deletion failure, destroyed guards and reentrant passes. Cursor ownership preserves disposal before deletion and during exceptions; live HashSet order/version checks remain a host obligation, so GarbageSystem.Update stays documented, not wholly reimplemented. Scratch capacity is reused between successful passes without sharing the active queue across callbacks.

- `src/core/gameplay/movement.hpp/.cpp` implements the complete per-entity MoveSystem body over borrowed Position/Speed components, including parent overrides. `tests/test_movement.cpp` covers three-axis integration, missing parent Position, self-parent, partial failure, binary32 precision and order-sensitive parent/child updates. `test_projectile_motion` now composes Fire, Projectile's motion/completion pass and Move for a full trajectory. Live subset enumeration and exception-disposal remain outside this kernel, so MoveSystem.Update stays documented rather than reimplemented as a whole.

- `src/core/ecs/component_slots.{hpp,cpp}` (`saga::core::ComponentSlots`, library `component_slots`): fixed slot vector of borrowed `Component*`, size fixed at construction, synchronous `ComponentObserver` callbacks (added/removed/changed), `add/change/remove/remove_all/get/contains/contains_all/contains_any`. Does not own, clone or free components; operations don't allocate. Index/size errors are C++ exceptions, not IL2CPP exception identity. Test: `tests/test_component_slots.cpp` (CTest `component_slots_contract`): duplicate add, null add, same-pointer change, callback args and visibility, ascending RemoveAll with later/current/earlier reinsertion, exception without rollback, invalid indices, empty array, predicate short-circuit order, empty/repeated lists, Change(null).
- `src/core/ecs/component_filter.{hpp,cpp}` (`matches_component_filter(entity, all, any, none)`): Filter.Matches predicate over ComponentSlots with borrowed nullable lists, re-read each call. Test: `tests/test_component_filter.cpp` (`component_filter_contract`).
- Missing: GC/pooling, array type checks, subclasses overriding virtual slot methods, EntityManager/registry/ID counter, Subset with HashSet slot-order semantics and notifications, Filter identity/hash/builders, World, SystemManager lists, dispatch, enable/pause/remove-all, (de)serialization passes. No link yet between component_slots and the animation module.
- Header comments in `src/core/ecs/*.hpp` still cite the old `docs/35-component-slots.md` path; should point here.

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

1. (blocking) Entity ID allocation: writers of `entity+0x28` besides Deserialize, use of the counter `+0x14`, free-ID policy and limit; whether IDs are stable for an entity's lifetime (they are the HashSet hash).
2. HashSet capacity policy (Initialize/IncreaseCapacity sizes) and Clear. Growth does not reorder slots, so order is reproducible without it, but bucket/limit equivalence needs it.
3. (blocking) Dictionary<int, Entity> (instance 399) enumeration order used by Serialize and DestroyAllEntities.
4. Full slot-index → component type table (`ComponentFactory.Create`), and EntityManager deletion callbacks/indirect mutations (`Entity.Destroy` wrapper is now ported; see [rewards.md](rewards.md#member-finalization-and-deferred-town-return)).
5. `World.GetSubSet` contract (dedup of equal filters, initial population of a new subset from existing entities), Filter hash/equality.
6. `Subset.get_entity` helper (`0x12af8d4`), `entitiesCache_`/`dirty_` usage, and delegate disposal of subsets.
7. Meaning of `World.byte+0x38`; entity+0x2c is the destroyed guard, set after successful manager deletion; who subscribes OnSystemEnabled/Disabled.
8. Which systems implement IUpdateSystem/IPausable and the final Update list order; which `NeedsUpdate` overrides check `enabled` (MoveSystem and ProjectileSystem are confirmed above).
9. Remaining SystemManager collections (`+0x38,+0x40,+0x50,+0x58,+0x60`) and BaseSystem ctor / SetWorld initial state.
