# Data

Static game data lives in `data.Data` as arrays of `data.BaseData` subclasses, loaded from 21 UTF-8 text tables (`Event.txt`, `Cloth.txt`, `Voice.txt`, ...) read through `JarInflater`, split on LF and indexed by record id. Ten of those arrays form the *data list* used for new-game reset (`BaseData.NewGame`), save-section deserialisation and check-script grouping. Indexing, line splitting and encoding are established statically; the generic per-row record constructor (`<LoadData>b__74_0`) remains unresolved. Selected record readers and id-indexed catalog projections are ported below; the complete production loader is not bound.

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
| `main.AppData.GetResourceId` | `game:0x0600023d` | Static name row1 lookup; mission resource index | high |
| `main.AppData.GetDataList` | `game:0x06000194` | 10 descriptors (array, selector) | high |
| `form.BootForm.Load` (cache segment) | `game:0x06000715` | Alias full area/quest catalogs to AppData after Data.Load | high |
| `form.SelectStageForm.Update` (selection store) | `game:0x0600084e` | Store chosen QuestData.id in normal-save slot9 before the remaining UI flow | high |
| `data.AreaData.Load` | `game:0x0600101d` | Ordered static area fields and retained partial writes; canonical mission area | high |
| `main.AppData.NewData` | `game:0x0600015b` | `BaseData.NewGame` on every list object; tail `InitCheckScriptData` | high |
| `main.AppData.DeserializeData` | `game:0x06000190` | Reads BaseData save sections | medium |
| `main.AppData.InitCheckScriptData` | `game:0x0600015c` | Builds check-script groups `AppData+0x248` | high |
| `main.Util.ArrayInit` (int[], int) | `game:0x06000281` | Fills int array with value | high |
| `java.util.JTool.MakeArrayRecursive` | `kairo:0x060000cf` | Recursive jagged array allocation | medium |
| `java.lang.JInteger..ctor` | `kairo:0x06000175` | Stores int at `+0x10` | high |
| `data.BaseData.NewGame` | `game:0x06001038` | Virtual slot 18; base = RET | high |
| `data.BaseData.Find` | `game:0x06001027` | Slot 8; `+0x10 = 1`, `byte+0x14 = arg&1` | high |
| `data.QuestData.Find` | `game:0x060011f0` | Sets `+0x10`/`+0x14` to 1 only if `+0x10 == 0` | high |
| `data.QuestData.Load` | `game:0x060011e2` | In-place static quest fields, nested groups and retained failure prefix | high |
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
- Building/MagicShop masks and remaining field semantics are open. None of these resets touches the ECS world.

### Equipment new-game state

EquipmentData.NewGame (`0x116f0c8`), ClothData.NewGame (`0x116e458`) and HatData.NewGame (`0x1171894`) share a60-byte contract: test flag bit0, call virtual Find(false) when set, re-read flags after that call, read initLevel, then store level and flag bit1 as stock0/1. Field pairs are Equipment+0x28/+0x2c from initLevel+0x80, Cloth+0x20/+0x24 from+0x4c and Hat+0x60/+0x64 from+0x44. No clamp, stock accumulation or unconditional discovery clear occurs. Find failure skips both final stores, retaining earlier callback effects. Callback changes to flags/initLevel affect the final values.

BaseData.Find (`0x116c790`,20 bytes) unconditionally stores discovery state1 and the supplied low bool into new_. It does not alter stock. The fresh constructors explicitly clear state_ at+0x10 and bytes+0x13..+0x16, including new_ at+0x14. Equipment/Cloth/Hat inline the same base stores. A fresh loaded gear catalog therefore starts undiscovered/not-new; NewGame marks bit0 records found/not-new, sets each record's level from its initLevel, and gives one stock only for bit1. Other NewGame methods do not follow this exact contract and are not replaced by this kernel.

The combat table loader retains initial_level from Equipment fixed token19 and Cloth/Hat token7 (zero-based), after consuming the unchanged schemas. `EquipmentNewGameState` and `reset_equipment_for_new_game` in combat_tables.cpp implement the three overrides through an injected virtual Find contract; the default adapter uses `find_item_stock` in stage_rewards.cpp. Tests cover signed flags, dirty prior state, callback mutations, failure prefixes and asymmetric table fields. The mission host's explicit gear_new_game mode runs these catalogs in GetDataList relative order (Equipment, Hat, Cloth), seeds canonical combat levels and per-record result inventory, and excludes saved gear snapshots. This is a projection of those resets, not complete AppData.NewData/NewGame.

Evidence: complete basedata-newgame/{M38250_EquipmentData_NewGame,M38182_ClothData_NewGame,M38387_HatData_NewGame}.asm, basedata-reset-callees/M38052_BaseData_Find.asm, combat-loaders/{EquipmentLoad,ClothLoad,HatLoad}.asm and mission-new-gear/{BaseData_ctor,EquipmentData_ctor,ClothData_ctor,HatData_ctor}.asm. Constructor bounded sizes are40/444/108/108 bytes; coverage has zero missing words. Addresses subtract Ghidra image base0x100000; field names come from v31 metadata. Confidence high for the static contract; no original runtime comparison.

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

### Area records at mission entry

AreaData.Load (`0x116c414`,200 bytes; game:0x0600101d) returns its receiver, preserving it for a null or empty line. Otherwise StringArrayStream reads/stores, in order, id int32 (`+0x18`), name string (`+0x40`), image int32 (`+0x48`), count-prefixed stages int32[] (`+0x50`) and inherited flag int32 (`+0x1c`). An array is stored only after its entire read succeeds; later failures retain earlier fields. Negative array counts mean null and zero means a distinct empty array. No discovery, arrival-stage or per-stage damage state is read from Area.txt. Trailing tokens are ignored. AreaData.GetStageNum (`0x116c3d4`,8 bytes) returns constant1; do not substitute stages-array length for that observed method.

QuestForm.Init calls Data.get_areaData at0x11da164, indexes it by AppData.exploreArea_, and passes that row with its stored QuestData and AppData.exploreStage_ to StageSystem.Start at0x11da1b8. Its QuestData field is selected earlier from AppData.questData_[exploreStage_] at0x11d9e48..0x11d9e68. AppData.areaData_/questData_ are separate cached instance fields at+0x1e0/+0x1e8, while the Data catalog is a separate owner. BootForm.Load assigns the full Data arrays directly to those caches; the selection contract is described below. Area.stages does not reindex the observed quest lookup.

Port: the existing [quest catalog](../../src/core/gameplay/quest_catalog.hpp), [implementation](../../src/core/gameplay/quest_catalog.cpp) and [contract test](../../tests/test_quest_catalog.cpp) cover Area Load, partial mutation, null/empty arrays, id gaps and duplicate replacement through the shared table reader. The mission catalog loads Area.txt when stage_start requests an area or mission_selection supplies decoded selection. An aliasing shared pointer retains the canonical catalog row and its table owner through Stage.Start, results and the held return callback. The native start/return argument retains canonical area identity; decoded selection is handled below while saved area progress remains external; host commands and provisional selections are in [integration](integration.md#table-backed-area-at-stage-start-s1-a1).

Evidence: complete ignored bounded exports mission-world-entry/AreaData_Load.asm, mission-area-entry/AreaData_GetStageNum.asm and mission-original-entry/form_QuestForm_Init.asm; registered field offsets0x277ece8, metadata type fields and library SHA25617a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f. Addresses subtract Ghidra base0x100000. Confidence high for static load/storage/call contracts; no Android comparison.

### Cached mission selection

BootForm.Load (`0x11d2e1c`,6468 bytes; game:0x06000715) loads Data and copies the full Data.get_questData result to AppData.questData_ (`+0x1e8`, store0x11d3ed0), then the full Data.get_areaData result to AppData.areaData_ (`+0x1e0`, store0x11d4154). These are aliases to the id-indexed catalogs, including null gaps. There is no area filtering, compaction or Area.stages remapping in either assignment. This establishes the cache portion, not the complete boot/resource-loading method.

The selection values are normal game-save integer slots, not SecureProperty block selectors. MyFormBase.apdat_ (`+0x90`) points to the AppData instance; its save_ (`+0x198`) owns the SecureInt reference array at SecureProperty+0x20. Slot8 is at array+0x60, slot9 at+0x68. QuestForm.Init reads/converts8 before9 at0x11d9d18..0x11d9d74, then calls StartExplore(area8,quest9). StartExplore first checks the unsigned area index against the cached area-array length at0x11152c4..0x11152d4; negative or out-of-range indices reach the bounds error. It does not dereference the area row for this gate. Its stores set exploreArea_=area8 and exploreStage_=quest9. QuestForm.Init indexes the full quest cache with exploreStage_, then selects Data.areaData[exploreArea_] and calls Stage.Start with that area, quest and the same global quest index as stage. A valid area-array gap can therefore supply a null area; do not substitute another area or silently compact the catalog.

The UI writers corroborate these identities. SelectForm.Update reads selArea_ (`+0x120`), converts it to SecureInt at0x11ecf48, stores it in slot8 at0x11ecf78, then resets selArea_=-1. SelectStageForm.Init reads slot8 into area_ (`+0xfc`) at0x1207884. SelectStageForm.Update reads the chosen QuestData.id (`+0x18`) at0x120803c, converts it at0x1208048 and stores slot9 at0x1208078. This is the record id/global catalog index, not a display-row index. Stamina eligibility/consumption and the remaining menu transition belong to that UI caller and are not supplied by the mission snapshot.

Port: select_mission_records in the existing [quest catalog](../../src/core/gameplay/quest_catalog.cpp) implements the selection projection for decoded integer inputs and ready catalogs. It preserves the area-length-before-quest-index order, null gaps and canonical row identity. Its contract test checks global indexing independent of Area.stages, live record identity, gaps and failure order. It is not a port of all BootForm.Load, QuestForm.Init or StartExplore's legacy Witch ledger. MissionProfile's mission_selection row wires the selected area/quest and global stage to native Stage.Start; --quest supplies an explicit effective slot9 override for that run. Binary save storage, native UI selection and the rest of bootstrap remain separate boundaries. See [integration](integration.md#decoded-mission-selection-s1-a1).

Evidence: complete ignored bounded exports mission-cache-entry/BootForm_Load.asm, mission-selection-writers/{SelectStageForm_Init,SelectStageForm_Update}.asm and mission-selection-area-writer/SelectForm_Update.asm; existing mission-original-entry/form_QuestForm_Init.asm and mission-explore-entry/main_AppData_StartExplore.asm. The candidate scans mission-cache-writers.json and mission-selection-store-candidates.json are bounded to Assembly-CSharp method FDEs and are not claims about reflective/generic/dynamic callers. Native field offsets and the library digest are recorded in the area section. Confidence high for these static stores and indexing; no Android comparison.

### Combat table projections

The existing character reader now retains img, weaponId, initLevel, parameters and the inherited int32 flags as well as the experience rate. It still consumes the full CharaData schema once; it does not duplicate the parser. `bind_character_parameters(record,saved_level)` shares the parameter array, uses the explicitly supplied saved level, and exposes the low flags byte used by `get_isMain`. initLevel is not substituted for saved level.

Additional loaders in `combat_tables.cpp` consume these schemas (int32 unless marked string or array):

| Original method | ELF VA / bytes | Sequential reads |
|---|---|---|
| MagicShopData.Load (game:0x0600119c) | 0x1171f2c / 232 | id, name string, category, basePrice, upValue, explain string, inherited flag |
| EquipmentData.Load (game:0x0600111e) | 0x116f428 / 504 | id, name string, four integers (+0x38..+0x44), resource string, two integers (+0x50/+0x54), attribute (+0x58), eleven integers (+0x5c..+0x84), status int32[][], explain string, inherited flag |
| ClothData.Load (game:0x060010c2) | 0x116e704 / 312 | id, name string, integer (+0x30), resource string, five integers (+0x40..+0x50), parameters int32[][], explain string, inherited flag |
| HatData.Load (game:0x06001171) | 0x1171734 / 312 | same token framing as cloth, different object offsets (+0x28, +0x38..+0x48, parameters +0x50) |

Each Load takes a line, mutates and returns its receiver, initially bypasses empty-string input, and stores immediately after each read. Its caller is the per-type data-loader route. Gameplay projections retain id, parameter endpoints, attribute where present, flags, shop category/upValue, resource/image references and the weapon timing/projectile fields described in combat.md; unrelated economy tokens are still parsed so malformed required numeric tokens fail. The fresh-record port does not reproduce observable partial mutation of a pre-existing receiver. Therefore these original Load methods remain documented, not fully reimplemented.

Shop buyCount and equipment level are mutable state absent from these records. Binding functions require explicit values; they neither assign a new-game state nor fabricate save progress. The separate gear_new_game reset supplies native initial levels when selected; snapshot mode still needs saved values. The loaders reuse `TableTokens` and `load_table_records`, preserving null outer arrays, id gaps and last duplicate wins.

Local shipped-table validation via `test_combat_tables` read both English and Japanese Chara/MagicShop/Equipment/Cloth/Hat files: each language has 67 populated character records, 8 shop records and 269 equipment records across the three equipment tables. Across selected levels and all 13 parameter indices, 17,467 numeric queries per language produced the same FNV-style hash 13925746254597776912. Shop arithmetic uses explicit synthetic purchase counts (0 for character queries, 3 for standalone shop queries), not recovered saves. This is data/contract integration, not original-runtime verification. Evidence: `research/exports/combat-loaders/` bounded exports and `game-progression-tables/F116dcc4.asm`; all game tables remain local.

### Bullet records

BulletData.Load (game:0x06001063, ELF 0x116d414, 264 bytes) consumes nine sequential tokens: id (+0x18), category (+0x20), attribute (+0x24), resource string (+0x28), image (+0x30), trail image (+0x34), impact-resource string (+0x38), impact image (+0x40), inherited flags (+0x1c). The initially empty input bypasses all reads; otherwise each field is stored immediately after its read and a failure preserves earlier writes. Extra tokens are ignored. Data.get_bulletData (0x116f064) returns Data.instance_.bulletData_ (+0xa8), requiring the singleton but allowing a null array.

[bullet_factory.cpp](../../src/core/gameplay/bullet_factory.cpp) provides load_bullet_record for the actual in-place contract and load_bullet_table for the existing UTF-8/LF/id-indexed catalog framing. The records retain every field and share their impact projection with the existing impact consumer. Tests cover empty input, duplicate IDs, gaps, truncation, invalid integers and partial mutation of a pre-existing record. Equipment projections now additionally retain type, bulletId, maxHeight and bulletSpeed (tokens 3/8/13/14, fields +0x3c/+0x54/+0x68/+0x6c), using the same parser and token consumption as before.

The optional test_bullet_factory audit loads both local English and Japanese Bullet/Equipment tables: each has 16 populated bullet slots, 91 equipment rows and 86 bullet references whose IDs are not -1; all references resolve. Numeric projection hash is 3045128064134888100 in each language. This validates shipped-data parsing/references, not runtime selection or saved equipment. Evidence: local human-shoot/BulletData_Load.asm, human-shoot/Data_get_bulletData.asm, existing combat-loaders/EquipmentLoad.asm, metadata field offsets and the audit arguments in test_bullet_factory.cpp. Production locale/catalog selection remains open.

### Treasure records

`TreasureData.Load` (`game:0x06001235`, `0x11737a4`, 312 bytes) consumes twelve logical fields: id, name (string), res (string), img, exp, money, crystal, weaponId, charaId, clothId, hatId, inherited flag. Numeric fields are int32. Stores target `+0x18`, `+0x20`, `+0x28`, `+0x30`, `+0x34`, `+0x38`, `+0x3c`, `+0x40`, `+0x44`, `+0x48`, `+0x4c`, `+0x1c`, respectively. It returns the receiver; the initial guard bypasses parsing, reads/stores are sequential and extra fields are ignored. Its data-loading caller and partial-mutation caveat are the same as the progression readers above. TreasureData.get_isBossReward (game:0x06001236, 0x11738dc) reads that inherited +0x1c field and tests mask 0x1; the collection port uses the existing decoded flags directly, with no new table parser.

`load_treasure_table` in `quest_rewards.cpp` retains all these fields using the shared `table_catalog.hpp` framing/id-indexing helper. Both extracted languages load 184 populated records into 184 slots. Confidence high: bounded instructions in local `research/exports/quest-reward-pipeline/` and metadata field offsets. The fresh-record reader does not implement in-place partial receiver mutation on parse failure.

### Enemy records

`EnemyData.Load` (`game:0x060010e3`, `0x116ec2c`, 344 bytes) reads id, name, enemySeb, exp (int32[]), treasureDrop (int32[][][]), voiceId, attribute, res (string), img, seb, frame, rank, parameters (int32[][]), and inherited flag. Fields map to +0x18, +0x28, +0x30, +0x38, +0x40, +0x48, +0x4c, +0x50, +0x58, +0x5c, +0x60, +0x64, +0x68 and +0x1c. defeatCount_ (+0x20) is separate mutable state and is not loaded here. The method mutates/returns the receiver in sequential read/store order; the initial string guard bypasses reads and extra tokens are ignored.

`GetIntArray` (`0x1126fd8`) permits any negative outer length as null. `GetIntTripleArray` (`0x11271bc`) also permits a negative outer length as null, but allocates nested group/row lengths directly: negative inner lengths fault, rather than creating null inner arrays. Zero lengths remain distinct from null. `load_enemy_table` shares the existing token/framing/id-indexing readers and retains the entire static record schema. As with other fresh-record loaders, it does not expose in-place partial mutations after a parse failure.

Both English and Japanese shipped tables decode to 83 populated slots. Evidence: bounded exports `research/exports/enemy-rewards/` and `enemy-reward-readers/`, metadata field offsets, plus synthetic schema/null/negative-inner-size tests. Confidence high. Runtime-added null inner arrays are not represented by the decoded-record projection.

## Port

### Selected mission catalog

The [gameplay catalog](../../src/platform/desktop/gameplay_catalog.hpp) reads Chara/Equipment/Cloth/Hat/MagicShop/Enemy/Bullet.txt through the existing native candidate/first-match archive lookup; explicit reward scenes additionally request Treasure.txt through its existing reader. ResourceArchive's explicit data_only mode skips img.inf/seb.inf preparation while preserving that lookup; data(name) returns the selected member name and bytes. It neither infers the Data.Load container nor replaces its full 21-table bootstrap. The caller supplies the language/DPI profile, CRLF gate and integer parser.

Equipment/cloth/hat projections now retain the resource string and image ID needed by the original entity factories. Equipment tokens6/7 map to res_+0x48/img_+0x50; cloth tokens3/4 to +0x38/+0x40; hat tokens3/4 to +0x30/+0x38. All other token consumption and numeric combat fields are unchanged. Existing Load statuses remain documented because the fresh-record readers still do not reproduce partial mutation of an existing receiver.

[MissionEntities](../../src/platform/desktop/mission_entities.hpp) owns fresh entities, component pools, registry subscriptions and dynamic subsets around the ported Human/Enemy/Bullet/Effect/Drop/Emitter/Particle factories. Main-character clothing, hat and weapon references come from decoded records, not fixture offsets such as 600+weapon. Saved level/EXP and shop purchase counts are separate explicit MissionProgress inputs; initial_level is never substituted. Factory level arguments remain distinct from saved values. The selected scene binds only supplied character progress rows; unbound saved state fails before starting that adapter's human creation. Resource-name-to-AppData-ID mapping and loaded animation resources are owner callbacks. Native entity deletion is bound; pooling reuse, save decoding and complete live world/system services remain open.

[MissionScene](../../src/platform/desktop/mission_scene.hpp) sends a supplied ordered entity group and view through RenderSystem.Update, with actual prepared SEBs and per-entity image/resource arrays. Its animation pass uses the existing header-derived clocks. The preview does not substitute the complete mission visible-set/camera/input implementation; unsupported camera-save/formation/touch services fail explicitly. Render faults retain the native command prefix and are surfaced to the caller.

Tests [gameplay catalog](../../tests/test_gameplay_catalog.cpp), [entity factory binding](../../tests/test_mission_entities.cpp) and [scene rendering](../../tests/test_mission_scene.cpp) use synthetic tables/SEBs to check selection, gaps, presentation fields, saved-state separation, real factory payloads, registry/subset notifications, clocks and render-fault reporting. The existing queue_capture pixel suite also loads a synthetic table archive, creates human/enemy entities and draws their generated queue without manual draw rows; its bounded loop runs the same animation/render path repeatedly. Local English data loads 67 characters, 91 equipment, 146 hats, 32 cloth, 83 enemies, 16 bullets and 8 shop rows. A selected original-data human0/enemy2 scene renders six commands/quads using five textures and completes 12 animation/render passes. Level1/EXP0 and zero shop purchases in that profile are supplied scenario values, not recovered saves. Limits: complete combat scheduling, player actions and Android verification are still absent from this table-backed scene.

`src/core/gameplay/progression_tables.cpp` loads complete level-table records and a character progression projection, preserving id gaps and last-duplicate-wins indexing. It shares token parsing with the event reader and accepts the existing integer-parser callback. `TableExperienceHost` resolves the live character id/rate and level row for the experience kernel. The character projection consumes and validates all fields but retains id, needExpRate, image, weaponId, initLevel, parameters and inherited flags. These fresh-record readers do not expose partial mutation of an existing record on failure; neither original Load method is claimed fully reimplemented. BaseData lifecycle, the general loader, data list and check-script groups remain unported. `tools/audit_game_table_text.py` audits the extracted tables (encoding/line endings) only; `tools/inspect_event_table.py` belongs to the events subsystem.

### Quest record reader

`data.QuestData.Load` (`game:0x060011e2`, ELF `0x1172b44`, 512 bytes) takes a nullable line and returns its receiver. Null/empty lines leave it unchanged. Nonempty lines use `StringArrayStream`; each successful field read stores immediately, so a later failure preserves the prefix and the previous destination array when its nested read fails. Extra tokens are ignored. Confidence high from the complete bounded ARM64 body and metadata field offsets.

The logical fields below are TAB-separated; each array includes its count prefixes. Offsets refer to the managed object, sizes to its fields (`int32` 4 bytes, references 8 bytes).

| Read order | Fields and offsets |
| --- | --- |
| 1–5 | id `+0x18`, name `+0x20`, image `+0x28`, resource `+0x30`, chip image array `+0x38` |
| 6–7 | Two skipped tokens; neither is parsed as an integer |
| 8–11 | base level `+0x40`, boss level `+0x44`, distance `+0x48`, time limit `+0x4c` |
| 12–15 | enemy interval `+0x50`, simultaneous maximum `+0x54`, total limit `+0x58`, money rate `+0x5c` |
| 16–18 | normal enemies `+0x60`, fixed enemies `+0x68`, boss id `+0x70` |
| 19–20 | event groups `+0x78`, fixed treasure groups `+0x80` |
| 21–25 | prisoner maximum `+0x88`, reward treasure `+0x8c`, cleared reward treasure `+0x90`, inherited flags `+0x1c`, stamina requirement `+0x94` |

Chip images use `GetIntArray`; the four group fields use `GetIntTripleArray`. Any negative outer count means null, zero means a distinct empty array, and negative inner group/row counts fault. Saved stage details `+0x98`, rescued prisoners `+0xa0`, completion byte `+0xa4` and quest record `+0xa8` are not loaded here. The completion getter tests flags mask2 OR the saved byte; static flags must not be copied into that byte.

Port: [quest_catalog.cpp](../../src/core/gameplay/quest_catalog.cpp) provides the in-place reader and the shared table framing/id indexing. Its projection preserves saved prisoner/completion fields across Load. The caller supplies the decimal parser; full runtime numeric culture and QuestData construction/NewGame are separate contracts. [Tests](../../tests/test_quest_catalog.cpp) cover field mapping, nonnumeric skips, ignored suffixes, null/empty distinction, nested failures, partial mutation, duplicates, gaps and CRLF framing. `MissionProfile` requests Quest.txt only for an explicit quest selection and shares its static projection with Human, Enemy, Stage and Map; see [integration](integration.md#table-selected-quests-s1-a1a3).

### Mission resource index

AppData.GetResourceId (game:0x0600023d, ELF0x11155dc,240 bytes) searches row1 of the static resource-name array at AppData.static+0xa8, returning the first matching index or -1. AppData.cctor fills this24-entry row at ELF0x11183c0..0x1118630. Resolving its relocated metadata string literals establishes the mission subset below; GetResourceManager's existing dispatch maps these IDs to the corresponding live fields. Confidence high for these IDs/names; this does not establish the complete application resource lifecycle.

| ID | TextAsset name |
| --- | --- |
| 1 | chara |
| 3 | enemy |
| 7 | quest |
| 18 | chip |
| 19 | bullet |
| 20 | effect |
| 21 | hat |
| 22 | weapon |
| 23 | gauge |

`python -I tools/extract_assets.py --out extracted/full --mission-index` writes a local mission-resources.tsv from an existing schema2/3 manifest of the supported APK. It selects those names plus the existing xls table archive, requires one matching entry each and verifies every decoded file hash. Numeric filename prefixes come from the manifest, not guesses. Existing differing indices are preserved and reported. No APK re-extraction or additional package is needed for this command.

The native `mission_resources "English.lproj" 1` scenario row requires `--data`, reads that small version1 index and supplies tables, application/global aliases and resource-name lookups through the existing synchronous archive owners. It rejects duplicate/missing IDs, non-child paths and mixed prior bindings. Language/DPI remain explicit; saved inputs and full async/locale/pack selection remain separate. Synthetic tests verify equivalent domain output against manual bindings, schema variants, hashes, missing entries and path rejection. Local evidence: mission-resource-entry/{GetResourceId.asm,AppDataCtor.asm,resource-names.json} and appdata-constants/AppData_cctor.asm; all native addresses use the ELF base and source hash recorded in coverage.tsv.

## Evidence

- Quest loader: ignored `research/exports/mission-quest-table/`, complete bounded coverage, library SHA256 `17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`; exported Ghidra addresses include image base `0x100000`, while this document uses ELF addresses. Both shipped English and Japanese Quest.txt tables load 76 populated slots through the portable reader. This is static artifact validation, not an Android comparison.

- Presentation fields: ignored `research/exports/gameplay-catalog/presentation-fields.json` resolves six selected properties through registered field offsets at ELF0x277ece8, metadata types4507/4509/4515 and the hash-verified ARM64 library (`17a6cf3a3cef9bd7b387c4b9627df113a8b214f0deb07e1778585dac1143526f`). Existing full bounded `combat-loaders/{EquipmentLoad,ClothLoad,HatLoad}.asm` establish their token read/store order; no new Ghidra/REA run was needed. Original-data scene/probe inputs and screenshots remain local in `research/exports/mission-data-scene/`; the explicit resource-name binding selects enemy->AppData3. The catalog audit identifies enemy-resource first ID2 and quest-resource first ID0; record0 is therefore not assumed to be the first ordinary enemy. Test data is entirely synthetic in Git.

- Local `research/exports/game-progression-tables/`: bounded ARM64 exports of both record loaders. `tests/test_progression_tables.cpp` covers field mapping, arrays, gaps, duplicates, CRLF gates, live rate/id changes and progression failure ordering. Local English and Japanese tables each load into 1201 level slots and 79 character slots; 80400 character/level cost lookups succeed per language (67 populated characters). Static artifact smoke tests, not Android verification.

- [../archive/reports/48-system-save-and-data-loading.md](../archive/reports/48-system-save-and-data-loading.md), [49-continue-game-preparation.md](../archive/reports/49-continue-game-preparation.md), [51-basedata-newgame-overrides.md](../archive/reports/51-basedata-newgame-overrides.md), [52-data-list-and-check-timing.md](../archive/reports/52-data-list-and-check-timing.md), [53-checkscript-arrays-and-consumer.md](../archive/reports/53-checkscript-arrays-and-consumer.md), [73-game-data-id-indexing.md](../archive/reports/73-game-data-id-indexing.md), [74-game-data-text-source.md](../archive/reports/74-game-data-text-source.md), [75-table-encoding-and-line-endings.md](../archive/reports/75-table-encoding-and-line-endings.md)
- Local (`build/research/`): `basedata-newgame-overrides.json`, `basedata-newgame.tsv`, `basedata-newgame/`, `basedata-slot8.json`, `basedata-reset-callees/`, `data-list-return-types.json`, `checkscript-context/`, `data-list-getters/`, `script-checks/`, `makearray-helpers/`, `checkscript-field-candidates.json`, `newdata-preload.tsv`, `newdata-tail.tsv`, `data-generic-load/`, `data-load-helpers/`, `data-load-entry/`, `data-text-reader/`, `string-lines/`, `string-lines-helpers/`, `data-encoding/`, `event-index-model.json`, `game-table-text-audit.json`

## Open questions

- (blocking for scene-free mission entry) Bind native NewGame saved/configuration producers and Continue party/system restoration to slot8/9 selection. Fresh InitWorld is host-bound; mission_selection still receives explicit decoded values and no binary-save provider exists.

1. (blocking) Body of `<LoadData>b__74_0`: does it call `EventData.Load`/`<T>.Load` per line, and what happens with invalid/null lines and trailing CR?
2. (blocking) Full list of the 21 table names and their `Data` fields/types (only Event.txt position 5 is resolved).
3. Which container `Data.Load(arg)` receives and how language variants are chosen.
4. Values of the static masks in BuildingData/MagicShopData `NewGame`; field meanings of the reset offsets.
5. Exact semantics of stream slots 13 and 6 in `DeserializeData`.
6. Who writes BaseData sections (`BaseData.Serialize` `game:0x06001028` presumably) and the per-type `Deserialize` bodies.
7. Meaning of `AppData+0x238`, `+0x2e8` (10000) and their consumers.
