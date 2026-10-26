# Combat

Combat is driven by ECS systems: `HumanSystem` (party targeting and attack interval), `EnemySystem` (enemy attack timing, `Damage`, `Defeat`) and the collision/projectile handlers that call `Damage`. The hit test, damage arithmetic up to the HP subtraction, the attribute-affinity tables, the two Easing factors and the defeat trigger are established from static ARM64 analysis. The port composes hit testing, damage arithmetic, parameter subtraction and the post-callback death predicate with an explicit RNG source and host interface. A seeded public System.Random backend is available. A resolved-record host now connects character/equipment/enemy parameters and event callbacks; ECS entity-to-record lookup and projectile/contact entry orchestration are ported; concrete effects, state services and playable combat remain open.

## Overview

- `BattleSystem.Update` (0x114236c) only iterates its subset's enumerator to completion (with dispose/exception handling); it does not run attacks or damage. Do not attach combat logic to it by name.
- `HumanSystem.SearchAndAttack` (0x1156074): checks `StageSystem.get_isAttackActive`, attacker conditions, finds a target with `EnemySystem.SearchNearestEnemy`, filters it; on success stores the target in a human component, calls a movement helper with three zero floats and `ChangeState(2)`, returns true. Guards return false. Filter semantics and state 2 are unnamed; do not reduce it to a nearest-distance check.
- Damage entry points: `EnemySystem.OnBulletHit` (0x114f4ac) and two branches of `EnemySystem.OnCollide` (0x114f604) call `EnemySystem.Damage(attacker, receiver, attack, attackAttribute)` (0x114fc44, 3924 bytes; argument names from metadata). `Damage` returns a death flag; callers decide whether to call `EnemySystem.Defeat` (0x1150b98).
- `Damage` sends event 15 (`World.Send`, generic instantiation 2565) with receiver and the final damage `D`; `StageSystem.OnDamage` is a confirmed subscriber (see [rewards.md](rewards.md#quest-record)).

Parameter lookup (used throughout `Damage`): for entity E and index k, if E has the human component use `MemberSystem.GetTotalParamValue(k)`; else if it has the enemy component use `EnemyData.GetParamValue(k, EnemyComponent+0x10)`; else 0. Human takes precedence when both are present. `GetTotalParamValue` sums, in order and in 32 bits, `MemberSystem.GetParamValue(k)` + weapon + shield + hat + cloth contributions for k. Nominal names of indices 0/1/2/6/7/8/9 are not yet tied to the parameter catalog (0 is very likely HP).

Equipment getters and attribute bonuses now have portable contracts. Human base stats still go through an explicit provider: `MemberSystem.GetParamValue(Entity,k)` resolves `Entity.get_human` then `HumanComponent.get_data`, uses the character's level at +0x40, and calls the character-record overload. The latter calls `CharaData.GetParamValue(k,level)` and, for `get_isMain`, asks `MagicShopData.GetDataIdForParamKey(k)`; a result other than -1 selects a shop record and adds its `GetAddValue` with int32 wrapping. The numeric growth/shop path is now ported; per-parameter maximum levels, character records and shop records remain explicit data providers.

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
| parameter.Parameter.get_rate | game:0x060000f3 | Wrapped total / total maximum percentage, base-maximum guard | high |
| ecs.MemberSystem.GetAttributeValue(Entity,int) | game:0x06000f41 | Attributes 0..2 select total parameters 10..12; others zero | high |
| data.EquipmentData.GetParamValue | game:0x060010f0 | Weapon/shield endpoint interpolation at stored level | high |
| data.ClothData.GetParamValue | game:0x060010ac | Clothing endpoint interpolation at stored level | high |
| data.HatData.GetParamValue | game:0x06001179 | Hat endpoint interpolation at stored level | high |
| data.EnemyData.GetParamValue | game:0x060010c8 | Endpoint interpolation with per-parameter maximum and amplitude -50 | high |
| parameter.Parameter.Sub | game:0x060000fd | Subtract with optional clamp, returns excess | high |
| main.AppData.GetAttributesAffinity | game:0x0600025d | Affinity selector 0..3 | high |
| ecs.EnemySystem.Damage | game:0x06000e82 | Hit test, damage, Sub, effects, event 15, death flag | medium |
| ecs.EnemySystem.OnBulletHit | game:0x06000e7e | Projectile hit -> Damage -> Defeat | medium |
| ecs.EnemySystem.OnCollide | game:0x06000e7d | Contact hit, two Damage call sites and recoil | high |
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

### Parameter rate, equipment and attribute bonuses

`Parameter.get_rate` (0x1178c84) first returns 0 if the **base** maximum (+0x18) is zero, even when extra maximum is nonzero. Otherwise numerator is `wrap32(wrap32(value + extraValue) * 100)`; denominator is `wrap32(maxValue + extraMaxValue)`. ARM64 signed division precedes clamp to [0,100]; zero denominator yields 0 and INT_MIN/-1 yields INT_MIN (thus clamps to 0). Neither double precision nor a combined rational expression is equivalent for overflowing inputs.

The three equipment getters take an int32 parameter index and return an int32. Each reads a jagged row's first two integers **before** interpolation, faults on missing/short rows or invalid index, ignores trailing row values, and calls zero-amplitude Easing with duration 99 and progress `wrap32(level-1)`. They have no state writes. Data layout, confirmed by metadata field offsets:

| Record | Level int32 | Parameter rows reference |
|---|---|---|
| EquipmentData (weapon/shield) | +0x28 `level_` | +0x88 `status_` |
| HatData | +0x60 `level_` | +0x50 `parameters_` |
| ClothData | +0x20 `level_` | +0x58 `parameters_` |

`MemberSystem.GetTotalParamValue(Entity,k)` calls the base provider, then resolves weapon, shield, hat and cloth separately in that order, querying parameter k on each. Missing equipment faults; there is no zero-contribution fallback. It then sums base and the four contributions with int32 wrapping. The current port preserves provider order and failure short-circuiting. The entity overload of `GetAttributeValue` uses an unsigned comparison: attributes 0,1,2 select total parameters 10,11,12; all other values (including negatives) return 0 without resolving anything. The second overload at 0x1158ed4 uses the same guard with the character-record total provider; that provider is not ported here.

`EnemyData.GetParamValue(k,level)` (0x116ea04) reads row k at +0x68, its first two endpoints, then static per-parameter maximum k at +0x8. It calls Easing with duration `wrap32(maximum-1)`, progress `wrap32(level-1)` and amplitude **-50**, so equipment's linear kernel cannot be reused for enemies. The initial maximum values are established below; the -50 curve is now ported through the shared easing kernel. `CharaData.GetParamValue` instead uses amplitude 0 and a different per-parameter maximum table; indices 0/1 above their maximum also have a second growth term toward level 1200. The extra endpoint is truncation toward zero of `binary64(lastEndpoint) * 0.086` (ELF constant 0x6274a8). Its duration is wrap32(1200-maximum), progress wrap32(level-maximum), start 0 and amplitude 0; the result is added with int32 wrapping. The extra term runs only for indices 0/1 and signed level > maximum. All endpoint reads precede the maximum-table lookup; invalid rows fault even at clamped levels. The 0.086 product is finite and within int32 range for every int32 endpoint.

`MagicShopData.GetDataIdForParamKey` (0x1172564) maps parameter keys 0..4 to ids 0..4 and keys 10..12 to ids 5..7; every other key returns -1. `GetAddValue` (0x11725c4) returns wrap32(upValue_ * buyCount), at offsets +0x30/+0x40, without clamping or writes. `MemberSystem.GetParamValue(CharaData,k,level)` first computes growth, then reads `isMain` (bit 1 of the flags byte at +0x1c). Only main characters and mapped keys resolve a shop record and add its value. Negative counts and overflow remain observable. The overload at 0x115dd68 uses the stored level (+0x40); the explicit-level overload at 0x115dd80 does not replace its argument with the stored level.

### Initial parameter maximum levels

`CharaData.PARAM_MAX_LEVELS` (static +0x18) and `EnemyData.PARAM_MAX_LEVELS` (static +0x8) are separate int32 arrays of length 13, initialized from the same metadata field. Indices 0..3 start at **1000**; indices 4..12 start at **200**. Thus character indices 0/1 use the first curve through level 1000 and the extra-growth curve from 1000 to 1200. There is no hard-coded level 100 cap in character growth: 100 is the equipment endpoint instead.

`initial_parameter_maximum_levels()` returns a fresh value for each owner. `TableCharacterParameterHost` references the caller's maximum array and nullable shop table, so later changes to either are visible. A missing selected shop record throws; nonmain characters and unmapped parameters do not read it. These are initial values, not proof that the original arrays remain immutable. Combat projections of character/shop/equipment tables are now loaded; full mutable-save restoration remains pending.

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

`OnBulletHit(projectile,receiver)` (0x114f4ac) requires receiver nonnull before testing Enemy presence. A non-enemy receiver returns without inspecting projectile. Otherwise projectile must be nonnull, have Projectile slot 27 and Collision slot 32; absent slots return without damage. It then reads Damage slot 33 twice for value +0x10 and attribute +0x14, and Projectile.parent +0x38 for the attacker. DamageComponent.parent +0x18 is not used here. The parent may be null; it is passed through to Damage, whose own parameter-access/failure order applies.

After Damage, true calls Defeat(receiver); either outcome then re-reads Projectile.parent and calls CreateImpactEffect(projectile,currentParent) at 0x114f578. A callback that removed Projectile makes that reread fault. After impact, it conditionally removes Projectile then Collision based on their current presence, and re-resolves Speed slot 1 for each store of +0 to vx/vy/vz. A damage miss still reaches impact/removal/zeroing. The entity itself and Damage component are not destroyed by this routine.

Generated removers capture the old component, call entity removal (including notifications), then push the captured object onto its ComponentFactory pool stack. Projectile uses static +0xe8; Collision uses +0x30. The port keeps return-to-pool explicit and after ComponentSlots.remove, so a late failure does not roll back removal or earlier damage. Complete pool initialization and collision geometry remain outside this handler. Only direct BL/B callers were searched.

### Contact collision orchestration

`EnemySystem.OnCollide` (0x114f604, 1600 bytes) selects the first Enemy entity, otherwise the second. Bullet slot 29 on either input delegates to `OnBulletHit`, preferring the first bullet. Without bullets, Damage slot 33 on either input selects the first such source. That route returns if there is no Enemy or its destroyed byte +0x2c is nonzero; otherwise it passes Damage.parent +0x18, value +0x10 and attribute +0x14 to Damage, calls Defeat on a true result, then continues the common reaction even after Defeat. Without a Damage component, a missing Enemy faults.

The common reaction re-resolves EnemyData, calls CutInAnimation with wrapping int32 `(enemySeb << 2) + 24` and `(enemySeb << 2) | 2`, locks Direction if present and creates effect `(0,2,0)` at `(x,y+10,z+12)`. It then selects the first Human, otherwise the second; no Human or human state 4/5 returns after this effect. EnemyData is resolved again. Data flags bit 0 selects FlyAway(enemy,-100,100,-100,-50,true), enemy state 4 and contact sound, bypassing damage and recoil.

Otherwise enemy P3 at the current EnemyComponent level is evaluated before reading human invincibleTime. Values greater than zero skip Damage but still recoil. Damage receives the enemy as attacker, human as receiver, P3 and the captured EnemyData attribute. A lethal result calls FlyAway(human,-100,100,50,100,false), ChangeAnimation(16,1), effect `(0,24,0)` at `(x,y+10,z)`, then human state 4. It does not call EnemySystem.Defeat for the human.

Recoil queries stage progress at the enemy's current z. Distance is binary32 3 when signed progress is greater than 79 or EnemyComponent flags bit 0 is set. Otherwise distance is 10, raised to 16 only when CharacterData flags bit 2 (isSub) is set and human z is strictly greater than the member hero's z. Equal/unordered comparisons retain 10. The current enemy z is reduced by distance, speed.z becomes negative distance, enemy state becomes 3, and contact sound is played last. Human invincibility and a nonlethal/missed Damage result do not bypass recoil. Data and component flag fields are distinct.

## Port

- `contact_collision.hpp/.cpp` implements the complete OnCollide orchestration over existing component slots, record resolution, enemy parameters and projectile handling. `ContactCollisionHost` leaves concrete Damage/Defeat, FlyAway, animation/effects, state transitions, stage progress, hero lookup and sound explicit. `tests/test_contact_collision.cpp` checks route precedence, callback-driven record changes, lethal ordering, state/invincibility guards, all recoil choices, NaN comparison and partial mutation on late failure. No runtime equivalence is claimed.

- `projectile_hit.hpp/.cpp::on_projectile_hit` ports the complete entry-point ordering over ComponentSlots. ProjectileHitHost supplies damage, defeat, impact and component-pool return; these are explicit game services, not silently omitted effects. `tests/test_projectile_hit.cpp` covers guards, owner reread, removal notifications before pool return, miss/nonlethal cleanup, callbacks removing components and late speed failures. `tests/test_ecs_combat.cpp` connects the handler to EcsDamageHost and verifies lethal damage, stage accounting, defeat invocation, impact and cleanup in one scenario. Defeat animation/spawn behavior, impact visuals and collision detection remain external.

- `ecs_combat.hpp/.cpp::EcsDamageHost` now binds DamageHost directly to `ComponentSlots`, decoded catalogs and explicit saved-level arrays. Each stat query re-reads the relevant component ID and catalog entry; member totals re-resolve each equipment slot. Character saved levels are separate from initial table levels, equipment levels come from the owning catalog's saved-level array, and enemy level comes from EnemyRecordLink. Table/level storage and captured parameter objects must outlive the hit. Presentation remains an explicit receiver-entity interface.
- Parameter lookup now uses `DamageParameterMap`, a standard unordered_map keyed by int32, in both hosts. `ParamSet.get_Item` is a dictionary indexer, **not** a positional vector access. Missing keys fault without insertion; present null values return null until the caller dereferences them. Sparse and negative keys are representable. `CombatParameterLinks` in slot 23 projects the original parameter-component/ParamSet indirection; it does not implement construction or serialization.
- `tests/test_ecs_combat.cpp` (CTest `ecs_combat_contract`) exercises real slot/ID/catalog lookup through damage, effects and stage accounting: equipment ID/level changes between attacks, receiver-record changes after the hit draw, missing keys after hit versus miss, sparse/negative keys, no insertion on failure, and replacement of the dictionary entry after subtraction while the final test still uses the captured object. This is headless ECS integration; projectile/contact entry points, live saves, visuals and a playable update loop remain open.

- `combat_records.hpp/.cpp` resolves decoded character, enemy and equipment records from the existing component slots and component IDs. It preserves null catalog entries and rejects invalid IDs. See [ecs.md](ecs.md#combat-record-links) for slot mapping, failure behavior and setter contracts. Lookup results can feed the explicit saved-level binding functions; EcsDamageHost now performs live resolution throughout damage; entity construction remains pending.

- `combat_tables.hpp/.cpp` loads the static shop/equipment inputs and extends the existing character reader to retain parameters. Explicit saved-level/purchase-count binding functions produce `CharacterParameters`, `CombatEquipment` and `ShopParameterUpgrade` for the resolved host; they preserve shared parameter-array identity. See [data.md](data.md#combat-table-projections) for schemas and real-table validation in both languages. Component ID lookup is now connected by EcsDamageHost; save-state restoration remains external.

- `combat_host.hpp/.cpp` adds `ResolvedDamageHost`, a concrete binding over non-owning `CombatEntity` projections. Human components take precedence for stat lookup; character growth plus shop bonuses and all four equipment records feed totals. Enemy components use their own level, decoded `EnemyRecord`, and separate live maximum array. Human defence resolves P2, weapon attribute and elemental bonus in order; enemy defence resolves P2 then data attribute, bonus zero. Other entities return parameter zero and defence {0,4,0}. Parameter-set slots provide the captured receiver zero and attacker one rate. Missing required rows/components fault rather than silently falling back to another entity kind.
- `CombatPresentation` separates effects, event-15 dispatch and diagnostics. The host invokes them in that order and re-reads DEBUG/+0x70 after listeners before diagnostics. Concrete effects and diagnostic text remain external; the host does not invent them. `record_stage_damage` ports the confirmed StageSystem listener: null receiver faults; enemy-component presence adds the offered amount with wrapping, independently of human precedence. The callback interface allows other listeners; their full set and ordering remain open.
- `tests/test_combat_host.cpp` (CTest `combat_host_contract`) integrates actual parameter functions with damage/subtraction/event accounting. Fixtures cover overkill, DEBUG-skipped subtraction still counting, callback resurrection, live diagnostic gates, miss short-circuiting, dual-component precedence, weapon elemental bonuses, missing equipment, curved enemy growth and counter overflow. These synthetic scenarios establish connected game rules, not a playable ECS loop or original-device comparison.

- `combat_rules.cpp::easing` now handles nonzero amplitude with ordered binary32 operations. It computes ratio, ratio*pi32, times 180, divided by pi32, then widens to binary64 for multiplication by pi64 and division by 180. Host `std::sin` is narrowed to binary32; amplitude/314.159271240234375, sine multiplication, ratio addition, endpoint multiplication and start addition each round separately. End-start wraps in int32 before float conversion. Signed conversion saturates before the directional endpoint clamp. Early progress guards and the existing zero-amplitude path remain unchanged. The finite int32 input domain cannot overflow binary32 to infinity on the interior path.
- `enemy_rewards.cpp::enemy_parameter` now consumes the existing decoded `EnemyRecord.parameters`, reading endpoints before the live 13-entry maximum table, and invokes the shared curve with amplitude -50 and wrapping maximum/level minus one. It faults on absent/short rows or invalid indices instead of substituting a stat. Tests include the recovered 1000/200 limits, below-range/wrapped levels and malformed rows.
- `tools/verify_combat_easing.py` models each float32 step independently with Python struct rounding; `tests/test_combat_easing.py` and C++ compare a 19,264-case fixture (ascending, descending, negative and large endpoints; durations 199/999; amplitudes -80/-50/0/80; endpoints and interior). Hash 17971640406678543181 agrees. This supports the static arithmetic port, not Android libm identity. At endpoints 100/1000, maximum 1000 and level 501, enemy growth is 407, not the linear result 550.

- `src/core/gameplay/combat_rules.hpp/.cpp` (`saga::core`): `hit_threshold`, `subtract_parameter` (`ParameterSubtraction{value, remainder}`), `AttributeAffinityTables` + `initial_attribute_affinity_tables()` + `attribute_affinity` (tables passed as explicit state; `.at()` rejects bad attackers), `noncritical_human_damage_factor`, `matching_attribute_damage_factor` (volatile float32 steps).
- Test `tests/test_combat_rules.cpp` (CTest `combat_rules_contract`): all interior Easing points, clamps and wrap; threshold bases, clamp edges, truncation of negative halves, INT32 wrap; Sub ordinary/overkill/max 0/negative amount/negative max/wrap; 25-cell affinity matrix, sentinel precedence, out-of-range receiver and attacker. `tools/verify_combat_easing.py` checks the 148 points against a binary32 model. Fixtures come from assembly, not captures.
- `src/core/gameplay/combat_damage.hpp/.cpp`: `apply_combat_damage` composes the numeric/state path with `RandomSource` and `DamageHost`. Receiver P9 is read before attacker kind/P7; a miss draws once and calls only the miss presentation suffix. A hit captures receiver parameter zero, resolves defence then attacker P8, draws variation then critical then the conditional floor. Noncritical human rate is fetched lazily. Each percentage wraps its product before signed division; percentages are never combined. DEBUG is read after the floor. Subtraction changes only the captured base value; the final wrapping base-plus-extra test follows `after_damage`, even if that callback replaces the host's current parameter pointer.
- `DamageHost` keeps parameter resolution, live kind checks, affinity state and presentation explicit. `after_damage` is an adapter boundary for effects, event 15 and diagnostics, not an implemented ECS dispatcher. The captured parameter must survive callbacks. Missing parameter zero faults at subtraction (or the final read if DEBUG skips subtraction), not when captured. Adapter failure propagates without undoing subtraction.
- `tests/test_combat_damage.cpp` (CTest `combat_damage_contract`) covers read/draw order, miss short circuit, critical exclusivity, attribute matching, human/enemy/sentinel floors, DEBUG, extra values, callback-driven death/resurrection, captured identity, overflow, malformed affinity/parameter failures and a seeded backend vector (seed 42: damage 47, four draws). These are static-contract tests, not Android observations.
- `src/core/gameplay/combat_parameters.hpp/.cpp` implements the three equipment getters with one shared function, entity total and elemental-bonus aggregation, and `Parameter.get_rate`. `EquipmentParameters` uses the existing nullable jagged int32 representation; it is a projection, not a complete equipment-table loader. `MemberParameterHost` keeps base growth/shop data and live equipment resolution explicit. Tests cover provider order, wrapped totals, malformed rows, level boundaries, extra maxima and ARM division edges; a damage integration fixture uses defence 36, bonus 30 and rate 25 to produce damage 58 from attack 100 at 100% variation. This establishes arithmetic integration, not live ECS/data loading.
- Character growth and main-character shop additions now live in `combat_parameters.cpp` too. `character_parameter`, `member_base_parameter` and `member_base_parameter_at_level` preserve endpoint/max/provider order and the extra growth term. `CharacterParameterHost` supplies live maximum levels and shop records, and `member_base_parameter` can back the existing member-total provider. Tests cover levels 1/100/650/1200, explicit-level override, extra growth restricted to indices 0/1, negative endpoints, truncation before easing, main/nonmain/unmapped shop gates, mapping, wrapped purchases and malformed rows. Static combat table loading is implemented in combat_tables; live ECS binding and saved-state restoration remain pending.
- Missing: concrete parameter resolution (GetTotalParamValue, EnemyData), attack timing, effective live RNG configuration/seeding, effect selection/animation/diagnostics, event-15 dispatch and full Defeat integration. `Damage` remains **documented**, because this port covers its numeric/state composition with explicit services rather than the entire method.

## Evidence

- Contact flow: local `research/exports/damage-consumers/M37626_OnCollide.asm`, ELF 0x114f604. Helper exports in `research/exports/contact-guards/` establish Bullet slot 29, Position slot 0, BaseData.Check bit intersection and CharacterData.isSub bit 2. Metadata identifies Human state +0x24/invincibleTime +0x70 and Enemy flags +0x14. Confidence high for branching, payloads and ordering; host services remain separately scoped.

- Projectile entry: local `research/exports/damage-consumers/M37627_OnBulletHit.asm` (0x114f4ac, 344 bytes); slots/removers in `research/exports/projectile-hit/`. Metadata offsets identify Damage.value/attribute, Projectile.parent and Speed.vx/vy/vz. Pool method usages at GOT 0x2730a48 and 0x27307a0 resolve to public `Stack<T>.Push`; no stack implementation was reverse engineered. Confidence high for guards, reads, calls and write ordering.

- `ParamSet.get_Item` at ELF 0x1179310 loads its dictionary field +0x10, requires it nonnull and forwards key w1 to a generic indexer (local `research/exports/parameter-slot/ParameterItem.asm`). GOT 0x27321a0 resolves usage 0xc001a165, method-spec 53426, method 12526, declaring type 1568 `System.Collections.Generic.Dictionary<TKey,TValue>.get_Item`. The lookup contract is public: [Microsoft Dictionary.Item documentation](https://learn.microsoft.com/en-us/dotnet/api/system.collections.generic.dictionary-2.item?view=net-8.0) specifies failure for missing keys and allows null reference values. Standard C++ keyed storage implements that bounded contract; no dictionary internals or enumeration policy were reconstructed.

- Resolved-host wiring uses the `EnemySystem.Damage` parameter/defence/rate call sites described above. Listener `StageSystem.OnDamage` is exported locally at `research/exports/stage-damage/M37944_OnDamage.asm`, ELF 0x11666f8: null guard branches to the error helper at 0x1166734; enemy predicate at 0x1166714; wrapping add/store at 0x1166720/0x1166724. This corrects the earlier ambiguous description of the null guard: null is a fault, not a no-op.

- General curve: local `research/exports/damage-helpers/M15901_Graph_Easing.asm` at ELF 0x1c5df9c, `combat-easing/Sin.asm` at 0x1c3d290 and `easing-radians/ToRadians.asm` at 0x1b11d60. These establish the exact float widths, operation order and standard sine call; the port uses `<cmath>` rather than reconstructing a public math library. Enemy call at 0x116eadc supplies amplitude -50. Confidence high for arithmetic/order, runtime libm agreement unverified.

- Maximum-level initialization: local `research/exports/growth-limits/CharacterInit.asm` and `EnemyInit.asm` (ELF 0x116e280/0x116eee0). Both allocate 13 int32 entries and initialize them through GOT 0x2731ca0 before storing at the offsets above. That GOT resolves to field reference kind 4, field 24334, metadata default_data +188208 (52 bytes). Its values give the two index ranges above. Metadata field offsets independently identify both `PARAM_MAX_LEVELS` fields. `CharaData.PARAM_IDS` uses a different field (24345, +188544) containing sequential parameter indices; it is not the maximum-level array.

- Character growth: existing `combat-parameters/CharacterParam.asm` (ELF 0x116d810); extension multiplier read directly as binary64 at 0x6274a8. Shop mapping, addition and main-character flag: local `research/exports/character-growth/` exports of 0x1172564, 0x11725c4 and 0x116d590. `MagicShopData` metadata field offsets confirm upValue_ +0x30 and buyCount +0x40. Confidence high for these numeric contracts; no original runtime comparison.

- Local `research/exports/combat-parameters/` contains FDE-bounded exports for the ten parameter/attribute methods above, all with full syntactic coverage; ELF addresses equal exported addresses minus 0x100000. The equipment field names/offsets were checked through `research/il2cpp-metadata-inventory.json` and the ELF field-offset table 0x277ece8. Total-provider ordering comes from `research/exports/attack-timing-helpers/M37817_MemberSystem_GetTotalParamValue.asm`; rate arithmetic from `research/exports/damage-helpers/M34160_Parameter_get_rate.asm`. These are static observations, not runtime verification.

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
5. `SearchAndAttack` filters; state numbers 2 and 4 and their transition implementations; indirect callers of `Damage`. OnBulletHit and OnCollide guards, payloads and call ordering are now ported; FlyAway, stage progress and contact presentation/state services still need concrete bindings.
6. (blocking) Resolve character/shop/equipment records from live ECS entities and restore saved levels/purchases. Static combat table projections, maximum-table initial values and the maximum/shop provider are ported. Character growth and main-character shop-upgrade arithmetic are ported. Equipment interpolation, total ordering and elemental-bonus selection are established above.
7. Whether the affinity/rate statics are modified after `AppData..cctor`.
8. Runtime comparison of host libm versus Android libm for the sine path; the general easing arithmetic and enemy parameter consumer are ported and statically tested. No bitwise runtime equivalence is claimed.
