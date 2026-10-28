# Rewards, quest result and town return

Enemy defeat spawns drop entities (EXP orb, treasure); collecting a drop credits a per-stage accumulator (`StageSystem.drop_`); only the boss-cleared path through `StageSystem.ShowQuestResult` turns that accumulator into money, crystals, prisoners, character EXP/levels and item unlocks. Every exit goes through `StageSystem.ReturnTown`, which records the quest, writes member levels/EXP back to `CharaData`, and sends event 31 (`game.QuestResult`) to `TownSystem.OnReturnFromQuest`. The port composes treasure totals, item grants, recruitment, currency adapters, rescued prisoners, quest records and the ordered member EXP/result cluster. Decoded treasure/progression tables feed this pipeline. Return-to-town progression writeback and deferred event ordering are also ported. Stage cleanup now composes with return/writeback through explicit ECS adapters. Full reward presentation, ECS binding, transition timing and original secure currency storage remain open.

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
| kairo.unity.ecs.World.CreateDrop | game:0x06000bf9 | Drop entity; type 3 forces scale 100 | high |
| ecs.StageSystem.OnBossDefeat | game:0x06000fb5 | Boss state, victory form and movement release | high |
| ecs.StageSystem.ReleaseMoveKeys | game:0x06000fca | Ordered key release and captured-control touch events | high |
| ecs.StageSystem.CollectDrop | game:0x06000fa4 | Start collection / boss reward pickup | high |
| ecs.StageSystem.UpdateCollectingDrops | game:0x06000fa5 | 31-tick fly-to-hero, then credit | high |
| ecs.StageSystem.GetFrontLineHuman | game:0x06000fa9 | First member with the strictly smallest Position.z | high |
| ecs.MemberSystem.get_hero | game:0x06000f2d | Current AppData world -> cached player-entity query | high |
| ecs.MemberSystem.AddMember / RemoveMember | game:0x06000f35 / game:0x06000f36 | Ordered party/sub-party maintenance | high |
| ecs.MemberSystem.InitMembers | game:0x06000f37 | Position, fill parameters, reset combat state and count sorties | high |
| ecs.MemberSystem.SetCharaDataId | game:0x06000f44 | Fill empty slot or clear a non-main occupant | high |
| ecs.MemberSystem.SetCharaDataIdEmptySpace | game:0x06000f45 | First accepting formation slot | high |
| ecs.MemberSystem.SwapFormationIndex / GetFormationNumber | game:0x06000f43 / game:0x06000f46 | Swap slots; find first matching character ID | high |
| parameter.Parameter.Full | game:0x060000fb | Copy nonzero base maximum into base value | high |
| ecs.StageSystem.OnCollectedDrop | game:0x06000fb9 | Accumulator mutation + effects/SE | high |
| ecs.DropComponent.get_isCollecting | game:0x06000d36 | `frame >= 0` | high |
| data.TreasureData.get_isBossReward | game:0x06001236 | Inherited BaseData.flag_ mask 1 | high |
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

`CollectDrop` (0x1163738) captures GetFrontLineHuman before requesting the current World.GetSubSet(And({34})); it enumerates that live group with its existing version checks. A null front entity is tolerated until a pending drop needs its Position; an empty group or collecting-only group does not require it. Every drop requires its component; frame >= 0 skips it before treasure/position work. There is no destroyed/HP guard, X-distance test or circular pickup radius in this method.

For type 3, it resolves TreasureData by value and tests inherited BaseData.flag_ (+0x1c) mask 1, not a guessed property of the image or reward amount. The decoded TreasureRecord.flags is that inherited field. Boss reward pickup requires ordered front.z <= drop.z, then isBossDefeated_ (+0x85). It calls OnCollectedDrop immediately, stores isRewardTreasureBoxPickedUp_ (+0x86) = true, then plays the jingle. It neither queues the drop nor changes its frame; another explicit CollectDrop call before Garbage removes it can credit it again. The method has no local reward-box dedup guard.

Other drops, including non-boss treasures and unknown types, require ordered front.z <= binary32(drop.z + 100). Equality is accepted; NaN in either operand is rejected (FCMP/B.HI). The method stores frame = 0 before appending to collectingDrops_ (+0x98); a missing list or append failure retains that store. No reward is credited on this path yet. Treasure lookup or callback-driven membership changes can leave a processed prefix before the next live cursor advance faults.

`UpdateCollectingDrops` (0x1163c18) first requires MemberSystem.get_hero and captures its Position.x/z for the whole pass, even if the collecting list is empty. It walks that list by live index, re-reading the state field/count after callbacks; it does not use a list enumerator or a snapshot. For each entry it writes Position.x, then Position.z from the existing float Graph.Easing(x0/z0, captured hero coordinate, duration 30, progress binary32(frame), amplitude -80). Position.y, x0/z0, Speed and visual modifiers are unchanged. It increments frame with int32 wrap and tests the new signed value >= 31. Starting at 0, calls 1..30 retain the entry; call 31 reaches the target and calls OnCollectedDrop, then removes that index from the current list and adjusts the index so the next entry is not skipped. Callback replacement of the list redirects removal and subsequent traversal.

`OnCollectedDrop` (0x1165988), jump table 0x645c3d `[0,6,53,59]` relative to 0x1165a34, performs the existing accumulator switch first: type 0 exp += value; 1 money += value; 2 prisonersNum += 1 (value ignored); 3 append value to treasureIds (order/duplicates retained); unknown types do not require or mutate the accumulator. Sums wrap at int32. It then performs these ordered effects:

1. For types 0/1 only: capture the effect world, then current x/y/z; create a full effect at (x,binary32(y+20),z), type 3/value, resource 2/clip 58, depth=false, loop=false, maxFrame=30, frame=0, parent=null, image=-1, anim=false.
2. For every type: capture the world again and re-read position; call the short effect factory with clip 0, image override 25, loop=false (resource 20, depth/playback enabled and maximum resolved from the clip).
3. AddGarbage(0) to the source, using the real pool/write/add helper; then play the collection sound. No source destruction occurs here; the later Garbage pass removes it.

World capture precedes position reads, but its null check follows them. Sound uses the effective AppData static SE field +0x258; boss jingle uses +0x27c, both explicit host actions rather than invented numeric IDs. Failure preserves all earlier credit, frame/position writes, created entities and component additions. In particular, failure after credit but before list removal keeps a frame >=31 entry queued; a successful retry can credit it again. Failure in the jingle leaves the already stored reward-box flag true. These are retained partial states, not rollback transactions. Collection credits StageRewards only; ShowQuestResult still performs permanent grants.

Port: `drop_collection.hpp/.cpp` supplies the three complete methods over DropCollectionState and narrow front/hero/subset/table/effect/audio bindings. Its effects call the actual factories, and its shared float curve is the existing projectile easing kernel exposed as easing_float; no second math or collection implementation was added. The host supplies initialized reward/component storage and list lengths fitting int32. Frame-call cadence and Android libm agreement remain unverified.

`tests/test_drop_collection.cpp` composes decoded-style enemy rewards -> actual EXP/treasure factories and registry/groups -> collection start -> 31 live passes -> stage totals -> actual number/common effects -> Garbage cursor/deferred destruction and registry removal. It also checks boss eligibility/immediate pickup, duplicate pickup before expiry, exact range/NaN gates, null-front empty groups, missing-list frame commit, effect/sound/jingle failure boundaries, retry credit, list replacement, live group invalidation and counter wrap. It does not run the complete StageSystem.Update or grant persistent player inventory.

Evidence: local `research/exports/drop-collection/{M37921_CollectDrop,M37922_UpdateCollectingDrops,M37942_OnCollectedDrop}.asm`, `drop-pickup-helpers/{TreasureBossReward,FloatEasing}.asm`, existing projectile easing evidence and field metadata in [data](data.md#treasure-records). CollectDrop stores frame at 0x1163a44 before list access 0x1163a48; boss credit/flag/jingle occur at 0x11639a0/0x11639a4/0x11639f0. Collection position stores are 0x1163d44/0x1163da0, increment 0x1163db8, credit 0x1163ddc and current-list removal 0x1163df4. Number/common effects are 0x1165b00/0x1165c08, Garbage 0x1165c18 and sound tail 0x1165c64. Boss predicate reads +0x1c and masks 1 at 0x11738dc..0x11738e4. Confidence high for static order; no runtime comparison.

### Front-line member and hero selection

StageSystem.GetFrontLineHuman (game:0x06000fa9, 0x11657c4, 452 bytes) captures MemberSystem.members_ through StageSystem +0x50 and MemberSystem +0x28, then scans that ordered list. The initial best Z is binary32 0x7f7fffff, the largest finite value. It requires every visited entity and Position, selects only when the current Z is strictly less than the best, re-reads the selected Position.z and retains that entity. Ties retain the first member; signed zero ties behave the same way. NaN, positive infinity and a Z equal to FLT_MAX do not become candidates. Negative infinity can win, but does not stop traversal or bypass errors in later entries. Empty/no-candidate lists return null.

This query does not check Human presence, HP, state, destroyed status or character flags. A state-4 member remains eligible while it stays in the list with a Position. Subsequent position changes or roster removal change the next query; the port does not invent death filtering. In the composed collection path, a sole member with NaN Z yields no front-line entity, so a pending pickup faults when CollectDrop requires that entity. A drop with NaN Z and a valid front-line member instead fails the ordered range test and is skipped.

MemberSystem.get_hero (game:0x06000f2d, 0x1156370, 100 bytes) ignores its receiver and reads the current AppData world's playerEntity property; a missing world faults. World.get_playerEntity (game:0x06000be0, 0x1133210, 160 bytes) returns its cached entity at +0x18 when nonnull. On a null cache it requests the real EntityManager.GetSubSet(And({30})), obtains that group's first entity and stores the result. An empty group returns null; a later call retries because the cache is still null. It does not search members_, compare Z, check the main-character flag or inspect HP. Once cached, component/group/registry removal does not automatically reselect the player; World.Destroy's established retention of this field still applies. The host must retain a cached object's lifetime even after its registration/components are removed, until the owning world releases or clears it.

Subset.get_entity (game:0x06000c78, 0x1139434) returns null for an empty set; otherwise its metadata usage resolves to the one-argument System.Linq.Enumerable.First, method 46563/spec 57487, through ELF GOT 0x2730f10. The public [Reference Source First implementation](https://github.com/microsoft/referencesource/blob/ec9fa9ae770d522a5b5f0607898044b7478574a3/System.Core/System/Linq/Enumerable.cs#L918) and [documented contract](https://learn.microsoft.com/en-us/dotnet/api/system.linq.enumerable.first?view=netframework-4.8.1) supply this dependency behavior (MIT; existing notice in third_party/licenses/dotnet-referencesource.txt). The adapter uses the already established ordered subset cursor. The exact Unity runtime revision is not identified; no LINQ generic body is reconstructed or added to the game-method meter. This property neither uses nor fills Subset.entitiesCache_.

Port: `member_selection.hpp/.cpp` supplies the front-line scan, narrow PlayerWorldState cache and current-world hero forwarding. `EntitySubset::first_entity` supplies the subset property over the existing cursor. Selection borrows initialized member storage for a synchronous call; its pure component reads expose no mutation callback, and concurrent list changes are outside this adapter's contract. `tests/test_member_selection.cpp` covers ties, coordinate changes, roster removal, ignored death state, NaN/infinity/FLT_MAX, missing members/positions, empty/retried player lookup, real And(30) population/notifications, stale cache retention and untouched diagnostic entity caches.

The drop-collection integration now registers a real player marker, resolves the front from the party list and resolves the hero through the world/group cache. A second test keeps a state-4 companion in front: its Z permits pickup, removing it prevents the next pickup, and the already collecting drop still flies to the separately cached player. The actual AddMember/RemoveMember operations now maintain this list in the collection test; party creation and complete StageSystem.Update remain open.

Evidence: local `research/exports/member-selection/{FrontLineHuman,Hero,PlayerEntity}.asm`, `first-method.json`, and existing `subset-filters/M37109_Subset_get_entity.asm`. Front list capture is 0x1165828..0x1165834; FLT_MAX initialization 0x1165864; strict selection 0x11658a4/0x11658a8; selected-Z read/store 0x11658bc/0x11658c0. Hero captures the world at 0x11563b8 and tail-calls the property at 0x11563cc. World cache load is 0x113323c, filter slot 30 at 0x1133264, GetSubSet at 0x1133288 and cache store at 0x1133298. Confidence high for these static rules; runtime comparison absent.

### Party roster, formation edits and stage initialization

MemberSystem stores formation_ at +0x20, members_ at +0x28 and subMembers_ at +0x30. Its constructor (0x115c1ec) creates two separate empty lists and a 16-element int32 formation filled with -1. AddMember (0x115d0a4) appends the entity to members_ only if absent, then requires the entity and checks Player slot 30. Without that marker it also appends to subMembers_ only if absent. It does not require Human, inspect character flags, move the entity or initialize its combat state. Null can be appended to members_ before the entity check faults. Re-adding a member after gaining Player does not remove an existing subMembers_ entry; losing Player and re-adding can add one. RemoveMember (0x115d90c, void return confirmed from metadata) removes the first occurrence from members_, then the first from subMembers_; it does not destroy the entity, clear the world's player cache or change formation IDs. A missing second list leaves the first removal committed.

SetCharaDataId (0x115c39c) returns false for a negative/out-of-range slot. For a new ID other than -1 it accepts only an empty (-1) slot, with no character validation or duplicate-ID check. Clearing an occupied slot resolves the old CharaData and refuses if get_isMain is true; otherwise it re-reads formation_ after that lookup before storing -1. Clearing an already empty slot succeeds without a data query. Other negative character IDs are not rejected here. SetCharaDataIdEmptySpace (0x115deec) rejects character -1 and tries SetCharaDataId from index 0 until the first success. GetFormationNumber (0x115df60) returns the first matching index, except ID -1 always returns -1. SwapFormationIndex (0x115de84) ignores negative, equal and out-of-range indices, otherwise swaps the two IDs without invoking the main-character protection.

InitMembers (0x115d200, 1048 bytes) calls GetFormationInitLeftTopX, then GetFormationInitLeftTopZ, then captures members_ and enumerates it in list order. Each member receives the following operations; duplicates repeat the entire sequence:

1. Capture Position; read Human.posOffsetX (+0x28), then Human.posOffsetZ (+0x2c). Set Position.x/z to binary32(wrap32(offset + origin)) and y to positive zero; Position offsets, parent, Cell and formationIndex are unchanged.
2. Capture Parameter[0], compute GetTotalParamValue(entity,0), store its base maximum; repeat for index 1. Re-resolve Parameter[0] and call Full, then re-resolve Parameter[1] and call Full. Full (0x1178dc8, 16 bytes) copies base maximum to base value only when that maximum is nonzero. Negative maxima are copied; extraValue/extraMaxValue are untouched. A total-provider callback can therefore replace the parameter used by Full after an earlier maximum was stored into the captured old object.
3. Write Direction.direction=0; ChangeAnimation(entity,36,true); resolve the current world's HumanSystem and call its ChangeState(entity,0). The existing state-transition rules remain observable, including a second waiting-animation change when no cut-in is active and walking-speed reset when leaving state 1.
4. Capture Human again; clear attackFrame, then opponent; resolve its CharaData and increment sortieCount_ (+0x48) with int32 wrap; finally clear that captured Human's invincibleTime. This is an in-memory sortie counter, not a file save or a new experience grant.

No rollback follows a later failure. A null Parameter[0] is checked after its total computation; failure computing total 1 retains maximum 0 but does not reach either Full. A missing HumanSystem retains the initial animation change. A missing character record retains state/attack/opponent changes and leaves invincibleTime unchanged. Structural changes to the captured member list invalidate the next enumeration step, including the final step; replacing members_ itself does not redirect the captured enumeration.

Port: [member_roster.hpp](../../src/core/gameplay/member_roster.hpp) and its C++ implementation provide formation edits, ordered roster operations and initialize_members, composed with existing parameter lookup, GetTotalParamValue providers, animation changes and human-state transitions. HumanRecordLink now carries the three formation fields. World map/stage origin getters, character-record storage and current HumanSystem resolution are explicit host services. PartyMembers uses std::vector and a mutation version, following the public [List.Remove first-occurrence contract](https://learn.microsoft.com/en-us/dotnet/api/system.collections.generic.list-1.remove?view=netframework-4.8.1) and [enumerator invalidation contract](https://learn.microsoft.com/en-us/dotnet/api/system.collections.generic.list-1.getenumerator?view=netframework-4.8.1). Metadata has no Equals override on Entity/BaseEntity, so roster comparisons use entity identity. No collection implementation is decompiled or added to the game-method count.

Tests: test_member_roster composes actual total-parameter arithmetic, component slots, animation and human-state kernels, then front selection/removal; covers duplicate entries/IDs, Player marker changes, main-character removal protection, callback reference replacement, partial failures, list invalidation, zero/negative maxima and int32 position/sortie wrap. test_drop_collection now uses AddMember/RemoveMember to control pickup eligibility while retaining the separate cached hero target. These do not establish full OnStart, a playable phase or Android agreement.

Evidence: local research/exports/member-roster/ contains bounded exports for the roster/formation methods and InitMembers; member-roster-helpers/ParameterFull.asm and member-roster-parameter/ confirm the parameter/position helpers. ELF field-offset table 0x277ece8 plus metadata identifies Human.posOffsetX/Z/formationIndex (+0x28/+0x2c/+0x74) and CharaData.sortieCount_ (+0x48). AddMember's first-list store precedes the entity check at 0x115d160; Player test is 0x115d16c. Formation protection is 0x115c418..0x115c478. InitMembers captures origins/list at 0x115d284..0x115d2d0, sets position at 0x115d350, stores maxima at 0x115d388/0x115d3c0, fills at 0x115d3e8/0x115d410, changes animation/state at 0x115d438/0x115d470 and increments sorties at 0x115d4a0..0x115d4a8. Confidence high for these static contracts; no Android comparison.

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

- `StageSystem.Init` (0x1162298) subscribes `StageSystem.OnDamage` to event 15 (GOT 0x2731a18, AddEventHandler instantiation 2565). `OnDamage` (0x11666f8): null entity faults; otherwise `Entity.get_hasEnemy` -> `totalGaveDamage_` (+0x88) += D (wrap32, no clamp, overkill and DEBUG-skipped damage still count). `StageSystem.Start` zeroes it (site 0x116526c).
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

### Member finalization and deferred town return

`finish_quest_members` ports OnFinish (`0x115d618`, 756 bytes). For each members_ entry, it fetches the human and resolves its CharaData, fetches the human again for the level, checks the captured CharaData for null, stores Clamp(level,1,1200), then fetches the human a third time for the plain exp_ assignment. The CharaData target stays captured across all three lookups. A failure on the third lookup leaves the level updated and the old EXP intact. There is no eligibility check, new EXP grant or file write here; duplicate members are processed repeatedly in order.

Only after the first enumeration succeeds does it enumerate subMembers_, invoking Entity.Destroy(true). The wrapper at `0x1131aa0` returns for destroyed byte `+0x2c` already set; otherwise it calls EntityManager.DeleteEntity (`0x1137950`) before setting that byte. A manager failure leaves the byte unset. The host retains ownership while the wrapper holds the state reference. After successful destruction enumeration, members_ is cleared before subMembers_. Failed writeback/destruction prevents both clears; failure clearing the second list leaves the first cleared. Cursor lifetime, version checking and the manager's full callback semantics stay with the ECS adapter.

`return_to_town` ports the orchestration at `0x1162dc4`: nonzero result codes update the quest record with EXP/money zero; then CleanUp, OnFinish, and MakeTransition(1, callback, true, 0xff000000). Result 0 does not count the completed quest again. Stage cleanup is bound through the host to cleanup_quest_stage (see below); the return test exercises this composition.

The callback (`0x116847c`, 216 bytes) captures the result code and StageSystem reference. When invoked, it reads the **current** area id (`StageSystem+0x28` -> `+0x18`) and stage (`StageSystem+0x38`), then switches to the town form with false, then sends event 31 carrying result/area/stage. Thus changes before callback execution affect its payload, but changes during the form switch do not. The host must outlive the deferred callback. Failure before transition scheduling leaves no scheduled callback; a form-switch failure prevents the event.

Evidence: existing `research/exports/member-finish/` and `member-finish-helpers/`; bounded ReturnTown/CleanUp/callback exports in `research/exports/quest-return/` (callback interpretation ends at `0x1168554`). Confidence high for the ported sequence. CleanUp's slot list and collection-copy contract are now resolved below.

### Stage cleanup and entity deletion

`StageSystem.CleanUp` (`0x11654b8`, 476 bytes) processes component slots **34, 29, 26, 9, 31**, in that order. The five int32 values come from metadata field-ref 116 (GOT `0x2731ab8`, field definition 24343, default-data index 188496). Per slot it constructs a one-slot And filter, queries the current world subset, copies `Subset.entities_` (+0x30), then destroys that copy. Each group is queried after all deletions in the preceding group; do not snapshot all groups up front.

The copy call at `0x11655e4` resolves through GOT `0x2731340` to method spec 57588 = (46556,-1,1063), `System.Linq.Enumerable.ToArray<Entity>`. Its immediate copy contract is public: [Microsoft ToArray documentation](https://learn.microsoft.com/en-us/dotnet/api/system.linq.enumerable.toarray?view=net-9.0). The port uses an owning std::vector snapshot and does not reimplement LINQ. The host must preserve original HashSet ordering; this work does not claim that a generic unordered_set iteration order matches the game.

**World wrapper matters:** `World.DestroyEntities` (`0x1135794`, 32 bytes) passes its entity collection and excluded entity to the manager and forces the remove-registration argument to true (`mov w3,#1`). The zero passed by CleanUp is the excluded entity, not the removal flag. Thus null snapshot members are skipped, and every non-destroyed entity is removed from the registry. Duplicate/already-destroyed references reach Entity.Destroy's guard and do not repeat deletion.

`EntityManager.DestroyEntity` (`0x1137950`) calls the entity's RemoveAll virtual slot, then DisposeEvents (clears delegates at +0x10/+0x18/+0x20), then, when requested, removes the current entity id (+0x28) from the registry. Component-removal callbacks are still active during RemoveAll and can change subsets; the snapshot prevents those changes from invalidating this pass. The entity wrapper sets destroyed only after the manager returns. A failure can therefore leave components removed/events cleared while destroyed remains false; no rollback is inserted.

After all five groups, CleanUp reads optional StageSystem+0xa0. If present, it calls Destroy(true), then writes null, even if callbacks changed that field. Finally it clears collectingDrops_ (+0x98). If optional destruction fails, neither the null store nor the collection clear runs. If it was already destroyed in a group, the guard skips repeated deletion but the field is still cleared. The optional entity's gameplay name remains unknown.

Evidence: local `research/exports/stage-cleanup-evidence.json`, `quest-return/F11654b8.asm`, `stage-cleanup-wrapper/`, and existing `entity-destruction-load/` / `entity-load-followup/`. Confidence high for slots, arguments and ordering. Limits: concrete world/subset/registry adapters and runtime comparisons remain open.

### Enemy experience and pending reward drops

`enemy_experience` ports EnemyData.GetExp (`0x116e9c0`, 68 bytes): it first requires exp_ (+0x38) and its first two elements, then calls the zero-amplitude Easing path with duration 999 and wrap32(level-1). Therefore even an out-of-range level still faults for a short EXP array. Progress below 0 returns the first endpoint; progress >=999 returns the second. Interior values use binary32 division, a binary32 product with wrap32(end-start), a separately rounded addition, saturating FCVTZS conversion and a directional endpoint clamp. Level INT_MIN wraps its progress to INT_MAX and returns the second endpoint. Integer interpolation is not equivalent for all endpoint/progress combinations.

`spawn_enemy_rewards` ports the reward-selection suffix of EnemySystem.Defeat after its motion/state/counter effects. It resolves the current enemy data for GetExp, creates a pending EXP drop only for a positive value, then resolves enemy data again to check treasureDrop_[0]. If the first group is nonempty, it resolves data once more and captures that group for iteration. Each row must have at least two elements; id and threshold are captured before Hits1000. Every visited row invokes Hits1000, including thresholds <=0 or >=1000; the first success spawns one treasure and stops, without reading later rows. Later outer groups are unused. Missing/short groups or rows fault; an already spawned EXP drop survives a later treasure failure.

The host retains original drop positioning, Easing scale, depth and entity-creation side effects, and supplies Hits1000 from the eventual confirmed RNG backend. The port does not credit the stage accumulator at spawn time. The integration test explicitly collects pending type-0/type-3 drops through record_collected_drop; no immediate or duplicate reward grant is inserted.

Evidence: `research/exports/enemy-rewards/`, `enemy-reward-readers/`, existing `gameplay-combat/M37632_EnemySystem_Defeat.asm` and `defeat-helpers/`. Static artifact audit: 83 enemy records x 1200 levels = 99600 C++ queries per language, matching an independent Python binary32 model (result hash 985714270517449481 for both languages, local `enemy-experience-model.json`). This is static-model agreement, not Android verification. Confidence high for the decoded schema, arithmetic and observed suffix ordering; the full Defeat orchestration is now ported with explicit save-counter and drop-factory providers.

### Boss defeat notification and movement release

StageSystem.OnBossDefeat (0x11663e4, 304 bytes) takes a nullable enemy entity. It first sets isBossDefeated_ (+0x85) true and maxZRate (+0xac) to 100; the metadata identifies this field as maxZRate, not a timer. It calls the current Canvas.KeyUp(0x80000). For a nonnull enemy it captures the current form manager before resolving EnemyComponent.data and requiring the current StageSystem.questData_ (+0x30). It captures EnemyData.id and evaluates QuestData.isCompleted: flags mask 2, or completed byte +0xa4 nonzero. QuestRecord.clearCount is not used. It calls SubForm.MakeVictory(id,completed), then requires the captured manager and Pushes the returned form. The null-manager check occurs after construction; the returned form itself has no extra null guard in this caller. A null enemy skips this entire form branch, including quest access.

Both paths then call ReleaseMoveKeys (0x1163fa0, 464 bytes), which releases keys 0x20000 and 0x80000 in that order. It resolves surface components (group 35,id 0x10011), then (group 35,id 0x10013), capturing both results before any event delivery. For each nonnull result, first then second, it captures the current world, constructs TouchEvent(type 1,component) and sends World event 9. Each canvas/surface/world access resolves the current service independently; a callback replacing a service can affect the next lookup. Changing the second control during the first event does not replace the already-captured second reference. TouchEvent initialization and concrete form/event dispatch remain host services, not a reconstruction of the UI framework.

There is no already-defeated guard, HP check, reward grant, stage-state transition or rollback here. A failure retains the initial stage stores and all earlier side effects; a victory-factory/Push failure prevents movement release. This notification does not by itself finish the quest: the existing reward-box pickup gate still applies.

## Port

- `boss_notification.hpp/.cpp` ports boss-state/victory orchestration and movement-release ordering over narrow presentation interfaces. `test_boss_notification` checks completion flags, null enemy/quest/catalog/manager, captured references, repeated notifications and failure boundaries. `test_enemy_impact` composes the actual three-emitter impact path with this notifier, checking 12 RNG draws, stage writes, victory presentation and the three key releases. The host owns UI objects and must retain captured controls/managers through callbacks.

- `enemy_defeat.hpp/.cpp` now composes the entire EnemySystem.Defeat call order: captured Speed zeroing, existing FlyAway/state transition, Color slot 6 raw int32 pattern, saved defeat counter, system counters and the existing reward suffix. The saved per-record counter is an explicit mutable host reference because catalog decoding does not restore save state. Data flags and component flags are re-read at their respective gates; they must not be collapsed into one boss boolean. Defeated-count increment wraps; remaining-count decrement wraps before applying floor 0/1. AddDefeatCount wraps and clamps at zero (INT_MAX+1 becomes zero).
- The reward adapter captures the initial enemy record before reading level, then uses fresh record lookups for later treasure queries. EXP drops receive `(x-5,y,z-10)` and `Easing(25,50,500,exp,-50)`; treasure drops receive `(x,0,z)` and scale argument 25. Both require returned Depth slot 4 and write extra +0x14 = -10. World.CreateDrop's treasure scale override and entity construction are now ported below; Cell and Image initialization are implemented; resource lookup/rendering remain external. A created drop survives a missing Depth error; counters/state changes survive later malformed reward data.
- `tests/test_enemy_defeat.cpp` covers counter flag combinations, overflow, repeated calls, callback-driven catalog/position changes and failure after movement/drop creation. `test_ecs_combat` now runs actual EcsDamageHost -> OnBulletHit -> Defeat -> FlyAway/state/counters/pending EXP and treasure drops, sharing one RNG stream (four damage draws, two flight draws, one treasure draw). No pending reward is credited as a collected reward by this path.

- `human_impact.cpp::stage_human_dead` resolves the current human record and invokes game-over reason 1 only for isMain. Its event-30 producer and a launch-to-landing integration scenario are ported/tested; see [human landing](combat.md#human-landing-and-death-notification). The retry dialog and OnGameOver state policy remain pending.

- `combat_host.cpp::record_stage_damage` implements `StageSystem.OnDamage`; the resolved combat host sends effects then event 15 then gated diagnostics before the death predicate. `tests/test_combat_host.cpp` verifies overkill/DEBUG accounting, dual human+enemy presence, wrapping and null failure. The listener is available to a real event dispatcher; full registration and other listeners remain pending.

- `src/core/gameplay/stage_rewards.hpp/.cpp`: `StageRewards{experience, money, prisoners, treasure_ids}` + `record_collected_drop(rewards, type, value)` (only the OnCollectedDrop accumulator switch, no effects/cleanup); `ItemStock{discovery_state, is_new, stock}` + `add_item_stock(item, amount)` (AddStock with its Find hook).
- Test `tests/test_stage_rewards.cpp` (CTest `stage_rewards_contract`): mixed sequence, separate totals, prisoner payload ignored, duplicate/negative ids, unknown types, overflow of the three sums; first grant, duplicate cap, state != 1, negative amount, overflow both ways.
- [experience_progression.cpp](../../src/core/gameplay/experience_progression.cpp) implements the three data accessors and the per-member level-up loop with a live table/cost host. [Tests](../../tests/test_experience_progression.cpp) cover eligibility, full rewards, multiple levels, cap/cursor separation, zero/negative costs, overflow, table gaps and partial mutation on failure.
- [quest_experience.cpp](../../src/core/gameplay/quest_experience.cpp) composes ordered member grants and result records through an explicit ECS cursor/component host. [Tests](../../tests/test_quest_experience.cpp) exercise the decoded-table adapter, full rewards, duplicate order, ineligible display records, snapshot-before-eligibility order, repeated component lookup, wrapping display arithmetic and failure boundaries. The caller must bind the cursor and invoke this cluster after money/crystal/prisoner/quest-record grants.
- [quest_rewards.cpp](../../src/core/gameplay/quest_rewards.cpp) composes the reward pipeline, reads TreasureData, ports recruitment and QuestRecord mutation. [Tests](../../tests/test_quest_rewards.cpp) check exact cross-subsystem order, duplicate grants, stock caps, full member EXP, live versus snapshot prisoners, skipped/invalid ids, overflow, non-clear records and partial effects at failure boundaries. The generic framing/indexing code is shared with the progression readers.
- [quest_return.cpp](../../src/core/gameplay/quest_return.cpp) ports OnFinish, the destroyed-flag wrapper, saved-level clamp and ReturnTown scheduling/callback order. [Tests](../../tests/test_quest_return.cpp) cover duplicate/order behavior, repeat human lookups, delayed null checks, clamped levels/unclamped EXP, manager/clear failures, live callback payload and event ordering. The reward test additionally runs grant -> member finalization and confirms EXP is copied without being granted twice.
- [stage_cleanup.cpp](../../src/core/gameplay/stage_cleanup.cpp) ports cleanup and the manager deletion sequence. [Tests](../../tests/test_stage_cleanup.cpp) use the existing ComponentSlots implementation to verify notifications before event disposal, live group changes versus snapshots, registry removal, null/already-destroyed entries and failure boundaries. The ReturnTown test binds this cleanup before member writeback.
- [enemy_rewards.cpp](../../src/core/gameplay/enemy_rewards.cpp) reads EnemyData, computes level EXP and selects pending reward drops through explicit creation/RNG interfaces. [Tests](../../tests/test_enemy_rewards.cpp) cover binary32 differences, null/short data, nested lengths, lookup/draw order, first-success behavior, partial spawn failures and collection into stage totals.
- Missing: image resource/rendering consumers and saved defeat-counter restoration, production StageSystem.Update/party creation/audio binding and collection cadence, ECS/storage binding and presentation for ShowQuestResult, AddMoney/AddCrystal storage helpers, transition timing, event-31 consumers and high score. Collection/credit/effects and later Garbage removal now compose through real drop entities in tests.

### Pending drop entity factory

World.CreateDrop (game:0x06000bf9, 0x1135298, 664 bytes) captures position/type/value/scale by value. Unsigned type >3 (including negative int32 values) returns null before allocation or table lookup. The four jump entries select:

| Type | Resource / clip | Image | Scale | SyncAnimation |
| --- | --- | --- | --- | --- |
| 0 EXP | 7 / 42 | 98 | argument | yes |
| 1 money | 7 / 42 | 99 when value==1, otherwise 105 | argument | yes |
| 2 prisoner | 7 / 43 | -1 | forced 100 | yes |
| 3 treasure | 7 / 41 | TreasureData[value].image (+0x30) | forced 100 | no |

The treasure record is required before creating the entity; invalid or null rows fault without consuming an entity ID. No RNG is used by this factory. Valid paths create a type-0, non-temporary registered entity, then insert Drop slot 34 (type/value, x0/z0 from position, frame=-1), Position 0 (zero offsets/null parent), zero Speed 1, Cell 5 (the same signed cell conversion as effects, null parent/fresh relations), Depth 4 (value 0, extra 10), Seb 2 (resource/clip/frame 0/layers {-1}), Image (image,7,-1,-1,-1,-1), ModifyAnimation 15 and optional SyncAnimation 13. AddImage is called even for image -1. ModifyAnimation is type 10, offsets zero, scaleX/scaleY = binary32(scale)/100, angle/frame/maxFrame 0, anchor 34, destroy=false, loop=true, alpha 255. SyncAnimation uses the shared factory marker, not a per-drop allocation. No Animation playback or Garbage lifetime is added here.

Port: `drop_factory.hpp/.cpp` uses existing ID creation, registration/groups and the shared entity_visual component/pool boundary. Shared EntityDepth gives both factory and EnemySystem.Defeat the same component type, allowing the caller's existing extra=-10 store. [Cell initialization](ecs.md#cell-creation-and-factory-binding) now uses the concrete pooled helper with null parent/fresh children, including live slot-5 group entry; Image now uses the concrete slot-7 helper with six singleton arrays; resource/rendering consumers remain external. The factory creates pending objects, not inventory grants. Tests cover all types, table failure before allocation, invalid-type no-op, Image failure after partial construction, negative scales, live group entry and the actual spawn_enemy_rewards -> first-success treasure -> drop factory sequence. RNG order remains in the existing reward suffix; the composed test confirms a single successful draw and two pending entities.

Evidence: existing local `research/exports/defeat-helpers/M36982_World_CreateDrop.asm`; table bytes [30,0,6,11] at ELF 0x627b94 relative to 0x1135320 (scaled by 4); generated helper exports in `research/exports/drop-factory/`. AddDrop stores its payload at 0x112de58 and inserts slot 34 at 0x112de84; AddSyncAnimation loads factory static +0x100 and inserts slot 13 at 0x1131678. Factory creation is 0x11353a0, Image call 0x1135498, modification 0x11354e4 and SyncAnimation 0x11354f8. Confidence high for factory branches/payloads; runtime comparison absent.

## Evidence

- Boss notification: local `research/exports/enemy-impact-helpers/BossDefeat.asm`, `boss-notification/ReleaseMoveKeys.asm`, `QuestCompleted.asm` and `TouchEventCtor.asm`. Metadata field offsets identify StageSystem.questData_ +0x30, isBossDefeated_ +0x85 and maxZRate +0xac. TouchEvent ctor (0x1c4fbbc) runs its initializer before storing type +0x10 and component +0x40; the initializer is deliberately left to the UI adapter. Confidence high for gameplay ordering and payload arguments; runtime comparison absent.

- Defeat composition: existing `research/exports/gameplay-combat/M37632_EnemySystem_Defeat.asm` (0x1150b98) and `defeat-helpers/M38215_EnemyData_AddDefeatCount.asm` (0x116eaf4); local `defeat-components/Color.asm` and `Depth.asm` confirm slots 6 and 4. Stores are Color +0x10..+0x1c and Depth.extra +0x14. The complete caller order, two independent flag gates and re-read points are statically established with high confidence; Android comparison remains absent.

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

1. (blocking) Complete production StageSystem.Update, MemberSystem.OnStart/character creation and saved-state bindings. Roster add/remove, formation editing and InitMembers are ported with explicit world-origin/character-storage adapters. Front/hero selection, enemy reward spawning, real drop/effect creation, 31-pass collection and Garbage removal now compose in tests; the full stage and permanent reward pipeline are not connected end to end.
2. (blocking) Effective live RNG configuration/seed for Hits1000 drop rolls (see [rng.md](rng.md)); an explicit-seed, source-matched System.Random route and drop integration are now tested.
3. Contracts of the AddMoney/AddCrystal value helpers (SecureLong-like? limits) and save persistence of money, records and high score.
4. Producers of money/prisoner drops; all CreateDrop jump targets and the inherited boss-reward mask are now ported.
5. Retry path of `OnGameOver`; the quest method in state 4; hero equipment update in state 3; concrete world/subset bindings for the ported cleanup.
6. Event 1133 content, `SelectForm` return-flag consumers, other event-31/event-15 subscribers.
7. Meaning of EnemyData flag bit 1 and EnemyComponent flag bit 1 (boss/minion?), and guards preventing double Defeat.
8. Scheduler cadence of `StageSystem.Update` (31-tick collection, remainingTime_ units).
