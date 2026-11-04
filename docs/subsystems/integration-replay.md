# Mission replay checkpoints

The selected PC mission exposes deterministic checkpoints of its reached CPU state graph. Ordered replays and local per-quest goldens exercise S1 A5; they do not verify the original Android game, full application entry or save restoration. [Integration](integration.md) describes the host and its service boundaries; [mission input](integration-input.md) describes replay actions.

## Overview

`saga_mission --replay <file> --frames N` reports schema `mission-v1-xxh64`, `state_hash`, `logic_state_hash`, `state_hash_bytes` and `logic_state_bytes`. Both digests use public XXH64 with seed 0 and sixteen lowercase hexadecimal digits. The executable's declared gameplay seed is a separate input, default 42 for fresh mission entry.

The checkpoint is a typed field traversal of the selected mission owner graph, not a hash of the JSON report or C++ object memory. Unsupported component types, opaque payloads, service bindings, continuations and unlabelled multicast callbacks fail explicitly. It covers the state represented by the port's reached owners; it does not imply that all original game fields or systems have been reconstructed.

## Behaviour

### Canonical representation

- Integer fields use their explicit widths and little-endian bytes; signed values retain their unsigned bit representation. Booleans occupy one byte, enums use int32, and collection lengths use uint64.
- `binary32` fields use their IEEE bits, preserving signed zero and NaN payloads. Object padding, allocator addresses and compiler object layout are absent. The archive rejects implicit `double` serialization.
- UTF-8 strings contain a byte count and exact bytes; UTF-16 strings contain a code-unit count and little-endian uint16 units. No locale conversion or Unicode normalization occurs during traversal.
- Objects and shared containers receive uint64 reference IDs by first encounter in deterministic traversal order, with 0 for null. Schema domains and an internal C++ type discriminator distinguish identities, including base/derived objects sharing an address. `std::type_index` is only a lookup discriminator: type names, hash codes, addresses and compiler RTTI bytes are never emitted.
- Aliases and cycles remain explicit. Two independent arrays with equal contents differ from two fields sharing one array. The writer anchors identity before expanding fields and emits a referenced object's contents once.

Registry and subset snapshots retain storage order, vacant slots, free-list heads/links and versions. The no-slot sentinel becomes `UINT64_MAX` on every platform. Registry keys are emitted in signed numeric order; subset lookup projections use slot index and stored hash. Dynamic-group publication order and bucket chains remain distinct. Dictionary bucket capacity and machine-dependent lookup structures are excluded where their contents are fully represented by the canonical projection.

### Reached graph

| Owner | State included |
| --- | --- |
| Entities/world | ID counter and stable handles, loaded/player cache, registered and temporary entities, destroyed flags, component slots/payloads, retired components, registry/subset tombstones, filters and cached filter hashes. |
| Gameplay systems | Stage/quest/roster/formation, Human/Enemy counters and section arrays, projectile/collision/expiry/EXP queues, drops/rewards, gauge/bubble/button/info state, camera/cell/visibility/cursor state, configuration and initialization/update gates. |
| Input/forms/events | Held keys and pointer captures, widget regions, pending replay actions in file order, frame pulses, modal stacks/dialog answers, event execution counts/delays/arguments, talk pages/reveal/layout state and pending result grants. |
| Progression/results | Live decoded character/equipment/shop/quest/inventory state, collection totals, grant/finish cursors and their versions, held return continuation and its captured result/host. |
| Data/resources | Effective bootstrap/profile options, selected catalog data, source archive fingerprints, selected font content fingerprint, shared resource/image clocks and animation frames. Workstation file paths are not identity inputs. |

RNG coverage includes the seeded `LegacySystemRandom` object's 56 cells and both indices, its draw counter, effective configuration and weighted-lottery scratch. A seed/draw-count pair alone is not used as the live RNG snapshot. Unsupported runtime RNG backends remain rejected.

Multicast events retain channel keys, receiver order, tracking versions, optional native delegate hashes, wrapper aliases and semantic method IDs. Callable receivers without semantic IDs fail. Fixed callback slots use their containing schema/service binding; callback code and closure memory are never hashed. The held town-return callback must be the typed `QuestTownContinuation`; its captured result and host are visited instead of treating any nonempty `std::function` as equivalent.

### Logic and presentation boundary

`logic_state_hash` covers the archive prefix containing the gameplay owners and their live inputs. `state_hash` covers that same prefix plus the presentation suffix: reached render queues, SEB packed/cache/work/bounds/offset state, retained sprite scratch, drawing styles, font scopes and selected drawing metadata. A window may warm SEB work that headless output does not need, so presentation equality is a separate claim from logic equality.

The shared gauge alpha scratch belongs to the **logic prefix**. Crop reads overwrite only a prefix and preserve the tail; native `UpdateGaugeDisplay` scans that retained buffer to derive image geometry. Excluding it as a drawing cache would omit a future game input.

Each successful mission step produces the selected native draw-side semantic queries with null output in headless mode. This includes live player/stat/resource queries, gauge crops and control registration. The window additionally submits bitmaps; repeating its drawing pass must be idempotent for game fields. Pure SEB/style work remains in the presentation suffix. This preparation does not claim the complete original ordinary drawing schedule.

`mission_state_bytes(const MissionProfile&)` and `mission_state_digest(const MissionProfile&)` traverse const state without calling lazy gameplay getters or drawing. The digest in `write_combat_report` is computed **before** that report's subsequent draw/report work. The report remains mutating; callers needing observational reads must use the digest APIs rather than repeatedly invoking the JSON report writer.

Exclusions are declared: GPU/platform renderer objects and submitted pixels, derived diagnostic draw-request copies, FreeType handles/provider internals, profiling/wall-clock measurements, allocator/container capacities and derived address lookup tables. Content fingerprints retain the selected font/archive identity; public font geometry and original rendering equivalence remain separate service/oracle questions.

## Port

`src/core/ecs/state_archive.*` owns canonical bytes and typed reference IDs. `state_component_fields.hpp`, `state_system_fields.hpp`, `state_data_fields.hpp` and `state_archive_access.hpp` enumerate public fields and selected private container/system storage. `src/platform/desktop/mission_checkpoint.cpp` binds those schemas to the actual mission graph, opaque payload types and named services. It has no unknown-type fallback.

The build uses [xxHash 0.8.3](https://github.com/Cyan4973/xxHash/tree/v0.8.3), BSD-2-Clause, pinned by source URL and SHA256 in `cmake/XxHash.cmake`. Upstream code stays in local `build/_deps/`; the project does not reconstruct the public hash algorithm.

`tools/mission_golden.py` selects the actual tables archive from `mission-resources-v2.tsv`, falling back to v1 only when v2 is absent. It requires one tables/-1/xls row and a bounded direct `textassets/` child. The existing `extract_assets.unpack_container` reads the Jar. The runner supports the exact ASCII/invariant `language/Quest.txt` member and rejects competing root/DPI candidates rather than guessing native precedence. The host retains full index and table validation.

Quest enumeration projects the first TAB-delimited ID, preserves gaps and rejects duplicate, negative or malformed IDs. The maintainer's selected English/Japanese tables contain 76 quests, IDs `0..75`. `--quests 1,4` restricts the batch; omitting it selects every enumerated ID.

For each quest the runner launches two sequential headless runs with identical argv tokens. It compares the entire parsed JSON, including both hashes, schema, byte counts and actual frame counts. JSON key order is irrelevant; raw table/report text is not copied into the output. The local summary records source index/table/archive hashes, replay/executable SHA256, effective seed/language/DPI/frame request, per-run checkpoint fields and a canonical report SHA256.

| Status | Meaning |
| --- | --- |
| `matched` | Both successful reports agree and execute all requested frames. |
| `held` | Both successful reports agree but execute fewer frames; actual counts remain recorded. This is not a completed N-frame run. |
| `failed` | Launch error, timeout, nonzero host return or invalid checkpoint JSON. |
| `mismatch` | Successful repeated reports differ, even if their checkpoint hashes agree. |

Failed/mismatched batches return nonzero. stderr is capped at 1KiB per run; timeout is explicit, default 120 seconds. Existing output files are rejected before launching and protected by exclusive creation at write time. Goldens belong under gitignored `research/`, not in the public repository.

## Evidence

Canonical archive tests cover fixed-width bytes, `binary32` distinctions, references and nested arrays sharing an address with their first element. Component/system and checkpoint contracts exercise aliases, cycles, tombstone/free-list differences, subscriptions, RNG state, hidden offsets, unsupported payload rejection and repeated observational digest reads. The 196 desktop tests include a transparent narrow gauge crop following a wider opaque crop, proving retained-alpha parity between null and bitmap output and repeated-pass idempotence. Four selected viewer checks and all 20 Python test files pass; the golden runner has 11 synthetic tests with mocked subprocesses and no game bytes.

The real quest1 540-frame replay retains three shots,174 RNG draws and total damage10. Against preserved playtest-1ce0a06, its only report changes are the corrected base update counter (+1) and the two state digests; all byte counts and other fields match. Its headless full/logic hashes are `a6f8e264c6fe3830`/`6c62139ed1bdd724`, with 1393601/1248437 bytes. Ordinary and Debug-mask456 captures repeat byte-identically at 66/329 quads; their logic hashes agree with headless execution, and ordinary pixels match the prior control. These observations are a regression check, not proof that report fields alone exhaust the state graph.

Local `research/exports/mission-checkpoint/goldens-update-counter0.json` records all 76 quests, IDs0..75, twice at 540 frames and seed42: 76 matched, 0 held, 0 failed, 0 mismatched (82080 executed frames). Its SHA256 is `334b1e0229402aad2fd2d79b99482c72ef6dd07ed3130113d6ec35293fb9fc94`; it pins executable `a8dc06d654322787020947d1b26b9c37003e91d99f0556a5824fad02effae297` and replay `6bd9772e28f77344ebed65d02e85bb4273b55d06b6ad7b47a463e0c97dfbe7b2`. This is the local S1 A5 reference for that exact schema, data, command context and finite input sequence; it is not complete mission-content or original-runtime validation.

```powershell
python -I tools/mission_golden.py --exe build/desktop/saga_mission.exe --data extracted/full --replay research/exports/mission-move-input/real-playable-input.replay --out research/exports/mission-checkpoint/goldens-update-counter0.json --frames 540 --seed 42 --language English.lproj --dpi 1
```

## Open questions

- Extend regression inputs beyond the 540-frame movement/combat sequence when closing mission endings and additional content; new contexts require separate local references.
- Compare the reached port state and timing with an original runtime trace when an Android oracle is available. Local hashes do not establish `verified` status, original font geometry or full application/save equivalence.
