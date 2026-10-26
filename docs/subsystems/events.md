# Events

The gameplay event/script system is statically recovered from the event table (`Event.txt` → `Data.eventData`) through condition checks, the matrix script executor, the `DelayEvent` continuation queue and the unit dispatcher, down to the numeric effects of the eight opcodes present in the extracted table (money, crystals, area/quest discovery, stamina, talk/event form requests, delay). The port implements the executor, counted continuation queue, argument substitution, the numeric effects and the stamina formula as isolated, tested C++ kernels, plus a native trace tool (`event_trace`) that replays one injected script from the local table. Condition evaluation now includes the record and matrix guards, the comparator and all 14 unit predicates used by Event.txt. The C++ table reader now builds an id-indexed catalog used by the trace tool and delay lookup. Indexed execution and automatic selection now compose the condition and execution kernels. Not ported: remaining condition predicates, the concrete application/frame-loop bindings, the hour-based delay path, `SubForm.delayEvent_` resumption and the forms themselves. Nothing has been compared with the running original.

## Overview

Pipeline (ELF VAs, load bias 0):

1. **Table.** `Data.Load` (`0x1170974`) loads resource name `Event.txt` into `Data+0x38` via `LoadData<EventData>`; records are tokenized by `EventData.Load` (`0x116fa4c`) — see [../formats/event-table.md](../formats/event-table.md) — and placed in an array indexed **by record id** (size `maxId+1`, gaps null; see [../formats/game-tables.md](../formats/game-tables.md)). `Data.get_eventData` (`0x116ff9c`) returns `Data+0x38`. `EventData` fields: `id_`+0x18, `execNum_`+0x20, `name_`+0x28, `timing_`+0x30, `termsScript_`+0x38, `execsScript_`+0x40 (scripts are `int[][]`).
2. **Selection.** `AppData.ExecEvent` (`0x110c580`, no index) scans `eventData` ascending; for each non-null record whose conditions pass (`AppData.CheckTerm`, `0x110b3e0` → `0x110b534` → `0x110b5f8`) it calls the indexed `AppData.ExecEvent` (`0x110c668`). Caller: `SelectForm.Update`, call site `0x11ec7c8`.
3. **Indexed execution.** `ExecEvent(index, …)` (`0x110c668`) bounds-checks, increments `execNum_`, then runs `execsScript_` through the matrix executor `AppData.ExecuteScript` (`0x110c8ec`). Direct callers (e.g. `TownSystem.OnReturnFromQuest` with event 1133, via the forwarder `0x110c65c`) bypass conditions.
4. **Matrix executor.** `ExecuteScript` (`0x110c8ec`) walks rows from a start line; each row is sent to the unit dispatcher `AppData.ExecuteScript` (`0x110cc4c`) except opcode 10, which builds a `DelayEvent`, appends it to the queue `AppData+0x200` and stops.
5. **Continuation queue.** A pass over `AppData+0x200` inside `SelectForm.Update` (`0x11ec578`–`0x11ec6d8`) counts down `wait_` and resumes due entries via `GetDelayScript` + `ExecuteScript`. A second resumption path exists in `SubForm.Finish` through field `SubForm.delayEvent_` (+0x1d0), whose producer is unknown.
6. **Unit dispatcher.** `ExecuteScript` (`0x110cc4c`) clones the row, substitutes argument sentinels, and switches on opcode into money/crystal, area/quest discovery, talk/event form creation + `AppData.Push`, and stamina effects.

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `main.AppData.ExecEvent` (`0x110c580`) | `game:0x060001c8` | Automatic selection scan over `eventData` | high |
| `main.AppData.ExecEvent` (`0x110c65c`) | `game:0x060001c9` | Forwarder to `0x110c668` with null extra args | high |
| `main.AppData.ExecEvent` (`0x110c668`) | `game:0x060001cb` | Indexed execution, `execNum_` increment | high |
| `main.AppData.ExecuteScript` (`0x110c8ec`) | `game:0x060001cc` | Matrix executor; opcode 10 scheduling | high |
| `main.AppData.ExecuteScript` (`0x110d3b0`) | `game:0x060001cd` | 8-byte forwarder to the unit dispatcher | high |
| `main.AppData.ExecuteScript` (`0x110cc4c`) | `game:0x060001ce` | Unit dispatcher (argument substitution + opcode switch) | high (opcodes 1/2/4/8/12/15/31), unknown (others) |
| `main.AppData.GetDelayId` (`0x110caa0`) | `game:0x060001d3` | Script → event index by reference identity | high |
| `main.AppData.GetDelayScript` (`0x110d6dc`) | `game:0x060001d2` | Delay id → `execsScript_` | high |
| `main.AppData.Push` (`0x110d3b8`) | `game:0x060001d1` | Push created form onto current form | high |
| `main.AppData.CheckTerm` (`0x110b3e0`) | `game:0x060001c0` | Per-record eligibility guard | high |
| `main.AppData.CheckTerm` (`0x110b534`) | `game:0x060001c1` | AND-aggregation of a condition matrix | high |
| `main.AppData.CheckTerm` (`0x110b5f8`) | `game:0x060001c3` | Unit condition evaluator (32-entry jump table) | high (14 table-used predicates), unknown (other branches) |
| `main.AppData.Compare` (`0x110c1d4`) | `game:0x060001c2` | Signed 64-bit comparator, 6 codes | high |
| `main.AppData.AddMoney` (`0x110946c`) | `game:0x060001a1` | Money credit | high |
| `main.AppData.SubMoney` (`0x11096e0`) | `game:0x060001a3` | Money debit, floor 0 | high |
| `main.AppData.AddCrystal` (`0x11099f0`) | `game:0x060001a7` | Crystal credit | high |
| `main.AppData.SubCrystal` (`0x1109ac4`) | `game:0x060001a8` | Crystal debit, `SYSSAVEI_YUGAMI` scaling, floor 0 | high |
| `main.AppData.GetStaminaMax` (`0x110d3fc`) | `game:0x06000224` | 10 or 20 from `ITEM_STAMINA_UP` state | high |
| `data.QuestData.Find` (`0x1172f0c`) | `game:0x060011f0` | Quest discovery override (opcode 15) | high |
| `data.EventData.Load` (`0x116fa4c`) | `game:0x0600112d` | Record tokenization | high |
| `data.EventData.get_isExecuted` (`0x116f9f8`) | `game:0x06001123` | `execNum_ > 0` | high |
| `data.Data.get_eventData` (`0x116ff9c`) | `game:0x06001142` | Returns `Data+0x38` | high |
| `game.DelayEvent..ctor` (`0x11c9508`) | `game:0x0600062e` | Continuation record constructor | high |
| `system.form.TapjoyForm.AcquireBalance` (`0x11b72a4`) | `game:0x0600053a` | Early-return guard before the queue pass in `SelectForm.Update` | medium |

Related, documented here but owned elsewhere: `form.SelectForm.Update` `game:0x06000800` (ui), `form.SubForm.Finish` `game:0x060008cb` (ui), `form.SubForm.MakeTalkForm` `game:0x060008d2` / `MakeEvent` `game:0x060008d5` (text/presentation), `data.BaseData.get_isFound` `game:0x06001022` / `BaseData.Find` `game:0x06001027` (data), `StringArrayStream.*` (data/game tables), `system.billing.ui.BillingItem.GetState` `game:0x0600058d` (services), `DelayEvent.Serialize/Deserialize` (save).

## Behaviour

### Indexed execution — `ExecEvent` `0x110c668`
- `index < 0` or `index >= eventData.Length` → returns null; `execNum_` unchanged. Index is the array slot (= record id after id-indexing), not a search for `id_`.
- Valid index: obtains default args if needed, then `execNum_` (+0x20) += 1 (wrap32) **before** checking `execsScript_`. Null or empty `execsScript_` → returns null (optional `Config` diagnostic); the counter has still changed. `termsScript_` is not checked here.
- Default args path (`0x110c720`?`0x110c750`): non-null incoming args bypasses lookup. Otherwise null AppData.sysSave (+0x198) yields null args; a present save requires its array at +0x30 and element 0, converted by `SecureString.op_Implicit` (`0x1c09b58`). A null SecureString slot converts to null; a non-null object dispatches its virtual conversion at class+0x168. Missing array/slot faults before the counter changes. Integer args are passed through unchanged.
- The catalog is read for the initial length guard and again after default-args preparation. Three subsequent reads capture the script record (`0x110c768`), counter destination (`0x110c788`) and counter source (`0x110c7ac`). The source value is incremented in int32 and stored to the destination before inspecting the saved script record. Null source/destination faults; a null script record reaches the empty-script diagnostic after the counter update. The port preserves these accesses through its catalog provider rather than taking an eligibility snapshot.
- Empty-script diagnostics are gated by Config.PRINT (static byte +0x17) (`0x110c850`?`0x110c8bc`), delegated to the execution host. Failure in script dispatch does not roll back the counter. The executor starts at line 0 (`w5=0` at `0x110c828`; `w2=1` is unused by the recovered matrix body).
- Event 1133 (from `TownSystem.OnReturnFromQuest`, `0x1169b04`, when `battleResult == 1`): the extracted table has no id 1133 and the id-indexed array model has length 229, so the guard would return null without effect. Conditional on the loader chain being the one modelled; not classified as penalty/message/no-op.

### Automatic selection — `ExecEvent` `0x110c580`
- Ascending scan; re-reads `eventData` and its length every iteration (no eligibility snapshot).
- Skips null records; `CheckTerm(index)` true → `ExecEvent(index, null, null)`. Returns early **only** if that call returns non-null. Null-returning executions (which may have changed money, `execNum_`, the queue) do not stop the scan — not "run the first eligible event".
- In `SelectForm.Update`, the call is gated by static byte `SelectForm+0x31`; non-null result jumps to `0x11ecb30`, null result clears the byte at `0x11ec7e8`. Flag is static `evenCheck_`, set during Init, by SelectForm.Push, and periodically in reached state-0 updates; see scheduling below. It is not an unconditional per-frame call.

### Conditions
- `CheckTerm(index)` `0x110b3e0`: `Lib.CheckLength` (shared body `0x12d3b4c`) → false for null array, negative index, empty array, index ≥ length. Then false for null record or `isExecuted`. `get_isExecuted` (`0x116f9f8`) is `execNum_ > 0` (negative counters pass). Then evaluates `termsScript_`.
- `CheckTerm(matrix)` `0x110b534`: `IListExtension.IsNullOrEmpty` (shared `0x12cd9b8`) true → **false** (empty conditions are *not* vacuously true). Otherwise evaluates rows in order, stops at first false; true only if all pass. Null rows are passed to the unit evaluator, not skipped.
- `Compare` `0x110c1d4`: signed 64-bit operands; jump table at `0x627b68` = `[0,5,8,11,14,17]` relative to `0x110c1f8`, scale 4. Codes: 0 `>`, 1 `>=`, 2 `<=`, 3 `<`, 4 `==`, 5 `!=`; any other (including negative) → false. Some evaluator branches compare inline instead.
- Unit evaluator `0x110b5f8`: jump table at `0x627ae0`, 32 uint16 entries; targets in [../formats/event-table.md](../formats/event-table.md). Its common prologue requires the AppData singleton and non-null, nonempty `characters_` storage at +0x1b0 before reading the opcode; it does not read element 0. Null/empty condition rows fault after this context check.

Recovered predicates (confidence high, addresses are ELF VAs):

| Opcode / row | Contract | Basis |
| --- | --- | --- |
| 15, quest id | True if quest flags byte +0x1c has bit 1 set, otherwise boolean +0xa4; invalid id or null quest faults. | Handler `0x110bad4`, quest getter call `0x110baf0`, tail call `0x110bb3c` to `QuestData.get_isCompleted` `0x116c3b4` (`game:0x060011ec`). |
| 16, threshold, comparison | Sum `QuestRecord.challengeCount` (+0x18) through each non-null quest's record pointer (+0xa8); null record faults. Sum wraps int32 before signed comparison with the threshold. | Handler `0x110bb40` calls `SumAllQuestsChallengeCount` `0x110c284` (`game:0x060001c4`); comparison table `0x627b20`. |
| 4, area id | True exactly when the area's int32 state (+0x10) equals 1; invalid id or null area faults. | Handler `0x110b8c4`, getter `0x117012c`, tail call `0x110b92c` to `BaseData.get_isFound` `0x116c3a4`. |
| 22, threshold, comparison | Count non-null characters whose int32 sub (+0x50) is nonzero and flags byte (+0x1c) has bit 2 set; signed int32 comparison. | Handler `0x110bf48`; predicate `0x117c598` (`game:0x06000265`) calls `CharaData.get_isSub` `0x116d59c` then `get_isRecruit` `0x116abdc`; comparison table `0x627b3e`. |
| 20, building type | Count all building rows whose element 1 equals condition element 1; true if count > 0. Null or short building rows fault, including after an earlier match. | Handler `0x110bd28` calls `TownSystem.GetBuildingList` `0x1169c44`, then LINQ Where/Count with predicate `0x117c60c` (`game:0x06000269`); count test `0x110bdf8`. |

Additional predicates recovered from the same dispatcher (confidence high):

| Opcode | Value compared to row[1] with code row[2] | Basis |
| --- | --- | --- |
| 1 / 2 | Signed int64 money / crystals from AppData+0x198, SecureLong array +0x28, slots 0 / 1. Threshold is sign-extended int32. | Handlers `0x110b7e0` / `0x110b848`; conversion `0x1c07074`; tables `0x627b62` / `0x627b5c`. |
| 5 | Signed int32 `level_` (+0x40) of the hero's CharaData. | `0x110b930`: MemberSystem.get_hero `0x1156370`, Entity.get_human `0x112f19c` (component slot 14), HumanComponent.get_data `0x113ffc4` (Data.charaData indexed by component+0x18); table `0x627b50`. Null links fault. Metadata field `0x0400109b` and `CharaData.get_level` (`0x116d5ac`, `game:0x0600106b`) confirm the field identity. |
| 7 | Wrapped int32 sum of QuestRecord.rescuedCount (+0x24), then `Config.DEBUG ? AppData.debugRescueNum : 0`. Null quest entries fault, unlike 16/18/19. | `GetTotalRescueCount` `0x1109fe8` (`game:0x060001af`), handler `0x110b9b8`, table `0x627b44`. After enumeration it reads a static enable byte (+0x10) via GOT `0x272f5c0`; when set adds static int32 +0x478 via GOT `0x272f360`. The decoded host evaluates the DEBUG gate and requires the bonus only when enabled. |
| 18 | Wrapped int32 sum of QuestRecord.clearCount (+0x1c), skipping null quests; null records fault. | `SumAllQuestsClearCount` `0x110c330` (`game:0x060001c5`), handler `0x110bc98`, table `0x627b2c`. |
| 19 | Wrapped int32 accumulation of challengeCount minus clearCount for each non-null quest; null records fault. | `SumAllQuestsLoseCount` `0x110c3dc` (`game:0x060001c6`), handler `0x110bce0`, table `0x627b32`. No clamp when clears exceed challenges. |
| 21 | Wrapped int32 capacity minus count of town records with flags bit 0 set. | `0x110be00`: Data.get_townData `0x11698c0`; LINQ predicate `0x117c5d0` (`game:0x06000266`) calls BaseData.Check(1) `0x1169924` (`game:0x06001039`), testing int32 +0x1c. Null towns fault. After full enumeration, reads SecureInt slot 19 of AppData+0x198 array +0x20; table `0x627b38`. |
| 31 | Low 32 bits of stamina interpreted as signed int32. | `GetStamina` `0x110c248` (`game:0x06000223`) reads SecureLong slot 1 of AppData+0x190 array +0x28; handler `0x110c058` compares W registers via table `0x627b4a`. |
| 32 | Signed int64 high score; sign-extended int32 threshold. | `GetHighScore` `0x110c48c` (`game:0x0600020d`) reads SecureLong slot 8 of AppData+0x190 array +0x28; handler `0x110c0ac`, table `0x627b56`. |

Each scalar query/sum completes before argument validation. All listed comparison tables implement codes 0 >, 1 >=, 2 <=, 3 <, 4 ==, 5 !=; invalid codes return false after the query. The port preserves the 32/64-bit distinction and modular arithmetic. It receives decoded storage and the hero field through explicit host queries; storage decoding, bootstrap and original static initialization remain outside this kernel.

For 16 and 22, comparison codes are the same six as `Compare`, but operands are int32. Collection queries and counting precede condition argument checks. For 15 and 4, the catalog getter precedes the id argument read. For 20, each predicate reads building element 1 before condition element 1; an empty building collection therefore returns false even if the condition lacks its type argument. This is full enumeration, not an early-exit existence query.

`TownSystem.GetBuildingList` returns cached array +0x30 when present; otherwise it builds id-indexed three-int rows from its source list +0x20. The condition port receives this result through its host interface; it does not implement cache construction. Catalog sizes must be valid managed-array lengths (0..INT32_MAX); QuestRecord counters are signed int32, including negative values. The earlier array-length interpretation of +0xa8/+0x18 was incorrect: the object is QuestRecord, as established by QuestData.NewGame, metadata and the adjacent counter accesses (see [rewards.md](rewards.md)).
- Consequence: a record with empty `terms` (113 of 228 in the extracted table, including id 10) is never selected automatically.

### Matrix executor — `ExecuteScript` `0x110c8ec`
- Null script → null. Start ≥ length → null; negative start fails the bounds check.
- For each row from `start`: null/empty row → error path (unlike the unit dispatcher). Opcode ≠ 10 → unit dispatch; the result is kept as "last result" and the walk continues regardless of it. Returns the last unit result.
- Opcode 10: calls `GetDelayId(script)`, reads the current talk character (state array element 7), constructs `DelayEvent(id, line+1, wait=row[1], hour=-1, talkChara, args, argi)`; if the row has a third int, writes it to `hour_` (+0x1c). Appends to `AppData+0x200` and returns immediately (remaining rows wait). Order: identify → read wait → read talk → append.
- Opcodes 8/12 do not imply a wait; the walk continues until the end or an opcode 10.

### `DelayEvent`
Fields: `delayId_`+0x10, `line_`+0x14, `wait_`+0x18, `hour_`+0x1c, `talkChara_`+0x20, `args_`+0x28 (object, stored by reference), `argi_`+0x30 (`int[]`, **copied** when non-null). Only direct constructor caller: `ExecuteScript` site `0x110ca5c`.

### Script identity — `GetDelayId` `0x110caa0`
- Scans `eventData` ascending, skips null records, compares the **reference** of `execsScript_` (+0x40) to the argument; returns the first matching index. Contents are never compared.
- No match: requires array `AppData.questData_` (+0x1e8), reads its length, and increments a local index without loading any elements (`0x110cbb0` through `0x110cbd8`). Null array faults. For valid managed-array lengths, both empty and nonempty arrays reach the Config.PRINT gate (`+0x17`), optionally emit a diagnostic, then return -1. -1 does not cancel scheduling; the `DelayEvent` is still queued and fails later on resolution.
- Port consequence: loader and executor must keep script object identity; equal-content clones do not identify.

### Script resolution — `GetDelayScript` `0x110d6dc`
- `delayId_ > 999` → null. Otherwise `eventData[delayId_ % 1000]` (signed remainder) → `execsScript_`. No early guard for negatives: negative remainder fails the bounds check (negative multiples of 1000 map to 0). Null record faults. Stricter than `ExecEvent`.

### Counted queue pass — `SelectForm.Update` `0x11ec578`–`0x11ec6d8`
- Reads the queue count once into a local limit, iterates by index.
- Only `hour_ == -1` entries are touched: `wait_ = wrap32(wait_ - 1)`; `> 0` → keep. Otherwise: (1) remove at index **before** resuming; (2) restore `talkChara_` (SecureInt) into state array `AppData+0x198`→+0x20 **element 7**; (3) decrement index and local limit; (4) `GetDelayScript(delayId_)` and `ExecuteScript(script, line_, args_, argi_)`.
- Entries appended during the pass are beyond the limit and wait for the next pass. Other callback mutations are visible (indexed access, no iterator).
- `wait_` 0 → -1 → fires on the first pass; `INT_MIN` → `INT_MAX` → does not fire.
- Unit = reached passes of this block, not frames/seconds. Entries with `hour_ != -1` are not processed here; their consumer is unknown.

### Update guards before the pass (`SelectForm.Update` `0x11ebb94`)
- `0x11ebde8`: if inherited MyFormBase.dialog_ (+0xc0) is non-null, clear it and return true.
- `0x11ebe1c`: dispatch on SelectForm.state_ (+0xf8) selects state paths. On the path reaching `0x11ec49c`: `UpdateNotifyValue` then `TapjoyForm.AcquireBalance`; true → return before the queue.
- `AcquireBalance` (`0x11b72a4`): Tapjoy.balance_ int32 at +0x5c of MyTapjoy.GetInstance (`0x1185ef4`) `< 1` → false; a FormManager form of the queried type with +0xe8 == 0 → false; otherwise allocates/constructs that form, inserts it into the manager, returns true. No network access in this body. The balance field is updated by Tapjoy.AddBalance and SubBalance; HandleGetCurrencyBalanceResponse passes its received amount to AddBalance. The SDK and persistence side effects of those producers are not ported.
- Then `SetActiveX`, `UpdateDecoObject`, `UpdateTownHuman`. Test `+0xd0 > 2` at `0x11ec4e0` skips the event/guide section but lands at `0x11ec570`, right before the queue — it does not disable the queue.
- So not every `Update` call is a queue pass.

### SelectForm scheduling and the reached event pass

Field identities from metadata plus native field-offset tables: SelectForm.state_ (+0xf8, `0x04000ae3`), static evenCheck_ (+0x31, `0x04000b42`), captured_ (+0x218, `0x04000b4e`); inherited MyFormBase.dialog_ (+0xc0, `0x04000a03`) and loopCounter_ (+0xd0, `0x04000a06`). Confidence high.

- Init (`0x11e9c28`, `game:0x060007ff`) resets loopCounter_ at `0x11ea108` and sets evenCheck_ true at `0x11ea12c` after resolving the SelectForm static fields. A bounded scan of the 67 SelectForm method bodies found immediate byte stores at +0x31 only in Init and Update; this scan excluded wider stores: SelectForm.Push also sets evenCheck_ together with autoSave_ using STRH 0x0101 at `0x11eeccc` (see [ui.md](ui.md)). This is not a whole-program alias/write proof.
- After the dialog early return, state 0 enters the normal update path. It first advances loopCounter_: wrapped int32 increment, then signed remainder modulo INT32_MAX (`0x11ebe34` through `0x11ebe60`). The normal sequence wraps INT32_MAX-1 to 0. Counter growth occurs before the later balance gate, so a reached queue pass is not synonymous with a counter increment.
- State 3: captured_ nonzero returns true immediately (`0x11ec1f8`). Otherwise input mask 0x100000 true calls ChangeState(4) and returns; false tests AppData.IsPushSoftLabel(3), optionally invokes `0x11ef234`, then follows the BGM path. State 4 calls Capture (`0x11ef25c`) and returns. Other nonzero states branch directly to the BGM path (`0x11ec73c`, AppData.PlayBgm `0x1102ffc`). These branches bypass the counted queue and automatic selection.
- State 0 updates cloud_ (+0x108). Its area/table marker refresh is conditional: sceneResume_ (+0x209) must be set and AppData.IsTopForm(this) (`0x1103cb0`) must pass at `0x11ec0c0` through `0x11ec0dc`; otherwise it jumps directly to UpdateNotifyValue. The refresh builds areaMark_ (+0x200), sets weaponNew_ (+0x208) from table predicates, then clears sceneResume_ at `0x11ec498` before UpdateNotifyValue and AcquireBalance. A true balance result returns before SetActiveX, UpdateDecoObject, UpdateTownHuman and the queue. loopCounter_ > 2 skips early setup but jumps to the queue, not past it.
- After the counted queue, `0x11ec6dc` reads loopCounter_. The ARM64 expression rotates `(uint32(counter) * 0x962fc963 + 0x01b4e818)` right by 2 and tests <= 0x00da740c. This is exactly signed `counter % 300 == 0`: multiplication by the inverse of 75 and rotation form a permutation; all signed multiples 300*k map to k+7158278, covering the entire accepted interval 0..14316556. On success it sets evenCheck_ true (`0x11ec79c`).
- If evenCheck_ is true, ExecEvent selection runs. Null result clears evenCheck_; non-null result retains it and branches to SelectForm.Push (`0x11eec34`) via `0x11ecb30`. Pushing/returning from that form is outside the portable reached-pass kernel.
- AcquireBalance's source is inherited Tapjoy.balance_ (field `0x04000602`). GetBalance `0x11873b4` returns +0x5c. AddBalance `0x11873bc` stores wrap32(balance+amount) at `0x1187418`; SubBalance `0x118746c` stores max(wrap32(balance-amount),0) at `0x11874c8`. HandleGetCurrencyBalanceResponse `0x1187a60` calls AddBalance with its incoming amount (`0x1187af4`), after its diagnostic call. These are static observations, not SDK execution.

### Area and equipment markers after scene resume

The block `0x11ec0c0` through `0x11ec498` first tests sceneResume_, then AppData.IsTopForm(this). A false test preserves the existing markers and sceneResume_. On success it allocates a zeroed areaMark_ array sized to the current area catalog, then re-reads the catalog at each loop bound, null-entry check and record fetch. Null area entries retain 0. A present area receives 2 when AreaData.CheckAllCompleted passes, otherwise 1. Allocation/partial writes are not rolled back if a later query fails. Confidence high.

AreaData.CheckAllCompleted (`0x116c1dc`, `game:0x06001013`) reads its stages_ int32 array at +0x50. Null array faults; empty array returns true. It visits IDs in order:

- ID -1 is skipped before reading Config.DEBUG or the quest catalog. Other invalid indices follow the catalog's bounds failure.
- With DEBUG false, the first quest query must return non-null; flags byte +0x1c bit 1 skips the remaining checks for that ID.
- The next quest query may be null and is skipped. Consequently DEBUG true tolerates a null entry that DEBUG false faults on in its earlier flag check.
- A fresh quest query checks state_ (+0x10) == 1. Other states are ignored. Another fresh query then checks completion: flags bit 1 or boolean +0xa4. A found, incomplete quest returns false immediately. If every ID is skipped or completed, return true.

After all area markers, Update fully enumerates equipmentData, hatData and clothData in that order using three LINQ predicates. It does not stop after a positive catalog or matching row. All predicates reject null entries and require stock_ > 0 and isNew_ byte +0x14 nonzero; EquipmentData additionally excludes flags bit 2 (BaseData.Check(4)). Stock offsets: equipment +0x2c, hat +0x64, cloth +0x24. The predicates are `0x12072e0` (`game:0x06000843`), `0x120732c` (`game:0x06000844`), and `0x1207364` (`game:0x06000845`). Their getter calls target `0x116efb8`, `0x1171874`, and `0x116e438`, respectively. Only after all three counts finish does Update assign weaponNew_ to whether any count is positive and clear sceneResume_. There is no mutation of stock or item new flags in this block.

### Form-attached continuation — `SubForm.Finish` `0x120488c`
- Calls base `Finish`; if `delayEvent_` (+0x1d0) is set, restores `talkChara_` into state array **element 5** (not 7), resolves `delayId_`, resumes at `line_` with `args_`/`argi_`; no `wait_` decrement. Keep the 5 vs 7 asymmetry.
- No writer of +0x1d0 found: not in `AppData.Push`, `SetEvent`, `MakeEvent`, the three `SubForm..ctor`; immediate `STR`/`STP`/`ADD` scans found only other types (`BootForm.Load`, `QuestForm..ctor`, `SelectForm.Init`). Not proof the field is dead.

### Unit dispatcher — `ExecuteScript` `0x110cc4c`
- Null/empty row tolerated (common return).
- Argument substitution: clone the row; for elements 1.. (not the opcode), values in `INT_MIN..INT_MIN+99` are replaced by `argi[uint32(v) ^ 0x80000000]`; single pass (a substituted sentinel is not re-resolved); original row untouched; null `argi` or bad index faults. `Config.DEBUG` only adds diagnostics.
- Jump table at `0x627b6e`, 15 uint16 entries relative to `0x110cee0`, scale 4; opcode 31 has a separate branch at `0x110cefc`. Opcode 10's entry is the common return (no scheduling here). Per-opcode effects: see the opcode table in [../formats/event-table.md](../formats/event-table.md).

### Money and crystals (opcodes 1, 2)
- Arg ≥ 0 → `Add*`; arg < 0 → negated **in 32 bits** (`INT_MIN` stays `INT_MIN`) → `Sub*`.
- `AddMoney`/`AddCrystal`: int64 balance + sign-extended int32, wrap64, no clamp.
- `SubMoney`: int64 − sign-extended int32, wrap64, then `< 0 → 0`. So `-INT_MIN` subtraction increases the balance.
- `SubCrystal`: reads SecureInt `AppData+0x190`→+0x20 at index `AppData+0x314` = `SYSSAVEI_YUGAMI` (field 20598, type 4165). Value == 10000 → amount unchanged; otherwise amount × 10000 (wrap32), then sign-extend, subtract, floor 0. Origin/meaning of the value open.
- Storage: money = `AppData+0x198`→array+0x28 element 0, crystals element 1 (SecureLong).

### Discovery (opcodes 4, 15)
- `get_isFound` (`0x116c3a4`) is `state_ == 1`. If false, virtual `Find(true)` (vtable slot 8).
- Area (`AreaData`, slot 8 = `BaseData.Find` `0x116c790`): sets `state=1`, `new=true`. Already-found areas keep their old `new` flag.
- Quest (`QuestData`, slot 8 = `QuestData.Find` `0x1172f0c`): ignores the argument; only `state == 0` → `state=1`, `new=true`. States 2 or negative are unchanged though `isFound` is false.
- Invalid index / null record faults.

### Stamina (opcode 31) and `GetStaminaMax`
- `GetStaminaMax` (`0x110d3fc`): `MyBilling.GetInstance` → `BillingManager.GetItem(MyBilling+0x1c = ITEM_STAMINA_UP, field 21703/type 4228)` → `BillingItem.GetState` (`0x11bc1dc`, reads +0x48 SecureInt). State 0 → 10, nonzero → 20. Null instance/item faults (no fallback).
- Opcode 31 (`0x110cefc`–`0x110cfd8`): (1) `GetStaminaMax` before checking the argument exists; (2) read SecureLong `AppData+0x190`→array+0x28 element 1; (3) `delta = wrap32(pct × limit1) / 100` (truncate toward zero); (4) `GetStaminaMax` again; (5) balance + sign-extended delta (wrap64), clamp to `limit2` (zero-extended to 64 bits), then floor 0; (6) store; returns null.

### Talk and event forms (opcodes 8, 12)
- 8 → `SubForm.MakeTalkForm(id, args)` (`0x1204abc`): `Data.talkData[id]`, int +0x20, text from static `AppData+0x378` → overload `0x1204bc4` (`game:0x060008d4`).
- 12 → `SubForm.MakeEvent(id, args)` (`0x1205028`): `Data.eventMsgData[id]`, string of int +0x20 + content +0x28 → overload `0x1205120`.
- Both: `AppData.Push(form)` (`0x110d3b8`: `FormManager.GetInstance` → `GetCurrentForm` → `Push`), then the form is the opcode's return value. Form preparation is in [text.md](text.md).

### State-0 cloud updates

Confidence high: SelectForm.Update ELF `0x11ebe64`?`0x11ec0bc`, local `research/exports/delay-resume/M35965.asm`. This block follows the counter increment and precedes scene-resume markers. It captures the list count once and visits indices backwards, retrieving the live list at each access. Each non-null int32 array increments slot 6 with wrap32; when it reaches slot 7, slot 0 increments with wrap32 only when the live loop counter is divisible by 3, then slot 6 resets to zero. The native multiply/add unsigned comparison implements that divisibility test. Missing slot 7 faults after the slot-6 mutation.

When slot 0 exceeds wrap32(form field +0x160 + AppData static +0x394), remove the entry first. Read AppData static +0x19c, GameView.GetGameHeight (`0x10f77f4`), AppData.IsLowAspect (`0x1113870`), Random(30), then Random(4) (`0x1102f70`), in that order. Append an eight-int32 array `[-30, y, resource, 0, 1, variant, 0, 20]`, with y = wrap32(height - (lowAspect ? 285 : 280) - firstRandom). Appended records are not updated in this pass. Failure after removal does not restore the old entry. Resource and right-edge fields remain explicit host inputs; their initialization and the RNG implementation are not inferred.

### Common SelectForm prelude and image-animation ticks

Confidence high from `research/exports/select-prelude/`, `research/exports/select-orientation/` and the Update export. The literal at ELF `0x627350` is the uint64 value 1: the 8-byte store at `0x11ebd8c` writes int32 `errFunc_=1` (+0xf0) and `errLine_=0` (+0xf4), not a floating-point animation parameter. Field names are confirmed through the metadata field-offset table.

`MyFormBase.Update` (`0x11d1684`, `game:0x06000747`) advances its independent `update_` counter (+0xb8) with wrap32 increment followed by signed remainder INT32_MAX. It then requires AppData/save storage, converts the SecureInt selected by static `SYSSAVEI_SCREEN_ORIENTATION` (+0x2e8) and, only for -1, calls `AppData.EnableLandSpace` (`0x1116ce8`). It sends -1 for true or 0 for false to `IApplication.SetOrientation` (`0x1b872a8`); the signed bit extraction is intentional. Finally it calls `get_IsLandspace_` (`0x11d2040`), which requires GameView and compares GetGameWidth to 240, discards that result and returns true. The counter write is retained on subsequent failure. `EnableLandSpace` is not pure: its successful event-6 unlock path can write a save flag; the host must preserve this side effect. Its full body and the platform orientation mapping are not yet ported.

SelectForm then captures loopCounter_, camX_ (+0x154) and static AppData.VIEW_X (+0x38c). A zero captured loop counter triggers `ResetImageAnimation` (`0x10ffac0`, `game:0x06000139`), then `UpdateImageAnimations` (`0x10ffc6c`, `game:0x0600013a`) always runs. Both get the values collection of `AppData.imageAnimationInfos_` (+0x170), enumerate it, and traverse each int32[][] group in increasing row order. Reset writes row[1]=0; update writes wrap32(row[1]+1). Empty groups are allowed; null groups/rows and rows shorter than two fail, preserving earlier writes. Aliased rows are visited for every occurrence. The update never takes a modulo or checks another slot. Reset and update access the dictionary independently. These operations precede dialog/state/balance gates, even if those gates return early.

The portable image helpers accept an ordered values view with stable membership during each pass. A concrete host must preserve dictionary traversal order, aliases and enumeration failure behavior; this binding remains open. `update_select_form_prefix` now performs the prelude and the composed pass, returning the original camera/view snapshots alongside the continuation outcome. Input code must use these snapshots where the ARM64 uses its saved registers, rather than re-read fields after callbacks.

### Composed SelectForm pass

`process_select_form_pass` composes the recovered interval beginning at ELF `0x11ebde8`. Its caller must first perform the common prelude (`MyFormBase.Update`, image-animation reset when counter is zero, then image-animation update), including the original null checks and snapshots needed by the later input continuation. The port consumes the dialog marker before dispatching the state. States 3/4 and other nonzero states follow the capture/BGM branches above; they do not touch the normal counter or event queue.

State 0 executes counter advance, clouds, conditional markers, notifications, balance guard, active-X, decorations and town-human updates, in that order. At `0x11ec4e0`, the live signed counter <=2 enters a loop starting at zero. Each condition re-reads sysSave integer slot 19 through SecureInt conversion (`0x11ec4f8`?`0x11ec520`). Each `TownSystem.UpdateEnable(index)` (`0x116a424`) returning true invokes `AppData.ExecEvent(1)` through the null-context forwarder (`0x110c65c`). This bypasses eligibility, can execute event 1 multiple times, and discards each returned form without stopping the loop. Afterward `CheckGuidArrow(true)` (`0x11ee8b8`) runs, including when the count is zero or negative. The counter >2 path skips only this setup, not the queue. Confidence high from the bounded Update export and method registry.

The composed pass then runs counted delays and selection. A selected form goes through `push_select_form`, preserving autoSave/evenCheck writes before manager access. `ReturnTrue` means the original returns true at this boundary. `ContinueInput` means resume at `0x11ec7ec`; it is deliberately not a successful whole-frame result. Notification, decoration, town, balance, capture and guide bodies remain required host methods with no default no-op implementation. The host also owns ChangeState's label updates and final state write. A desktop binding must preserve shared application/static flag ownership across forms, rather than create independent copies per form.

### Touch movement and town-human contact

Confidence high: Update ELF `0x11ec7ec`?`0x11ec944`, helper exports in `research/exports/select-camera/`. Metadata/field-offset mapping identifies touchMove_ (+0x14a), witchX_ (binary32 +0x124), witchDirection_ (int32 +0x128), touchDragDistance_ (binary32 +0x164), townHuman_ (+0x1d0) and collisionId_ (int32 +0x1ec). This block moves the witch, not camX_. It follows successful queue/selection continuation; a returned form bypasses it.

When touchMove_ is false, no movement or collision query occurs. Otherwise compare x with target. For x<target set direction=1, compute binary32 step=Clamp(abs(target)+x/(-3),1,6), then min(x+step,abs(target)). For x>target set direction=2, compute step=Clamp(abs(target)+x/3,1,6), then max(x-step,target). Preserve each binary32 rounding point and the asymmetric absolute target in the first branch: x=-5,target=-4 advances to 0. Equal or unordered comparisons leave x and direction unchanged but still check contact. Clamp helpers (`0x1101820`, `0x1101884`, `0x1101878`) use ordered comparisons; NaNs are not replaced by a bound.

After a movement branch, ARM64 FCVTZS converts toward zero with signed saturation (NaN becomes zero), then SCVTF returns binary32. An explicit +infinity check overrides that result to -2147483648. Finite positive overflow saturates to INT32_MAX, whose binary32 conversion rounds to 2147483648. The C++ kernel defines these cases without an out-of-range float-to-int cast.

`CheckCollisionTownHuman` (`0x11eecdc`, `game:0x0600081f`) scans the live list in increasing index order, re-reading it between iterations. Each required int32[] row must have slot 8; value 2 skips the row. Otherwise convert row[1] to binary32, subtract the current witchX_, take absolute value, and return the first index with distance strictly less than 5. No match returns -1; null storage/rows and short rows fail. A newly encountered nonnegative index invokes `SetTalkFuki(index,false)` before assigning collisionId_; the same index does not repeat the call. Losing contact assigns -1. A failing bubble callback leaves the prior collisionId_ intact. The callback now has a concrete TownBubbleTouchHost binding to the selection routine described below; lifetime updates occur later in Update.

`update_select_form_prefix` now includes this block after `process_select_form_pass` returns ContinueInput. Its next continuation address is `0x11ec948` (the standalone pass still stops at `0x11ec7ec`). The trigger paths are composed below; the remaining buttons and decoded voice/character catalog providers remain open.

### Post-movement progression triggers

Confidence high: Update `0x11ec948`?`0x11eca24`, helper bodies in `research/exports/select-triggers/` and `select-bits/`. First `GetStamina` (`0x110c248`) reads sysSave_.longArray[1] as int64, but the caller compares W0 with 1: only the low 32 bits participate. If equal and `CheckAlreadyEvent(1118)` is false, invoke indexed ExecEvent(1118) with default contexts, discarding its form result. Then read save_.intArray[0], test bit 0x80, and when absent call AppData.On(0x80). The normal save_ is AppData+0x198; sysSave_ is +0x190. They must not be conflated.

`AppData.On` (`0x1110708`) captures a fresh normal-save integer array, reads slot 0, ORs the mask through BitUtil.On (`0x1bee8fc`), converts back to SecureInt, and stores into the retained array. The portable decoded binding reads the array separately for the initial test and the update and preserves all other bits. A missing array/slot fails after any prior event effects, without reaching the second trigger.

Next query `FlagCheck(0x2000)` (`0x1101944`): sysSave_.intArray at the index selected by static SYSSAVEI_FLAG (+0x2bc), followed by a nonzero bit intersection (BitUtil.Check `0x1bee8e4`). If true and CheckAlreadyEvent(1133) is false, call indexed ExecEvent(1133) with default contexts and discard the result. The second flag read happens after the first event and save update, so their side effects can change whether it executes. Both triggers continue to the button path; they do not use the automatic-selection form-return rule.

`CheckAlreadyEvent` (`0x110c4c4`, `game:0x060001c7`) returns false immediately for negative ids; otherwise requires a catalog and returns false for id >= length. For an in-range id it gets the catalog again, requires the row and returns execution_count >0. A replaced/shrunk catalog or in-range null row fails. An out-of-range event id therefore suppresses neither the first trigger's normal-save flag mutation nor the later button processing. The extracted 229-entry catalog cannot resolve ids 1118/1133; no extra records are invented by the port.

The required SelectTriggerHost supplies decoded stamina, the system flag word and normal-save integer storage. SecureInt storage/encoding and effective SYSSAVEI_FLAG initialization remain outside this kernel. `update_select_form_prefix` now runs these triggers after touch movement on ContinueInput, and its next continuation address is `0x11eca28`. Tests use synthetic event catalogs to cover both effect-bearing ids and the observed short-catalog shape.

### Town bubble selection and lifetime

`SetTalkFuki` (`0x11eedd8`, `game:0x06000811`) writes townTalkIndex_ (+0x1d8) before any row lookup. Its parameters are int32 index and bool force; force is unused in the observed body. It requires townHuman_[index], reads character id from row[0] and a sequence value from row[10], obtains voiceData before charaData, then uses CharaData.voiceId_ (+0xa8) to index the retained voice catalog. VoiceData.text (+0x28) is a string array; select `sequence % length` with signed remainder. Empty text arrays fail; negative nonzero remainders fail; a negative multiple of length selects element 0. A null selected string is allowed.

After normal text selection, re-read charaData and test isRecruit (int32 +0x50 !=0). Only when true, re-read it again and test isLiving (int32 +0x4c !=-1). Recruited/nonliving characters obtain voiceData again and search from the start for the first record whose type (+0x20) equals 4. Predicate `0x120739c` (`game:0x06000846`) requires each encountered record: a null before the first match fails, while later records are not examined. The generic FirstOrDefault body (`0x12b13b0`, FDE size 852 bytes) returns null on no match, which SetTalkFuki rejects. Choose the replacement text with the same signed remainder. Normal text selection always precedes this path, so an invalid normal voice is not rescued by a valid type-4 voice.

Only after successful selection does the method assign townTalkText_ (+0x1e0), then townTalkFrame_=120 (+0x1e8). Failures retain the newly written index and previous text/count. `TownBubbleTouchHost` binds this complete selection routine to the existing touch-contact interface.

The lifetime block is later in Update, ELF `0x11ed538`?`0x11ed660`; it must not run immediately at the contact callback. A negative frame count bypasses it. For index !=-1, get the current human row, form x=wrap32(saved camX + saved VIEW_X + row[1]), then y=wrap32(GetGameHeight()-1), and call CheckViewArea(x,y,16,0). If outside, set index/count=-1 and text to the empty string. If inside (or index was -1), decrement the live count with wrap32 and set index=-1 only if the result is negative. Ordinary expiry retains text, unlike offscreen cancellation. Zero is a live count and becomes -1 on this pass.

`CheckViewArea` (`0x11f43ec`, `game:0x06000838`) uses inclusive bounds and ordered calls: reject x < wrap32(-marginX) before any dimension query; query width; reject y < wrap32(-marginY), then x > wrap32(width+marginX); finally query height and accept y <= wrap32(height+marginY). Dimensions are SelectForm's scaled dimensions. The width/height providers remain explicit host callbacks; their raw-size/scale binding is not supplied here.

Evidence: `research/exports/select-camera/F11eedd8.asm`, `select-bubble/` (voice getter, predicates), `select-bubble-lifetime/` (FirstOrDefault and view helpers), and the Update export. Field identities use native field offsets and metadata. GOT 0x2734278 maps to the type-4 predicate; GOT 0x272ee88 maps to string literal 0, whose metadata length is zero. Confidence high for these static contracts; no Android comparison yet.

### Room/shop navigation and remaining button priority

The button chain begins at ELF `0x11eca28`, after the progression triggers. `process_select_navigation` ports its first two branches and the deferred room callback; it is called only after prefix ContinueInput. Confidence high from the Update export and `research/exports/select-buttons/`.

1. Input mask 0x04000000 wins first. Play RESSOUND_SE_BUTTON (AppData static +0x250), construct a delegate bound to this SelectForm, compute Graphics.GetColorOfRGB(48,48,48), and invoke TransitionSystem.MakeTransition(3,delegate,true,color), then return true. The color is 0xff303030: `0x1b62c34` clamps each integer channel to 0..255 and ORs alpha 0xff (local `select-button-color/`). This branch does not clear weaponNew_ or push the room immediately.
2. Only if that input is false, query mask 0x01000000. A true result calls ShopForm.MakeShopForm(11) (`0x1209b6c`, allocation then constructor with type 11), then SelectForm.Push and returns. There is no button-sound call in this branch. A null factory result is still passed through Push.

The deferred `<Update>b__117_0` (`0x11f5a2c`, `game:0x0600083d`) clears weaponNew_ (+0x208) first, then requires AppData and reads frmRoom_ (+0xc0), then uses SelectForm.Push. Thus missing AppData leaves the marker cleared without setting Push flags; a manager failure occurs after autoSave/evenCheck are set. The portable callback retains shared ownership of the original form and host until the transition completes or is cancelled. The transition host must release its retained callback at that point; scheduling does not imply immediate invocation. TransitionSystem.MakeTransition creates a transition form with type +0xf0, callback +0x100, bool +0xf4, color +0xf8 and zero counter +0xfc, then inserts it through FormManager. Its animation/callback timing remains an explicit host responsibility, not an assumed duration.

If both initial buttons are false, continuation is `0x11ecb5c`. The subsequent priority established by branch addresses is:

| Mask / start | Observed action; remaining work |
| --- | --- |
| 0x08000000 / 0x11ecb5c | Selected building: sound, building-list validation, save position, dispatch by building id/type. Invalid selection can fall through to the next button. Ported below; data and form factories remain host bindings. |
| 0x10000000 / 0x11ecdcc | Sound; write normal-save slots 11,12,10,8 from camX_, camY_, integer-converted touchDragDistance_ and selected area; clear selected area; Push AppData.frmSelectStage_ (+0xa8). Ported in the town-button chain below. |
| 0x20000000 / 0x11ecf94 | Clear two selection-message fields, sound, MakeObstacle(selected building), Push, clear selected building. Ported in the town-button chain below. |
| 0x40000000 / 0x11ed038 | MakeStoreInvite then Push; update first town row with state 1 to state 2, save a system value and reset bubble fields. Control flow ported below; decoded data/form bindings remain explicit. |
| 0x02000000 / 0x11ed0d4 | Find first town row with state 3, clear its state, apply reward, build/push message form, SetTalkFuki; returns even when no matching row. Control flow ported below; decoded data/form bindings remain explicit. |
| no button / 0x11ed144 | Selected-building guidance, then conditional autosave and later common updates before bubble lifetime. Pending port. |

These are input masks passed to the original input helper, not desktop key assignments. No later branch is treated as implemented by the first-two-button kernel.

### Selected-building action and saved position

`process_select_building_button` covers input 0x08000000 and its branches, beginning at `0x11ecb5c`. False input continues to the area button. True input first plays the button sound, captures TownSystem.GetBuildingList, then reads selectChip_ (+0x11c). Negative selection continues without requiring the returned list; a nonnegative out-of-range index also continues. A valid index writes the normal-save integer slots 11,12,10 in that order, obtaining a fresh array each time: camX_ (+0x154), camY_ (+0x158), integer-converted touchDragDistance_ (+0x164). The conversion is ARM64 FCVTZS with the explicit +infinity-to-INT32_MIN override; NaN gives zero and finite positive overflow INT32_MAX. Earlier writes survive a later bounds/null failure. Portable camera_x and drag_distance fields are references to the prelude/movement storage, not copies.

Then re-read the current selection within the retained building list. Null row clears selection and returns; otherwise row[1] is the building-data id. For -1, TownSystem.CheckEnable(index) (`0x116a59c`, required bool array +0x28 with bounds check) gates construction. When enabled, write dispUnLockInfoNumber_/Frame_ (+0x18c/+0x190) both -1, call MakeBuildList(current selection), then Push. Disabled construction still clears selection and returns.

For an existing building, call IsShop (`0x116c9c4`, `game:0x0600103c`) on the first catalog record, then obtain the record again to read kind (+0x40). IsShop is true for kinds 1..5,9,10. If true, MakeShopForm uses the second kind read, then Push. If false and second kind is 6, get the living-member array and current selected slot. A non-null member goes to MakeHumanHouse(member,index,false); null goes to MakeShopForm(6), requires its result, and stores the live selected index at ShopForm+0x12c before Push. Otherwise obtain a fresh building record for kind==7 (construct SubForm type 15), then, if false, another for kind==12 (read sysSave integer selected by AppData static+0x304 and call MakeShareWindow(value==0)). Other kinds open nothing.

All handled valid-slot paths clear selectChip_ to -1 only after any Push returns. Push failure therefore retains selection and its already-set autoSave/evenCheck flags. Position persistence happens before every one of these routes, including a null row or unrecognized kind. `update_select_through_building` composes this action after the room/shop pass; false continuation is the area button at `0x11ecdcc`.

Factory boundaries are explicit: MakeBuildList allocates SubForm type 12 and stores the chip at +0x174; MakeHumanHouse allocates type 16 and stores the member at +0x128 and chip at +0x174 (the observed option is false); MakeShareWindow allocates type 27 and stores the argument at +0x170. Full form constructors/content, GetBuildingList and GetLivingSubMemberArray remain host bindings. Evidence: `research/exports/select-building/` plus Update `0x11ecb5c`?`0x11ecdcc`, `0x11ed260`?`0x11ed310`, `0x11ed5b4`?`0x11ed61c`, `0x11ed834`?`0x11ed93c`. Confidence high for the dispatch and mutation contracts; runtime behavior remains unverified.

### Area, obstacle, invitation and present buttons

After the selected-building action falls through, `process_select_town_buttons` handles the remaining four buttons in native order. Confidence high from Update `0x11ecdcc`?`0x11ed144`, invitation continuation `0x11ed16c`?`0x11ed25c` and present continuation `0x11ed314`?`0x11ed3dc`. All handled branches return before the later bubble-lifetime pass.

- **Area (0x10000000):** play button sound; write normal-save slots 11,12,10 through the shared position helper; capture a fresh normal-save integer array and store live selArea_ (+0x120) into slot 8; set selArea_=-1; require AppData and obtain nullable frmSelectStage_ (+0xa8); Push it. Failure obtaining the form leaves the save/selection changes in place. This does not clear selectChip_.
- **Obstacle (0x20000000):** set dispUnLockInfoNumber_/Frame_ both -1 before playing sound; MakeObstacle(live selectChip_) then Push; clear selectChip_ only after Push returns. No bounds guard is added to the selected index.
- **Store invitation (0x40000000):** MakeStoreInvite and Push first, without a button-sound call here. Scan the live town-human list in increasing order for the first row[8]==1. Re-fetch its row and write row[8]=2; no match still continues. Call SumAllQuestsChallengeCount, capture sysSave_.intArray, resolve SYSSAVEI_STORE_KAIRO_LIMIT (AppData static +0x300), and store the int32 sum. Finally set townTalkIndex_=-1, townTalkFrame_=0, townTalkText_ to the empty string. This is distinct from offscreen cancellation's frame=-1. Push failure occurs before any row mutation; a later save failure preserves both Push and row changes but leaves the bubble unchanged.
- **Resident present (0x02000000):** scan the live list for first row[8]==3. No match still returns true. Re-fetch the row and write state 0, then AddMoney with residentPresentValue_ (int32 SelectForm+0x1f0). Re-read that field after credit, format it with the original literal, MakePopUpContens with type 3, original title, formatted value and option 0, then Push. Only after Push succeeds call SetTalkFuki(index,false). Thus a formatting/Push/bubble failure does not roll back credit or resident state. The factory and localized literals remain explicit host inputs; no game text is embedded in the port.

Each scan requires list storage and non-null rows with slot 8, visits only until its first match and queries the live list between iterations. The mutation deliberately re-fetches the matched row. The sum/system-save slot and decoded catalogs remain required host providers; their missing values are not substituted. Invitation/present handling does not decrement the bubble timer in the same returned path.

`update_select_through_town_buttons` composes these branches after the building/navigation/prefix chain. When no button consumes the update it returns the preserved camera/view snapshot and continues at `0x11ed144`: selected-chip guidance, autosave, soft-label processing and then the late lifetime block are still pending.

## Port

- `src/core/gameplay/select_town_buttons.{hpp,cpp}` ports the remaining area/obstacle/resident button chain and composes it after building actions. `tests/test_select_town_buttons.cpp` verifies button priority, separate save stores, first-resident-only mutation, mutation-before-failure behavior, post-credit value re-read, concrete money progression and bubble selection.

- `src/core/gameplay/select_building.{hpp,cpp}` ports the complete selected-building dispatch, position writes and IsShop predicate. Tests in `test_select_building.cpp` cover all routes, invalid/empty slots, live catalog replacement, null housing-shop failure, nonfinite position conversion, partial save writes and Push failure. The composed entry now reaches the area-button boundary.

- `src/core/gameplay/select_navigation.{hpp,cpp}` ports room-transition and type-11 shop button priority plus the room callback. `tests/test_select_navigation.cpp` verifies deferred marker/Push effects, callback ownership, null child handling, failure ordering and continuation when neither button is pressed. `update_select_through_navigation` composes it with the existing prefix through SelectUpdateBindings and stops before building buttons; the integration test also verifies dialog bypass and deferred room Push.

- `src/core/gameplay/town_bubble.{hpp,cpp}` implements ordered normal/recruitment text selection, view bounds and the late lifetime kernel; `TownBubbleTouchHost` connects contact detection to text selection. `tests/test_town_bubble.cpp` covers repeated catalog access, signed remainder, nullable text, partial failure, first voice selection, visible countdown, offscreen clearing and movement composition. The late lifetime call still awaits the intervening button-path integration.

- `src/core/gameplay/select_triggers.{hpp,cpp}` ports CheckAlreadyEvent and the two ordered post-movement triggers with normal/system save separation. `tests/test_select_triggers.cpp` covers low-word stamina, repeat eligibility for nonpositive counters, dynamic second-gate effects, discarded forms, out-of-range ids, catalog replacement and missing-save failure ordering. `test_select_form_pass.cpp` checks composition after movement and early-return bypass.

- `src/core/gameplay/select_touch_move.{hpp,cpp}` ports binary32 witch movement, integer conversion and first-match town contact. Strict floating-point compiler options prevent contraction/reassociation. `tests/test_select_touch_move.cpp` covers direction, signed-target overshoot, bounds, repeated/new contact, callback failure and nonfinite values. `test_select_form_pass.cpp` verifies that dialog returns skip it and normal event continuation reaches it.

- `src/core/gameplay/select_prelude.{hpp,cpp}` ports MyFormBase.Update with required orientation/view host callbacks, SelectForm prelude ordering/snapshots and ordered image-animation row operations. `tests/test_select_prelude.cpp` covers aliasing, wrap32, null/short-row partial writes, automatic-orientation arguments and save failure after counter mutation; the composed prefix is also covered in `test_select_form_pass.cpp`. Dictionary storage and EnableLandSpace/platform bodies remain external.

- `src/core/gameplay/select_form_pass.{hpp,cpp}` composes the recovered state/dialog/balance guards, cloud/marker refresh, first-cycle city events, queue/selection and returned-form Push. `tests/test_select_form_pass.cpp` exercises these together with synthetic catalogs, real event execution and delays, including mutation surviving manager failure. Common prelude and post-selection input remain outside this interval.

- `src/core/gameplay/select_clouds.{hpp,cpp}` ports the complete cloud-update block with explicit host queries. `tests/test_select_clouds.cpp` covers backward replacement, RNG order, no second update of appended entries, signed counter gating, int32 overflow, aspect offsets and partial mutation on failure. This kernel does not complete SelectForm.Update.

- `src/core/gameplay/select_markers.{hpp,cpp}` ports AreaData.CheckAllCompleted, the three equipment predicates and the scene-resume marker refresh block. The host supplies decoded records, DEBUG, the top-form test and fresh catalog accesses. It preserves partial marker writes and final dirty-flag clearing order. Tests cover sentinel IDs, empty/null stage arrays, DEBUG-dependent null behavior, discovery/completion states, sparse area marks, stock/flag filtering, full catalog enumeration and failure after successful area refresh. It is called after cloud updates and before UpdateNotifyValue, not as a complete form update.

- `src/core/gameplay/select_event_pass.{hpp,cpp}` ports the state-0 counter arithmetic and the reached queue/selection block as separate entry points. `process_select_event_pass` resumes counted delays first, then checks the live counter and request flag, then selects events; it clears the flag only after a null selection result. The caller must establish the UI/state/balance guards before invoking it and handle a returned form. Tests cover modulo boundaries, queue effects changing later eligibility, retained versus cleared requests, signed periodic counters and failure ordering. This is not a full SelectForm.Update port.

`DecodedEventExecutionHost` now binds default args, Config.PRINT diagnostics and GetDelayId fallback to `DecodedEventExecutionState`, forwarding effects and talk-character access to an existing flow host. The state distinguishes unknown save presence, absent save, absent string array, empty string array and a null converted string slot. It preserves shared identity for decoded args. Missing PRINT state fails only when a diagnostic gate is reached; disabled PRINT does not invoke a diagnostic sink. A successful script lookup does not access questData_ or PRINT. Unmatched scripts require an explicit quest-array count and return -1 after the diagnostic gate; the length-only loop is elided under the stated managed-array size invariant. The adapter checks delay id >999 before accessing the catalog. Tests cover each of these boundaries and composition with indexed execution. Save decoding, original string virtual conversion and initialization producers remain external; the inputs are decoded values, not inferred defaults.

- `src/core/gameplay/event_execution.{hpp,cpp}` ? `execute_indexed_event` and `select_and_execute_events` compose the id-indexed catalog, condition matrix evaluator and script executor. `EventExecutionHost` exposes the current catalog, default SecureString args and the empty-script diagnostic, alongside the existing effect/delay callbacks. Catalog replacements are observed at the original getter boundaries. Selection's eligibility check retains the terms record while re-reading the execution-count record, matching `CheckTerm(int)` at `0x110b49c`/`0x110b4d4`; the earlier standalone `check_event_eligibility` remains a stable-catalog convenience. Tests in `tests/test_event_execution.cpp` compose `DecodedEventConditionHost` and synthetic effects: counter overflow, empty effects, explicit/default args, preparation failure, distinct counter source/destination, dispatch failure without rollback, null gaps, changed eligibility after prior effects, a form stopping selection, a delay not stopping selection, and catalog replacement/growth. This does not supply the SelectForm scheduling guards or populate live state from an Android save.

`DecodedEventConditionState` and `DecodedEventConditionHost` provide concrete bindings for the 14 table-used predicates. The host observes the supplied state by reference, including catalog updates; values absent from the decoded state remain absent and fail when queried. It distinguishes a missing AppData context, missing/empty characters storage, null catalogs/records, and valid zero values. `hero_level` is the result of the MemberSystem/Entity/HumanComponent lookup, not an invented default hero. `debug_enabled=false` returns a zero adjustment without reading `debug_rescue_count`; true requires that value. The conditional bonus is read after quest enumeration. Unknown condition opcodes fail explicitly. Tests cover absent scalars, bootstrap checks, DEBUG gating, scalar bindings and conditions changing after area discovery. The state must still be populated by the game-data/save/ECS layers; this host is not an integrated original-game loader.

- `src/core/gameplay/event_conditions.{hpp,cpp}` ? `check_event_eligibility`, `check_event_conditions`, `compare_event_values`, and the 14 table-used branches of `check_event_condition`. `EventConditionHost` supplies context checks, decoded catalogs and the building list. Unrecovered opcodes are delegated explicitly; the test host rejects them. The eligibility boundary accepts one stable catalog per call, preserving null gaps and script references; original repeated singleton/catalog retrieval is not reproduced. `tests/test_event_conditions.cpp` (`event_conditions_contract`) covers empty-false, AND short-circuit, negative versus positive execution counts, all comparison codes, completion flags, wrapped signed quest counters, rescue null handling and adjustment, scalar widths, town capacity/count ordering, recruitment filters, lookup order and building enumeration after a match. Invalid input uses C++ exceptions rather than managed exception objects. The unit method remains `traced` in the registry because its other handlers are unresolved.

- `src/core/gameplay/event_flow.{hpp,cpp}` — `execute_event_script` (= `0x110c8ec`, including opcode 10 / `DelayEvent` capture with `argi` clone) and `process_counted_event_delays` (= the `SelectForm.Update` queue block; one call = one reached pass). `EventFlowHost` must supply unit dispatch, `identify_script` (by reference), `resolve_script`, talk-character read/restore. Test `tests/test_event_flow.cpp` (CTest `event_flow_contract`): effect/delay/resume/re-delay chain, callback order, identity, null vs empty, explicit hour untouched, wait 0/negative, `INT_MIN` wrap, adjacent removals, row errors, resolve failure after removal, caller reference replacement, queue mutation by callback.
- `src/core/gameplay/event_progression.{hpp,cpp}` — `resolve_event_arguments` (sentinel substitution) and `apply_event_progression` (opcodes 1/2/4/15 on decoded `EventProgression`; `crystal_subtraction_mode` must be supplied, no default). Test `tests/test_event_progression.cpp` (`event_progression_contract`).
- `src/core/gameplay/event_dispatch.{hpp,cpp}` — `stamina_limit_from_item_state`, `event_stamina_balance`, `dispatch_known_event_instruction` (opcodes 1/2/4/8/10/12/15/31; 10 is a no-op; others return `handled=false`). `EventEffectsHost` supplies progression state, stamina read/write/limit, form creation and push. Test `tests/test_event_dispatch.cpp` (`event_dispatch_contract`); `tests/test_presentation_forms.cpp` also drives the dispatcher.
- `src/core/gameplay/event_catalog.{hpp,cpp}` ? record loading, id indexing, identity lookup and delay script resolution. Accepts decoded UTF-8 text, an explicit CRLF-normalization gate and an integer parser callback; `parse_event_decimal` implements the observed ASCII decimal subset. The host remains responsible for invalid UTF-8/culture policy. `identify_event_script` scans non-null records by shared script identity (including null), invokes an explicit fallback callback when no match exists, then returns -1. The generic callback must reproduce the original +0x1e8 access or fail when unavailable; the decoded execution adapter implements the complete lookup and fallback contract with fresh catalog accesses. `resolve_event_script` preserves the >999 guard, signed modulo, negative-multiple-of-1000 behavior and null-record failures. Tests: `event_catalog_contract`.
- `src/platform/desktop/event_trace.cpp` ? research harness protocol `SAGA_EVENT_TRACE_3`: mode (`inject` or `auto`), event id, reached queue passes, fixture balances/settings, hex-encoded original table bytes and explicit state fields. The native loader supplies the catalog. Auto mode performs exactly one selection scan through `DecodedEventExecutionHost` and `DecodedEventConditionHost`, then the requested counted queue passes; it does not infer SelectForm scheduling or repeat selection every pass. Inject mode retains direct matrix execution for a chosen id (no eligibility check or ExecEvent increment). Snapshots include balances, queue waits and execution counters indexed by id (null gaps retained); form requests, diagnostics and whether selection returned a form are emitted separately.
- Effect dispatch refreshes condition balances and area discovery state before the next eligibility check. Fixture quest discovery state is separate from completion flags/counters, matching the recovered contracts. Unknown effects fail; forms are logged, not displayed. Missing fixture data remains absent and fails if queried.
- `tools/run_event_scenario.py` validates UTF-8 and sends the table and JSON fixture to the native executable. Use `--auto --state <fixture.json>` for one selection scan or `--id <id>` for direct injection. Both require explicit `--money`, `--crystals`, `--crystal-mode`, `--stamina`, `--item-state`, `--talk-character`, and `--passes`. Example injection (local data):
  ```
  python -I tools/run_event_scenario.py extracted/members/06_xls/0007_English.lproj_Event.txt --exe build/desktop/event_trace.exe --id 10 --passes 50 --money 7 --crystals 11 --crystal-mode 10000 --stamina 3 --item-state 0 --talk-character 2 --output research/exports/event-auto-injection-regression.json
  ```
  Record 10 retains money 8007, crystals 1011 and an empty queue after pass 50. Integration tests use only synthetic tables: a money-gated discovery enables a later area condition, queues a delay, then resumes to change money from 5 to 12; preexisting execution counters skip records; a form stops the scan; missing context fails. Test `tests/test_event_scenario.py` runs fixture validation standalone and native integration through CTest (`event_scenario_integration`). This is not a comparison against a running APK.

Fixture JSON schema (all keys optional; absence preserves missing state rather than supplying defaults):

| Fields | JSON shape |
| --- | --- |
| `context_present`, `sys_save_present`, `print_enabled`, `debug_enabled` | Boolean |
| `context_character_count`, `quest_data_count` | Nonnegative integer array lengths |
| `hero_level`, `debug_rescue_count`, `town_capacity` | Signed int32 |
| `high_score` | Signed int64 |
| `sys_save_strings` | Array of strings or null converted SecureString slots |
| `execution_counts` | Array of `[event_id, signed_count]`; ids must exist |
| `areas` | Array of `[state, is_new]` or null gaps |
| `quests` | Array of `[state, is_new, flags_byte, completed, record]` or null gaps; record is `[challenges, clears, rescued]` or null |
| `characters` | Array of `[sub, flags_byte]` or null gaps |
| `town_flags` | Array of uint32 flags or null gaps |
| `buildings` | Array of int32 rows or null gaps |

For a synthetic money-only event with nonempty effects, a minimal auto fixture is `{"context_present": true, "context_character_count": 1, "sys_save_present": false}`. This is test state, not an inferred new-game state. PRINT is required when an empty script or failed identity lookup reaches its diagnostic gate; quest_data_count is required only for that lookup's fallback. Additional predicates require their corresponding catalogs/scalars. Tool collection lengths are limited to 100000; these are harness limits, not recovered game limits.

- Last recorded build: Windows/MSVC, 36/36 CTest (35 C++ contracts plus native scenario integration).
- Missing: population of decoded default-args/diagnostic state from saves/configuration, SelectForm integration, remaining unit `CheckTerm` predicates, integration with save-loaded execution counters; hour-based delays; `SubForm.delayEvent_` path (slot 5); `SelectForm.Update` guards; opcodes other than the eight; `SecureLong`/`SecureInt` storage; billing item source for stamina; UI forms.

## Evidence

- Marker evidence: `research/exports/select-markers/` (area predicate and three LINQ predicates), `research/exports/select-equipment/` (stock getters), and `research/exports/delay-resume/M35965.asm` (gating, allocation and mutation order). GOT method usages 0x2734210/0x2734218/0x2734220 resolve to metadata methods 36032/36033/36034. Native field-offset metadata maps AreaData.stages_ to +0x50 and each stock_ field to the offsets above. Build log: `research/exports/select-markers-build.log`.

- Scheduling: `research/exports/delay-resume/M35965.asm`, `research/exports/select-gates/` (Init, ChangeState) and `research/exports/select-balance/` (balance getter, arithmetic producers, response callback). Field identities use metadata definitions and the field-offset table at ELF 0x277ece8. Portable contract validation: `tests/test_select_event_pass.cpp`, local build `research/exports/select-event-pass-build.log`.

- Context binding evidence: `research/exports/delay-origin/M34384.asm` (`GetDelayId`), `research/exports/event-default-args/F1c09b58.asm` (`SecureString.op_Implicit`) and `research/exports/event-execution-context-fields.json`. Relocation-aware field offsets map instance AppData.questData_ (metadata field 20733, token `0x0400046e`) to +0x1e8 and Config.PRINT (7533, `0x0400012d`) to static +0x17. The unrelated AppData static constant at offset +0x1e8 is not the instance field used by this method. Confidence high for the observed access contracts; runtime initialization state remains unobserved.

- Execution composition: local bounded exports `research/exports/event-conditions/F110c668.asm`, `F110c580.asm` and `F110b3e0.asm`, plus `research/exports/event-executor/M34377.asm`. Displayed addresses include Ghidra base `0x100000`; ELF call sites are given above. `registry.py lookup 0x1c09b58` maps the default-args conversion to SecureString.op_Implicit. Test build: `research/exports/event-execution-build.log`.

- Condition host field identities: relocation-aware metadata-registration tables at ELF `0x26b5100` (25389 native types, array `0x26d61b0`) and `0x26b5120` (8619 field-offset arrays, array `0x277ece8`). GOT `0x272f5c0` resolves type usage index 10231 to type-definition 1778 `kairo.common.cfg.Config`, field `DEBUG` (`0x04000126`) at static +0x10. GOT `0x272f360` resolves index 9048 to type-definition 4165 `main.AppData`, field `debugRescueNum` (`0x04000566`) at static +0x478. CharaData field `level_` (`0x0400109b`) maps to +0x40, independently confirmed by the getter body exported in `research/exports/condition-level/`; the unit predicate reads that field directly. Local field evidence: `research/exports/condition-host-fields.json`. Confidence high for identities and reads; initializer values remain unknown.

- Local bounded ARM64 exports: `research/exports/condition-data/` (quest completion, area discovery, data getters, building list, both LINQ predicates), `condition-character/` (sub/recruit getters), `condition-scalars/` (quest sums, rescue count, scalar accessors and hero access chain), `condition-capacity/` and `condition-town/` (town predicate and flag test), and `event-conditions/` (three CheckTerm overloads, comparator, challenge sum). Metadata/code map resolves the method ids and field offsets above. ELF SHA-256: `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`. These exports report Ghidra image base `0x100000`; subtract it from their instruction addresses to obtain the ELF VAs used here.

- [../archive/reports/72-return-event-dispatch-and-data-gap.md](../archive/reports/72-return-event-dispatch-and-data-gap.md) — `ExecEvent` guard/counter, `EventData` fields, event 1133
- [../archive/reports/76-event-table-tokenization.md](../archive/reports/76-event-table-tokenization.md) — tokenizer, record layout
- [../archive/reports/77-event-opcodes-and-delay.md](../archive/reports/77-event-opcodes-and-delay.md) — opcode usage, jump table, opcode 10
- [../archive/reports/78-delay-event-resumption.md](../archive/reports/78-delay-event-resumption.md) — queue pass, `SubForm.Finish`, `GetDelayScript`
- [../archive/reports/79-gameplay-event-update-guards.md](../archive/reports/79-gameplay-event-update-guards.md) — `SelectForm.Update` guards, `AcquireBalance`, `Push`
- [../archive/reports/81-delay-origin-and-script-identity.md](../archive/reports/81-delay-origin-and-script-identity.md) — `GetDelayId`, `delayEvent_` writer search, wait histogram
- [../archive/reports/82-native-event-flow.md](../archive/reports/82-native-event-flow.md) — `event_flow` port
- [../archive/reports/83-event-progression-effects.md](../archive/reports/83-event-progression-effects.md) — money/crystals/discovery, substitution
- [../archive/reports/84-stamina-and-event-dispatch.md](../archive/reports/84-stamina-and-event-dispatch.md) — stamina, opcodes 8/12, `event_dispatch`
- [../archive/reports/109-native-event-table-scenario.md](../archive/reports/109-native-event-table-scenario.md) — `event_trace` scenario
- [../archive/reports/110-automatic-event-selection.md](../archive/reports/110-automatic-event-selection.md) — selection, conditions, `Compare`
- Related: [71](../archive/reports/71-town-return-handler.md) (1133 caller), [73](../archive/reports/73-game-data-id-indexing.md) (id indexing, size 229, gap 180)
- Local (`research/exports/`, migrated from `build/research/`): `return-event/`, `event-data/`, `event-loading/`, `event-tokenizer/`, `event-int/`, `event-executor/`, `event-opcode-usage.json`, `delay-resume/` (incl. `M35965.asm`), `delay-script-callers.json`, `delay-attach/`, `delay-attach-more/`, `delay-origin/`, `subform-store-candidates.json`, `subform-pair-address-candidates.json`, `event-delay-usage.json`, `progression-effects/`, `event-conditions/` (incl. `direct-callers.json`), `event-condition-helpers/`, `event-scenario-en.json`, `event-scenario-ja.json`, build logs `event-flow-build.log`, `event-progression-build.log`, `event-dispatch-build.log`, `event-trace-build.log`

## Open questions

1. (blocking) Concrete context/catalog bindings for the 14 recovered table-used conditions; automatic selection is composed with the kernels but not yet called by a desktop form/frame loop. Populate the decoded host from data/save/ECS providers, including hero level and the Config.DEBUG/AppData.debugRescueNum initialization state. Other unit branches remain unresolved for scripts outside Event.txt.
2. (blocking) Bind the composed select_form_pass to a desktop form host: recover selected-chip guidance/autosave and remaining late updates at 0x11ed144, including the town_bubble lifetime call and outer Update exception handlers, and implement its required notification/decor/town/guide/capture callbacks plus orientation and image-dictionary bindings for the recovered prelude. The guards, clouds, markers, counter, initial event loop and reached-pass scheduling are composed and tested.
3. (blocking) Initial/real values of `SYSSAVEI_YUGAMI`, the `ITEM_STAMINA_UP` item state, and the money/crystal/stamina slots from save/new game.
4. Unit dispatcher opcodes other than 1/2/4/8/10/12/15/31 (jump table has 15 entries); scripts outside `Event.txt` (quests, built events) may use them.
5. Consumer of `DelayEvent` entries with `hour_ != -1` (none in the extracted table).
6. Writer of `SubForm.delayEvent_` (+0x1d0) and why `Finish` restores slot 5 instead of 7.
7. Meaning of state array elements 5 and 7 (talk character slots); producer/lifecycle of AppData.questData_ (+0x1e8), whose fallback only reads length.
8. Event 1133: confirm the effective `eventData` length at runtime (array model gives 229).
9. Meaning of `timing_` (0 in every extracted record) and its consumer.
