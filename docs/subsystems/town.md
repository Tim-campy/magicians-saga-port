# Town data, residents and selection

The town is SelectForm UI state backed by TownSystem records and resource archives, rather than the mission's ECS map. The S2 host projects native fresh Town initialization and building-cache operations from extracted data; drawing, residents and the town-to-mission loop remain unbound.

## Overview

`SelectForm.Init` (`0x11e9c28`, 5644 bytes) retains MemberSystem at `+0x1b8` and TownSystem at `+0x1c0`. It owns a separate `townHuman_` list at `+0x1d0`. `_Draw` (`0x11f0714`, 10820 bytes) consumes TownSystem.GetBuildingList and DrawTownHuman, using the `select`, `resident` and `select_stage` archives. Reusing the mission's Map/Cell renderer would not implement this ownership model. Confidence high for these direct accesses/calls; complete rendering-provider semantics remain open.

The maintainer's extracted files are `textassets/20_select.bin`, `textassets/34_resident.bin` and `textassets/26_select_stage.bin`. These are local asset filenames, not ECS factory indices or inferred AppData resource IDs. Town table data comes from the indexed `xls` archive using the existing resource lookup profile.

## Key methods

| Method | Registry id | Role | Confidence |
| --- | --- | --- | --- |
| ecs.TownSystem.Init | game:0x06000feb | Fresh permissions/list and saved town capacity | high |
| ecs.TownSystem.SetBuildingList | game:0x06000ff1 | Ordered triple replacement/append; optional cache reset | high |
| ecs.TownSystem.GetBuildingList | game:0x06000fed | Lazily published capacity-indexed building cache | high |
| ecs.TownSystem.ResetBuildingList | game:0x06000fee | Invalidate cache and repair missing source triples | high |
| form.SelectForm.Init | game:0x060007ff | Own town/resident UI state; later reset building cache | medium |

## Behaviour

### Building list and cache

TownSystem owns an ordered list of `int32[3]` at `+0x20`, permission bytes at `+0x28` and a nullable cache at `+0x30`. A triple contains town index, building ID and value. The decoded normal-save integer slot19 supplies cache capacity. Fresh Init seeds the list from TownData.firstBuild for flag1 records and raises slot19 only when the flagged count is greater; it does not run ResetBuildingList. [Initialization](ecs-lifecycle.md#fresh-town-initialization) defines those gates and the forward-removal quirk of SetBuildingList.

GetBuildingList (`0x1169c44`, 452 bytes) returns an existing cache directly, without reading capacity or the source list. Otherwise it allocates and publishes a slot19-sized reference array before enumerating the source list. Each row must have at least three cells: read its signed town index, allocate a fresh three-int row, copy cells0..2, and store the copy at that index. Extra cells are ignored, gaps stay null and later duplicates overwrite earlier entries. The source row is not aliased. A late malformed row/index leaves the already-published partial cache available to the next call.

ResetBuildingList (`0x1169e08`, 604 bytes) clears `+0x30` before reading capacity, allocates an unpublished temporary projection and fills it with the same cloning/index rules. It then scans temporary slots in ascending order. For each null slot, read TownData at that index and call SetBuildingList(index, firstBuild, 0, true). This repairs source rows without recursively resetting the cache. The temporary array is never published and the cache remains null on success. Later failure retains earlier repairs. No entity creation, rendering or destruction occurs in this method. SelectForm.Init calls it at `0x11eaf88`.

### Resident ownership and remaining initialization

Each accepted resident receives a fresh zero-initialized `int32[11]`, with metadata indices ID, X, Y, VX, VY, SEB, DIR, MOVE, TYPE, FRAME and EX_VAL. Init supplies SEB38/direction2; VX/VY start zero. Living sub-members use the building position of their address, ordinary records can use town or area positions, and fallback placement consumes RNG. Appearance-rate Hits precedes a nonempty resident-term check. Successful rows consume Random60 and Random9999 after any position/lane draws. These observations establish the owner/data requirements, not a complete port of Init.

Decorations and clouds consume RNG before resident construction. A later Hits10 branch can change the first eligible row to type3 and draws the resident-present value even without an eligible row. Their draws cannot be silently omitted while claiming faithful resident initialization. The current combat Chara projection discards resource, voice, town, area, residentTerm and appearRate fields; these and Building/Voice records must reach the town provider before those consumers execute.

### Quest selection boundary

The area action stores normal-save slot8. SelectStageForm enumerates the selected Area.stages and filters by quest discovery; confirmation stores QuestData.id in slot9. SelectStageForm.Update checks/consumes stamina and waits41 production-state updates before changing to QuestForm. Its stamina comparison lies at `0x1208084..0x1208094`, and the UseStamina call is at `0x1208150`. StartExplore separately establishes the selected records and exploration ledger. These are separate from the S1 CLI's explicit quest scenario. The existing selection kernels and evidence are described in [mission selection](integration.md#native-mission-selection-and-area-records-s1-a1).

## Port

`get_town_buildings` and `reset_town_buildings` live beside the Init/SetBuildingList kernels in [pass_initialization.cpp](../../src/core/gameplay/pass_initialization.cpp). Their cache reference represents native `+0x30` separately from the existing initialization state. [Contract tests](../../tests/test_pass_initialization.cpp) cover aliases, clones, duplicates, gaps, negative/out-of-range inputs, publication/failure prefixes and deferred repair order. Existing standard containers implement the storage operations; no public collection internals are reconstructed.

`saga_town --data extracted/full` reads the existing resource index and APK-derived new-game defaults, loads Town.txt, runs the fresh Town data branch, ResetBuildingList and GetBuildingList, then prints permissions, ordered source triples and the indexed projection. Its scope is the Town data subsequence, not all of SelectForm.Init. It does not skip resident RNG while claiming to initialize residents. Language/DPI are explicit CLI inputs with declared defaults English.lproj/1. Shared archive/index/default readers also serve `saga_mission`; no mission is bootstrapped to obtain town state.

The snapshot consumes TownData.firstBuild, permissions and slot19 only. Mutable Area/Building/KingReward NewGame state must be bound before town menus, condition evaluation or other consumers read those records. Source triples and the cache are in-memory data; this host does not write a save.

## Evidence

- Existing bounded `research/exports/select-gates/F11e9c28.asm` and `select-form-push/F11f0714.asm`: town ownership, resident allocation and draw consumers.
- Bounded `research/exports/town-entry/`: GetBuildingList, ResetBuildingList, GetLivingSubMemberArray, UpdateTownHuman and DrawTownHuman; all five have complete syntactic instruction coverage. Coverage proves available bytes, not semantic completion of the three unported resident methods.
- Get publishes cache at `0x1169d1c`; Reset invalidates at `0x1169e84`, fills the local projection through `0x1169fb0`, and repairs source gaps at `0x1169fdc..0x116a040`.
- Resident construction lies in Init `0x11ea5d4..0x11eadd4`; metadata indices and direct stores support the row layout. Full prior RNG/context and remaining helper contracts are still required.

The real-data command `build/desktop/saga_town.exe --data extracted/full` reads19 Town records, raises native capacity1 to4, retains three enabled permission entries and produces four source/cache triples, including native building ID-1 entries. Two runs produce identical output; desktop/viewer builds agree. Local `town-entry/town-data.json` has SHA256 `7ce6d5b240326646f999d04274b204306286cac2a6edce816061a214e1f87b7b`; its desktop executable SHA256 is `391e09c9f71f3c1a143c23ea464afdbfde499d6f084095981d2cef3917e777b3`.

Windows Release validation passes197 desktop and201 viewer CTest contracts plus all21 Python test files. Shared-reader refactoring preserves the complete reports and both hashes for all four real mission endings. The540-frame combat report matches the preserved mission executable, and its GPU capture matches the earlier pixels byte-for-byte; logic hash remains `b4600393cb097b19`. Local `town-entry/mission-regression*.json`/`.bmp` retain those observations. These checks establish local regression behavior, not Android equivalence or a playable town.

## Open questions

- (blocking) Bind SelectForm drawing and resident initialization with the real Chara/Building/Voice fields and archive owners, preserving decoration/cloud/resident RNG order.
- Establish GetBuildingPos (`0x11eb8f4`, 156 bytes), GetAreaPos (`0x11eb990`, 156 bytes), GetActiveX (`0x11eb714`, 480 bytes) and the initial resident filter (`0x12072c8`, 24 bytes) before constructing resident rows.
- Bind the actual area/quest-selection/stamina/form lifecycle and retained mission-return callback to one persistent town owner.
- Compare rendering, form timing and live configuration with the Android original when an oracle is available.
