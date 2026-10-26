# Random number generation

Gameplay randomness goes through the static wrapper `kairo.unity.math.Random` (Rand/Hits/Hits1000/Choose/Lot), which forwards to a static `java.util.JRandom` instance. JRandom owns three generator objects (System.Random, Random2018, Xorshift128) and picks one per call from a static `Logic` integer. The wrapper arithmetic is established from assembly; the selected backend, its algorithm and the seed are not. No generator is ported, and none may be substituted while claiming deterministic fidelity.

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
- The seeded JRandom constructor (0x1b04ba8) constructs **all three** backends, not only the selected one: System.Random at `this+0x10`, Random2018 at `+0x18`, Xorshift128 at `+0x20`. A byte flag at static offset 0 forces the seed to zero when set; otherwise the low 32 bits of the argument are passed. Metadata names a static candidate `fix_` for this flag; nothing writing it has been traced. `SetSeed` (0x1b04e50) rebuilds the three objects the same way. The unseeded constructor (0x1b04b88) calls CurrentTimeMillis and delegates.
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

## Port

None. `src/core/gameplay/combat_rules.*` deliberately takes pre-resolved values and never draws random numbers.

## Evidence

- [17-random-selection.md](../archive/reports/17-random-selection.md)
- [59-combat-hit-threshold.md](../archive/reports/59-combat-hit-threshold.md) (wrapper re-read)
- [65-defeat-and-reward-order.md](../archive/reports/65-defeat-and-reward-order.md) (Hits1000)
- Local: `research/local/random-calls.json` (81 static transfers over 16 methods), `build/research/rng/`, `build/research/rng.tsv`, `build/research/combat-rng/`

## Open questions

1. (blocking) Effective value of `JRandom.Logic` at runtime and all writers of it (direct static stores, reflection).
2. (blocking) Algorithms of `System.Random.Next` (IL2CPP mscorlib build), `Xorshift128.Next` and `Random2018.Init` state setup; can any backend return negative values?
3. (blocking) Seed: when `Random..cctor` runs, whether `SetSeed` is called later (e.g. per stage/save), and writers of the `fix_` zero-seed flag.
4. Purpose of the 256-element array created by `Random..cctor`.
5. Bodies of `Random.Choose` and `Random.Lot` (only their direct callees are known).
6. Controlled-seed differential test against the Android build.
