# Save

The save path is located statically end to end at the call level: `AppData.PackSave` writes selector-numbered blocks (2 = delayed events, 13 = world), `AppData.LoadGame` reads them back from a `SecureProperty` loaded through `RecordStore`, and the Continue path is `TitleForm.ContinueGame` → `NewGame` → `LoadGame` → `InitWorld`. Inner block codecs are known for the world (see [../formats/entity-save.md](../formats/entity-save.md)), BaseData sections, delayed events and the script-row payload; the outer `SecureProperty`/`RecordStore` envelope, encryption and CRC64 check are not recovered. No real save has been read. The port has only the script-row codec (`src/core/persistence/script_payload.cpp`).

## Overview

Two persisted objects live in the singleton `AppData` instance (distinct from its static world binding):

- **System save**: `SecureProperty` at `AppData+0x190`, built by `InitSystemSaveData`, filled from RecordStore record 0 by `LoadSystem`, written by `SaveRecorder.SaveSystem`.
- **Game save (per slot)**: `SecureProperty` at `AppData+0x198`, rebuilt by `NewGame` and by `LoadGame`, read from record `slot+1`.

A `SecureProperty` holds an array of byte blocks at `+0x10`; each block is selected by an integer *selector*. `PackSave` produces a block per selector; `LoadGame` and `DeserializeData` open block `n` (`SecureProperty+0x10` element `n`) as a `java.io.ByteArrayInputStream` and parse it.

Known selectors (game save):

| Selector | Content | Writer | Reader |
|---:|---|---|---|
| 0, 7–11, 14–17 | BaseData sections (one per data descriptor, see [data.md](data.md)) | not traced | `DeserializeData` |
| 2 | Delayed events (`AppData+0x200`, `DelayEvent` list) | `PackSave` branch at `0x1107440` | `LoadGame` at `0x1108738`.. |
| 13 | World (`EntityManager` then `SystemManager`) | `PackSave` → `World.Serialize` at `0x1107434` | `LoadGame` → `World.Deserialize` at `0x11086b4` |

The selector is not a file offset, record number or version. Other selectors, and how `PackSave` iterates them, are not recovered.

Normal-save integer slots are a different namespace from those selector blocks. SecureProperty+0x20 holds SecureInt references; QuestForm.Init reads decoded slots8/9 for area/global quest selection, and the UI stores the selected area/QuestData.id there. See [cached mission selection](data.md#cached-mission-selection). The mission host accepts decoded values at this boundary; it does not read or write the unresolved binary save envelope.

Storage layer (`kairo.unity.io`): `RecordStore..ctor` calls `Storage.Open` (`0x1c997b4`) and `Storage.Read` (`0x1c9b8b0`). `RecordStore.ReadRecord(int)` (`0x1c98344`) calls `OpenRecordStore` and the long `ReadRecord` overload (`0x1c97bd4`), which calls `GetFileName`, `KairoPlugin.GetPreference` (`0x1c27f10`), `Storage.Read` (`0x1c99dd0`), two `Encrypter.Decode` overloads, `EncodeUtil.ByteAryToLong`, `CRC64.GetValue` and `IllegalFileException..ctor`. These are alternative paths, not a proven sequence. The name RecordStore does not prove a J2ME RMS layout.

## Key methods

| Method | Registry id | Role | Confidence |
|---|---|---|---|
| `main.AppData.PackSave` | `game:0x0600018d` | Builds one game-save block per selector (2, 13 traced) | medium |
| `main.AppData.LoadGame` | `game:0x06000197` | Loads slot: LoadSystem, RecordStore read, SecureProperty, BaseData, world, delayed events | medium |
| `main.AppData.LoadSystem` | `game:0x06000192` | Opens RecordStore 0, reads record 0 into system SecureProperty | medium |
| `main.AppData.InitSystemSaveData` | `game:0x06000158` | Creates system SecureProperty at `AppData+0x190` | medium |
| `main.AppData.NewGame` | `game:0x0600015a` | SaveSystem, new game SecureProperty, InitSystemSaveData, NewData, check-script consumer | medium |
| `main.AppData.InitWorld` | `game:0x06000159` | Loaded-byte construction gate, map creation and World.Init; fresh mission binding | high |
| `form.TitleForm.ContinueGame` | `game:0x06000916` | Continue: NewGame → LoadGame(0) → InitWorld → SelectForm | high |
| `kairo.unity.ui.SaveRecorder.SaveSystem` | `kairo:0x06001728` | Writes system save via thread + `RunOnUiThread` | low |
| `kairo.unity.io.RecordStore..ctor` | `kairo:0x06000f96` | Storage.Open / Storage.Read | low |
| `kairo.unity.io.RecordStore.OpenRecordStore` | `kairo:0x06000fa6` | Opens store n | low |
| `kairo.unity.io.RecordStore.ReadRecord` (int) | `kairo:0x06000fa9` | Open + read record | low |
| `kairo.unity.io.RecordStore.ReadRecord` (impl) | `kairo:0x06000fa1` | File name, preference, decode, CRC64, exception | low |
| `kairo.unity.io.RecordStore.GetFileName` | `kairo:0x06000fb1` | Record file name | low |
| `kairo.unity.io.Storage.Open` / `Storage.Read` ×2 | `kairo:0x06000fd0`, `kairo:0x06000fdb`, `kairo:0x06000fd4` | Platform storage | low |
| `kairo.unity.util.SecureProperty..ctor` | `kairo:0x0600051b` | Container of selector blocks | medium |
| `kairo.unity.util.SecureProperty.Load` | `kairo:0x06000526` | Parses record bytes into blocks | low |
| `java.io.ByteArrayInputStream..ctor` | `kairo:0x060001f6` | Wraps a block in `System.IO.MemoryStream` (`this+0x18`) | high |
| `java.io.InputStream.Read` | `kairo:0x0600022d` | vtable slot 11; maps managed `Read` result `<1` to `-1` | high |
| `game.DelayEvent..ctor` ×2 | `game:0x0600062d`, `game:0x0600062e` | Empty / full constructor | high |
| `game.DelayEvent.Serialize` / `Deserialize` | `game:0x0600062f`, `game:0x06000630` | Delayed-event record codec | high |
| `main.AppData.SerializeScripts` / `DeserializeScripts` | `game:0x060001ed`, `game:0x060001ee` | Script-row payload codec (not linked to save) | high |
| `StreamUtil.Write` (bytes, flag) / `StreamUtil.Read` (len-prefixed) | `kairo:0x060005c0`, `kairo:0x0600059b` | int32 length + bytes; `-1` = null | high |
| `StreamUtil.WriteString` / `ReadString` | `kairo:0x060005db`, `kairo:0x060005ba` | Save strings (UTF-8 write, detected decode, localisation hooks) | medium |
| `StreamUtil.ReadTranslate` / `WriteTranslate` | `kairo:0x060005e7`, `kairo:0x060005e8` | Save-string translation filters | medium / low |
| `StringUtil.GetString` / `GetCode` | `kairo:0x06000615`, `kairo:0x06000616` | Encoding detection for byte → string | low |
| `kairo.common.cfg.Config..cctor` | `kairo:0x06000271` | Initialises `SAVEDATA_LOCALIZE` (static `+0x54`) to false | high |

`World.Serialize`/`Deserialize`/`Destroy` (`game:0x06000c18`, `game:0x06000c19`, `game:0x06000c02`) are ECS methods; see [ecs.md](ecs.md). The `Dictionary.Values` enumerator used by `EntityManager.Serialize` is runtime code, not in the registry.

## Behaviour

### Persisted hero level and experience

HumanComponent.Serialize (game:0x06000d6e, 0x1140078, 288 bytes) writes its scripts list, then five int32 scalar fields in order: charaDataId (+0x18), level_ (+0x1c), exp (+0x20), posOffsetX (+0x28), posOffsetZ (+0x2c), followed by five visual-array helper calls for +0x40/+0x48/+0x50/+0x58/+0x60. Deserialize (game:0x06000d6f, 0x1140198, 288 bytes) reads/stores the same sequence. Its second scalar store at 0x1140210 restores level_; the third at 0x1140220 restores exp. It does not copy these values from CharaData and does not write Human.state/frame/attackFrame/opponent/invincibleTime in this body.

The established two-pass entity/component loader invokes component deserialization before World.Deserialize sets the loaded byte to 1; see [ECS lifecycle](ecs.md#lifecycle). MemberSystem.Init skips CreateHero for every nonzero loaded byte, whereas the fresh-world CreateHero path explicitly starts at level 1/EXP 0. These observed paths explain persistent actor restoration on Continue without inventing a CharaData-to-Human copy during StageSystem.Start. Exact fresh/Continue entry-state preparation and the secure save envelope remain open. Script and visual-array wire codecs have not been recovered here, so neither full Human payload method is claimed reimplemented or given a complete byte format.

Evidence: local research/exports/stage-placement/F1140078.asm and F1140198.asm, existing entity-save/lifecycle evidence and hero-bootstrap exports. Confidence high for scalar read/write order; no real-save or Android comparison.

### LoadGame order (`0x11083f8`, 2056 bytes)

1. `LoadSystem` (`0x1107e8c`).
2. `RecordStore.OpenRecordStore(0)` (`0x1c98208`), `ReadRecord(0, slot+1)` (`0x1c98344`).
3. New `SecureProperty` (`0x1c07a98`) stored at `AppData+0x198`; `SecureProperty.Load` (`0x1c08470`) with the record bytes.
4. `DeserializeData` (`0x11077b4`) with the data descriptors and `false` → BaseData sections ([data.md](data.md)).
5. At `0x1108608`–`0x110864c`: element 13 of `SecureProperty+0x10` (array offset `0x88` with header) → `ByteArrayInputStream` → `World.Deserialize` at `0x11086b4`.
6. At `0x1108738`: replaces `AppData+0x200` with a new `Vector`, opens block 2, reads int32 count, then per entry: empty `DelayEvent`, `Deserialize`, add. Count `<1` → empty list. Failure mid-way leaves earlier entries added; no restore of the old list.

No direct call clears the ECS registry before `World.Deserialize` (see open question 1).

### PackSave (`0x1107304`, 1196 bytes)

- Selector 13: `World.Serialize` at `0x1107434` on the output stream. `World.Serialize` (64 bytes) = `EntityManager.Serialize` (`World+0x20`) then `SystemManager.Serialize` (`World+0x28`), same stream; matches `World.Deserialize` order.
- Selector 2 (compare at `0x11073bc`, branch `0x1107440`): writes `Size` of `AppData+0x200` as int32 BE, then each element (type-checked) via `DelayEvent.Serialize` at `0x1107500`. `Size` is re-queried each iteration (no snapshot).
- Entity order: `Dictionary.Values` enumerator (`GetEnumerator 0x19e4b08`, ctor `0x15918c4`, `MoveNext 0x15918ec`, `Dispose 0x15918e8` = RET) walks the entry array (`dictionary+0x18`, stride 24, up to `dictionary+0x20`) by increasing index, skipping negative-hash entries, value at `entry+0x10`. Order is insertion/slot order, not id order. `MoveNext` throws (`0x1e825f4`) if the version at `dictionary+0x2c` changed. Two passes visit the same order only if the registry is stable between them; there is no cross-pass version check.

### Streams and EOF

- `ByteArrayInputStream..ctor` (`0x1be8544`) allocates `System.IO.MemoryStream` (ctor `0x1da6748`) over the block and stores it at `this+0x18`. It inherits `InputStream.Read` (vtable 20432, 14 entries, slot 11 → `0x1be9ba8`).
- `InputStream.Read`: if `this+0x18` is non-null and not a `java.io.WrappedSystemStream`, calls managed `Read` (slot 31); results `>=1` pass through, `<1` become `-1` (`CMP 1` + `CSINV` at `0x1be9c40`).
- `MemoryStream.Read` (`0x1da6f74`) copies `min(requested, remaining)`; returns 0 at end.
- Composition: partial read is accumulated by `StreamUtil.Read`; at EOF 0 → `-1` → StreamUtil stops and fails for insufficient data. The "persistent zero loops forever" behaviour of StreamUtil (report 42) therefore does **not** apply to this path. Exact exception types/messages not recovered.

### System save

- `LoadSystem` (552 bytes): opens RecordStore 0, checks for records, reads record 0 when present, calls `InitSystemSaveData`, then `SecureProperty.Load` into `AppData+0x190`, closes. No direct World destroy/remove.
- `InitSystemSaveData` (`0x1101a10`, 568 bytes): passes five `AppData` statics (`+0x2b0`, `+0x320`, `+0x328`, `+0x330`, `+0x338`) to `SecureProperty..ctor`, stores at `AppData+0x190`. Then two conditional writes into `SecureProperty+0x20` at indices from statics `+0x2ec` and `+0x2c8`, values from `SecureInt.op_Implicit` (`0x1c068a8`) with 1 and 0: the first gated by `0x1b1b4f4`'s return, the second by static flag `+0xc1` and field `AppData+0x1a0`. Meaning of these options unknown. No World creation.
- `SaveRecorder.SaveSystem` (`0x1bac1e0`) only delegates: `ThreadStart..ctor` and `IApplication.RunOnUiThread` (`0x1bac2a4`/`0x1bac2c8`); the serializing body is not located.

### Continue path (`TitleForm.ContinueGame`, FDE `0x1218a14`–`0x1218d28`)

When `AppData.FlagCheck(4)` is true (otherwise a dialog is prepared): updates soft labels, then

1. `AppData.NewGame` (`0x1218ad0`, second argument register 0 — not used as a control by the body),
2. `AppData.LoadGame(0)` (`0x1218ae4`; the only direct caller found),
3. `AppData.InitWorld` (`0x1218af4`),
4. new `SelectForm` stored at `AppData+0xa0`, `FormManagerBase.ChangeCurrentForm`.

`LoadGame` alone is not a complete load; the port must reproduce this sequence. The [InitWorld entry kernel](ecs-lifecycle.md#initworld-entry) preserves the loaded-byte gate but the desktop selects its native path only for a fresh world; no restored Continue owner exists. Meaning of flag 4 is not recovered.

### NewGame (`0x1101d58`, 1420 bytes)

Calls `SaveRecorder.SaveSystem` first (unconditionally — the zero argument does not gate it), then creates a new `SecureProperty` at `AppData+0x198`, `InitSystemSaveData`, `NewData` (BaseData reset, see [data.md](data.md)), recreates collections and initialises data fields. At `0x11020b0` it walks every row of `AppData+0x248` (check-script groups) from row 0, calling `CheckNotFind` (slot 16) and, if bit 0 of the result is set, `CheckTerm` (slot 15); both return 0 in this build, and the groups are empty. Do not invent a "prepare without writing" mode.

### World destroy and title

- `World.Destroy` (`0x11356f4`, 128 bytes): generic op `0x1634dbc` on `World+0x40` (context GOT `0x2730d58`, probably a collection clear, unresolved), `EntityManager.DestroyAllEntities` (`World+0x20`, `0x1135744`), `SystemManager.RemoveAllSystems` (`World+0x28`, `0x1135754`), tail `EventManager.RemoveAllEventHandlers` (`World+0x30`, `0x113576c`). Does not reset `World+0x38` (checked by `InitWorld`, set by `Deserialize`). No local finally: a throwing step skips the rest.
- No direct BL/B caller of `World.Destroy` exists (indirect calls not excluded).
- `TitleForm.Init` (`0x1217c60`, 680 bytes) and `TitleForm.Finish` (`0x1219d40`, 68 bytes; clears `AppData+0x138`) do not call Destroy/DestroyAllEntities/RemoveAllSystems directly.
- Consequence: do not add an arbitrary world clear at the start of LoadGame; "world already empty at title" is a hypothesis.

### Delayed events (block 2)

Record layout and codec in [../formats/save.md](../formats/save.md#delayed-event-record). Fields (type 4249, native offsets `[16,20,24,28,32,40,48]` from `0x65b4b8`): `delayId_ +0x10`, `line_ +0x14`, `wait_ +0x18`, `hour_ +0x1c`, `talkChara_ +0x20`, `args_ +0x28` (string), `argi_ +0x30` (int[]).

- Empty ctor (`0x11c94a0`): ints `[-1,0,0,-1,-1]` (SIMD constant `0x625fa0`), `args_ = ""` (literal GOT `0x272ee88`); does not write `argi_`.
- Full ctor (`0x11c9508`): stores five ints, keeps `args_` reference, copies `argi_` when non-null, else keeps it null. Created by `ExecuteScript` opcode 10 (see [events.md](events.md)).
- Units of `wait_`/`hour_` and sentinels unknown.

### Strings, encoding and localisation

- `WriteString` (`0x1c0f834`): if `Config.SAVEDATA_LOCALIZE` (static byte `+0x54`, field 7551, Config type 1778, GOT `0x272f5c0`) calls `WriteTranslate` (returns void; original string kept in `x20`, so it does not replace the string). Non-null string → `Encoding.UTF8.GetBytes` → `StreamUtil.Write(bytes, 1)`.
- `StreamUtil.Write` (`0x1c045bc`) flag 1: int32 BE byte length then bytes; null array → `-1` and no payload; empty → `0`.
- `ReadString` (`0x1c0d810`): loads the flag first, `StreamUtil.Read(1)` (`0x1c03e98`: `ReadInt` length then `Read` 14430). `-1` → null, returned without conversion; `0` → empty bytes. Other negatives: not established. Non-null → `StringUtil.GetString` (`0x1c03f84`) → `GetCode` (`0x1c1a054`) tries **UTF-8 then Shift_JIS**, rejecting a candidate containing U+FFFD; other detection branches open. Decodes via `JString.New(bytes, encName)` (`0x1b1258c`, special Shift_JIS path) or fallback `JString.New(bytes)` (`0x1b120f0`, UTF-8, strips `EF BB BF`, zero length → shared empty string). Exception filtering in `GetString` is selective (not a catch-all). If the flag was set, returns `ReadTranslate(result)`.
- `Config..cctor` (`0x1bed0a0`) zeroes bytes `0x51`–`0x54` of Config statics (`STUR WZR` at `0x1bed33c`, statics via `+0xb8`), so `SAVEDATA_LOCALIZE` starts false; `Config.Create` (`0x1beb228`) uses `FieldInfo.SetValue` (`0x1beb71c`), so it may change later.
- `ReadTranslate` (`0x1c0d8d8`): returns input if `StreamUtil.TranslateEnable` (static `+0x18`) is false. Table `translateTable_` (static `+0x0`); null table → callback-driven init (unresolved). With a table, returns input unchanged when length `>127` (UTF-16 length), when `Regex.IsMatch("[^0-9]+")` is false, or when it starts with `http://`, `https://` or `GPA.`. Else key = input with CRLF→LF, LF→`\n` (two chars), TAB→`\t`, `0x08`→removed; `ContainsKey`/`get_Item` (Dictionary instantiation 298). Miss → **original** string; hit → value with `\n` then `\t` unescaped. No other escapes.
- UTF-8 invalid-input/substitution policy (`Encoding.get_UTF8 0x1d07230`) not audited; lossless round trip of arbitrary strings not claimed.

## Port

- `src/core/persistence/script_payload.{hpp,cpp}` + `tests/test_script_payload.cpp` (CTest `script_payload_contract`): the script-row codec only, see [../formats/script-payload.md](../formats/script-payload.md).
- `src/core/gameplay/mission_map.{hpp,cpp}` + `tests/test_mission_map.cpp`: AppData.InitWorld orchestration through explicit world/system/map/Init providers. The mission host binds fresh entry; no save bytes are decoded by this function.
- Missing: RecordStore/SecureProperty envelope, encryption, CRC64, BaseData sections, world block (see ecs.md), DelayEvent codec (blocked on string decode/localisation), save strings, LoadGame/NewGame/Continue orchestration.

## Evidence

- [../archive/reports/20-persistence-entrypoints.md](../archive/reports/20-persistence-entrypoints.md), [46-save-order-and-callers.md](../archive/reports/46-save-order-and-callers.md), [47-save-stream-eof.md](../archive/reports/47-save-stream-eof.md), [48-system-save-and-data-loading.md](../archive/reports/48-system-save-and-data-loading.md), [49-continue-game-preparation.md](../archive/reports/49-continue-game-preparation.md), [50-world-destroy-and-title.md](../archive/reports/50-world-destroy-and-title.md), [54-script-payload.md](../archive/reports/54-script-payload.md), [55-delay-event-save-block.md](../archive/reports/55-delay-event-save-block.md), [56-save-strings.md](../archive/reports/56-save-strings.md), [57-save-localization.md](../archive/reports/57-save-localization.md)
- Local (`build/research/`): `resource-methods/`, `world-save-direct-callers.json`, `save-dictionary-order.tsv`, `save-callers.tsv`, `save-input-preparation.tsv`, `save-input-read.tsv`, `save-memory-read.tsv`, `save-system-init.tsv`, `loadgame-direct-callers.json`, `continue-game.tsv`, `newgame-preload.tsv`, `world-destroy-callers.json` (empty), `world-destroy-path.tsv`, `title-cleanup.tsv`, `script-payload-callers.json` (empty), `delay-event-callers.json`, `delay-event/`, `save-string/`, `save-string-helpers/`, `string-decode/`

## Open questions

1. (blocking) What guarantees an empty ECS registry before `World.Deserialize` on Continue: empty world at title, an earlier transition, or a BaseData override? No direct clear found; `World.Destroy` has no direct caller.
2. (blocking) `SecureProperty.Load`/`Read`/`Write`/`ToByteArray` layout, `Encrypter` scheme, CRC64 polynomial/position/coverage and rejection condition in `ReadRecord`; `RecordStore.GetFileName`/`Storage` paths. Do not remove the check or brute-force it; validate only against a save copied in an authorised environment.
3. (blocking) Full selector list written by `PackSave` (BaseData sections, system blocks) and the writer of BaseData sections.
4. (blocking for DelayEvent codec) Rest of `GetCode` detection, Shift_JIS path of `JString.New`, `WriteTranslate` body, translation-table source, effective values of `SAVEDATA_LOCALIZE` and `TranslateEnable` (`Config.Create`/`Config.Init`).
5. `SaveRecorder.SaveSystem` delegate body (what the system save writes and when).
6. Meaning of `FlagCheck(4)`, `InitSystemSaveData` options (`+0x2ec`, `+0x2c8`, `+0xc1`, `+0x1a0`) and the generic op on `World+0x40` in `World.Destroy`.
7. Consumer that decrements/evaluates `wait_`/`hour_`/`line_` of delayed events (events subsystem).
8. Whether `SerializeScripts`/`DeserializeScripts` are reached indirectly (no direct callers).
