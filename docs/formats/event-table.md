# Event table (Event.txt)

`Event.txt` is the tab-separated text table loaded by `Data.Load` into `Data.eventData` (`Data+0x38`) via `LoadData<EventData>`; each line becomes one `data.EventData` through `EventData.Load` (`0x116fa4c`) and `StringArrayStream`. Each record carries an id, a name, a timing value and two integer script matrices: `terms` (conditions) and `execs` (effects). The record layout and the eight effect opcodes used by the extracted table are recovered statically. The 14 condition opcodes used by this table are documented and ported behind explicit data-host queries; see [events](../subsystems/events.md). File encoding, line splitting and id-indexing are shared with the other game tables, see [game-tables.md](game-tables.md).

## Layout

### Container

- UTF-8 (BOM, if present, skipped), lines split on LF. CRLF→LF normalisation only when Unity ≥ 6000.0.0; this build is Unity 2022, so each line keeps its trailing CR. No trim. Details in [game-tables.md](game-tables.md).
- Records are stored in an array indexed by `id` (length `maxId+1`, gaps null, duplicates: last wins, negative id faults). The extracted table: 228 lines, ids 0..228, array length 229, gap at 180, no duplicates.
- Resource name `Event.txt` (string literal, GOT `0x2731d30`; name slot 5 ↔ `Data+0x38`). Extracted copies `English.lproj_Event.txt` and `Japanese.lproj_Event.txt` are byte-identical (SHA-256 `b625c2892879394ee150563771a09230b2176e29b7c670198fd7cb43bb5ca9fa`).

### Tokenizer (`StringArrayStream`)

| Method | VA | Contract |
|---|---|---|
| `..ctor` | `0x1126cb4` | `Split` on TAB (GOT `0x272feb8`), flags 0/0/0: empty fields kept, no trim, no escapes |
| `GetString` | `0x1126dcc` | Next token; cursor advances before the access check |
| `GetInt` | `0x1126e4c` | Next token → `JInteger.ParseInt` → `System.Int32.Parse`; cursor advances first; empty token is an error, not 0 |
| `GetIntDoubleArray` | `0x112708c` | Outer count; negative → null; 0 → empty array (≠ null); else per row: size, then `size` ints. Negative inner size reaches an invalid allocation (not a null row) |

### Record (`EventData.Load`)

| # | Field | Read with | Stored at | Notes |
|---|---|---|---|---|
| 1 | `id_` | `GetInt` | +0x18 | Array index |
| 2 | `name_` | `GetString` | +0x28 | May be empty |
| 3 | `timing_` | `GetInt` | +0x30 | 0 in all 228 extracted records; consumer unknown |
| 4 | `termsScript_` | `GetIntDoubleArray` | +0x38 | `count, (size, ints…)*` |
| 5 | `execsScript_` | `GetIntDoubleArray` | +0x40 | same encoding |

Not read from the file: `execNum_` (+0x20, execution counter; `isExecuted` = `execNum_ > 0`). Tokens after the second matrix are not rejected. Each script row is `[opcode, arg1, arg2, …]`.

Extracted table statistics: no null matrices; 113 records with empty `terms`, 115 non-empty; 208 `execs` rows; no trailing tokens.

### Argument sentinels

In `execs` rows (and in the unit dispatcher only), arguments (positions ≥ 1) in `INT_MIN..INT_MIN+99` are replaced by `argi[v ^ 0x80000000]` from the call context; one pass, opcode never substituted.

### Effect opcodes (`execs`, unit dispatcher `AppData.ExecuteScript` `0x110cc4c`)

Jump table `0x627b6e`: 15 uint16 entries, target = `0x110cee0 + 4·entry`. Counts = rows in the extracted table (total 208).

| Op | Count | Name (descriptive) | Arguments | Effect | Target | Port |
|---|---:|---|---|---|---|---|
| 1 | 3 | money | amount | ≥0 `AddMoney` (wrap64); <0 negate in 32 bits → `SubMoney` (floor 0) | `0x110cee0` | `event_progression` |
| 2 | 6 | crystals | amount | ≥0 `AddCrystal`; <0 negate → `SubCrystal` (×10000 wrap32 unless `SYSSAVEI_YUGAMI`==10000, floor 0) | `0x110cfdc` | `event_progression` |
| 4 | 8 | discover area | area id | `areaData[id]`; if `state != 1` → `state=1, new=true` | `0x110cff8` | `event_progression` |
| 8 | 57 | talk | talk id | `SubForm.MakeTalkForm(id, args)` → `AppData.Push`; returns form | `0x110d070` | `event_dispatch` (host creates form) |
| 10 | 37 | delay | wait, [hour] | Matrix executor only: queue `DelayEvent(line+1, wait, hour or -1)` and stop; unit dispatcher: no-op | common return | `event_flow` |
| 12 | 22 | event message | eventMsg id | `SubForm.MakeEvent(id, args)` → `AppData.Push`; returns form | `0x110d100` | `event_dispatch` (host creates form) |
| 15 | 74 | discover quest | quest id | `questData[id]`; only `state == 0` → `state=1, new=true` | `0x110d258` | `event_progression` |
| 31 | 1 | stamina % | percent | `delta = wrap32(pct·max)/100`; add (wrap64), clamp to max (10/20), floor 0 | separate branch `0x110cefc` | `event_dispatch` |

Other opcodes (0, 3, 5–7, 9, 11, 13, 14, and above 15 except 31): not in the extracted table, semantics unknown; the port returns "unhandled". Opcode 10 usage: wait 1 ×1, 3 ×1, 5 ×21, 30 ×10, 50 ×4; none has a third (hour) argument.

### Condition opcodes (`terms`, unit evaluator `AppData.CheckTerm` `0x110b5f8`)

Jump table `0x627ae0`: 32 uint16 entries. Matrix semantics: null/empty → false; otherwise AND with short-circuit. Comparator codes (`AppData.Compare`): 0 `>`, 1 `>=`, 2 `<=`, 3 `<`, 4 `==`, 5 `!=`, other → false (signed 64-bit).

| Op | Count | Target | Semantics |
|---|---:|---|---|
| 1 | 3 | `0x110b7e0` | int64 money comparison |
| 2 | 2 | `0x110b848` | int64 crystal comparison |
| 4 | 8 | `0x110b8c4` | area state == 1 |
| 5 | 1 | `0x110b930` | hero level int32 comparison |
| 7 | 1 | `0x110b9b8` | rescued count plus DEBUG-gated debugRescueNum |
| 15 | 75 | `0x110bad4` | quest completion bit or boolean |
| 16 | 29 | `0x110bb40` | sum quest challengeCount |
| 18 | 1 | `0x110bc98` | sum quest clearCount |
| 19 | 1 | `0x110bce0` | sum quest challengeCount - clearCount |
| 20 | 7 | `0x110bd28` | count matching building type > 0 |
| 21 | 3 | `0x110be00` | town capacity minus flagged town count |
| 22 | 10 | `0x110bf48` | count recruited sub characters |
| 31 | 2 | `0x110c058` | stamina low int32 comparison |
| 32 | 1 | `0x110c0ac` | int64 high score comparison |

Condition and effect opcode numbers are separate spaces (e.g. condition 15 ≠ effect 15).

## Reading it

- `tools/inspect_event_table.py <Event.txt> --output <json>`: research reader of the recovered layout. Splits on LF without CRLF normalisation (CR preserved), TAB fields, explicit int32 bounds, accepts only the observed decimal/ASCII-whitespace subset of `Int32.Parse`; reports trailing tokens, timings, null matrices, exec row count and SHA-256. It does not emulate cultures, all `Int32.Parse` errors, cursor state after failure or runtime allocation exceptions, and does not apply id-indexing (records are in file order).
- Validated on both extracted tables (228 records each, all lines fully consumed, trailing CR harmless in this subset) and on synthetic cases: empty name, null matrix, int32 limits, trailing tokens, truncation, negative inner size, integer overflow. Tool-level checks only; no comparison with the game's loaded array.
- `tools/run_event_scenario.py` reuses `parse_row` to feed `execs` matrices to `event_trace` (see [../subsystems/events.md](../subsystems/events.md)).
- No C++ loader yet.
- Outputs (`build/research/english-events-parsed.json`, `japanese-events-parsed.json`, `event-opcode-usage.json`, `event-delay-usage.json`) contain proprietary data and stay local.

## Port

`src/core/gameplay/event_catalog.cpp` reads records into `EventConditionRecord` (including zero-initialized execution count), preserves null versus empty matrices, and indexes by id with null gaps and last-duplicate-wins semantics. Script identity is retained for delays. The integer parser is injected; `parse_event_decimal` is the observed ASCII decimal/whitespace subset, not a full culture-sensitive Int32.Parse implementation. The reader consumes decoded valid UTF-8 supplied by the host, ignores unused trailing fields and takes the Unity-version CRLF gate explicitly. C++ exception types/allocation-failure timing are not claimed to match managed exceptions. `tests/test_event_catalog.cpp` covers indexing, matrix states, identity, CRLF gates, signed boundaries and malformed input; the native trace also reproduces the existing record-10 fixture.

## Evidence

- [../archive/reports/72-return-event-dispatch-and-data-gap.md](../archive/reports/72-return-event-dispatch-and-data-gap.md), [76-event-table-tokenization.md](../archive/reports/76-event-table-tokenization.md), [77-event-opcodes-and-delay.md](../archive/reports/77-event-opcodes-and-delay.md), [81-delay-origin-and-script-identity.md](../archive/reports/81-delay-origin-and-script-identity.md), [83-event-progression-effects.md](../archive/reports/83-event-progression-effects.md), [84-stamina-and-event-dispatch.md](../archive/reports/84-stamina-and-event-dispatch.md), [110-automatic-event-selection.md](../archive/reports/110-automatic-event-selection.md)
- Shared table mechanics: [73-game-data-id-indexing.md](../archive/reports/73-game-data-id-indexing.md), [74-game-data-text-source.md](../archive/reports/74-game-data-text-source.md), [75-table-encoding-and-line-endings.md](../archive/reports/75-table-encoding-and-line-endings.md)
- Local: `build/research/event-table-audit.json`, `event-tokenizer/`, `event-int/`, `event-loading/`, `event-index-model.json`

## Open questions

1. (blocking) Concrete host data for the 14 recovered condition opcodes; integration with the id-indexed event loader.
2. The generic delegate `<LoadData>b__74_0` that calls `EventData.Load` and its policy for invalid/null lines (see [game-tables.md](game-tables.md)).
3. Meaning of `timing_`.
4. Effect opcodes absent from the extracted table; whether other script sources (quests, built events) use them.
5. Exact `Int32.Parse` behaviour on inputs outside the observed subset (culture, invalid UTF-8).
