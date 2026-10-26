# Events

The gameplay event/script system is statically recovered from the event table (`Event.txt` → `Data.eventData`) through condition checks, the matrix script executor, the `DelayEvent` continuation queue and the unit dispatcher, down to the numeric effects of the eight opcodes present in the extracted table (money, crystals, area/quest discovery, stamina, talk/event form requests, delay). The port implements the executor, counted continuation queue, argument substitution, the numeric effects and the stamina formula as isolated, tested C++ kernels, plus a native trace tool (`event_trace`) that replays one injected script from the local table. Condition evaluation now includes the record and matrix guards, the comparator and all 14 unit predicates used by Event.txt. The C++ table reader now builds an id-indexed catalog used by the trace tool and delay lookup. Indexed execution and automatic selection now compose the condition and execution kernels. Not ported: remaining condition predicates, the concrete application/frame-loop bindings, the `GetDelayId` fallback context, the hour-based delay path, `SubForm.delayEvent_` resumption and the forms themselves. Nothing has been compared with the running original.

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
- Default args path (`0x110c720`?`0x110c750`): non-null incoming args bypasses lookup. Otherwise null AppData.sysSave (+0x198) yields null args; a present save requires its array at +0x30 and element 0, converted by `SecureString.op_Implicit` (`0x1c09b58`). Missing array/slot faults before the counter changes. Integer args are passed through unchanged.
- The catalog is read for the initial length guard and again after default-args preparation. Three subsequent reads capture the script record (`0x110c768`), counter destination (`0x110c788`) and counter source (`0x110c7ac`). The source value is incremented in int32 and stored to the destination before inspecting the saved script record. Null source/destination faults; a null script record reaches the empty-script diagnostic after the counter update. The port preserves these accesses through its catalog provider rather than taking an eligibility snapshot.
- Empty-script diagnostics are gated by Config static byte +0x17 (`0x110c850`?`0x110c8bc`), delegated to the execution host. Failure in script dispatch does not roll back the counter. The executor starts at line 0 (`w5=0` at `0x110c828`; `w2=1` is unused by the recovered matrix body).
- Event 1133 (from `TownSystem.OnReturnFromQuest`, `0x1169b04`, when `battleResult == 1`): the extracted table has no id 1133 and the id-indexed array model has length 229, so the guard would return null without effect. Conditional on the loader chain being the one modelled; not classified as penalty/message/no-op.

### Automatic selection — `ExecEvent` `0x110c580`
- Ascending scan; re-reads `eventData` and its length every iteration (no eligibility snapshot).
- Skips null records; `CheckTerm(index)` true → `ExecEvent(index, null, null)`. Returns early **only** if that call returns non-null. Null-returning executions (which may have changed money, `execNum_`, the queue) do not stop the scan — not "run the first eligible event".
- In `SelectForm.Update`, the call is gated by static byte `SelectForm+0x31`; non-null result jumps to `0x11ecb30`, null result clears the byte at `0x11ec7e8`. Flag identity/producers unknown; not a per-frame call.

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
- No match: walks array `AppData+0x1e8` without producing an id (null array faults), optional diagnostic, returns -1. -1 does not cancel scheduling; the `DelayEvent` is still queued and fails later on resolution.
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
- `0x11ebde8`: if field +0xc0 non-null, clear it and return (meaning unknown).
- `0x11ebe1c`: dispatch on +0xf8 selects state paths. On the path reaching `0x11ec49c`: `UpdateNotifyValue` then `TapjoyForm.AcquireBalance`; true → return before the queue.
- `AcquireBalance` (`0x11b72a4`): int at +0x5c of the object from `0x1185ef4` `< 1` → false; a FormManager form of the queried type with +0xe8 == 0 → false; otherwise allocates/constructs that form, inserts it into the manager, returns true. No network access in this body; source/unit of +0x5c unknown.
- Then `SetActiveX`, `UpdateDecoObject`, `UpdateTownHuman`. Test `+0xd0 > 2` at `0x11ec4e0` skips the event/guide section but lands at `0x11ec570`, right before the queue — it does not disable the queue.
- So not every `Update` call is a queue pass.

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

## Port

- `src/core/gameplay/event_execution.{hpp,cpp}` ? `execute_indexed_event` and `select_and_execute_events` compose the id-indexed catalog, condition matrix evaluator and script executor. `EventExecutionHost` exposes the current catalog, default SecureString args and the empty-script diagnostic, alongside the existing effect/delay callbacks. Catalog replacements are observed at the original getter boundaries. Selection's eligibility check retains the terms record while re-reading the execution-count record, matching `CheckTerm(int)` at `0x110b49c`/`0x110b4d4`; the earlier standalone `check_event_eligibility` remains a stable-catalog convenience. Tests in `tests/test_event_execution.cpp` compose `DecodedEventConditionHost` and synthetic effects: counter overflow, empty effects, explicit/default args, preparation failure, distinct counter source/destination, dispatch failure without rollback, null gaps, changed eligibility after prior effects, a form stopping selection, a delay not stopping selection, and catalog replacement/growth. This does not supply the SelectForm scheduling guards or populate live state from an Android save.

`DecodedEventConditionState` and `DecodedEventConditionHost` provide concrete bindings for the 14 table-used predicates. The host observes the supplied state by reference, including catalog updates; values absent from the decoded state remain absent and fail when queried. It distinguishes a missing AppData context, missing/empty characters storage, null catalogs/records, and valid zero values. `hero_level` is the result of the MemberSystem/Entity/HumanComponent lookup, not an invented default hero. `debug_enabled=false` returns a zero adjustment without reading `debug_rescue_count`; true requires that value. The conditional bonus is read after quest enumeration. Unknown condition opcodes fail explicitly. Tests cover absent scalars, bootstrap checks, DEBUG gating, scalar bindings and conditions changing after area discovery. The state must still be populated by the game-data/save/ECS layers; this host is not an integrated original-game loader.

- `src/core/gameplay/event_conditions.{hpp,cpp}` ? `check_event_eligibility`, `check_event_conditions`, `compare_event_values`, and the 14 table-used branches of `check_event_condition`. `EventConditionHost` supplies context checks, decoded catalogs and the building list. Unrecovered opcodes are delegated explicitly; the test host rejects them. The eligibility boundary accepts one stable catalog per call, preserving null gaps and script references; original repeated singleton/catalog retrieval is not reproduced. `tests/test_event_conditions.cpp` (`event_conditions_contract`) covers empty-false, AND short-circuit, negative versus positive execution counts, all comparison codes, completion flags, wrapped signed quest counters, rescue null handling and adjustment, scalar widths, town capacity/count ordering, recruitment filters, lookup order and building enumeration after a match. Invalid input uses C++ exceptions rather than managed exception objects. The unit method remains `traced` in the registry because its other handlers are unresolved.

- `src/core/gameplay/event_flow.{hpp,cpp}` — `execute_event_script` (= `0x110c8ec`, including opcode 10 / `DelayEvent` capture with `argi` clone) and `process_counted_event_delays` (= the `SelectForm.Update` queue block; one call = one reached pass). `EventFlowHost` must supply unit dispatch, `identify_script` (by reference), `resolve_script`, talk-character read/restore. Test `tests/test_event_flow.cpp` (CTest `event_flow_contract`): effect/delay/resume/re-delay chain, callback order, identity, null vs empty, explicit hour untouched, wait 0/negative, `INT_MIN` wrap, adjacent removals, row errors, resolve failure after removal, caller reference replacement, queue mutation by callback.
- `src/core/gameplay/event_progression.{hpp,cpp}` — `resolve_event_arguments` (sentinel substitution) and `apply_event_progression` (opcodes 1/2/4/15 on decoded `EventProgression`; `crystal_subtraction_mode` must be supplied, no default). Test `tests/test_event_progression.cpp` (`event_progression_contract`).
- `src/core/gameplay/event_dispatch.{hpp,cpp}` — `stamina_limit_from_item_state`, `event_stamina_balance`, `dispatch_known_event_instruction` (opcodes 1/2/4/8/10/12/15/31; 10 is a no-op; others return `handled=false`). `EventEffectsHost` supplies progression state, stamina read/write/limit, form creation and push. Test `tests/test_event_dispatch.cpp` (`event_dispatch_contract`); `tests/test_presentation_forms.cpp` also drives the dispatcher.
- `src/core/gameplay/event_catalog.{hpp,cpp}` ? record loading, id indexing, identity lookup and delay script resolution. Accepts decoded UTF-8 text, an explicit CRLF-normalization gate and an integer parser callback; `parse_event_decimal` implements the observed ASCII decimal subset. The host remains responsible for invalid UTF-8/culture policy. `identify_event_script` scans non-null records by shared script identity (including null), invokes an explicit fallback callback when no match exists, then returns -1. The callback must reproduce the original +0x1e8 access or fail when unavailable. `resolve_event_script` preserves the >999 guard, signed modulo, negative-multiple-of-1000 behavior and null-record failures. Tests: `event_catalog_contract`.
- `src/platform/desktop/event_trace.cpp` ? research harness protocol `SAGA_EVENT_TRACE_2`: event id, reached queue passes, fixture balances/settings and the hex-encoded original table bytes. The tool uses the C++ loader and id-based delay lookup. Conditions and ExecEvent counters are not yet invoked; initial execution is injected directly into the matrix executor. Unknown effects and missing discovery state fail; form requests are logged.
- `tools/run_event_scenario.py` validates UTF-8 and passes the intact table to the native loader. `--id` replaces the former catalog-position `--index`. Example (local data):
  ```
  python -I tools/run_event_scenario.py extracted/members/06_xls/0007_English.lproj_Event.txt --exe build/desktop/event_trace.exe --id 10 --passes 50 --money 7 --crystals 11 --crystal-mode 10000 --stamina 3 --item-state 0 --talk-character 2 --output research/exports/event-catalog-scenario.json
  ```
  Record 10 still produces money 8007, crystals 1011 and an empty queue after pass 50. A synthetic table with records 0 and 181 resumes id 181 correctly across the gap: wait 1, then money 7 -> 27. Evidence: `research/exports/event-catalog-scenario.json`, `event-catalog-gap-scenario.json`. Neither executes the original APK.
- Last recorded build: Windows/MSVC, 23/23 CTest, including `event_execution_contract`.
- Missing: concrete default-args and diagnostic storage bindings, SelectForm integration, remaining unit `CheckTerm` predicates and concrete condition host; `GetDelayId` fallback context and integration with save-loaded execution counters; hour-based delays; `SubForm.delayEvent_` path (slot 5); `SelectForm.Update` guards; opcodes other than the eight; `SecureLong`/`SecureInt` storage; billing item source for stamina; UI forms.

## Evidence

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
2. (blocking) Gate flag `SelectForm+0x31` (producers) and the full set of `SelectForm.Update` guards (+0xc0, +0xf8 states, `AcquireBalance` +0x5c source) that decide when selection and queue passes run.
3. (blocking) Initial/real values of `SYSSAVEI_YUGAMI`, the `ITEM_STAMINA_UP` item state, and the money/crystal/stamina slots from save/new game.
4. Unit dispatcher opcodes other than 1/2/4/8/10/12/15/31 (jump table has 15 entries); scripts outside `Event.txt` (quests, built events) may use them.
5. Consumer of `DelayEvent` entries with `hour_ != -1` (none in the extracted table).
6. Writer of `SubForm.delayEvent_` (+0x1d0) and why `Finish` restores slot 5 instead of 7.
7. Meaning of state array elements 5 and 7 (talk character slots) and of `AppData+0x1e8` in `GetDelayId`'s fallback.
8. Event 1133: confirm the effective `eventData` length at runtime (array model gives 229).
9. Meaning of `timing_` (0 in every extracted record) and its consumer.
