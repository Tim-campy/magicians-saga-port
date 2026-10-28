# Roadmap

The plan in one page. Agents take the first unblocked task of the current work package (or what the maintainer asks), do it, and then edit this file: delete finished tasks, add the follow-ups they found. Keep each task one line, concrete and sized for one session.

## Milestones

| Milestone | Goal | State |
| --- | --- | --- |
| M0 | APK inventory, architecture, subsystem map | Done: [overview](subsystems/overview.md). |
| M1 | Asset extraction and technical rendering | Partial: containers, OPT, SEB, Unity media extracted and verified; scene rendering, fonts and audio playback not ported. |
| M2 | Boot, forms/menus and input | Static research only. |
| M3 | First playable gameplay loop, deterministic tests | **Current.** Partial deterministic gameplay schedules are tested; no playable loop. |
| M4 | Save/load, progression, economy, content coverage | Research and the script-payload codec; save envelope not decoded. |
| M5 | Windows/Linux release candidate | Windows builds and CTest; Linux builds in CI. |
| M6 | 3DS feasibility benchmark | Not started; nothing is claimed. |

## Work packages (priority order)

Current maintainer priority: game-specific decompilation and gameplay integration. Start with the experience/level progression cluster below, then combat and stage completion. Public/runtime dependency reconstruction is deferred unless a concrete gameplay scenario cannot proceed through an explicit adapter. Consult public source first as required by AGENTS.md.

1. **Events and scripts** ([events](subsystems/events.md), [event table](formats/event-table.md)): the game's progression runs on Event.txt scripts.
2. **Game data and saves** ([data](subsystems/data.md), [save](subsystems/save.md)): table loader with ids and gaps; save envelope.
3. **Combat, RNG and rewards** ([combat](subsystems/combat.md), [rng](subsystems/rng.md), [rewards](subsystems/rewards.md)).
4. **ECS world and animation** ([ecs](subsystems/ecs.md), [animation](subsystems/animation.md)): the world that runs a quest stage.
5. **Text and fonts** ([text](subsystems/text.md), [fonts](subsystems/fonts.md)): dialogue and UI text.
6. **Boot, forms and frame loop** ([boot](subsystems/boot.md), [ui](subsystems/ui.md)): first form, form stack, host loop.
7. **Assets and rendering** ([assets](subsystems/assets.md)): resource chain to the screen.
8. **Platform replacement** ([platform](subsystems/platform.md)): what Android/Java services the PC host must provide or stub.
9. **3DS feasibility** (M6), after M5.

## Next tasks

### Current gameplay focus
- [ ] Bind the full system update schedule and event subscriptions around the tested live Projectile -> Move -> Collision passes and final Garbage pass; the 26-system order and all NeedsUpdate gates are ported/tested; generic Init dispatch and concrete Map/Human/Enemy/Projectile/Move/Collision/Garbage/Animation/Member initialization are ported/tested; other initialization and update bodies remain before composing the complete frame. Entity-event removal and teardown are ported with explicit original/wrapper hash inputs; extend payload bindings only as required by gameplay.
- [ ] Complete catalog/file/locale selection and populate production manager fields, bind effective global frame flags and implement image rendering around the ported effect/drop factories; missing/duplicate-ID placement, header-derived clocks and AppData resource selection are now ported, with a selected-file desktop scenario validated on all 303 SEBs; Cell and Image initialization now use actual payloads in both factories, and effects insert actual slot-10 playback; the reward suffix creates pending EXP/treasure entities through the concrete drop factory in tests. World.Destroy and registered-entity teardown preserve the group directory, with GetSubSet creation/dedup/population and live forwarding ported on top of ID selection, entity registration and component-event subscriptions. Projectile/Move/Collision/Garbage and Animation now have live group passes; effect creation feeds five tested animation passes; launch/landing and collision/expiration sequences pass integration tests.
- [ ] Bind the tested Start -> map construction -> party/enemy entry -> seven-state Update -> collection/recovery/boss entry -> result grants -> equipment levels -> completion -> town-return chain to production data and the full frame; port HumanSystem.Shoot and World.CreateBullet/spell selection, bind QuestForm's cursor UI, then add a desktop mission scenario. Human Init/Update now cover live state dispatch, movement/dash/recovery, XYZ acquisition, MP/timing/burst gates, position limits and invincibility; Human -> Enemy -> Move and both contact receivers are composed in tests. Shoot is a required unresolved producer, with no default success/projectile. Enemy Init now binds actual event 23/29/30 receivers; tests compose recoil/recovery, numeric damage/defeat/EXP and nested human-death/retarget/projectile cleanup. Map/MapChip factories and MapSystem.Init/LoadMap/range/grid updates now use the actual InitWorld geometry and are composed with full enemy updates. Resolve the mission-transition owner of previous map-chip entities: LoadMap replaces its grid without destroying them. UI/audio/ad and secure storage remain required. Human Continue scalar restoration is identified; script/visual-array codecs and load entry state remain open.

### 1. Events and scripts
- [ ] Bind decoded billing-item/catalog providers and complete notification manager adapters, preserving task execution gates.
- [ ] Port decor/town/capture callbacks, then bind update_select_form to a desktop host with orientation/image-dictionary storage, decoded saves, camera dimensions and form factories.
- [ ] Find the consumer of `hour_ != -1`, the writer of `SubForm.delayEvent_` (`+0x1d0`) and the remaining effect opcodes of the 15-entry table at `0x627b6e`.

### 2. Game data and saves
- [ ] Decode the save envelope: `SecureProperty.Load/Read/Write/ToByteArray`, `Encrypter.Decode` and the CRC64 check in `ReadRecord` (blocking for reading any real save).
- [ ] Find the body of `<LoadData>b__74_0` and the complete list of 21 tables; port the data loader with ids and gaps preserved.
- [ ] Close string decoding: rest of `GetCode`, the Shift_JIS path of `JString.New`, `WriteTranslate`, effective `SAVEDATA_LOCALIZE` and `TranslateEnable`; then port the DelayEvent block codec.
- [ ] Find the origin and initial value of `SYSSAVEI_YUGAMI`, the `ITEM_STAMINA_UP` state and the money/crystal/stamina slots.
- [ ] Enumerate every selector `PackSave` writes and who writes the BaseData sections (`BaseData.Serialize` and overrides).

### 3. Combat, RNG and rewards
- [ ] Resolve effective live `JRandom.Logic` and seeding/reseeding; seeded System.Random and the replay fix_ gate are established. Inspect alternative generators only if the gameplay configuration selects them.
- [ ] Bind the now-named HP/MP/Defense/AttackSpeed/HitRate/CritialRate/EvadeRate parameters, equipment contributions and attribute bonus; resolve `MyConfig.FPS`/`DEBUG` for the composed damage host and attack timing. Metadata enum defaults establish the index names; saved values still need production providers.
- [ ] Bind the tested enemy reward/drop/collection pipeline to production stage updates and permanent ShowQuestResult storage/UI; enemy, treasure, character-rate and level-cost records are decoded/tested.
- [ ] Bind the tested Enemy Init/events/full Update to the production frame and complete HumanSystem.Shoot and World.CreateBullet/spell selection for real attack production; Human initialization/frame/movement/targeting/burst orchestration are now ported with explicit Shoot and animation-clock services; recoil/recovery, real damage/defeat/EXP and nested death/retarget/projectile cleanup now pass integrated scenarios. Resolve event 1133 separately.

### 4. ECS world and animation
- [ ] Build the slot index → component type table (`ComponentFactory.Create`) and recover the payloads and Extra hooks of the remaining components (blocking for an entity reader).
- [ ] What guarantees an empty ECS world before `World.Deserialize` on Continue (`World.Destroy` has no direct caller) (blocking).
- [ ] Bind registry consumers to the ported ordered Values cursor and audit remaining writers of `entity+0x28`; counter allocation/free-ID scanning are ported.
- [ ] Establish Continue/deserialization entry state without adding an implicit directory reset: World.Destroy retains groups/counter/loaded flag; GetSubSet and registry teardown are ported; fixed collision/garbage groups already use real membership notifications.
- [ ] Bind World.GetSubSet and system initialization to the established IUpdateSystem/IPausable lists and tested update dispatcher.

### 5. Text and fonts
- [ ] Reconstruct block generation in `_makeTextBlocks` for the tags the tables use (`br`, `co`, `/co`, `po`), including the scanner fallback for unmatched or too-long `<…>`.
- [ ] Settle the talk-font measurement backend: can `useFontTexture_` become true on talk fonts, which Unity font English really loads (`en-US` matches no font key); define an IMGUI reference measurement set to validate a desktop font.
- [ ] Close the translation path (`CallTranslateText` flags and fallback order, `UnicodeToChanakya`), the boot language state and the SanSanRemove value.
- [ ] Port the font localization scope (predicate, scale, stack) with its two cleanup scopes, plus the `Frame` blink and draw counters.
- [ ] Port the talk page-advance state machine on top of the existing modules; trace the type-4 event form and the producers of `nextIndex_`.

### 6. Boot, forms and frame loop
- [ ] Find the first form `Main.OnCreate` passes to `ChangeCurrentForm` and what `RecordStore.ReadRecord` and `LoadStringArray` read at startup (blocking for boot).
- [ ] Find the writers of the Config/IApplication flags (`Config+0x63/+0x30`, `MyConfig+0x50`, `I.static+0x1c/+0x40/+0x48/+0x60`): frame wait, message and splash modes.
- [ ] Find the writers of `_execute` (`manager+0x88/+0x90/+0x98`, `form+0x48/+0x50`) and form stack states 1–4; port `FormManager`, `DoMethod` and the pass formula with tests.
- [ ] Build a `src/platform` host loop that runs `Update` and `OnGUI` in the documented gate order, with `TotalFrames` separate from logic passes.

### 7. Assets and rendering
- [ ] Close the chain TexId → `img.inf` id → ResourceManager → Image/OPT, including `Image.GetOptimizeSeb` and which ResourceManager serves each Seb.
- [ ] Rebuild the language candidate order of `AssetReader.GetAccessFilesList`; port the INF parser and alias rules to C++ with tests.
- [ ] Audit how `Seb.GetSpriteLocal` combines neighbouring records; port `ConvBufferToSprite` and frame advance (explicit policy for `maxFrame` 0).
- [ ] Decide whether the desktop/3DS build reads the Python extractor output or ports the XOR decode and Unity media extraction to C++; audio streaming policy for 74 MB of PCM.

### 8. Platform replacement
- [ ] Read `Main.OnResume/OnSuspend`, `FormManagerBase.OnDraw` and the conditions around `IsIllegalPackage` and `GooglePlayLicenseCheck`: what must be stubbed.

### Deferred dependency research

Mono/.NET collation, culture and execution-context internals are not the active gameplay queue. Existing committed modules remain available; unfinished character-classification work is preserved in a local Git stash. Consult matching public Mono sources before any further binary analysis.

- [ ] Recover ignorable/CJK guards and tailoring interpretation, then the comparison backend (seven original indexer configurations extracted/read and all UTF-16 category/weight indices bounds-checked; interval indexing/byte-weight access/exact tailoring lookup ported; version-3 core/tailoring C++ reader tested with original local data; seven resources extracted with manifest integrity); bind live entry state (collation selection cache/internal prefix dispatch ported; lazy LCID population for the integer-parsing projection connected to the desktop setter; native null synchronization fallback and initial async-culture switch resolved; synchronization selection/setter, public Run/capture/copies/switch/culture rollback composed). AOSP duplicate selection awaits device comparison. Replace prefix providers only after recovering the collation contract; recover further Hashtable mutations only if reached.

### Tooling
- [ ] Extract `declaringTypeIndex` (and nested types) from `global-metadata.dat` in `tools/inspect_il2cpp_metadata.py`, so compiler-generated closures (`<>c`, `<>c__DisplayClass*`) get their owner's group in the registry instead of `unassigned`.
- [ ] Map the 516 null-slot methods (generic instantiations) or record why they have no code.

## Decisions waiting for the maintainer

- An Android device or emulator as the behavioural oracle (needed for `verified` and for run-time-only questions).
