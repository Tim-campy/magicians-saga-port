# Combat

Combat is driven by ECS systems: `HumanSystem` (party targeting and attack interval), `EnemySystem` (enemy attack timing, `Damage`, `Defeat`) and the collision/projectile handlers that call `Damage`. The hit test, damage arithmetic up to the HP subtraction, the attribute-affinity tables, the two Easing factors and the defeat trigger are established from static ARM64 analysis. The port composes hit testing, damage arithmetic, parameter subtraction and the post-callback death predicate with an explicit RNG source and host interface. A seeded public System.Random backend is available. Concrete ECS bindings, effects and playable combat remain open.

## Overview

- `BattleSystem.Update` (0x114236c) only iterates its subset's enumerator to completion (with dispose/exception handling); it does not run attacks or damage. Do not attach combat logic to it by name.
- `HumanSystem.SearchAndAttack` (0x1156074): checks `StageSystem.get_isAttackActive`, attacker conditions, finds a target with `EnemySystem.SearchNearestEnemy`, filters it; on success stores the target in a human component, calls a movement helper with three zero floats and `ChangeState(2)`, returns true. Guards return false. Filter semantics and state 2 are unnamed; do not reduce it to a nearest-distance check.
- Damage entry points: `EnemySystem.OnBulletHit` (0x114f4ac) and two branches of `EnemySystem.OnCollide` (0x114f604) call `EnemySystem.Damage(attacker, receiver, attack, attackAttribute)` (0x114fc44, 3924 bytes; argument names from metadata). `Damage` returns a death flag; callers decide whether to call `EnemySystem.Defeat` (0x1150b98).
- `Damage` sends event 15 (`World.Send`, generic instantiation 2565) with receiver and the final damage `D`; `StageSystem.OnDamage` is a confirmed subscriber (see [rewards.md](rewards.md#quest-record)).

Parameter lookup (used throughout `Damage`): for entity E and index k, if E has the human component use `MemberSystem.GetTotalParamValue(k)`; else if it has the enemy component use `EnemyData.GetParamValue(k, EnemyComponent+0x10)`; else 0. Human takes precedence when both are present. `GetTotalParamValue` sums, in order and in 32 bits, `MemberSystem.GetParamValue(k)` + weapon + shield + hat + cloth contributions for k. Nominal names of indices 0/1/2/6/7/8/9 are not yet tied to the parameter catalog (0 is very likely HP).

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| ecs.BattleSystem.Update | game:0x06000dc7 | Enumerator drain only, no combat work | high |
| ecs.HumanSystem.SearchAndAttack | game:0x06000ec5 | Target acquisition, ChangeState(2) | medium |
| ecs.HumanSystem.GetAttackSpeed | game:0x06000eea | `trunc((P6 + 100) / 2)` | high |
| ecs.HumanSystem.GetAttackInterval | game:0x06000ee8 | `max(2, FPS*100 / speed)` | high |
| ecs.MemberSystem.GetTotalParamValue | game:0x06000f3c | Base + 4 equipment contributions | high |
| ecs.EnemySystem.IsAttackTiming | game:0x06000e72 | `enemy+0x1c % max(1, FPS/V) == 0` | high |
| parameter.Parameter.get_value | game:0x060000e8 | `value_ + extraValue_` (32-bit) | high |
| parameter.Parameter.Sub | game:0x060000fd | Subtract with optional clamp, returns excess | high |
| main.AppData.GetAttributesAffinity | game:0x0600025d | Affinity selector 0..3 | high |
| ecs.EnemySystem.Damage | game:0x06000e82 | Hit test, damage, Sub, effects, event 15, death flag | medium |
| ecs.EnemySystem.OnBulletHit | game:0x06000e7e | Projectile hit -> Damage -> Defeat | medium |
| ecs.EnemySystem.OnCollide | game:0x06000e7d | Contact hit, two Damage call sites | medium |
| ecs.EnemySystem.Defeat | game:0x06000e83 | Death sequence and drop selection (see rewards.md) | high |
| ecs.EnemyComponent.Check | game:0x06000d47 | Flag test on +0x14 via BitUtil.Check | medium |

Helpers used but owned elsewhere: `main.AppData.Clamp` (game:0x0600014b, signed clamp), `main.AppData.ClampMin` (game:0x0600014e, signed max), `IntExtension.Clamp` (game:0x06000020), `kairo.unity.graph.Graph.Easing` (kairo:0x06000b58), `kairo.unity.m3d4.util3d.FastMath.Sin` (kairo:0x06000833), `java.lang.JMath.ToRadians` (kairo:0x060001a1).

## Behaviour

### Attack timing

- Human: P = `GetTotalParamValue(6)`; speed = (P + 100) / 2 with 32-bit add and truncation toward zero (ADD/CSEL/ASR, also for negative sums); interval = `ClampMin(SDIV(FPS*100, speed), 2)` with 32-bit multiply. No zero-divisor guard.
- Enemy: V = `Parameter.get_value` of `ParamSet[6]` (value_+0x14 plus extra +0x1c); interval = `ClampMin(SDIV(FPS, V), 1)`; attack moment iff `enemy+0x1c % interval == 0` (SDIV/MSUB) where enemy = `Entity.enemy`. Not the human formula (no +100, /2 or ×100). The name and writers of `enemy+0x1c` (frame counter?) are open; a true result is not proven to always produce an attack.
- `MyConfig.FPS`: static field offset 0x50 (field 24199, type 4527, GOT 0x27312d8). Value and consumption rate not fixed; do not convert intervals to seconds assuming 50/60 FPS.

### Damage pipeline (`EnemySystem.Damage`)

1. **Hit test** (0x114fd10-0x114fe94): `base = attacker human ? 90 : 80`; `adj = Clamp(wrap32(atk.P7 - trunc(rcv.P9 / 2)), -60, 15)`; threshold = base + adj (human 30..105, other 20..95; no extra clamp at 100). `hit = Random.Hits(threshold)`. On miss: `World.CreateEffect`, return false (before any further parameter read).
2. Read receiver `ParamSet[0]` (kept for the Sub). Receiver defence/attribute: human -> P2 via GetTotalParamValue, attribute = weapon field +0x58, bonus B = `MemberSystem.GetAttributeValue`; enemy -> P2 via EnemyData, attribute = data +0x4c, B = 0; other -> P2 = 0, attribute 4, B = 0. Attacker P8 by the usual lookup.
3. **Arithmetic** (0x1150108-0x1150218), all intermediates 32-bit, each /100 is signed truncating (magic 0x51eb851f):
   ```text
   R = Random.Rand(95, 105)
   I = GetAttributesAffinity(attackAttribute, rcvAttribute)
   X = trunc(wrap32(R * attack) / 100) - trunc(rcv.P2 / 4)
   Y = trunc(wrap32(X * wrap32(B + 100)) / 100)
   D = trunc(wrap32(ATTRIBUTE_AFFINITY_RATE[I] * Y) / 100)
   C = Random.Hits(trunc(atk.P8 / 2))            # critical
   if C:                 D = D * 170 / 100
   elif attacker human:  D = D * Easing(10,100,50, attacker.Parameter[1].rate, 0) / 100
   if rcvAttribute == attackAttribute:
                         D = D * (100 - Easing(1,30,98, wrap32(B - 1), 0)) / 100
   floor: human receiver Rand(1,3); enemy receiver Rand(3-I, 6-2*I) if (unsigned)I < 3 else Rand(1,1)
   D = ClampMin(D, floor)                          # "other" receivers skip the floor
   ```
   Crit and the human non-crit factor are exclusive, not cumulative. Do not merge percentages or defer rounding.
4. **Subtraction**: if `MyConfig.DEBUG` (static +0x4) is true and the attacker is not human, `Parameter.Sub` is skipped; the rest of the function still runs. Otherwise `Parameter.Sub(ParamSet[0], D)`.
5. **Effects**: parameters chosen from static tables +0x450/+0x458 with attribute/affinity branches; a numeric effect showing D; enemy receivers may have `ModifyAnimation` removed/recreated; a positional effect; then event 15 (receiver, D) at 0x11507b8 via `World.Send` (GOT 0x2731400).
6. **Diagnostics and result**: after event 15, the code reads `MyConfig.DEBUG` (+0x4) and a second flag (+0x70); only when both are set does it format a seven-element diagnostic payload and call the logging route (0x11507bc-0x1150a54). This presentation path remains in the host boundary. At 0x1150a64: return `Parameter.get_value(ParamSet[0]) < 1` (includes extraValue_). Event-15 callbacks run **before** this test, so they can affect it. `Damage` does not return the damage amount and does not call `Defeat`.

### Parameter.Sub (0x1178dec)

Layout: `value_` +0x14, `maxValue_` +0x18, `extraValue_` +0x1c, `extraMaxValue_` +0x20.
```text
cand = wrap32(value_ - amount); new = cand
if maxValue_ != 0: new = cand < 0 ? 0 : (cand > maxValue_ ? maxValue_ : cand)   # lower test first
value_ = new; return wrap32(cand - new)
```
Max 0 disables the clamp (value may go negative). Overkill returns a negative excess. With a negative maximum the lower test still wins. Extras are untouched. Not `std::clamp`.

### Attribute affinity

`AppData..cctor` (0x1117b30, region 0x1119ef8-0x111a078) builds two statics:
- `+0x440`: 5 rows of 2 ints (pairs, per attacker attribute 0..4; rows 3 and 4 are `[-1,-1]` sentinels).
- `+0x448` `ATTRIBUTE_AFFINITY_RATE`: 4 ints from `RuntimeHelpers.InitializeArray` (field 24324 of PrivateImplementationDetails, default_data offset 187880, SHA-256 a83b4f7b...7f339 matching the field name). Selector->rate: 0->90, 1->45, 2->20, 3->60.

`GetAttributesAffinity(att, rcv)`: row = pairs[att]; `row[0]==rcv` -> 0; `row[1]==rcv` -> 2; else any element == -1 -> 3; else 1. Receiver is compared, never used as an index; attacker <0 or >=5 is an out-of-range fault (no fallback). Resulting rates: attributes 0-2 form a cycle (90 strong / 20 weak / 45 neutral, also 45 vs 3/4); attackers 3 and 4 always 60 except receiver -1, which matches the sentinel first and returns selector 0 (rate 90). These are initial values; later mutation is not excluded. Element names are not assigned.

### Easing factors

`Graph.Easing(start, end, duration, progress, amplitude)` (0x1c5df9c): progress < 0 -> start; progress >= duration -> end; inside: ratio = (float)progress/duration, plus amplitude-scaled sine correction (`FastMath.Sin` -> double, `JMath.ToRadians` [double pi 0x626c28, /180 0x627428], libc `sin` via PLT 0x25fe590; float pi at 0x627830, divisor 314.1592712402344 at 0x6278ac), times (end-start), plus start, truncated to int and limited toward the start/end direction. Separate float32 multiply and add, no FMA. Both combat calls have amplitude 0, so the sine term contributes nothing; across their 148 interior points the result equals `start + progress*(end-start)/duration` (integer). The zero-amplitude numeric path is now shared with EnemyData.GetExp, including descending endpoints, wrapping differences and saturating float-to-int conversion; `wrap32(B-1)` makes INT32_MIN become INT32_MAX (upper limit, factor 70). IEEE-754 default rounding assumed; FPCR not captured.

### RNG consumption order

Per `Damage` call: `Hits(threshold)` [1 draw, always] -> on hit: `Rand(95,105)` -> `Hits(P8/2)` -> floor `Rand(...)` (human or enemy receiver; `Rand(1,1)` still draws). Order must be preserved. Defeat adds draws (see [rewards.md](rewards.md#defeat-sequence)).

### Damage callers

| Caller | Damage site | On true |
|---|---|---|
| OnBulletHit | 0x114f548 | Defeat at 0x114f558, then impact handling continues |
| OnCollide branch 1 | 0x114f948 | FlyAway, animation, effect, `HumanSystem.ChangeState(4)`; no Defeat here |
| OnCollide branch 2 | 0x114fc28 | Defeat at 0x114fc38 |

OnBulletHit requires an Enemy component on the receiver, reads attack/attackAttribute from a projectile component, and passes as attacker the entity referenced at +0x38 of another component (not the projectile). Only direct BL/B callers were searched.

## Port

- `src/core/gameplay/combat_rules.hpp/.cpp` (`saga::core`): `hit_threshold`, `subtract_parameter` (`ParameterSubtraction{value, remainder}`), `AttributeAffinityTables` + `initial_attribute_affinity_tables()` + `attribute_affinity` (tables passed as explicit state; `.at()` rejects bad attackers), `noncritical_human_damage_factor`, `matching_attribute_damage_factor` (volatile float32 steps).
- Test `tests/test_combat_rules.cpp` (CTest `combat_rules_contract`): all interior Easing points, clamps and wrap; threshold bases, clamp edges, truncation of negative halves, INT32 wrap; Sub ordinary/overkill/max 0/negative amount/negative max/wrap; 25-cell affinity matrix, sentinel precedence, out-of-range receiver and attacker. `tools/verify_combat_easing.py` checks the 148 points against a binary32 model. Fixtures come from assembly, not captures.
- `src/core/gameplay/combat_damage.hpp/.cpp`: `apply_combat_damage` composes the numeric/state path with `RandomSource` and `DamageHost`. Receiver P9 is read before attacker kind/P7; a miss draws once and calls only the miss presentation suffix. A hit captures receiver parameter zero, resolves defence then attacker P8, draws variation then critical then the conditional floor. Noncritical human rate is fetched lazily. Each percentage wraps its product before signed division; percentages are never combined. DEBUG is read after the floor. Subtraction changes only the captured base value; the final wrapping base-plus-extra test follows `after_damage`, even if that callback replaces the host's current parameter pointer.
- `DamageHost` keeps parameter resolution, live kind checks, affinity state and presentation explicit. `after_damage` is an adapter boundary for effects, event 15 and diagnostics, not an implemented ECS dispatcher. The captured parameter must survive callbacks. Missing parameter zero faults at subtraction (or the final read if DEBUG skips subtraction), not when captured. Adapter failure propagates without undoing subtraction.
- `tests/test_combat_damage.cpp` (CTest `combat_damage_contract`) covers read/draw order, miss short circuit, critical exclusivity, attribute matching, human/enemy/sentinel floors, DEBUG, extra values, callback-driven death/resurrection, captured identity, overflow, malformed affinity/parameter failures and a seeded backend vector (seed 42: damage 47, four draws). These are static-contract tests, not Android observations.
- Missing: concrete parameter resolution (GetTotalParamValue, EnemyData), attack timing, effective live RNG configuration/seeding, effect selection/animation/diagnostics, event-15 dispatch, collision guards and full Defeat integration. `Damage` remains **documented**, because this port covers its numeric/state composition with explicit services rather than the entire method.

## Evidence

- Numeric composition: local `research/exports/gameplay-combat/M37631_EnemySystem_Damage.asm`, ELF 0x114fc44-0x1150b98 (export addresses have a +0x100000 image base). High confidence: entry 0x114fc74 retains attacker x1 in x20; 0x115025c passes x20 to `Entity.get_parameter` (0x113070c), then indexes 1 at 0x1150284 and calls `Parameter.get_rate` at 0x1150290. Thus the human noncritical rate belongs to the attacker, not the receiver. No dependency reimplementation was needed.
- `Random.Hits` at 0x1128f34 calls Rand(100) unconditionally and uses signed LT; local `research/exports/combat-hit/M36634_Random_Hits.asm`. The port reuses the existing seeded RNG and game wrapper arithmetic.

- `combat_rules.cpp::easing_zero_amplitude` preserves separate binary32 division/multiplication/addition and the wrapping int32 endpoint difference. Tests include descending (100 -> 1, duration 999, progress 555 -> 44 rather than mathematical 45), large positive endpoints, overflowed differences and branch ordering. The sine correction is zero on this finite zero-amplitude path; nonzero amplitude remains unported.

- [58-gameplay-combat-entry.md](../archive/reports/58-gameplay-combat-entry.md)
- [59-combat-hit-threshold.md](../archive/reports/59-combat-hit-threshold.md)
- [60-damage-pipeline-and-subtraction.md](../archive/reports/60-damage-pipeline-and-subtraction.md)
- [61-initial-attribute-affinity.md](../archive/reports/61-initial-attribute-affinity.md)
- [62-damage-result-and-consumers.md](../archive/reports/62-damage-result-and-consumers.md)
- [64-combat-easing-factors.md](../archive/reports/64-combat-easing-factors.md)
- [65-defeat-and-reward-order.md](../archive/reports/65-defeat-and-reward-order.md)
- Local: `build/research/combat-calls.json`, `gameplay-combat/`, `attack-timing-helpers/`, `combat-hit/`, `combat-rng/`, `appdata-constants/`, `affinity-initial-values.json`, `damage-consumers/`, `damage-defeat-callers.json`, `damage-helpers/`, `combat-easing/`, `easing-radians/`

## Open questions

1. (blocking) Names/meaning and concrete bindings of parameter indices 0, 1, 2, 6, 7, 8, 9 and `EnemyComponent+0x10`; the human noncritical rate owner is now established above.
2. (blocking) Effective live RNG selection and seeding (see [rng.md](rng.md)); explicit-seed System.Random damage sequences are now reproducible in contract tests.
3. (blocking) Value of `MyConfig.FPS` and `MyConfig.DEBUG` in release; scheduler cadence consuming the attack interval; writers of `enemy+0x1c`.
4. Effect tables +0x450/+0x458 and the effect/animation side of `Damage`; other event-15 subscribers.
5. Full guards of `OnCollide`/`OnBulletHit` and `SearchAndAttack` filters; state numbers 2 and 4; indirect callers of `Damage`.
6. Contributions inside `GetTotalParamValue` (equipment modifiers) and `MemberSystem.GetAttributeValue`.
7. Whether the affinity/rate statics are modified after `AppData..cctor`.
8. General `Graph.Easing` with non-zero amplitude, needed by drop presentation (amplitude -50/-80).
