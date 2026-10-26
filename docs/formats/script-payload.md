# Script payload

Big-endian list of `int[]` rows written by `AppData.SerializeScripts` and read by `AppData.DeserializeScripts`. The codec is recovered from the method bodies and reimplemented in `src/core/persistence/script_payload.cpp` with a contract test on a synthetic fixture. No direct caller of either method exists, so its place in the save is **not** established; it is distinct from the delayed-event block (selector 2, see [save.md](save.md#delayed-event-record)).

## Layout

All integers are int32 big-endian (`StreamUtil.WriteInt` → `EncodeUtil.IntToByteAry(…, 1)`, big-endian per report 43).

| Offset | Size | Type | Meaning |
|---|---|---|---|
| 0 | 4 | int32 BE | `count` = number of rows |
| then, only if `count > 0`, repeated `count` times: | | | |
| +0 | 4 | int32 BE | `length` of the row |
| +4 | 4×length | int32 BE[] | row elements (signed) |

Contracts of the original:

- Writer (`0x110f59c`, 444 bytes) writes `Size` once, then re-queries `Size` in the loop (no snapshot); each element is cast to `int[]`; a null row has no sentinel and reaches the error helper. Empty rows are valid (`length = 0`).
- Reader (`0x110f758`, 392 bytes) creates a new `Vector`, reads `count` (`StreamUtil.ReadInt` `0x1c03458`); `count < 1` (including negative) → empty collection, return. Per row: read `length`, allocate `int[length]`, read elements, then append. Negative `length` → invalid allocation (exception type not established), not null.
- Row element values are opaque here. `ExecuteScript` instructions are also int rows whose first int is an opcode (e.g. 10 = delayed event, see [events.md](../subsystems/events.md)), but the link between those rows and this payload is not established.

## Reading it

`saga::core::encode_script_payload(const ScriptRows&)` / `decode_script_payload(const std::vector<uint8_t>&)` in `src/core/persistence/script_payload.{hpp,cpp}`; `ScriptRows = vector<vector<int32_t>>`; decode returns rows + bytes consumed.

- Encoder requires a stable, non-null snapshot; sizes above INT32_MAX throw `length_error`.
- Decoder works on a finite buffer, consumes a prefix (trailing bytes untouched), preserves signed values without out-of-range unsigned→signed conversion; truncation → `std::out_of_range`; negative row length → `std::length_error`. Counts are checked against the remaining bytes before reserving, so failure point/order can differ from the original incremental reader; no partial mutation is exposed.
- Non-canonical input: a negative outer count decodes as empty but re-encodes as `0` — no byte-exact round trip for such input.
- Not a save envelope, DelayEvent codec, script executor or incremental StreamUtil emulation.

Test `tests/test_script_payload.cpp` (CTest `script_payload_contract`): independent BE fixture with three rows (empty; `10, -1, INT32_MIN, INT32_MAX`; `0x12345678`), every truncated prefix, unconsumed suffix, outer counts 0/-1/INT32_MIN, negative row length, huge positive counts without payload, positive/negative bit patterns. Built and passed on Windows x64 Release (8/8 at the time); not validated on Linux; fixture is synthetic, not captured from the game.

## Evidence

- [../archive/reports/53-checkscript-arrays-and-consumer.md](../archive/reports/53-checkscript-arrays-and-consumer.md), [54-script-payload.md](../archive/reports/54-script-payload.md), [55-delay-event-save-block.md](../archive/reports/55-delay-event-save-block.md) (no direct callers)
- Local: `build/research/script-checks/`, `build/research/script-persistence/`, `build/research/script-payload-callers.json` (empty), `build/research/script-payload-build.log`

## Open questions

1. Is the block reached indirectly (reflection/delegate) and under which selector, if any?
2. Which `AppData` collection is passed to `SerializeScripts` and when.
