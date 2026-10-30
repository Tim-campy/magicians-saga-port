# Roadmap

The plan in one page. Work happens in **vertical slices**: each ends in something the maintainer can run on the PC with their own data. Agents take the first unblocked task of the current slice (blockers first), and when done delete it and add the follow-ups they found. One task per line, at most 300 characters (`tools/check_roadmap.py`). Partial states go to [integration](subsystems/integration.md), not here.

## Milestones

| Milestone | Goal | State |
| --- | --- | --- |
| M0 | APK inventory, architecture, subsystem map | Done: [overview](subsystems/overview.md). |
| M1 | Asset extraction and technical rendering | Partial: archives, OPT, SEB, Unity media verified; sprites of real entities render through GL; fonts and audio not ported. |
| M2 | Boot, forms/menus and input | Static research; SelectForm passes ported in isolation (slice S2/S3). |
| M3 | First playable gameplay loop | **Current = slice S1.** Scene host runs table-backed shots, terminal damage, enemy defeat, pending drops, enemy population, contact/area collisions, pickup and stage accumulation; clear/result grants, cleanup and decoded character writeback reach the scene host; save-backed quest selection, interactive end flow and fresh-world bootstrap remain open. |
| M4 | Save/load, progression, economy, content | Slice S3; save envelope not decoded. |
| M5 | Windows/Linux release candidate | Windows builds and CTest; Linux builds in CI. |
| M6 | 3DS feasibility benchmark | Not started; nothing is claimed. |

## Slices

### S1 — Playable mission (current)

Host: `saga_mission` (headless or SDL window; explicit scenario state remains required). Acceptance, checked by running it on the maintainer's extracted data:

- [ ] A1. `saga_mission --data extracted --quest <id>` opens a window with the quest's map, party and enemies from the real tables.
- [ ] A2. Mouse/keyboard moves the hero and triggers attacks through the ported Human/Shoot/Projectile passes.
- [ ] A3. The full system schedule runs each frame: enemies act, collisions, damage, defeat, drops, EXP.
- [ ] A4. The stage ends (clear, game over, retreat) and prints the quest result (EXP, money, treasures, levels).
- [ ] A5. `--replay <inputs> --frames N` prints a state hash; the same inputs give the same hash; one local golden run per quest is recorded in integration.md.
- [ ] A6. Every service the host fakes is listed in the stub table of integration.md.

Tasks:
- [ ] (blocking) Bind Quest1 events212..215 through the existing indexed event/opcode8 kernels and talk-form modal lifecycle in `saga_mission`; preserve explicit acknowledgement and saved text inputs (A1/A4).
- [ ] Bind saved-state/configuration and original InitWorld fresh/loaded entry to `saga_mission --data/--quest` so a diagnostic `--scene` profile is no longer required (A1).
- [ ] Replace the synthetic catalogs/services of the 240-frame combat regression with the table-backed owner used by `saga_mission` (A3).
- [ ] Add complete reached-world state hashing and record local per-quest golden replays (A5).
- [ ] Bind retry/defeat/victory form answers and modal form lifecycle to Stage Update without automatic crystal or ad choices (A4).
- [ ] Bind remaining QuestForm touch/cursor widgets and gesture routing to SDL input events (A2).
- [ ] Bind Retry recovery to the live mission host before result/return (A4).
- [ ] Bind resource image-animation clocks, complete World.DynamicDraw order/gates and Human/Stage HUD to the existing bitmap backend (A3).
- [ ] Group `src/core/gameplay/` (138 files) into subdirectories by subsystem (`combat/`, `mission/`, `town/`, `render/`, `events/`, `text/`, `runtime/`) without behaviour changes; CMake targets and tests keep their names.

### S2 — Town ↔ mission loop

Host: `saga_town`. Acceptance: SelectForm town runs with the real map and residents; choosing a quest starts S1's mission; the result returns to town and progression (EXP, money, levels, stock, quest record) is applied in memory; event scripts fire on return (event 31, 1133).

- [ ] (blocking) Find the owner and data of the town map/residents that SelectForm draws, and the quest-selection path into QuestForm.
- [ ] Bind `update_select_form` to a desktop host with orientation/image-dictionary storage, camera dimensions and form factories.
- [ ] Port decor/town/capture callbacks needed by the town host.
- [ ] Bind billing-item/catalog providers as stubs and the notification manager adapters, preserving task execution gates.
- [ ] Find the consumer of `hour_ != -1`, the writer of `SubForm.delayEvent_` (`+0x1d0`) and the remaining effect opcodes of the table at `0x627b6e`.
- [ ] Resolve event 1133.

### S3 — Boot, title, new game and saves

Host: `saga` (the game executable). Acceptance: boots to the title from real data; New Game reaches the town of S2; saving and loading round-trip; a save written by the Android game loads.

- [ ] (blocking) Find the first form `Main.OnCreate` passes to `ChangeCurrentForm` and what `RecordStore.ReadRecord` and `LoadStringArray` read at startup.
- [ ] (blocking) Decode the save envelope: `SecureProperty.Load/Read/Write/ToByteArray`, `Encrypter.Decode` and the CRC64 check in `ReadRecord`.
- [ ] Find the body of `<LoadData>b__74_0` and the complete list of 21 tables; port the data loader with ids and gaps preserved.
- [ ] Find the writers of the Config/IApplication flags (`Config+0x63/+0x30`, `MyConfig+0x50`, `I.static+0x1c/+0x40/+0x48/+0x60`).
- [ ] Find the writers of `_execute` and form stack states 1–4; port `FormManager`, `DoMethod` and the pass formula.
- [ ] Build the host loop that runs `Update` and `OnGUI` in the documented gate order, with `TotalFrames` separate from logic passes.
- [ ] Close string decoding (`GetCode`, Shift_JIS path of `JString.New`, `WriteTranslate`, `SAVEDATA_LOCALIZE`, `TranslateEnable`); port the DelayEvent block codec.
- [ ] Enumerate every selector `PackSave` writes and who writes the BaseData sections.
- [ ] Find the origin and initial value of `SYSSAVEI_YUGAMI`, the `ITEM_STAMINA_UP` state and the money/crystal/stamina slots.
- [ ] Establish the Continue/deserialization entry state (empty world before `World.Deserialize`; `World.Destroy` keeps groups/counter/loaded flag).
- [ ] Build the slot index → component type table (`ComponentFactory.Create`) and the remaining component payloads and Extra hooks.
- [ ] Read `Main.OnResume/OnSuspend`, `FormManagerBase.OnDraw`, `IsIllegalPackage` and `GooglePlayLicenseCheck`: what must be stubbed.

### S4 — Dialogue, text on screen and audio

Host: `saga`. Acceptance: talk and event forms show their text with the correct font, wrapping, reveal and page advance; music and sound effects play.

- [ ] (blocking) Choose the desktop font backend: can `useFontTexture_` be true on talk fonts, which Unity font English loads; define a reference measurement set.
- [ ] Reconstruct `_makeTextBlocks` for the tags the tables use (`br`, `co`, `/co`, `po`), including the scanner fallback.
- [ ] Port the talk page-advance state machine on the existing modules; trace the type-4 event form and the producers of `nextIndex_`.
- [ ] Port the font localization scope with its two cleanup scopes, plus the `Frame` blink and draw counters.
- [ ] Close the translation path (`CallTranslateText` flags and fallback, `UnicodeToChanakya`), the boot language state and SanSanRemove.
- [ ] Decide the audio path (FSB5 Vorbis from the extractor output, streaming policy for 74 MB of PCM) and play BGM/SE through SDL.

## Backlog (pull into a slice only when it is on its critical path)

- [ ] Close the chain TexId → `img.inf` id → ResourceManager → Image/OPT, including `Image.GetOptimizeSeb`.
- [ ] Bind the SEB evaluator to the pixel consumer, including negative TexId commands and automatic drawing clocks.
- [ ] Decide whether the PC/3DS builds read the Python extractor output or port the XOR decode and Unity media extraction to C++.
- [ ] Revisit cell relation hooks and parameter event21/restored queues when the mission host reaches them.
- [ ] Bind registry consumers to the ordered Values cursor and audit remaining writers of `entity+0x28`.
- [ ] Extract `declaringTypeIndex` from `global-metadata.dat` so compiler-generated closures get their owner's registry group.
- [ ] Map the 516 null-slot methods (generic instantiations) or record why they have no code.
- [ ] Deferred: Mono/.NET collation, culture and execution-context internals (state in [integration](subsystems/integration.md)); only if S1–S4 hit them, and from public Mono source first.

## Decisions waiting for the maintainer

- An Android device or emulator as the behavioural oracle: needed for `verified`, for golden replays and for run-time-only questions (live RNG backend, FPS, camera, render order).
