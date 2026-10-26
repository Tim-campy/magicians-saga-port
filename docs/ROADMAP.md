# Roadmap

The plan in one page. Agents take the first unblocked task of the current work package (or what the maintainer asks), do it, and then edit this file: delete finished tasks, add the follow-ups they found. Keep each task one line, concrete and sized for one session.

## Milestones

| Milestone | Goal | State |
| --- | --- | --- |
| M0 | APK inventory, architecture, subsystem map | Done: [overview](subsystems/overview.md). |
| M1 | Asset extraction and technical rendering | Partial: containers, OPT, SEB, Unity media extracted and verified; scene rendering, fonts and audio playback not ported. |
| M2 | Boot, forms/menus and input | Static research only. |
| M3 | First playable gameplay loop, deterministic tests | **Current.** Event, combat, reward and text modules exist in isolation; no integrated loop. |
| M4 | Save/load, progression, economy, content coverage | Research and the script-payload codec; save envelope not decoded. |
| M5 | Windows/Linux release candidate | Windows builds and CTest; Linux builds in CI. |
| M6 | 3DS feasibility benchmark | Not started; nothing is claimed. |

## Work packages (priority order)

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

### 1. Events and scripts
- [ ] Recover invariant culture data and compose thread/default-culture precedence on the decoded specific factory and current-culture cache; bind the host loader for the extracted tables/neutral map and invariant Unicode lowercasing. AOSP duplicate selection awaits device comparison. Trace culture setters and collation availability/backend before replacing prefix/sign providers.
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
- [ ] RNG generator (blocking for damage and drops): effective `JRandom.Logic`, writer of `fix_`, seeding, IL2CPP `System.Random.Next` and `Xorshift128.Next`, whether any backend returns negatives.
- [ ] Name parameter indices 0/1/2/6/7/8/9 and resolve `MyConfig.FPS`/`DEBUG`; then compose a native `damage()` that keeps the documented RNG order.
- [ ] Bind `EnemyData` (`exp_`, `treasureDrop_`), `TreasureData`, `CharaData+0x9c` and `tableData+0x24` to the data loader; port `GetExp`, `GetNextExp` and the level-up loop of `ShowQuestResult`.
- [ ] Read `OnGameOver`'s retry path, `CleanUp`, the state-4 quest method and event 1133.
- [ ] Port `Graph.Easing` in the general case (amplitudes −50 and −80), checked against the binary32 model.

### 4. ECS world and animation
- [ ] Build the slot index → component type table (`ComponentFactory.Create`) and recover the payloads and Extra hooks of the remaining components (blocking for an entity reader).
- [ ] What guarantees an empty ECS world before `World.Deserialize` on Continue (`World.Destroy` has no direct caller) (blocking).
- [ ] Recover the enumeration order of `Dictionary<int, Entity>` and the entity id allocation/reuse policy (counter `+0x14`, writers of `entity+0x28`).
- [ ] Port `Subset` with HashSet slot-order semantics (LIFO free list, version, notifications) and `EntityManager` on top of `ComponentSlots`.
- [ ] Close `AnimationSystem.Update` (Seb selection by res/id, maxFrame comparison and modulo); port the loop on top of `animation_transitions`.
- [ ] Map `World.GetSubSet` and the final IUpdateSystem/IPausable system list (which `NeedsUpdate` test `enabled`).

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

### Tooling
- [ ] Extract `declaringTypeIndex` (and nested types) from `global-metadata.dat` in `tools/inspect_il2cpp_metadata.py`, so compiler-generated closures (`<>c`, `<>c__DisplayClass*`) get their owner's group in the registry instead of `unassigned`.
- [ ] Map the 516 null-slot methods (generic instantiations) or record why they have no code.

## Decisions waiting for the maintainer

- An Android device or emulator as the behavioural oracle (needed for `verified` and for run-time-only questions).
