# Animation component payloads (Animation, Direction, Seb)

Byte layouts written by `AnimationComponent`, `DirectionComponent` and `SebComponent` `Serialize` and read by their `Deserialize`, inside a present slot of the entity component block ([entity-save.md](entity-save.md)). Byte order (big-endian), widths, sign extension, boolean rule and the layers-array encoding are established from assembly; C++ codecs exist and are tested exhaustively on synthetic fixtures. Not checked against a real save.

## Layout

Integers are **big-endian, signed**. `WriteShort` → `EncodeUtil.ShortToByteAry(v, true)` → `BitConverter.GetBytes(short)` (STRH, little-endian on AArch64, truncates to the low 16 bits) → `ConvertEndian` swaps because the flag is 1. Ints likewise via `IntToByteAry`/`GetBytes(int)`. Readers reverse (`ByteAryToShort`: swap, then LDRH) and components apply SXTB/SXTH when storing to int32. `WriteByte` sign-extends then writes the low byte. Booleans: write `v & 1`; read `byte == 1`.

### AnimationComponent (3 bytes)

| Offset | Size | Type | Field |
|---|---|---|---|
| 0 | 1 | int8 | `rate` (+0x10) |
| 1 | 2 | int16 BE | `backup` / return clip (+0x14); -1 = none |

Examples: `(1, -1)` → `01 ff ff`; `(-128, 0x1234)` → `80 12 34`; `(256, 0x12345678)` → `00 56 78` (truncated: int32 values do not round-trip).

### DirectionComponent (2 bytes)

| Offset | Size | Type | Field |
|---|---|---|---|
| 0 | 1 | int8 | `dir` (+0x10) |
| 1 | 1 | bool | `locked` (+0x14); true only for byte 1 |

Example: `(-1, true)` → `ff 01`.

### SebComponent (5 + n bytes)

| Offset | Size | Type | Field |
|---|---|---|---|
| 0 | 1 | int8 | `res` (+0x10) |
| 1 | 2 | int16 BE | `id` / clip (+0x14) |
| 3 | 1 | int8 | `frame` (+0x18) |
| 4 | 1 | int8 | layers length L (`StreamUtil2.WriteIntArrayAsByteArray(arr, 1)`) |
| 5 | L | int8[L] | layer values (+0x20), each via WriteByte, in order |

Layers encoding (length width 1; the 2/4/8 variants exist in StreamUtil2 but Seb doesn't use them):
- null → length byte `0xff` (-1), no elements. Null, empty and `[-1]` are three distinct states (`[-1]` = `01 ff`).
- Writer: length must be **< 127**; 127 or more throws. Length 0 writes `00`.
- Reader (`ReadIntArrayFromByteArray`): sign-extended length; -1 → null; other negatives (`0x80–0xfe`) are passed to the array allocation and fail; 0..127 allocate and read. So `0x7f` (127 elements) is readable but cannot be re-written.

Examples (res 1, id 0x1234, frame -128): null → `01 12 34 80 ff`; empty → `01 12 34 80 00`; `[-1, -128, 127]` → `01 12 34 80 03 ff 80 7f`.

None of the three types writes extra bytes: their `SerializeExtra`/`DeserializeExtra`/`OnBeforeSerialize`/`OnDeserialized` are the inherited RET hooks. Deserialize stores each field as soon as it is read (no temporary, no rollback).

## Reading it

`src/core/animation/animation_payload.{hpp,cpp}` (library `animation_transitions`):
- `encode_playback/decode_playback` (`std::array<uint8_t,3>`), `encode_direction/decode_direction` (`std::array<uint8_t,2>`, `DirectionState {direction, locked}`): exact-size arrays, so truncation is unrepresentable. Encode truncates to 8/16 bits; decode sign-extends without implementation-defined casts; boolean true only for byte 1.
- `encode_sprite_payload` / `decode_sprite_payload` (`SpritePayloadState {SpriteAnimation sprite; optional<vector<int32_t>> layers}`): `nullopt` = null array, empty vector = length 0. Decode takes a finite buffer and returns the state plus bytes consumed; trailing bytes are untouched. Truncation → `std::out_of_range`; negative non-sentinel length → `std::length_error`; encode of ≥ 127 layers → error. These are API choices for whole-block inspection, **not** emulation of StreamUtil EOF, partial stores or IL2CPP exception identity.
- Test `tests/test_animation_payload.cpp` (CTest `animation_payload_contract`): byte fixtures above, int32 extremes and truncation, all 65,536 int16 values and all 256 direction/boolean bytes compared with the signed definition (not just encoder/decoder agreement), every truncated prefix of a sprite payload, trailing bytes, all read lengths 0–127, write rejection at 127, all 127 invalid negative lengths, null sentinel followed by data.
- Not handled: stream wrapper, entity/slot framing, other component types.

## Evidence

- [30-animation-transitions.md](../archive/reports/30-animation-transitions.md) — first Serialize/Deserialize observation
- [42-component-save-fields.md](../archive/reports/42-component-save-fields.md) — field order, sign extension, layers helper, boolean rule
- [43-animation-payload-codec.md](../archive/reports/43-animation-payload-codec.md) — byte order proof, fixed codecs
- [44-sprite-payload-codec.md](../archive/reports/44-sprite-payload-codec.md) — Seb/layers codec
- [45-entity-write-layout.md](../archive/reports/45-entity-write-layout.md) — hooks are RET for these three types
- Methods: `AnimationComponent.Serialize/Deserialize` `0x113e3a8/0x113e424`, `DirectionComponent` `0x113ecf0/0x113ed6c`, `SebComponent` `0x11417a8/0x1141848`, `StreamUtil2.WriteIntArrayAsByteArray/ReadIntArrayFromByteArray` `0x1125098/0x1125338`, `EncodeUtil.ShortToByteAry/IntToByteAry/ConvertEndian/ByteAryToShort` `0x1bf34a0/0x1bf3338/0x1bf2f90/0x1bf36a8`. StreamUtil behaviour (Read loop, EOF) is in [save.md](save.md).
- Local exports `build/research/component-persistence/`, `component-stream-followup/`, `save-byte-order/`, `save-native-conversion/`.

## Open questions

1. Validation on a real save block (requires the envelope and the other components' payloads).
2. Whether any caller writes Seb layers of length ≥ 127 (writer would throw).
