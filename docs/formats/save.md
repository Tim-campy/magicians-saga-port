# Save

A save is a set of RecordStore records (record 0 = system save, record `slot+1` = game slot), each loaded into a `SecureProperty` whose byte blocks are indexed by selector. Inner blocks are big-endian streams: BaseData sections, the world block (13) and the delayed-event block (2) have known layouts; the record envelope, `SecureProperty` serialisation, encryption and CRC64 check are not recovered. No real save file has been opened; nothing here is validated on real data. Behaviour and callers: [../subsystems/save.md](../subsystems/save.md).

## Layout

### Envelope (unrecovered)

| Layer | Known |
|---|---|
| RecordStore | Store 0. Record 0 = system save (`LoadSystem`), record `slot+1` = game slot (`LoadGame`). `ReadRecord` impl uses `GetFileName`, `KairoPlugin.GetPreference`, `Storage.Read`, `Encrypter.Decode` (×2), `EncodeUtil.ByteAryToLong`, `CRC64.GetValue`, `IllegalFileException` — order, key derivation, polynomial, checksum position/coverage unknown. Must not be removed or brute-forced |
| SecureProperty | Array of byte blocks at `+0x10` indexed by selector; second array at `+0x20` (system save options); built from five `AppData` statics. `Load`/`Read`/`Write`/`ToByteArray` layout unknown |

### Game-slot blocks by selector

| Selector | Block | Layout |
|---:|---|---|
| 0, 7, 8, 9, 10, 11, 14, 15, 16, 17 | BaseData sections (CharaData, AreaData, MagicShopData, QuestData, EquipmentData, BuildingData, EventData, HatData, ClothData, KingRewardData) | [BaseData section](#basedata-section) |
| 2 | Delayed events | [Delayed-event block](#delayed-event-block) |
| 13 | World | `EntityManager` block then `SystemManager` block, same stream: [entity-save.md](entity-save.md) |
| others | unknown | — |

Reading order in `LoadGame`: BaseData sections (descriptor order: 11, 0, 7, 8, 9, 10, 14, 15, 16, 17) → 13 → 2.

All integers are **big-endian**, signed. Each block is read through `ByteArrayInputStream` over a `MemoryStream`; reading past the block end fails (no zero-loop).

### BaseData section

| Offset | Size | Type | Meaning |
|---|---|---|---|
| 0 | 2 | int16 BE | record count (sign-extended; `<1` = none) |
| repeated: | | | |
| +0 | 2 | int16 BE | index into the data array |
| +2 | 4 | int32 BE | record size |
| +6 | size* | — | record payload, consumed by `<Type>.Deserialize` (then `Deserialized`) |

\* `size` is used only to skip records whose index is out of range or whose slot is null; for present records the payload length is whatever `Deserialize` reads. Negative index → exception. Per-type payloads not recovered.

### Delayed-event block

| Offset | Size | Type | Meaning |
|---|---|---|---|
| 0 | 4 | int32 BE | count (`<1` = empty) |
| then count × | var | DelayEvent record | below |

### Delayed-event record

| Order | Type | Field (object offset) |
|---:|---|---|
| 1 | int32 BE | `delayId_` (`+0x10`) |
| 2 | int32 BE | `line_` (`+0x14`) |
| 3 | int32 BE | `wait_` (`+0x18`) |
| 4 | int32 BE | `hour_` (`+0x1c`) |
| 5 | int32 BE | `talkChara_` (`+0x20`) |
| 6 | save string | `args_` (`+0x28`); writer substitutes `""` for null |
| 7 | int32 BE | `argi_` length; writer emits 0 for null |
| 8 | int32 BE × length | `argi_` elements |

Reader stores each field as soon as it is read (partial update on failure); length 0 → empty array (null/empty not preserved); negative length → invalid allocation. Empty-constructor defaults: `[-1, 0, 0, -1, -1]`, `args_ = ""`.

### Save string

| Offset | Size | Type | Meaning |
|---|---|---|---|
| 0 | 4 | int32 BE | byte length; `-1` = null, `0` = empty |
| 4 | length | bytes | written as UTF-8 |

Reading decodes by detection (UTF-8, then Shift_JIS, rejecting U+FFFD; BOM stripped on the UTF-8 fallback), not unconditional UTF-8. If `Config.SAVEDATA_LOCALIZE` is set, `WriteTranslate` runs before writing and `ReadTranslate` filters the result after reading. Other negative lengths: undefined here.

## Reading it

- No reader in `tools/` or `src/` for the envelope, BaseData sections, DelayEvent or save strings. Only the separate script-row payload has a codec ([script-payload.md](script-payload.md)).
- Do not implement DelayEvent with a presumed UTF-8 string prefix until decode detection and localisation are closed.
- Any validation must use a save copied in an authorised environment, preserving the original.

## Evidence

- [../archive/reports/20-persistence-entrypoints.md](../archive/reports/20-persistence-entrypoints.md), [46-save-order-and-callers.md](../archive/reports/46-save-order-and-callers.md), [47-save-stream-eof.md](../archive/reports/47-save-stream-eof.md), [48-system-save-and-data-loading.md](../archive/reports/48-system-save-and-data-loading.md), [52-data-list-and-check-timing.md](../archive/reports/52-data-list-and-check-timing.md), [55-delay-event-save-block.md](../archive/reports/55-delay-event-save-block.md), [56-save-strings.md](../archive/reports/56-save-strings.md), [57-save-localization.md](../archive/reports/57-save-localization.md)
- Local (`build/research/`): `save-callers.tsv`, `save-input-preparation/`, `delay-event/`, `save-string/`, `save-string-helpers/`, `string-decode/`

## Open questions

1. (blocking) Record envelope: encryption, CRC64 and `SecureProperty` serialisation.
2. (blocking) Complete selector set and the writer of BaseData sections; per-type `Deserialize` payloads.
3. (blocking) Save-string decode detection beyond the first UTF-8/Shift_JIS attempt; translation table source and flag values.
4. System-save (record 0) block contents.
