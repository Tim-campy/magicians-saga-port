# Entity save block (EntityManager stream)

The `EntityManager` layer of the world save block: a counter, a list of entity headers, then one component-slot block per entity, written and read in two passes. Layout and hook order are established statically from `EntityManager.Serialize/Deserialize`, `Entity.Serialize/Deserialize` and `BaseEntity.SerializeComponents/DeserializeComponents`; payloads are known for three component types (Animation, Direction, Seb — see [animation-payload.md](animation-payload.md)). No real save has been parsed; there is no reader for this block. It is followed in the same stream by the `SystemManager` block; the surrounding envelope is in [save.md](save.md).

## Layout

All multi-byte integers are **big-endian** (StreamUtil wrappers byte-swap via `EncodeUtil.ConvertEndian` on little-endian AArch64; see [animation-payload.md](animation-payload.md)). Bytes are read signed (SXTB). Booleans are written as `v & 1` and read as **`byte == 1`** (2 or 0xff read false).

### World block order

`World.Deserialize` (`0x1135938`): EntityManager block, then SystemManager block, same stream; afterwards `World.byte+0x38 = 1`.

### EntityManager block

| Offset | Size | Type | Meaning |
|---|---|---|---|
| 0 | 4 | int32 BE | Manager counter `manager+0x14` (factory/ID counter) |
| 4 | 4 | int32 BE | N = number of dictionary entries (`manager+0x18`) |
| 8 | 6·N | header[N] | Pass 1: one entity header per entity |
| 8+6N | var | slots[N] | Pass 2: one component-slot block per entity, same entity order |

Entity header (6 bytes):

| Offset | Size | Type | Meaning |
|---|---|---|---|
| 0 | 1 | int8 | Type selector (`get_type`, vtable slot 4), written by the manager. Only 0 is constructible (`EntityManager.Create` returns null otherwise → load error) |
| 1 | 4 | int32 BE | Entity ID `entity+0x28` (also the HashSet hash key) — `Entity.Serialize` |
| 5 | 1 | bool | `entity+0x2c` (meaning unknown) |

Component-slot block (`BaseEntity.SerializeComponents`):

| Offset | Size | Type | Meaning |
|---|---|---|---|
| 0 | 1 | int8 | S = **number of slots** (array length, low byte), not number of present components. 0x23 (35) on the known path |
| 1 | var | entry[S] | For slot 0..S-1 ascending |

Slot entry: `bool present`; if false, nothing follows. If true: component payload (`Serialize`, vtable slot 4) followed by `SerializeExtra` (slot 6) output. Writer order per present slot: `OnBeforeSerialize` (slot 8) → `Serialize` → `SerializeExtra`. Reader order: `ComponentFactory.Create(slotIndex)` → slot 5 `Deserialize` → slot 7 `DeserializeExtra` → slot 9 `OnDeserialized` → `entity.Add(slotIndex, component)` (fires subset callbacks).

For Animation, Direction and Seb, slots 6–9 are the inherited `IComponent` hooks, all RET (vtables of metadata types 4410, 4420, 4442, 10 entries each): no extra bytes. Other component types may override them.

Example (illustrative, derived): one type-0 entity, ID 5, flag false, only an Animation(1,-1) in slot 10 of 35:
`<counter:4> 00 00 00 01 | 00 00 00 00 05 00 | 23 00×10 01 01 ff ff 00×24`.

### Write sequence (`EntityManager.Serialize`, `0x1138890`)

1. `OnBeforeSerialize` (slot 5; RET in EntityManager/MyEntityManager).
2. WriteInt counter, WriteInt dictionary count.
3. `Dictionary.Values` enumerator #1: per entity, WriteByte type, `Serialize` (slot 14).
4. Dispose, take `Dictionary.Values` again, enumerator #2: per entity `SerializeComponents` (slot 16).

Two fresh enumerations, not a frozen array: header order and component-block order coincide only if the dictionary is not mutated between them (hooks could). Do not interleave header and components per entity, and do not sort by ID.

### Read sequence (`EntityManager.Deserialize`, `0x1138ba8`)

1. ReadInt → `manager+0x14`; ReadInt → N; allocate temp array[N].
2. For each: ReadByte (signed) → `Create(selector, id 0)` (slot 4) → `Deserialize` (slot 15: ReadInt ID, ReadBoolean flag) → `AddEntity(entity, false)` (dictionary insert + subscriptions) → temp[i].
3. For each temp in order: `DeserializeComponents` (slot 17), then temp[i] = null.
4. `OnAfterDeserialize` (slot 6, RET).

`DeserializeComponents` (`0x1136e84`): ReadByte signed = S; allocate a **new** slot array of `manager+0x10` (35) entries replacing `entity+0x38` (no RemoveAll); loop i in 0..S-1 reading the present flag. S < 1 skips the loop. S is not clamped: i ≥ 35 with a present flag fails in `Add` bounds checking. A null from `ComponentFactory.Create` is an exception, not a skip. Note the asymmetry: the writer uses the low byte of the array length, the reader sign-extends it; arrays ≥ 128 slots would not round-trip (not reached with 35).

No rollback anywhere: each field is stored as soon as it is read; a failure leaves earlier entities/fields applied.

## Reading it

No reader or writer exists in `tools/` or `src/`. Payload codecs for Animation/Direction/Seb exist in `src/core/animation/animation_payload.cpp` (finite buffers, not stream I/O). A reader would need the slot-index → component type table and every other component's payload first. Validation done: static assembly reading only; no save file opened.

## Evidence

- [41-entity-load-and-destruction.md](../archive/reports/41-entity-load-and-destruction.md) — read passes, DeserializeComponents
- [42-component-save-fields.md](../archive/reports/42-component-save-fields.md) — StreamUtil readers, booleans, sign extension
- [43-animation-payload-codec.md](../archive/reports/43-animation-payload-codec.md) — big-endian proof
- [45-entity-write-layout.md](../archive/reports/45-entity-write-layout.md) — write passes, IComponent hooks, header
- [36-pause-hooks.md](../archive/reports/36-pause-hooks.md) — World.Deserialize order
- [33-ecs-lifecycle.md](../archive/reports/33-ecs-lifecycle.md) — SystemManager block hook order
- Local exports `build/research/entity-destruction-load/`, `entity-load-followup/`, `component-hooks-write/`, `entity-write-header/`.
- Behaviour context: [../subsystems/ecs.md](../subsystems/ecs.md).

## Open questions

1. (blocking) Slot index → component type mapping (`ComponentFactory.Create`) and payload layouts of every other component type, including any non-empty `SerializeExtra`/`OnDeserialized` (cross-entity references).
2. (blocking) Enumeration order of `Dictionary<int, Entity>` (generic instance 399) — determines entity order in the stream.
3. Meaning of the entity flag `+0x2c` and how the counter relates to ID allocation.
4. SystemManager block contents per system (each system's Serialize/SerializeExtra); AnimationSystem writes nothing.
