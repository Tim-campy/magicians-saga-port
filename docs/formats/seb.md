# SEB sprite / animation

`.seb` files hold groups of sprite records consumed by `kairo.unity.ui.Seb`. The disk layout (modes 0 and 1) is proven by `Seb._load` assembly and by exact consumption + byte-exact rebuild of all 303 shipped files (302 mode 0, 1 mode 1, 452 groups, 2,686 records); a C++ reader exists. The runtime compact buffer, the Sprite slot names, mirroring arithmetic and frame-advance rules are mapped statically; interpolation between records, TexId -> image resolution and presentation are not closed.

## Layout

`h` = s16 big-endian, `B` = u8, `I` = u32 big-endian. Sizes 28..2,672 bytes (56,576 total).

| Element | Mode 0 | Mode 1 |
|---|---|---|
| Prefix | none (first byte non-negative) | `ff` (signed -1; other negative values = unknown mode) |
| Header | `group_count:h` (the two bytes read high/low), `header_value:h` | `group_count:h`, `header_value:h` (stored at `this+0x1c`) |
| Group | `record_count:h`, `legacy_value:h` (read at 0x1baf7cc, result unused) | `record_count:h` |
| Record | 10 x `h` -> slots 0..9 (0x1baf918..0x1bafa50) | 4 x `h` -> slots 0,1,6,7; `control:B` -> slot 10; then optional fields |

Mode 1 optional fields, with `d` = second short, `c` = control:

| Field | Present when | Slots | Evidence |
|---|---|---|---|
| `extra_word:I` | -6 <= d <= -2, or (d >= 0 and c != 0) | — | 0x1baf904..0x1bafb20 (negative-d branch taken before the c test; d = -9 never reads it) |
| `pair:2h` | d >= 0 or d = -8 | 2, 3 | 0x1bafb24..0x1bafb70 |
| `tail_pair:2h` | always | 4, 5 | 0x1bafb74..0x1bafbb4 |
| `flags:B` | d >= 0 | bits 0/1 -> 8/9; other bits preserved | 0x1bafbb8..0x1bafbfc |

Endianness proof: `DataInputStream..ctor` (0x1be89e8) wraps a `BinaryReader` stored at `this+0x38`; `DataInputStream.ReadShort` (0x1be8b60) calls `BinaryReader.ReadInt16` (vtable slot 13, method 10009, `ldrh` at 0x1db9548) then `IPAddress.NetworkToHostOrder` (`rev` + shift, 0x2323814..0x232381c). Overload `_load` kairo:0x06001761 only wraps a byte array in `ByteArrayInputStream` and calls kairo:0x06001762.

**Runtime compact buffer** (built by `_load`, 0x1bafc7c..0x1bafdd0, arrays at `this+0x20/+0x28/+0x30`, offsets accumulated 0x1bb00bc..0x1bb01b8): slot 0 (FrameNo) goes to a short array `this+0x28`; other fields packed 8 bytes per record in `this+0x30`: TexId s8, X s8, Y s8, U u8, V u8, W u8, H u8, last byte = Blend (low nibble) | ReverseV (bit 5) | ReverseU (bit 6). TexId byte **-128** marks the extended form: the next byte indexes 7 shorts in `this+0x38` (TexId, X, Y, U, V, W, H), four buffer bytes form Color (high to low). Mode-0 derivations for negative values and colour/control are not emulated by any reader.

**Sprite slots** (int32 array, elements from offset 0x20; getters `kairo.unity.ui.Sprite.get_*`): 0 FrameNo, 1 TexId, 2 U, 3 V, 4 W, 5 H, 6 X (= TransX), 7 Y (= TransY), 8 ReverseU, 9 ReverseV, 10 Blend, 11 Color.

**Mirroring** (`ConvBufferToSprite`, 0x1bb2dd0..0x1bb2e80): bit 0 of the reverse argument XOR bit 1 of byte `this+0x40` flips ReverseU and X; bit 0 of `this+0x40` flips vertically. For TexId != -8 and positive size: `X' = -2*floor(W/2) - X`, `Y' = -2*floor(H/2) - Y` (W=5 gives -4), with truncation to s16. For TexId = -8: negate X, U, W (horizontal) or Y, V, H (vertical). Negative TexIds are commands, never PNG indices.

**Frame selection.** `GetSpritesLocal` calls `GetSpriteLocal` (BL 0x1bb0c5c) for every group (count = length of `this+0x20`); results may be null. `GetSpriteLocal` takes `arg % 10000` (0x1bb233c..0x1bb235c), compares against the group's FrameNo shorts (0x1bb2608..0x1bb2644), caches, converts an exact match (0x1bb27ec, reverse flag derived from comparing the original argument with 9999) or two neighbouring records (0x1bb2854, 0x1bb286c) whose combination is unaudited. FrameNo is a selection key, not time.

**Timing.** Fields: `+0x18` current frame (`GetCurFrame`/`SetCurFrame`, set without normalisation), `+0x1c` max frame (`GetMaxFrame`), static `+0x14` AutoAnimation (setter stores `arg & 1`), `+0xa0` registered flag, `+0xa4` TotalFrames at registration; static `+0x8` collection, `+0x10` active count (static block via GOT 0x272f880, `class+0xb8`).
- `Frame`: returns unchanged if AutoAnimation on or `IApplication.static+0x1c` off (0x1bb1748..0x1bb1778); else `frame = (frame+1) mod maxFrame` via ADD/SDIV/MSUB 32-bit (0x1bb177c..0x1bb178c) with **no guard on maxFrame** (port must define div-by-zero/sign/overflow policy explicitly). Called by `ecs.AnimationSystem.Update`, `RoomForm.Update`, `SubForm.Update` and other forms.
- `PushThisFrameRender`: false if AutoAnimation off or already registered (0x1bb3840..0x1bb3854); if `IApplication.TotalFrames(+0x58) - this+0xa4 >= 2` (signed32) reset frame to 0 (0x1bb38b0..0x1bb38c4); append to collection, growing if needed; count++, set `+0xa0`, store TotalFrames, return true.
- `ThisFrameRenderAnimation`: if AutoAnimation, for each active Seb with maxFrame >= 1, frame = (frame+1) mod maxFrame (guarded). `ResetThisFrameRender`: if AutoAnimation, clear `+0xa0` of each active Seb and zero the count; collection not cleared. `IApplication.OnGUI` calls ThisFrameRenderAnimation at 0x1b84090 (gated by `IApplication.static+0x1c`), then Reset at 0x1b840b0, before main drawing.
- `Draw` overloads (kairo:0x0600178b/8c/8d) call Push, then read `+0x18` and draw regardless of Push's result.
- `Seb..cctor` creates the collection; no explicit AutoAnimation write; no direct BL to the setter found (28 direct BL/B to the five targets scanned).
- Separate per-entity counter: `ecs.AnimationSystem.Update` (0x1141bc0) first calls `Seb.Frame` on its collection, then per entity adds `animation+0x10` to the component counter (`Entity.get_seb` component `+0x18`), calls `RestoreAnimation` when `animation+0x14 != -1` and counter >= maxFrame, then takes the counter mod the earlier maxFrame. Do not merge with the shared Seb frame.

Sample: `load/copy.seb` (28 bytes, one record) = FrameNo 0, TexId 0, U 0, V 0, W 82, H 20, X -41, Y 0; `load/img.inf` ID 0 = `copy.png` (82x20). Naive "TexId = root img.inf ID, U/V/W/H = direct crop": of 2,376 non-negative-TexId records, 46 lack an ID, 47 lack the literal member, 1,515 crops fit, 768 exceed the PNG (OPT remapping, e.g. `hat_com.seb` 30x30 crops vs 39x15 `hat_000.png`). Direct cropping is not a valid renderer rule.

## Reading it

- `tools/inspect_seb.py` (`parse`/`encode`, stdlib): neutral field names (`values`, `header_value`, `legacy_value`, `control`, `extra_word`, `pair`, `tail_pair`, `flags`); rebuild from scalars only; rejects modes other than 0/1, negative counts, truncation, trailing bytes. `tests/test_seb.py` (6). Inventory SHA-256 `70b8d7c9…61d0a0`.
- `src/assets/seb.cpp` (`parse_seb`): `std::optional` for absent fields (JSON `null`), contiguous groups/records; `SebLimits` 32 MiB / 32,767 groups / 1,000,000 records; throws `SebError`. Test `tests/test_seb.cpp`. `asset_inspect --seb` matched `dataclasses.asdict(inspect_seb.parse())` on all 303 files.
- `src/core/gameplay/animation_pass.cpp` implements the shared manual Seb.Frame clock step and the separate ECS entity loop, documented in [animation](../subsystems/animation.md#animationsystemupdate-per-pass). Effective flags and loaded clip clocks are explicit inputs; this does not derive maxFrame from parsed records or implement automatic drawing clocks. The tests preserve ARM signed remainder for divisor zero, negative divisors and INT_MIN/-1, alongside int32 wrapping increments.

## Evidence

- [13-animation-formats](../archive/reports/13-animation-formats.md), [16-sprite-layout](../archive/reports/16-sprite-layout.md), [29-seb-animation-timing](../archive/reports/29-seb-animation-timing.md), [15-native-assets](../archive/reports/15-native-assets.md)
- Local: `build/research/seb-readers/`, `seb-readshort/`, `seb-input-chain/`, `seb-byte-reader/`, `sprite-layout/`, `sprite-fields/`, `seb-timing/` (+ `direct-references.json`), `native-seb/differential.json`, `research/local/seb-inventory.json`

## Open questions

1. (blocking) Record combination between neighbouring FrameNo values in `GetSpriteLocal` (interpolation?).
2. (blocking) TexId -> Image/OPT resolution and `GetOptimizeSeb`; semantics of negative TexId commands (-8 and others) and mode-0 derived colour/control.
3. Writers of AutoAnimation and `IApplication.static+0x1c`; real tick rate of `Frame`/`OnGUI`.
4. Meaning of `header_value`, `legacy_value`, `extra_word` and unused flag bits.
