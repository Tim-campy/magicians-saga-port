# Data

Static game data lives in `data.Data` as arrays of `data.BaseData` subclasses, loaded from 21 UTF-8 text tables (`Event.txt`, `Cloth.txt`, `Voice.txt`, ...) read through `JarInflater`, split on LF and indexed by record id. Ten of those arrays form the *data list* used for new-game reset (`BaseData.NewGame`), save-section deserialisation and check-script grouping. Indexing, line splitting and encoding are established statically; the per-row record constructor (`<LoadData>b__74_0`) and field tokenizers are not. No data loader exists in the port.

## Overview

Load chain (text → arrays):

1. `Data.Load(arg)` (`0x1170508`) builds a `JarInflater` from its argument and calls `Data.Load(jar)` (`0x117057c`).
2. That overload builds 21 table names, calls `StringUtil.LoadStringArray(jar, name, true)` for each, and passes the string arrays to `Data.Load(string[][])` (`0x1170974`).
3. For each table, the third overload picks the destination field of `Data` and a generic `LoadData<T>` instantiation (method 38355), e.g. position 5 = `Event.txt` → `Data+0x38`, `LoadData<EventData>` (GOT `0x2731db0`, usage `0xc001c065`, instantiation 3820).
4. Shared `LoadData` body (`0x129fbe8`) maps the line sequence through delegate `<LoadData>b__74_0` (method 38361, type `<>c__74`1`), materialises an array and passes it to the id-indexing helper `0x129fafc`, whose result is stored in the field.

Which container the caller passes and how the language is selected are not confirmed. Text format and indexing: [../formats/game-tables.md](../formats/game-tables.md).

Data list (`AppData.GetDataList`, `0x11023fc`, 1872 bytes): a list of 10 descriptors, each `{BaseData-subclass array from a Data getter, JInteger selector}`. `JInteger..ctor` (`0x1b11454`) stores the value at `+0x10`. The selector is the SecureProperty block index used by `DeserializeData` (see [save.md](save.md)), not the ordinal.

| Order | Getter | Array type | `Data` field | Selector |
|---:|---|---|---|---:|
| 0 | `get_buildingData` | `BuildingData[]` | `+0x20` | 11 |
| 1 | `get_charaData` | `CharaData[]` | `+0x90` | 0 |
| 2 | `get_areaData` | `AreaData[]` | `+0x58` | 7 |
| 3 | `get_magicShopData` | `MagicShopData[]` | `+0xa0` | 8 |
| 4 | `get_questData` | `QuestData[]` | `+0x10` | 9 |
| 5 | `get_equipmentData` | `EquipmentData[]` | `+0x30` | 10 |
| 6 | `get_eventData` | `EventData[]` | `+0x38` | 14 |
| 7 | `get_hatData` | `HatData[]` | `+0x88` | 15 |
| 8 | `get_clothData` | `ClothData[]` | `+0x28` | 16 |
| 9 | `get_kingRewardData` | `KingRewardData[]` | `+0x78` | 17 |

Types were resolved from metadata return types and SZARRAY element pointers, not getter names. Each getter (100 bytes) runs the class-init guard and returns the field of the object in `Data`'s first static field; none allocates records. Order matters for both reset and deserialisation.

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `data.Data.Load` (arg) | `game:0x06001153` | Builds JarInflater, forwards | high |
| `data.Data.Load` (jar) | `game:0x06001154` | 21 names → `LoadStringArray` | high |
| `data.Data.Load` (arrays) | `game:0x06001155` | Per-table `LoadData<T>` → `Data` fields | medium |
| `data.Data.get_<x>Data` ×10 (list above) | see TSV | Return `Data` array fields | high |
| `main.AppData.GetDataList` | `game:0x06000194` | 10 descriptors (array, selector) | high |
| `main.AppData.NewData` | `game:0x0600015b` | `BaseData.NewGame` on every list object; tail `InitCheckScriptData` | high |
| `main.AppData.DeserializeData` | `game:0x06000190` | Reads BaseData save sections | medium |
| `main.AppData.InitCheckScriptData` | `game:0x0600015c` | Builds check-script groups `AppData+0x248` | high |
| `main.Util.ArrayInit` (int[], int) | `game:0x06000281` | Fills int array with value | high |
| `java.util.JTool.MakeArrayRecursive` | `kairo:0x060000cf` | Recursive jagged array allocation | medium |
| `java.lang.JInteger..ctor` | `kairo:0x06000175` | Stores int at `+0x10` | high |
| `data.BaseData.NewGame` | `game:0x06001038` | Virtual slot 18; base = RET | high |
| `data.BaseData.Find` | `game:0x06001027` | Slot 8; `+0x10 = 1`, `byte+0x14 = arg&1` | high |
| `data.QuestData.Find` | `game:0x060011f0` | Sets `+0x10`/`+0x14` to 1 only if `+0x10 == 0` | high |
| `data.BaseData.GetTiming` | `game:0x0600102d` | Slot 14; returns 0 | high |
| `data.BaseData.CheckTerm` / `CheckNotFind` | `game:0x0600102e`, `game:0x0600102f` | Slots 15/16; return 0 | high |
| `data.BaseData.Deserialize` / `Deserialized` / `DeserializeShare` | `game:0x06001029`, `game:0x0600102a`, `game:0x0600102c` | Slots 10/11/13 used by `DeserializeData` | medium |
| `<Type>Data.NewGame` overrides (9) + `AreaData.oldNewGame` | see table below | Per-type reset | medium |
| `game.QuestRecord..ctor` | `game:0x06000661` | Forwards to `Object..ctor` | high |
| `StringUtil.LoadStringArray` (jar,name,flag) / (stream) | `kairo:0x06000610`, `kairo:0x06000612` | Table file → lines | high |
| `StringUtil.Split` (impl) | `kairo:0x06000602` | LF split, empty-segment drop, version-gated CRLF | medium |
| `JarInflater.GetInputStream` | `kairo:0x060003c8` | Opens a table member | medium |
| `UnityUtil.CheckUnityVersion` | `kairo:0x0600067b` | Runtime Unity version ≥ threshold | high |
| `java.lang.JString.New` (bytes) | `kairo:0x060001b7` | UTF-8 decode, BOM strip | high |

## Behaviour

### DeserializeData (`0x11077b4`, 956 bytes)

Arguments: descriptor table, `SecureProperty`, bool. Per descriptor: element 0 cast to `BaseData[]` (GOT `0x272fb08` → native type 8031, SZARRAY of definition 4503 `data.BaseData`); element 1 → string → `JInteger.ParseInt` → selector → block in `SecureProperty+0x10`. For a non-null block, a `ByteArrayInputStream` and:

1. `ReadShort`, sign-extended → record count; `<1` skips.
2. Per record: int16 (signed) index, int32 size.
3. Index `>=` array length or null slot → stream slot 13 with size widened to 64 bits (probably skip; unconfirmed). Negative index fails the unsigned bounds check (exception), it is not skipped.
4. Object present: flag false → `Deserialize` (slot 10) then `Deserialized` (slot 11); true → `DeserializeShare` (slot 13) only. Overrides may apply.
5. The int32 size does **not** bound the stream given to `Deserialize`; each implementation consumes its own payload. Do not wrap records in a limited substream.
6. End of section: stream slot 6 (probably close).

`LoadGame` passes `false`. Layout in [../formats/save.md](../formats/save.md#basedata-section).

### NewData and NewGame overrides

`NewData` (`0x11022e4`, 280 bytes) calls `GetDataList` (`0x1102318`), casts each array to `BaseData[]` and calls slot 18 `NewGame` on each non-null object; then tail-calls `InitCheckScriptData`. Must stay virtual; do not replace by zeroing fields.

22 types derive from (or are) BaseData; 10 distinct slot-18 bodies. 12 inherit the RET (BulletData, EnemyData, EventData, EventMsgData, ManualData, NewsData, TableData, TalkData, TownData, TreasureData, UnlockShopData, VoiceData). Relevant to the data list: 9 overrides plus EventData's RET.

| Type | Id | VA | Body |
|---|---|---|---|
| AreaData | `game:0x06001012` | `0x116bf68` | `Find(true)` if bit 0 of `+0x1c`; then `oldNewGame` |
| BuildingData | `game:0x0600103a` | `0x116c868` | `Find(true)` under static mask; copy `+0x48`→`+0x20` |
| CharaData | `game:0x0600106d` | `0x116d628` | Recreates arrays, copies fields; 3-int array = `[0,300,0]`; conditional `Find(true)` |
| ClothData | `game:0x060010a9` | `0x116e458` | `Find(false)` if bit 0; `+0x4c`→`+0x20`, bit 1→`+0x24` |
| EquipmentData | `game:0x060010ed` | `0x116f0c8` | `Find(false)` if bit 0; `+0x80`→`+0x28`, bit 1→`+0x2c` |
| HatData | `game:0x06001176` | `0x1171894` | `Find(false)` if bit 0; `+0x44`→`+0x60`, bit 1→`+0x64` |
| KingRewardData | `game:0x0600118d` | `0x1171d64` | Zeroes byte `+0x48` |
| MagicShopData | `game:0x0600119d` | `0x1172014` | `Find(true)` under static mask; zero `+0x40` |
| QuestData | `game:0x060011ee` | `0x1172e88` | Conditional `Find(true)`; zero byte `+0xa4`; new `QuestRecord` at `+0xa8` |

- `Find` is slot 8 (`class+0x1b8`); 21 types use `BaseData.Find` (`0x116c790`, 20 bytes), QuestData overrides it (`0x1172f0c`, 28 bytes, ignores its argument). Find has side effects; it is not a lookup.
- `AreaData.oldNewGame` (`0x116bf9c`, 576 bytes): allocates and zeroes 3×6 int32 at `+0x20`, 3×2 at `+0x38`, 6 int32 at `+0x28`; zero int `+0x30`; finally writes bit 0 of `+0x1c` into `+0x10` (overwriting what Find wrote).
- Mask values and field semantics are open. None of these resets touches the ECS world.

### Check-script groups

`InitCheckScriptData` (`0x1102b4c`, 1060 bytes): calls `GetDataList`, zeroes `AppData+0x238`, allocates int32[5] at `AppData+0x240`, calls `JTool.MakeArray<BaseData[][]>([5,0])` (method 13203, instantiation 3905, shared body `0x12d0570`) → `AppData+0x248`.

- `MakeArray`: `Type.GetTypeFromHandle`, assembly + full name, drops last two chars (`Substring`) to get the element type, `MakeArrayRecursive(type, dims, 0)` (`0x1b0572c`), type-checks the result. Empty dims → null → error helper. `MakeArrayRecursive` uses `Array.CreateInstance`, and recursively fills each slot with a fresh row (`Array.SetValue`) when a further dimension exists. Result for `[5,0]`: 5 distinct empty `BaseData[]` (not nulls, not shared).
- Loop `i = 1 .. len(+0x248)-1`: over descriptors in order and their non-null objects, collect those whose `GetTiming()` (slot 14, `class+0x218`) equals `i` (Vector `AddElement`/`ElementAt`/`Size`, GOT `0x272faf8`/`0x272fb38`/`0x272fb40`); copy to a new `BaseData[]` at row `i`, store its length in `+0x240[i]`. Row 0 keeps the empty row.
- `GetTiming` returns 0 for all 22 types → rows 1–4 empty in this build. This is not a global script-disable; keep the code.
- End: `Util.ArrayInit(AppData+0x2e8, 10000)` (`0x117eb08`, writes the value into every int32; null → error helper). Meaning of 10000 unknown.
- Consumer: `NewGame` at `0x11020b0` (see [save.md](save.md#newgame-0x1101d58-1420-bytes)); `CheckNotFind`/`CheckTerm` are `MOV W0, WZR; RET`. A bounded LDR/STR scan of AppData for `0x238/0x240/0x248/0x2e8` found no other consumers (not exhaustive).

### Id indexing, text source, encoding

See [../formats/game-tables.md](../formats/game-tables.md): `LoadStringArray` → `JString.New` (UTF-8, BOM stripped) → `Split(LF, flags 0,1,0)` (no trim, no escaped separators, zero-length segments dropped); CRLF → LF only when Unity ≥ 6000.0.0; array sized `maxId+1`, gaps null, last duplicate wins, negative ids throw.

### Progression record readers

`TableData.Load` (`game:0x0600120a`, `0x11732d0`, 280 bytes) and `CharaData.Load` (`game:0x060010a0`, `0x116dcc4`, 424 bytes) take a line and mutate/return the receiver. The initial string guard returns without reading fields; otherwise they construct `StringArrayStream` (`0x1126cb4`). Each read is immediately stored, so a later parse failure can leave an existing receiver partly modified. Extra tokens are ignored. Caller: the per-type data-loading path; the generic delegate body remains unresolved.

`TableData.Load` reads ten int32 tokens in order: id (`+0x18`), level (`+0x20`), exp (`+0x24`), shopParam1/2/3 (`+0x28/+0x2c/+0x30`), unlockMoney (`+0x34`), unlockCrystal (`+0x38`), unlockWebPage (`+0x3c`), unlockRescue (`+0x40`). The progression kernel indexes by id, not by the separately stored level value.

`CharaData.Load` reads: id; baseName (string); lockLevel; res (string); img; res2 (string); weaponId; recruitType; recruitCost; initLevel; rank; needExpRate; parameters (count-prefixed int32[][]); voiceId; townId; areaId; residentTerm (count-prefixed int32[][]); appearRate; inherited flag (`+0x1c`). The first twelve logical fields are fixed tokens; nested arrays make later token positions variable. needExpRate is token 11 (zero-based), stored at `+0x9c`. Saved level/exp and witchExp are not loaded here.

Confidence: high for read/store order from bounded ARM64 instructions and metadata field offsets. Calls are GetInt `0x1126e4c`, GetString `0x1126dcc`, GetIntDoubleArray `0x112708c`. These game record mappings require no new Mono/runtime implementation.

### Combat table projections

The existing character reader now retains weaponId, initLevel, parameters and the inherited int32 flags as well as the experience rate. It still consumes the full CharaData schema once; it does not duplicate the parser. `bind_character_parameters(record,saved_level)` shares the parameter array, uses the explicitly supplied saved level, and exposes the low flags byte used by `get_isMain`. initLevel is not substituted for saved level.

Additional loaders in `combat_tables.cpp` consume these schemas (int32 unless marked string or array):

| Original method | ELF VA / bytes | Sequential reads |
|---|---|---|
| MagicShopData.Load (game:0x0600119c) | 0x1171f2c / 232 | id, name string, category, basePrice, upValue, explain string, inherited flag |
| EquipmentData.Load (game:0x0600111e) | 0x116f428 / 504 | id, name string, four integers (+0x38..+0x44), resource string, two integers (+0x50/+0x54), attribute (+0x58), eleven integers (+0x5c..+0x84), status int32[][], explain string, inherited flag |
| ClothData.Load (game:0x060010c2) | 0x116e704 / 312 | id, name string, integer (+0x30), resource string, five integers (+0x40..+0x50), parameters int32[][], explain string, inherited flag |
| HatData.Load (game:0x06001171) | 0x1171734 / 312 | same token framing as cloth, different object offsets (+0x28, +0x38..+0x48, parameters +0x50) |

Each Load takes a line, mutates and returns its receiver, initially bypasses empty-string input, and stores immediately after each read. Its caller is the per-type data-loader route. Combat projections retain only id, parameter endpoints, attribute where present, flags, and shop category/upValue; unrelated presentation/economy tokens are still parsed so malformed required numeric tokens fail. The fresh-record port does not reproduce observable partial mutation of a pre-existing receiver. Therefore these original Load methods remain documented, not fully reimplemented.

Shop buyCount and equipment level are mutable state absent from these records. Binding functions require explicit values; they neither assign a new-game state nor fabricate save progress. The loaders reuse `TableTokens` and `load_table_records`, preserving null outer arrays, id gaps and last duplicate wins.

Local shipped-table validation via `test_combat_tables` read both English and Japanese Chara/MagicShop/Equipment/Cloth/Hat files: each language has 67 populated character records, 8 shop records and 269 equipment records across the three equipment tables. Across selected levels and all 13 parameter indices, 17,467 numeric queries per language produced the same FNV-style hash 13925746254597776912. Shop arithmetic uses explicit synthetic purchase counts (0 for character queries, 3 for standalone shop queries), not recovered saves. This is data/contract integration, not original-runtime verification. Evidence: `research/exports/combat-loaders/` bounded exports and `game-progression-tables/F116dcc4.asm`; all game tables remain local.

### Treasure records

`TreasureData.Load` (`game:0x06001235`, `0x11737a4`, 312 bytes) consumes twelve logical fields: id, name (string), res (string), img, exp, money, crystal, weaponId, charaId, clothId, hatId, inherited flag. Numeric fields are int32. Stores target `+0x18`, `+0x20`, `+0x28`, `+0x30`, `+0x34`, `+0x38`, `+0x3c`, `+0x40`, `+0x44`, `+0x48`, `+0x4c`, `+0x1c`, respectively. It returns the receiver; the initial guard bypasses parsing, reads/stores are sequential and extra fields are ignored. Its data-loading caller and partial-mutation caveat are the same as the progression readers above.

`load_treasure_table` in `quest_rewards.cpp` retains all these fields using the shared `table_catalog.hpp` framing/id-indexing helper. Both extracted languages load 184 populated records into 184 slots. Confidence high: bounded instructions in local `research/exports/quest-reward-pipeline/` and metadata field offsets. The fresh-record reader does not implement in-place partial receiver mutation on parse failure.

### Enemy records

`EnemyData.Load` (`game:0x060010e3`, `0x116ec2c`, 344 bytes) reads id, name, enemySeb, exp (int32[]), treasureDrop (int32[][][]), voiceId, attribute, res (string), img, seb, frame, rank, parameters (int32[][]), and inherited flag. Fields map to +0x18, +0x28, +0x30, +0x38, +0x40, +0x48, +0x4c, +0x50, +0x58, +0x5c, +0x60, +0x64, +0x68 and +0x1c. defeatCount_ (+0x20) is separate mutable state and is not loaded here. The method mutates/returns the receiver in sequential read/store order; the initial string guard bypasses reads and extra tokens are ignored.

`GetIntArray` (`0x1126fd8`) permits any negative outer length as null. `GetIntTripleArray` (`0x11271bc`) also permits a negative outer length as null, but allocates nested group/row lengths directly: negative inner lengths fault, rather than creating null inner arrays. Zero lengths remain distinct from null. `load_enemy_table` shares the existing token/framing/id-indexing readers and retains the entire static record schema. As with other fresh-record loaders, it does not expose in-place partial mutations after a parse failure.

Both English and Japanese shipped tables decode to 83 populated slots. Evidence: bounded exports `research/exports/enemy-rewards/` and `enemy-reward-readers/`, metadata field offsets, plus synthetic schema/null/negative-inner-size tests. Confidence high. Runtime-added null inner arrays are not represented by the decoded-record projection.

## Port

`src/core/gameplay/progression_tables.cpp` loads complete level-table records and a character progression projection, preserving id gaps and last-duplicate-wins indexing. It shares token parsing with the event reader and accepts the existing integer-parser callback. `TableExperienceHost` resolves the live character id/rate and level row for the experience kernel. The character projection consumes and validates all fields but retains only id and needExpRate. These fresh-record readers do not expose partial mutation of an existing record on failure; neither original Load method is claimed fully reimplemented. BaseData lifecycle, the general loader, data list and check-script groups remain unported. `tools/audit_game_table_text.py` audits the extracted tables (encoding/line endings) only; `tools/inspect_event_table.py` belongs to the events subsystem.

## Evidence

- Local `research/exports/game-progression-tables/`: bounded ARM64 exports of both record loaders. `tests/test_progression_tables.cpp` covers field mapping, arrays, gaps, duplicates, CRLF gates, live rate/id changes and progression failure ordering. Local English and Japanese tables each load into 1201 level slots and 79 character slots; 80400 character/level cost lookups succeed per language (67 populated characters). Static artifact smoke tests, not Android verification.

- [../archive/reports/48-system-save-and-data-loading.md](../archive/reports/48-system-save-and-data-loading.md), [49-continue-game-preparation.md](../archive/reports/49-continue-game-preparation.md), [51-basedata-newgame-overrides.md](../archive/reports/51-basedata-newgame-overrides.md), [52-data-list-and-check-timing.md](../archive/reports/52-data-list-and-check-timing.md), [53-checkscript-arrays-and-consumer.md](../archive/reports/53-checkscript-arrays-and-consumer.md), [73-game-data-id-indexing.md](../archive/reports/73-game-data-id-indexing.md), [74-game-data-text-source.md](../archive/reports/74-game-data-text-source.md), [75-table-encoding-and-line-endings.md](../archive/reports/75-table-encoding-and-line-endings.md)
- Local (`build/research/`): `basedata-newgame-overrides.json`, `basedata-newgame.tsv`, `basedata-newgame/`, `basedata-slot8.json`, `basedata-reset-callees/`, `data-list-return-types.json`, `checkscript-context/`, `data-list-getters/`, `script-checks/`, `makearray-helpers/`, `checkscript-field-candidates.json`, `newdata-preload.tsv`, `newdata-tail.tsv`, `data-generic-load/`, `data-load-helpers/`, `data-load-entry/`, `data-text-reader/`, `string-lines/`, `string-lines-helpers/`, `data-encoding/`, `event-index-model.json`, `game-table-text-audit.json`

## Open questions

1. (blocking) Body of `<LoadData>b__74_0`: does it call `EventData.Load`/`<T>.Load` per line, and what happens with invalid/null lines and trailing CR?
2. (blocking) Full list of the 21 table names and their `Data` fields/types (only Event.txt position 5 is resolved).
3. Which container `Data.Load(arg)` receives and how language variants are chosen.
4. Values of the static masks in BuildingData/MagicShopData `NewGame`; field meanings of the reset offsets.
5. Exact semantics of stream slots 13 and 6 in `DeserializeData`.
6. Who writes BaseData sections (`BaseData.Serialize` `game:0x06001028` presumably) and the per-type `Deserialize` bodies.
7. Meaning of `AppData+0x238`, `+0x2e8` (10000) and their consumers.
