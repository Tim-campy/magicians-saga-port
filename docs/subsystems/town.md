# Town data, residents and selection

The town is SelectForm UI state backed by TownSystem records and resource archives, rather than the mission's ECS map. The S2 host runs native fresh Town/building data, Area reset/camera kernels and the initial SelectForm background drawing prefix from extracted data. Decorations, building/resident drawing and the town-to-mission loop remain unbound.

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
| form.SelectForm._Draw | game:0x0600080b | Initial color fill and tiled/parallax SEB background; remaining suffix unported | high for prefix |
| data.AreaData.NewGame / oldNewGame | game:0x06001012 / game:0x06001014 | Conditional Find followed by ordered mutable progress reset | high |

## Behaviour

### Building list and cache

TownSystem owns an ordered list of `int32[3]` at `+0x20`, permission bytes at `+0x28` and a nullable cache at `+0x30`. A triple contains town index, building ID and value. The decoded normal-save integer slot19 supplies cache capacity. Fresh Init seeds the list from TownData.firstBuild for flag1 records and raises slot19 only when the flagged count is greater; it does not run ResetBuildingList. [Initialization](ecs-lifecycle.md#fresh-town-initialization) defines those gates and the forward-removal quirk of SetBuildingList.

GetBuildingList (`0x1169c44`, 452 bytes) returns an existing cache directly, without reading capacity or the source list. Otherwise it allocates and publishes a slot19-sized reference array before enumerating the source list. Each row must have at least three cells: read its signed town index, allocate a fresh three-int row, copy cells0..2, and store the copy at that index. Extra cells are ignored, gaps stay null and later duplicates overwrite earlier entries. The source row is not aliased. A late malformed row/index leaves the already-published partial cache available to the next call.

ResetBuildingList (`0x1169e08`, 604 bytes) clears `+0x30` before reading capacity, allocates an unpublished temporary projection and fills it with the same cloning/index rules. It then scans temporary slots in ascending order. For each null slot, read TownData at that index and call SetBuildingList(index, firstBuild, 0, true). This repairs source rows without recursively resetting the cache. The temporary array is never published and the cache remains null on success. Later failure retains earlier repairs. No entity creation, rendering or destruction occurs in this method. SelectForm.Init calls it at `0x11eaf88`.

### Resident ownership and remaining initialization

Each accepted resident receives a fresh zero-initialized `int32[11]`, with metadata indices ID, X, Y, VX, VY, SEB, DIR, MOVE, TYPE, FRAME and EX_VAL. Init supplies SEB38/direction2; VX/VY start zero. Living sub-members use the building position of their address, ordinary records can use town or area positions, and fallback placement consumes RNG. Appearance-rate Hits precedes a nonempty resident-term check. Successful rows consume Random60 and Random9999 after any position/lane draws. These observations establish the owner/data requirements, not a complete port of Init.

Decorations and clouds consume RNG before resident construction. A later Hits10 branch can change the first eligible row to type3 and draws the resident-present value even without an eligible row. Their draws cannot be silently omitted while claiming faithful resident initialization. The current combat Chara projection discards resource, voice, town, area, residentTerm and appearRate fields; these and Building/Voice records must reach the town provider before those consumers execute.

### Area state and camera

AreaData.NewGame calls virtual Find(true) when flags bit0 is set, then oldNewGame. The default Find overwrites discovery and is_new through BaseData.Find; a callback failure prevents all later reset work. oldNewGame replaces the three-row/six-int stage-detail array (`+0x20`), the three-row/two-int damage array (`+0x38`) and six-int area-detail array (`+0x28`), retaining the native allocation/publication order. It zeroes arrival-stage (`+0x30`), then rereads flags and writes bit0 to discovery (`+0x10`); is_new is not cleared by oldNewGame. Earlier array aliases retain their old storage. These resets do not serialize a save.

The background projection loads the full Area table, applies those kernels and supplies its discovery values to the existing SetActiveX/SetCameraPosition contracts. It does not infer discovery from filenames or choose a visible region from appearance. View dimensions, scale100, AppData view origin0/53 and the camera request0/-9999 are declared desktop context inputs; the native camera clamps that request. [Camera arithmetic](events.md#select-camera-limits-and-inertial-movement) defines the bounds and separate GameView/SelectForm dimensions.

### Initial drawing prefix

`draw_select_background` implements `_Draw` at `0x11f0850..0x11f0a6c`, ending before DrawDecoObject(type0). It does not stand in for the complete form. The earlier ResetTouchList/IsTopForm calls and all later decoration, construction, resident, cloud, marker and menu work remain outside this selected segment.

Read resSelect.images[RESSELECT_IMG_BG].GetPixel(0,0); multiply RGB by binary32 255, convert with ARM64 FCVTZS semantics and pass alpha255 to Color. Fill `(0,0,SelectWidth,SelectHeight)`. Capture baseY as wrapped cameraY+VIEW_Y. Start tileX at the captured left extent; before each test reread the right extent and resSelect. While tileX is smaller, query raw GameView height, require the captured resource, read the live background clip and draw frame0 at `(tileX, baseY-125+viewHeight)`, then wrap-add240. The final false test still refreshes resSelect. Draw frame1 using fresh signed cameraX/30 truncated toward zero, plus30, and captured baseY+20. These are explicit frames, not the resource's current clock frame. Resource-null failures preserve the native height/clip access prefixes.

The background selectors are established constants for this APK, not provisional guesses: RESOURCE[1][11] is `select`; RESSELECT_SEB_BG static `+0x164` is0 and RESSELECT_IMG_BG static `+0x1e0` is9. Native GetPixel flips the texture's bottom-up Y using height-y-1; decoded top-down RGBA sampling uses its top-left pixel. The original texture color-space/atlas policy still needs runtime comparison.

### Quest selection boundary

The area action stores normal-save slot8. SelectStageForm enumerates the selected Area.stages and filters by quest discovery; confirmation stores QuestData.id in slot9. SelectStageForm.Update checks/consumes stamina and waits41 production-state updates before changing to QuestForm. Its stamina comparison lies at `0x1208084..0x1208094`, and the UseStamina call is at `0x1208150`. StartExplore separately establishes the selected records and exploration ledger. These are separate from the S1 CLI's explicit quest scenario. The existing selection kernels and evidence are described in [mission selection](integration.md#native-mission-selection-and-area-records-s1-a1).

## Port

`get_town_buildings` and `reset_town_buildings` live beside the Init/SetBuildingList kernels in [pass_initialization.cpp](../../src/core/gameplay/pass_initialization.cpp). Their cache reference represents native `+0x30` separately from the existing initialization state. [Contract tests](../../tests/test_pass_initialization.cpp) cover aliases, clones, duplicates, gaps, negative/out-of-range inputs, publication/failure prefixes and deferred repair order. Existing standard containers implement the storage operations; no public collection internals are reconstructed.

`saga_town --data extracted/full` reads the existing resource index and APK-derived new-game defaults, loads Town.txt, runs the fresh Town data branch, ResetBuildingList and GetBuildingList, then prints permissions, ordered source triples and the indexed projection. Its scope is the Town data subsequence, not all of SelectForm.Init. It does not skip resident RNG while claiming to initialize residents. Language/DPI are explicit CLI inputs with declared defaults English.lproj/1. Shared archive/index/default readers also serve `saga_mission`; no mission is bootstrapped to obtain town state.

The base snapshot consumes TownData.firstBuild, permissions and slot19. Background mode additionally resets Area progress and binds the existing camera kernels. Mutable Building/KingReward NewGame state remains required before town menus or other consumers read those records. Source triples, Area progress and the cache remain in memory; this host does not write a save.

`--background` executes the drawing prefix through the shared CPU bitmap/SEB adapter and reports its requests. Viewer builds accept `--capture <new.bmp>` for a hidden GL capture using the existing mesh/material/submission adapters. `--width`, `--height`, `--view-x`, `--view-y`, `--camera-x` and `--camera-y` supply the declared projection context; view/form scale100 is the supported profile. Captures are partial town frames, without a claim that full SelectForm.Init or its resident RNG ran.

Generate `town-resources.tsv` with `python -I tools/extract_assets.py --town-resources --out extracted/full`. It indexes the already extracted, hash-checked xls/select/resident/select_stage archives; no Unity media is extracted again. The shared index reader keeps the mission index and plain town data mode compatible. The ordinal13 entry names select_stage; no new GetResourceManager selector behavior is implied.

## Evidence

- Existing bounded `research/exports/select-gates/F11e9c28.asm` and `select-form-push/F11f0714.asm`: town ownership, resident allocation and draw consumers.
- Bounded `research/exports/town-entry/`: GetBuildingList, ResetBuildingList, GetLivingSubMemberArray, UpdateTownHuman and DrawTownHuman; all five have complete syntactic instruction coverage. Coverage proves available bytes, not semantic completion of the three unported resident methods.
- Get publishes cache at `0x1169d1c`; Reset invalidates at `0x1169e84`, fills the local projection through `0x1169fb0`, and repairs source gaps at `0x1169fdc..0x116a040`.
- Resident construction lies in Init `0x11ea5d4..0x11eadd4`; metadata indices and direct stores support the row layout. Full prior RNG/context and remaining helper contracts are still required.
- Selector provenance: AppData cctor `0x11192b0` stores lane3 of ELF `0x625cd0` at static `+0x164` (0); `0x11192c8` stores lane2 of `0x6261a0` at `+0x1e0` (9). RESOURCE name assignment for slot11 is `0x11184e0..0x11184f0`. SelectForm.Init publishes resSelect at `0x11e9f50`, calls RecordStore.ReadRecord(1,11) at `0x11e9f6c`, and loads mode3 at `0x11e9f84`.
- `town-background/` retains bounded MyFormBase.Color and SelectForm cctor exports; `town-color/Town_RGBA.asm` confirms clamped ARGB32 packing; `town-pixel/Town_GetPixel.asm` establishes the native Y flip and readiness/atlas boundaries. Existing `basedata-reset-callees/M38033_AreaData_oldNewGame.asm` supplies the complete Area reset body.

The real-data command `build/desktop/saga_town.exe --data extracted/full` reads19 Town records, raises native capacity1 to4, retains three enabled permission entries and produces four source/cache triples, including native building ID-1 entries. Two runs produce identical output; desktop/viewer builds agree. Local `town-entry/town-data.json` has SHA256 `7ce6d5b240326646f999d04274b204306286cac2a6edce816061a214e1f87b7b`; the plain data report matches that control unchanged.

`build/viewer/saga_town.exe --data extracted/full --capture <new.bmp>` renders the native sky and sun prefix. At600x640 the native camera clamps to(0,-80), extents are[-729,960], fill is ARGB32 `0xff353f4f`, and nine draw requests produce11 quads. Eight frame0 tiles start at x=-729 with240 spacing and y488; frame1 is at(30,-7). Area discovery is[1,0,0,0,0,0,0,0,0]. Local `town-background/default.bmp` has SHA256 `012836f7d56637f13d1cc5910ddbf56ece27e5c85a792243fc7cf1583cc72109`.

Adding `--width 480 --height 320 --camera-x -60 --camera-y 0` gives camera(-60,0), extents[-609,840], eight requests and10 quads. Local `town-background/shifted.bmp` has SHA256 `c905950760858542b41a64ada919090a1e58042914405e8f0615cff95d2f0036`. For both contexts, CPU `--background`, GL capture and repeated GL capture produce identical complete JSON reports; repeated BMP files are byte-identical. These images contain sky/sun and the background fill, without ground, decorations, buildings or residents. Synthetic pixel tests check the complete output, tile placement, camera parallax, repeated capture and headless parity.

Windows Release validation passes197 desktop and202 viewer CTest contracts plus all22 Python test files. The complete reports and both hashes for all four real mission endings match the preserved `playtest-f40d711` executable. The540-frame combat report also matches, and its GPU capture matches the earlier pixels byte-for-byte; logic hash remains `b4600393cb097b19`. Local `town-background/mission-*.json` and `mission-regression-540.bmp` retain those observations. These checks establish local regression behavior, not Android equivalence or a playable town.

## Open questions

- (blocking) Bind the remaining SelectForm decoration/building/resident layers and initialization with real Chara/Building/Voice fields, preserving decoration/cloud/resident RNG order.
- Establish GetBuildingPos (`0x11eb8f4`, 156 bytes), GetAreaPos (`0x11eb990`, 156 bytes), GetActiveX (`0x11eb714`, 480 bytes) and the initial resident filter (`0x12072c8`, 24 bytes) before constructing resident rows.
- Bind the actual area/quest-selection/stamina/form lifecycle and retained mission-return callback to one persistent town owner.
- Compare rendering, form timing and live configuration with the Android original when an oracle is available.
