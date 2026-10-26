# Game tables

The 21 static data tables (`Event.txt`, `Cloth.txt`, `Voice.txt`, ...) are UTF-8 text members read through `JarInflater` by `Data.Load`, split into lines on LF, turned into `BaseData` records by a per-type delegate and stored in id-indexed arrays. Encoding, line splitting and indexing are established statically and audited on the 44 extracted text files; the per-line field syntax is owned by the record loaders (event tables: [events.md](../subsystems/events.md)). This doc covers framing only, not contents.

## Layout

| Level | Rule |
|---|---|
| Container | Member of a `JarInflater` archive (see [assets.md](../subsystems/assets.md)); name is the literal table name, e.g. `Event.txt` (GOT `0x2731d30`). Missing stream → `LoadStringArray` returns null |
| Encoding | UTF-8 (`Encoding.get_UTF8`, `0x1d07230`). Optional BOM `EF BB BF` is skipped (decode from offset 3). Invalid-sequence policy not recovered |
| Lines | Split on `\n` (literal GOT `0x272fa98`). Zero-length segments are dropped; segments with only spaces are kept; no trim; no escaped-separator handling |
| CRLF | Replaced by LF before splitting **only** if `UnityUtil.CheckUnityVersion("6000.0.0")` is true. This build is Unity 2022.3, so CR stays at the end of each segment, and a blank CRLF line yields a segment `"\r"` (not dropped) |
| Records | One record per segment via `<LoadData>b__74_0` (body not located) |
| Indexing | Array of size `wrap32(maxId+1)` where `maxId` = max of `id_` (`BaseData+0x18`) over records, initial max 0; empty input → size 1. Each record stored at `array[id]` in sequence order |

Indexing edge cases (helper `0x129fafc`):

- gaps stay null; do not fill with defaults or compact ids;
- duplicate ids: last occurrence wins;
- negative id → index-check exception (not skipped);
- null record → dereference fault (not filtered);
- size overflow: runtime allocation error, no recovery policy.

Line count, max id and array size are distinct quantities. Example (modelled, `build/research/event-index-model.json`): both extracted `Event.txt` files give array size 229, one gap at index 180, no duplicates; id 1133 (referenced elsewhere) lies outside the array and is ignored by the `ExecEvent` guard — do not extend the array.

## Reading it

- Original chain: `Data.Load` (`0x1170508` → `0x117057c` → `0x1170974`) → `StringUtil.LoadStringArray(jar, name, true)` (`0x1c19ce4`: `JarInflater.GetInputStream`, then overload `0x1c19dc0`; stream closed in the dispose path; exception handlers exist and must not become "return empty array") → `StreamUtil.Read` (14433) → `JString.New(bytes)` (`0x1b120f0`) → `StringUtil.Split` (14535) → `LoadData<T>` (`0x129fbe8`) → indexing helper `0x129fafc` → `Data` field.
- `CheckUnityVersion` (`0x1c1ea50`) caches its result, parses the runtime Unity version into numeric components and returns true when ≥ threshold. Runtime value not captured; keep the condition.
- Audit: `python tools/audit_game_table_text.py data/extracted/members/06_xls --output build/research/game-table-text-audit.json` — 44 files strict UTF-8, no BOM, 5103 CR = 5103 LF = 5103 CRLF pairs, no lone CR. Records hashes/sizes/segment counts, no content. Not an emulation of the original decoder.
- `src/core/gameplay/event_catalog.cpp` applies this framing and indexing to EventData with an explicit CRLF-normalization gate. It preserves trailing CR and drops only zero-length LF segments. `progression_tables.cpp` also reads TableData and the CharaData progression projection using the same framing and shared token reader; schemas and limits are in [data.md](../subsystems/data.md#progression-record-readers). `enemy_rewards.cpp` reads EnemyData and its count-prefixed nested arrays; `quest_rewards.cpp` reads TreasureData; its framing/indexing helper is shared with progression readers. Remaining record types and invalid UTF-8 decoding remain unported.

## Evidence

- [../archive/reports/73-game-data-id-indexing.md](../archive/reports/73-game-data-id-indexing.md), [74-game-data-text-source.md](../archive/reports/74-game-data-text-source.md), [75-table-encoding-and-line-endings.md](../archive/reports/75-table-encoding-and-line-endings.md)
- Local (`build/research/`): `data-generic-load/`, `data-load-helpers/`, `data-load-entry/`, `data-text-reader/`, `string-lines/`, `string-lines-helpers/`, `data-encoding/`, `event-index-model.json`, `game-table-text-audit.json`

## Open questions

1. (blocking) Per-record constructor `<LoadData>b__74_0`: which `Load` it calls and how it treats invalid lines and the trailing CR.
2. Complete list of the 21 names, their order and destination fields.
3. UTF-8 decoder behaviour for invalid input (not exercised by the shipped files).
4. Runtime value of `CheckUnityVersion("6000.0.0")` (expected false for 2022.3, not observed).
