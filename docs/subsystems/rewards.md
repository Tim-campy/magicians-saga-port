# Rewards, quest result and town return

Enemy defeat spawns drop entities (EXP orb, treasure); collecting a drop credits a per-stage accumulator (`StageSystem.drop_`); only the boss-cleared path through `StageSystem.ShowQuestResult` turns that accumulator into money, crystals, prisoners, character EXP/levels and item unlocks. Every exit goes through `StageSystem.ReturnTown`, which records the quest, writes member levels/EXP back to `CharaData`, and sends event 31 (`game.QuestResult`) to `TownSystem.OnReturnFromQuest`. The port composes treasure totals, item grants, recruitment, currency adapters, rescued prisoners, quest records and the ordered member EXP/result cluster. Decoded treasure/progression tables feed this pipeline. Full reward presentation, ECS binding and original secure currency storage remain open.

## Overview

```
EnemySystem.Defeat ── CreateDrop(type 0 EXP) / CreateDrop(type 3 treasure, first Hits1000 success)
StageSystem.Update state 1 ── CollectDrop ── UpdateCollectingDrops (31 ticks) ── OnCollectedDrop -> drop_
   boss defeated + reward box picked up -> state 3 -> (key pulse) ShowQuestResult -> state 4 -> ReturnTown(0)
   time out -> OnGameOver(2);  main character dead -> OnGameOver(1)  -> state 5/6 or dialog
   state 5 -> ReturnTown(1); state 6 -> ReturnTown(2); exit dialog -> ReturnTown(3)
ReturnTown(code) ── [code!=0] UpdateQuestRecord(code,0,0) ── CleanUp ── MemberSystem.OnFinish ── transition
   callback <ReturnTown>b__0 ── form switch ── World.Send<QuestResult>(31) ── TownSystem.OnReturnFromQuest
```

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| data.EnemyData.AddDefeatCount | game:0x060010ca | wrap32 add then Clamp(0, INT_MAX) | high |
| data.TreasureData.Load | game:0x06001235 | Twelve-field treasure record reader | high |
| data.EnemyData.GetExp | game:0x060010c7 | Easing(exp_[0], exp_[1], 999, level-1, 0) | high |
| kairo.unity.ecs.World.CreateDrop | game:0x06000bf9 | Drop entity; type 3 forces scale 100 | medium |
| ecs.StageSystem.CollectDrop | game:0x06000fa4 | Start collection / boss reward pickup | high |
| ecs.StageSystem.UpdateCollectingDrops | game:0x06000fa5 | 31-tick fly-to-hero, then credit | high |
| ecs.StageSystem.OnCollectedDrop | game:0x06000fb9 | Accumulator mutation + effects/SE | high |
| ecs.DropComponent.get_isCollecting | game:0x06000d36 | `frame >= 0` | high |
| ecs.StageSystem.Update | game:0x06000f9d | Stage state machine 0..6 | medium |
| ecs.StageSystem.ShowQuestResult | game:0x06000fa0 | Grant totals, EXP/levels, treasures | high |
| resultCharacter_t..ctor | game:0x060007de | Result display state from entity and entry snapshots | high |
| ecs.StageSystem.ReturnTown | game:0x06000fb1 | Record, CleanUp, OnFinish, transition | high |
| <>c__DisplayClass75_0.<ReturnTown>b__0 | game:0x06000fda | Form switch, send event 31 | medium |
| ecs.StageSystem.OnGameOver | game:0x06000fb7 | Retry flag / code -> state 5/6 or dialog | medium |
| ecs.StageSystem.OnHumanDead | game:0x06000fb6 | Main character death -> OnGameOver(1) | high |
| ecs.StageSystem.Init | game:0x06000f9b | Subscribes OnDamage to event 15 | medium |
| ecs.StageSystem.Start | game:0x06000f9e | Zeroes totalGaveDamage_ (among others) | medium |
| ecs.StageSystem.OnDamage | game:0x06000fbb | totalGaveDamage_ += D for enemy receivers | high |
| ecs.StageSystem.UpdateQuestRecord | game:0x06000fb2 | QuestRecord counters and bests | high |
| ecs.StageSystem.GetBestTotalGaveDamage | game:0x06000fc9 | Max of bestTotalGaveDamage over quests | high |
| main.AppData.AddMoney | game:0x060001a1 | Money += int32 via 64-bit add | medium |
| main.AppData.AddCrystal | game:0x060001a7 | Crystal += int32 via 64-bit add | medium |
| data.QuestData.AddRescuedPrisoner | game:0x060011f1 | QuestData+0xa0 += n (32-bit) | high |
| data.CharaData.GetExp | game:0x06001079 | witchExp_[0], distinct from saved exp_ | high |
| data.TableData.GetExp | game:0x0600120b | wrap32(exp_ * rate) / 100, truncation toward zero | high |
| data.CharaData.GetNextExp | game:0x06001078 | Level cost from tableData | high |
| data.CharaData.set_level | game:0x0600106c | Clamp(1,1200) -> level_ +0x40 | high |
| data.CharaData.Recruit | game:0x0600107b | recruit_=1, Find(true) | high |
| data.BaseData.get_isUnfound | game:0x06001021 | `state_ == 0` | high |
| data.BaseData.Find | game:0x06001027 | state_=1, new_=arg | high |
| data.EquipmentData.AddStock | game:0x060010ee | Find if state_!=1; stock = min(wrap32(stock+n), 1) | high |
| data.ClothData.AddStock | game:0x060010aa | Same rule | high |
| data.HatData.AddStock | game:0x06001177 | Same rule | high |
| ecs.MemberSystem.OnFinish | game:0x06000f34 | Copy level/EXP to CharaData, destroy sub-members | high |
| ecs.TownSystem.Init | game:0x06000feb | Subscribes OnReturnFromQuest to event 31 | medium |
| ecs.TownSystem.OnReturnFromQuest | game:0x06000fec | Event 1133 on code 1, high score, return flag | high |
| main.AppData.GetHighScore / SetHighScore | game:0x0600020d / game:0x0600020e | SecureLong slot 8 of AppData+0x190->+0x28 | high |
| form.SelectForm.SetReturnFlag | game:0x0600082d | Static byte SelectForm+0x32 = true | high |

## Behaviour

### Defeat sequence

`EnemySystem.Defeat` (0x1150b98), in order:
1. SpeedComponent vx/vy/vz = 0; `FlyAway(e, -100, 100, -100, -50, 1)`; `ChangeState(e, 4)`.
2. ColorComponent = four ints `[255,0,255,0]` (semantics depend on its consumer; not normalized RGBA).
3. `EnemyData.AddDefeatCount(1)`; `EnemySystem.defeatedEnemiesNum_` (+0x34) += 1 only if `EnemyData.Check(1)` (mask 1 at BaseData +0x1c) is false.
4. `remainingEnemiesNum_` (+0x68) = max(wrap32(prev-1), (~EnemyComponent.Check(1)) & 1) (component flag +0x14 via BitUtil.Check; a different flag object from step 3).
5. exp = `GetExp(EnemyComponent.level +0x10)`; if exp > 0: scale = `Easing(25, 50, 500, exp, -50)`, `CreateDrop(type 0, value exp, pos (x-5, y, z-10), scale)`, DepthComponent.extra = -10.
6. First group of `EnemyData.treasureDrop_` (+0x40): rows of (id, threshold) in order; `Hits1000(threshold)` per row until the first success, which creates `CreateDrop(type 3, id, pos (x, 0, z), 25)` with depth extra -10 and stops. Later rows draw nothing.

No local "already defeated" guard; null/short arrays fault (do not turn into "no reward"). `AddDefeatCount`: INT_MAX + 1 wraps then clamps to 0 (not saturating). `GetExp` uses float32 Easing with a 32-bit end-start difference; do not replace with integer interpolation. `CreateDrop` (0x1135298) jump table at 0x627b94 `[30,0,6,11]` relative to 0x1135320 (×4): type 0 -> 0x1135398 keeps the scale; type 3 -> 0x113534c looks up data by id and **overwrites scale with 100**. It returns the entity; nothing is granted here.

### Drop collection

`DropComponent`: type +0x10, value +0x14, x0 +0x18, z0 +0x1c, frame +0x20. Accumulator `StageSystem.drop_` (+0x40): exp +0x10, money +0x14, prisonersNum +0x18, treasureIds +0x20 (list).

- `CollectDrop` (0x1163738): front = `GetFrontLineHuman`; iterate the drop group (component 34), skip drops with frame >= 0. Treasure (type 3) with `TreasureData.get_isBossReward`: requires front.z <= drop.z and `isBossDefeated_` (+0x85); calls `OnCollectedDrop` directly, sets `isRewardTreasureBoxPickedUp_` (+0x86), plays a jingle. All others: requires front.z <= drop.z + 100; sets frame = 0 and appends to `collectingDrops_` (+0x98). No credit yet.
- `UpdateCollectingDrops` (0x1163c18): target = `MemberSystem.get_hero`; x/z interpolated from x0/z0 with float Easing (duration 30, progress frame, amplitude -80); frame += 1 (32-bit); if frame >= 31 -> `OnCollectedDrop`, remove by index (index adjusted). 31 passes from frame 0; cadence per frame is unverified.
- `OnCollectedDrop` (0x1165988), jump table 0x645c3d `[0,6,53,59]` relative to 0x1165a34: type 0 exp += value; 1 money += value; 2 prisonersNum += 1 (value ignored); 3 append value to treasureIds (order and duplicates kept); others: no mutation. All wrap32. Types 0/1 then create a number effect; every type gets another effect, `Entity.AddGarbage(0)`, `AppData.PlaySe`. No dedup.

### Stage state machine (`StageSystem.Update`, 0x11625e8)

Jump table 0x645c36 relative to 0x1162718 (×4): 0 wait for external condition -> 1; 1 CollectDrop, UpdateCollectingDrops, ExecEvent, stage progression; 2 pause/resume and exit dialog; 3 wait `CheckKeyPulse(0x100000)`, `ShowQuestResult` (0x11628a8), update hero equipment, -> 4; 4 quest method then `ReturnTown(0)`; 5 `ReturnTown(1)`; 6 `ReturnTown(2)`.
- State 1 -> 3 requires both `isBossDefeated_` and `isRewardTreasureBoxPickedUp_` (after a member state pass). Killing the boss alone is not enough.
- While the boss is alive, state 1 decrements `remainingTime_` (floor 0); at 0 -> `OnGameOver(2)`. `OnHumanDead`: if `CharaData.get_isMain` -> `OnGameOver(1)`. So 1 = main character died, 2 = time out.
- `OnGameOver`: if `isRetried_`, code 2 -> state 6, else state 5; before a retry, opens a dialog whose non-retry choice picks 5/6 by stored code. Retry path not ported. Dialog with `+0x240 == 99` and answer `+0xc8 == 0` (prepared in state 2) -> `ReturnTown(3)` (voluntary exit). Button labels unknown.
- Only state 3 reaches `ShowQuestResult` in normal flow. A second direct caller, `QuestForm.Update` (site 0x11db0b0), needs `Config.DEBUG` (static +0x10) plus key mask 0x20 - debug only.

### Quest result grant (`ShowQuestResult`, 0x1164548)

1. Start from drop_ totals; for each treasure id in order (duplicates processed again): TreasureData exp_ +0x34, money_ +0x38, crystal_ +0x3c added (32-bit) to local totals; then weaponId +0x40, charaId +0x44, clothId +0x48, hatId +0x4c in that order, -1 skipped, others bounds-checked indices. For each object: if `get_isUnfound` -> virtual slot 8 (class+0x1b8 = `BaseData.Find`, no override in the four types) with true; equipment/cloth/hat -> `AddStock(1)`; character -> `Recruit()`; then a pair `[id, 1]` is appended to result-display lists (display only).
2. `AppData.AddMoney(totalMoney)` 0x1164c04; `AddCrystal(totalCrystal)` 0x1164c20; `QuestData.AddRescuedPrisoner(drop.prisonersNum)` 0x1164c3c; `UpdateQuestRecord(0, totalExp, totalMoney)` 0x1164c50.
3. EXP: enumerates `MemberSystem.members_` (+0x28; `World.GetSystem<MemberSystem>` resolved). For each member with `ParameterComponent[0].value >= 1`: HumanComponent exp (+0x20) += totalExp (wrap32, full total per member, no split). Level-up loop from the previous level_ (+0x1c): cost = `GetNextExp(idx)`; stop if signed exp < cost (ARM64 SUBS/B.LT uses overflow-aware signed comparison); else exp -= cost, level_ = Clamp(level_+1, 1, 1200), idx++; also bounded by the entry count of `Data.tableData`.
4. `GetNextExp` (0x116d9e4): idx clamped to [1,1200]; `trunc(wrap32(tableData[idx]+0x24 * CharaData+0x9c) / 100)`. Metadata identifies these fields as TableData.exp_ and CharaData.needExpRate_; loaded values are bound by TableExperienceHost (see [data.md](data.md#progression-record-readers)).
- `AddMoney`/`AddCrystal`: distinct slots of the array at AppData+0x198 -> +0x28; read via helper, sign-extended int32 added with a 64-bit ADD, written back via helper. No clamp in the body; helper limits unknown.

### Return to town

- `ReturnTown(code)` (0x1162dc4): code != 0 -> `UpdateQuestRecord(code, 0, 0)`; then `CleanUp`, `MemberSystem.OnFinish`, transition with callback. Codes 1/2/3 never pass through `ShowQuestResult`, so no money/EXP grant on death, time out or exit (in the examined bodies).
- `MemberSystem.OnFinish` (0x115d618): for each of `members_` (+0x28): `CharaData.set_level(HumanComponent.level_)`, `CharaData.exp_` (+0x44) = HumanComponent.exp (plain copy, no clamp, no eligibility check); each of `subMembers_` (+0x30): `Entity.Destroy(true)` (no-op if byte +0x2c set; else manager delete, then mark); clear members_ then subMembers_. No HP/MP restore; no file write.
- Callback `<ReturnTown>b__0` (0x116847c): switches form, `World.Send<game.QuestResult>(31)` (GOT 0x2731b18, instantiation 2563). `QuestResult` struct: battleResult, areaId, stage (unboxed offsets 0/4/8; metadata 16/20/24 include the object header); packed as x2 = code | areaId<<32, x3 = stage.
- `TownSystem.OnReturnFromQuest` (0x1169b04, subscribed in `TownSystem.Init` at 0x116961c): battleResult == 1 -> `AppData.ExecEvent(1133)` (forwarder to 0x110c668; event content unknown). Always: if (int64)`GetBestTotalGaveDamage()` > `GetHighScore()` -> `SetHighScore`. If AppData+0xa0 non-null -> `SelectForm.SetReturnFlag`. areaId/stage unused here.

### Quest record

- `StageSystem.Init` (0x1162298) subscribes `StageSystem.OnDamage` to event 15 (GOT 0x2731a18, AddEventHandler instantiation 2565). `OnDamage` (0x11666f8): entity non-null and `Entity.get_hasEnemy` -> `totalGaveDamage_` (+0x88) += D (wrap32, no clamp, overkill and DEBUG-skipped damage still count). `StageSystem.Start` zeroes it (site 0x116526c).
- `UpdateQuestRecord(w1, exp, money)` (0x1165728) on `QuestData+0xa8` (QuestRecord): always challengeCount += 1 and defeatedCount += EnemySystem+0x34; return if w1 != 0; else bestExp/bestMoney = max, clearCount += 1, rescuedCount += drop_.prisonersNum, bestTotalGaveDamage = max(totalGaveDamage_). Note ReturnTown(0) after a clear does not call it again (code 0), so a clear counts one challenge.
- QuestRecord layout: bestExp +0x10, bestMoney +0x14, challengeCount +0x18, clearCount +0x1c, defeatedCount +0x20, rescuedCount +0x24, bestTotalGaveDamage +0x28.
- `GetBestTotalGaveDamage` (0x116818c): signed max over all `Data.questData` records starting at 0; null elements reach the error helper.

### Item stock and recruitment

`AddStock(n)` for EquipmentData (stock_ +0x2c), ClothData (+0x24), HatData (+0x64): if state_ != 1 -> `Find(true)`; stock = `ClampMax(wrap32(stock + n), 1)` (no lower bound). So stock never exceeds 1, a found item does not get new_ again, negatives and INT_MAX+1 -> INT_MIN pass through. `Recruit` (0x116db84): recruit_ (+0x50) = 1 and `Find(true)` unconditionally (re-marks new_); no gameplay entity created.

### Experience progression cluster

CharaData.GetExp (`0x116db54`) returns index 0 of witchExp_ (+0x20); it does not read saved exp_ (+0x44). A missing/empty array faults. TableData.GetExp (`0x116db30`) and CharaData.GetNextExp (`0x116d9e4`) use the same wrapping int32 product followed by signed division by 100, truncating toward zero. GetNextExp first clamps its level argument to [1,1200], indexes Data.tableData without removing gaps, and multiplies that row's exp_ (+0x24) by CharaData.needExpRate_ (+0x9c). A missing row or out-of-range table faults rather than supplying a cost.

The per-member mutation inside ShowQuestResult (`0x1164d30..0x1164dd0`) runs only for parameter slot 0 value >=1. It grants the entire total EXP to each eligible member using wrap32 addition; it does not divide the reward among members. Its loop cursor starts at the entry level, checks the current table count on every pass, resolves the current character cost, and compares the live experience against that cost as signed int32. Only a successful comparison stores the wrapping subtraction and clamps wrap32(level+1) to [1,1200]. The separate cursor increments even at the level cap. Zero/negative costs are not rejected, and a later data failure does not undo earlier grants or level changes.

Correction to the earlier description: testing the wrapped difference for negativity is not equivalent to ARM64 SUBS/B.LT. For example, INT_MAX experience and cost -1 accept the level-up and store INT_MIN; INT_MIN experience and cost 1 reject it. Tests preserve both edge cases. No claim is made that these extreme values occur during ordinary play.

Evidence: local `research/exports/game-experience/`, bounded ARM64 exports for the three data methods and complete ShowQuestResult. Relocation-aware field offsets resolve the game metadata backing fields; their stored names are not inferred from the arithmetic. Confidence high for these contracts. Limits: ECS member cursor binding, reward UI and Android comparison remain open; decoded level/character records are now available.

### Ordered member grants and result display records

The block at `0x1164c7c..0x1164e84` captures `MemberSystem.members_`, obtains its enumerator and processes entries in that order. It does not sort, deduplicate or filter the member list before reading components. Each entry performs these steps:

1. Fetch HumanComponent through `Entity.get_human` (`0x112f19c`); snapshot level (`+0x1c`) and experience (`+0x20`). Missing entity/component faults.
2. Fetch ParameterComponent and slot 0's value. A value below 1 skips the mutation, but does **not** skip the result record. A failure here occurs before granting EXP to this member.
3. Run the experience kernel with the previously captured starting level as its loop cursor, using the captured human object and its live experience. Character data is resolved from that same human on each cost lookup. The eligibility query is between the snapshot and mutation; the port retains this order even if a supplied host changes state during the query.
4. Compute wrap32(current captured human level - snapshot level). Construct `resultCharacter_t` (`0x11e4a90`, 84 bytes), passing entity, level difference, snapshot EXP and the full total EXP. The constructor fetches `Entity.get_human` **again**, then stores display level as wrap32(newly fetched human level - level difference), display EXP as the snapshot, and levelUpFrame=-1. The second lookup can fail after the grant.
5. Append the record and advance the iterator. An exception stops later members; it does not restore previous grants. Presentation is invoked only after enumeration succeeds.

Unboxed result layout: entity `+0x00` (64-bit reference), upLevel `+0x08`, addExp `+0x0c`, dispLevel `+0x10`, dispExp `+0x14`, levelUpFrame `+0x18` (int32 fields; 32-byte padded value). For an ineligible member, addExp still carries the full total, while upLevel is normally zero. It is display input, not proof that EXP was granted. No member reward splitting occurs.

Evidence: bounded ShowQuestResult instructions in local `research/exports/game-experience/`; constructor in `research/exports/quest-member-results/`; IL2CPP metadata names the six result fields. Confidence high. Enumeration lifetime/version checking remains the supplied ECS cursor's responsibility; no .NET collection implementation was reconstructed.

### Composed reward pipeline

`grant_quest_rewards` follows ShowQuestResult's grant order with live data/storage interfaces. It snapshots drop EXP, money and display prisoner count at `0x116468c..0x1164700`, initializes crystals to zero, then visits every treasure id (including duplicates). Each row adds wrapping int32 totals before granting weapon, character, cloth and hat. Only -1 skips an item; other negative ids still reach the checked lookup. Item stock uses the existing AddStock port; character recruitment sets recruit_=1 and Find(true), including already-found characters. Display pairs [id,1] are appended after each grant. The row remains live for subsequent item fields and display id reads.

After treasure enumeration, money and crystal adapters run in order even for zero/negative totals. The prisoner credit reads the live drop count at `0x1164c24..0x1164c3c`; it does not reuse the earlier display snapshot. Then UpdateQuestRecord runs with result code 0, followed by the member EXP/result cluster. The returned value contains totals and display lists; the UI/area/stage presentation call is not implemented. Earlier inventory/currency/member mutations survive later failures, and no result is returned on failure.

`update_quest_record` also accepts nonzero result codes for future ReturnTown binding. It increments challengeCount before reading defeatedEnemiesNum; adds that count; nonzero result returns here. For a clear it updates signed best EXP/money, increments clearCount, reads/credits the live prisoner count and updates signed best damage. Counters wrap int32. Failures during defeated/prisoner retrieval preserve earlier stores. Evidence: `0x1165728..0x11657c4`, bounded instructions in `research/exports/quest-reward-pipeline/`. Confidence high. Host accessors preserve unresolved storage/world contracts rather than inventing currency limits or rebuilding runtime libraries.

## Port

- `src/core/gameplay/stage_rewards.hpp/.cpp`: `StageRewards{experience, money, prisoners, treasure_ids}` + `record_collected_drop(rewards, type, value)` (only the OnCollectedDrop accumulator switch, no effects/cleanup); `ItemStock{discovery_state, is_new, stock}` + `add_item_stock(item, amount)` (AddStock with its Find hook).
- Test `tests/test_stage_rewards.cpp` (CTest `stage_rewards_contract`): mixed sequence, separate totals, prisoner payload ignored, duplicate/negative ids, unknown types, overflow of the three sums; first grant, duplicate cap, state != 1, negative amount, overflow both ways.
- [experience_progression.cpp](../../src/core/gameplay/experience_progression.cpp) implements the three data accessors and the per-member level-up loop with a live table/cost host. [Tests](../../tests/test_experience_progression.cpp) cover eligibility, full rewards, multiple levels, cap/cursor separation, zero/negative costs, overflow, table gaps and partial mutation on failure.
- [quest_experience.cpp](../../src/core/gameplay/quest_experience.cpp) composes ordered member grants and result records through an explicit ECS cursor/component host. [Tests](../../tests/test_quest_experience.cpp) exercise the decoded-table adapter, full rewards, duplicate order, ineligible display records, snapshot-before-eligibility order, repeated component lookup, wrapping display arithmetic and failure boundaries. The caller must bind the cursor and invoke this cluster after money/crystal/prisoner/quest-record grants.
- [quest_rewards.cpp](../../src/core/gameplay/quest_rewards.cpp) composes the reward pipeline, reads TreasureData, ports recruitment and QuestRecord mutation. [Tests](../../tests/test_quest_rewards.cpp) check exact cross-subsystem order, duplicate grants, stock caps, full member EXP, live versus snapshot prisoners, skipped/invalid ids, overflow, non-clear records and partial effects at failure boundaries. The generic framing/indexing code is shared with the progression readers.
- Missing: Defeat, CreateDrop, collection cadence, ECS/storage binding and presentation for ShowQuestResult, AddMoney/AddCrystal storage helpers, ReturnTown/OnFinish, event 31, high score.

## Evidence

- `src/core/gameplay/progression_tables.cpp` connects decoded character rates and level costs to the per-member kernel; see [data record schema](data.md#progression-record-readers). Real English/Japanese table cost lookups pass locally, with synthetic tests for holes and live record changes.

- [63-damage-event-and-quest-record.md](../archive/reports/63-damage-event-and-quest-record.md)
- [65-defeat-and-reward-order.md](../archive/reports/65-defeat-and-reward-order.md)
- [66-drop-collection-and-stage-rewards.md](../archive/reports/66-drop-collection-and-stage-rewards.md)
- [67-quest-result-reward-grant.md](../archive/reports/67-quest-result-reward-grant.md)
- [68-stage-exits-and-result-guards.md](../archive/reports/68-stage-exits-and-result-guards.md)
- [69-member-finish-and-return-payload.md](../archive/reports/69-member-finish-and-return-payload.md)
- [70-treasure-inventory-grants.md](../archive/reports/70-treasure-inventory-grants.md)
- [71-town-return-handler.md](../archive/reports/71-town-return-handler.md)
- Local (`build/research/`): `defeat-helpers/`, `defeat-field-offsets.json`, `drop-collection/`, `drop-collection-evidence.json`, `quest-result/`, `reward-grant/`, `quest-result-fields.json`, `stage-exits/`, `stage-exit-evidence.json`, `result-callers/`, `member-finish/`, `member-finish-fields.json`, `reward-find/`, `treasure-grants/`, `stock-clamp/`, `treasure-stock-fields.json`, `stage-damage/`, `stage-damage-total/`, `stage-damage-fields.json`, `town-return/`, `town-return-helpers/`, `town-return-registration.json`

## Open questions

1. (blocking) Loaded data for EnemyData exp_/treasureDrop_. TreasureData, CharaData.needExpRate_ and TableData.exp_ are decoded; the reward pipeline is ported with explicit ECS/storage hosts. Complete stage binding remains.
2. (blocking) RNG backend for `Hits1000` drop rolls (see [rng.md](rng.md)).
3. Contracts of the AddMoney/AddCrystal value helpers (SecureLong-like? limits) and save persistence of money, records and high score.
4. `TreasureData.get_isBossReward` semantics; `CreateDrop` types 1/2 and its other jump targets; who creates money/prisoner drops.
5. Retry path of `OnGameOver`; the quest method in state 4; hero equipment update in state 3; `CleanUp` body.
6. Event 1133 content, `SelectForm` return-flag consumers, other event-31/event-15 subscribers.
7. Meaning of EnemyData flag bit 1 and EnemyComponent flag bit 1 (boss/minion?), and guards preventing double Defeat.
8. Scheduler cadence of `StageSystem.Update` (31-tick collection, remainingTime_ units).
