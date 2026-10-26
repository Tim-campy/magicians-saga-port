# Random number generation

Gameplay randomness goes through the static wrapper `kairo.unity.math.Random` (Rand/Hits/Hits1000/Choose/Lot), which forwards to a static `java.util.JRandom` instance. JRandom owns three generator objects (System.Random, Random2018, Xorshift128) and picks one per call from a static `Logic` integer. The wrapper arithmetic, default configuration and one replay seed gate are established. A compatible explicit-seed System.Random route is adapted from public .NET Core 3.1 source and tested through enemy drops; effective live selection, reseeding and Android comparison remain open. No alternative generator is silently substituted.

## Overview

```
Random.Hits(p) / Hits1000(p) / Choose / Lot
        -> Random.Rand(bound) / Rand(min,max) / Rand()
            -> JRandom.NextInt()            (static instance created by Random..cctor)
                -> backend.Next()  [virtual slot 5, class+0x188]
                   Logic==2 -> Xorshift128   (this+0x20)
                   Logic==1 -> Random2018    (this+0x18)
                   other    -> System.Random (this+0x10)
```

- `Random..cctor` (0x1128ce4) allocates a 256-element array (meaning unknown) and constructs JRandom with `JSystem.CurrentTimeMillis()` as the seed argument. When this initializer actually runs is not pinned.
- The seeded JRandom constructor (0x1b04ba8) constructs **all three** backends, not only the selected one: System.Random at `this+0x10`, Random2018 at `+0x18`, Xorshift128 at `+0x20`. A byte flag at static offset 0 forces the seed to zero when set; otherwise the low 32 bits of the argument are passed. Metadata names a static candidate `fix_` for this flag; a writer has now been traced in IApplication.Awake (see initialization below). `SetSeed` (0x1b04e50) rebuilds the three objects the same way. The unseeded constructor (0x1b04b88) calls CurrentTimeMillis and delegates.
- Vtable slot 5 (vtable origin 0x138, stride 16, so `class+0x188`) resolves via metadata v31 to: System.Random (type 247) -> `System.Random.Next`; Xorshift128 (type 1728) -> `Xorshift128.Next`; Random2018 (type 1729) -> inherited `System.Random.Next`. Random2018 therefore has no own `Next`; its `Init` (0x1b050c8) alters inherited state through reflection (`Type.GetTypeFromHandle`, `FieldInfo.SetValue`), which rules out swapping in a standard library generator.

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| kairo.unity.math.Random..cctor | game:0x06000a99 | 256-element array + JRandom(CurrentTimeMillis) | medium |
| kairo.unity.math.Random.Rand() | game:0x06000a9a | Forward to NextInt of the static instance | high |
| kairo.unity.math.Random.Rand(bound) | game:0x06000a9b | Conditional negate, signed remainder | high |
| kairo.unity.math.Random.Rand(min,max) | game:0x06000a9c | `min + NextInt % (max-min+1)` (32-bit) | high |
| kairo.unity.math.Random.Hits | game:0x06000a9d | `Rand(100) < p` | high |
| kairo.unity.math.Random.Hits1000 | game:0x06000a9e | `Rand(1000) < p` | high |
| kairo.unity.math.Random.Choose | game:0x06000a9f | Calls Hits; body not read | low |
| kairo.unity.math.Random.Lot | game:0x06000aa1 | Calls Rand; body not read | low |
| java.util.JRandom..ctor() | kairo:0x060000b8 | CurrentTimeMillis then seeded ctor | medium |
| java.util.JRandom..ctor(seed) | kairo:0x060000b9 | Builds all three backends, `fix_` zero-seed flag | high |
| java.util.JRandom.SetSeed | kairo:0x060000bd | Rebuilds all three backends | high |
| java.util.JRandom.get_Logic / set_Logic | kairo:0x060000b6 / kairo:0x060000b7 | Static int at offset +4 | high |
| java.util.JRandom.NextInt() | kairo:0x060000ba | Backend select by Logic, slot-5 dispatch | high |
| java.util.JRandom.NextInt(bound) | kairo:0x060000bb | One NextInt, signed SDIV/MSUB remainder | high |
| java.util.JRandom.NextLong | kairo:0x060000bc | One NextInt, sign-extended to 64 bits | high |
| Xorshift128.Next | kairo:0x060000c2 | Slot-5 override for Logic==2; algorithm not read | low |
| Random2018.Init | kairo:0x060000c5 | Reflection-based state setup | low |

## Behaviour

- `Rand(bound)`: v = NextInt(); if v < 0 then v = -v (32-bit, so INT32_MIN stays negative); return v % bound (signed SDIV/MSUB). Output range 0..bound-1 is therefore not guaranteed for every NextInt value. Do not replace with `std::abs` or a distribution.
- `Rand(min,max)`: n = wrap32(max - min + 1); return min + NextInt() % n, all 32-bit, **no** negation. A negative NextInt would yield a value below `min`; whether the backend can return negatives is unresolved (System.Random.Next's managed contract is non-negative, not verified in this build).
- `Hits(p)`: `Rand(100) < p`, signed LT; `Hits1000(p)`: `Rand(1000) < p`. No clamp; every call consumes one NextInt even when p <= 0 or p >= 100/1000.
- `JRandom.NextInt(bound)` makes one NextInt call and takes the signed remainder; no rejection loop. `NextLong` makes one NextInt call and sign-extends (it does not combine two draws).
- No direct BL/B to `set_Logic` was found in the ELF. This does not prove Logic is 0 (inlining, direct static stores, reflection, indirect calls are not covered).
- Edge cases (overflow, INT32_MIN, divisor 0, inverted ranges) were not executed. A port must express ARM64 semantics (SDIV of INT32_MIN by -1, division by zero = 0 on ARM64) without C++ undefined behaviour, and must not add validation or exceptions.

Known gameplay consumers (call order matters for determinism) are listed in [combat.md](combat.md#rng-consumption-order) and [rewards.md](rewards.md).

## Initialization and public implementation reuse

JRandom has five fields (random_, random2018_, randomXor_, fix_, Logic backing field) and eight methods; the metadata declares no static constructor. Managed field initialization therefore gives fix_=false and Logic=0 before any writes ([C# initialization specification](https://learn.microsoft.com/en-us/dotnet/csharp/language-reference/language-specification/classes#1555-field-initialization)). This establishes the initial state, not an immutable runtime policy. Logic 0 selects System.Random; any value other than 1/2 also selects it. The port reads Logic for each draw rather than caching it.

A bounded ADRP/LDR scan of the unique frozen JRandom type-handle GOT (`0x272fed0`) followed by static-field store candidate analysis found the Logic setter's store at `0x1b04b78` and a fix_ write at `0x1b7ec80`. Candidate generation is not a whole-program proof: register propagation, indirect/reflection writes and computed aliases remain outside its coverage. The two selected store sites were checked in Ghidra.

IApplication.Awake at `0x1b7ebe0..0x1b7ec80` checks Config.UNITY_EDITOR (static +0x30), then ActionReplay.CheckReplayFile (`0x1b42088`). If no replay exists, it queries PlayerPrefs.GetString with key DEVCMD_REPLAY_RECORD and default "off", comparing exactly to "on". If the editor flag and either replay condition hold, it writes fix_=true; the false path does not reset an existing true value. The query short-circuits, and the port preserves this. The live editor/config/replay inputs are still supplied by a host.

Three distinct instances are established: gameplay Random..cctor (`0x1128ce4`) stores its JRandom at Random static +0x00; AppData..cctor creates an unseeded JRandom at `0x11199bc` and stores it at AppData static +0x370; Lib..cctor calls CurrentTimeMillis and the explicit-seed constructor at `0x1122f78`, storing Lib static +0x30. Do not merge their streams. JRandom constructors/reseed truncate the millisecond argument to int32 and force zero when fix_ is true, then construct all three backends; the current port exposes seed conversion and backend selection, not the entire three-backend construction lifecycle.

Public source route: [.NET CoreCLR v3.1.0 Random.cs](https://raw.githubusercontent.com/dotnet/coreclr/v3.1.0/src/System.Private.CoreLib/shared/System/Random.cs), MIT ([retained license](../../third_party/licenses/dotnet-coreclr.txt)). Its _inext/_inextp/_seedArray/t_threadRandom/s_globalRandom fields match this APK's metadata. Bounded checks of the explicit-seed constructor (`0x1e72584`), InternalSample (`0x1e72754`) and Next (`0x1e72804`) corroborate the 56-slot initialization, seed constant, special INT_MIN handling, four mixing passes, indices 0/21, subtractive recurrence and Next forwarding. The adaptation includes only explicit-seed construction/Next; it does not copy platform entropy, locking, thread-local seeding or the unused Random API. This is a compatible algorithm source, not proof of the APK runtime's exact source revision.

Seven public seed vectors (0, 1, -1, 42, 123456789, INT_MIN, INT_MAX) match the C++ implementation and a Windows System.Random reference run. With seed 42, the enemy drop test rejects threshold 500 on the first draw (remainder 710), accepts the next (119), and confirms the third value remains unconsumed. This compares public/static contracts, not the Android executable. Confidence high for the documented code paths and source match; effective gameplay configuration remains unverified.

## Port

`game_random.cpp` ports live backend selection, game Rand(bound)/Rand(min,max)/Hits1000 and JRandom.NextInt(bound) arithmetic. It preserves one draw per call, inclusive range wrapping, negative remainder behavior, INT_MIN negation and ARM64 division-by-zero/remainder behavior without C++ undefined operations. `legacy_system_random.cpp` adapts the public seeded backend; missing selected alternatives throw explicitly rather than silently using it. The replay gate and seed conversion are also tested. `enemy_rewards.cpp` now has a deterministic seeded-backend integration test.

Tests in `test_game_random.cpp` cover public reference vectors, independent instances, gate query order/no-reset behavior, live backend switches, unsupported selections and wrapper edge cases. Full startup/SetSeed, Random2018/Xorshift128 and the effective runtime seed are not implemented/verified. Public runtime source should remain the first reference for those follow-ups.

## Evidence

- Local `research/exports/rng-type-candidates.json`, `rng-static-store-candidates.json`, `rng-initialization/`, `rng-gates/` and `rng-public-match/`; `game-random-build.log`. These are bounded static observations, not runtime traces.

- [17-random-selection.md](../archive/reports/17-random-selection.md)
- [59-combat-hit-threshold.md](../archive/reports/59-combat-hit-threshold.md) (wrapper re-read)
- [65-defeat-and-reward-order.md](../archive/reports/65-defeat-and-reward-order.md) (Hits1000)
- Local: `research/local/random-calls.json` (81 static transfers over 16 methods), `build/research/rng/`, `build/research/rng.tsv`, `build/research/combat-rng/`

## Open questions

1. (blocking) Effective value of `JRandom.Logic` at runtime and all writers of it (direct static stores, reflection).
2. (blocking) Public-source matching for the alternative `Xorshift128.Next` / `Random2018.Init` paths if selected; explicit-seed System.Random.Next is now source-matched/ported. Can alternative backends return negatives?
3. (blocking) Seed: when `Random..cctor` runs, whether `SetSeed` is called later (e.g. per stage/save), and further writers of the `fix_` zero-seed flag (editor/replay activation is now known).
4. Purpose of the 256-element array created by `Random..cctor`.
5. Bodies of `Random.Choose` and `Random.Lot` (only their direct callees are known).
6. Controlled-seed differential test against the Android build.
