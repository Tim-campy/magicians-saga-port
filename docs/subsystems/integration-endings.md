# Fresh mission endings

The selected mission host reaches native clear, game-over and retreat paths from fresh extracted data. It reports accumulated drops, permanent in-memory grants and the captured exit before holding the town handoff. [Rewards](rewards.md) defines the game rules; [integration](integration.md) lists the remaining services and [replay checkpoints](integration-replay.md) defines deterministic state comparisons.

## Overview

`saga_mission --data <root> --quest <id>` initializes the actual records, resource owners, world, hero/formation and Stage.Start. Ending replays use normal movement, attack, dialogue acknowledgement, pause and modal answers. They do not provide a scene, replacement HP, equipment, Stage state or debug result trigger. The declared bootstrap context and seeded RNG remain provisional PC inputs; quest selection is an explicit CLI scenario choice, not the original unlock/town selection flow.

The selected world remains owned after `ReturnTown` cleanup and member writeback. Its typed `QuestTownContinuation` is retained without invoking the S2 town form switch or event31 consumer. This allows the host to print the actual ending and post-quest state without inventing a town or save-file result.

## Behaviour

### Exit and grant reporting

With quest results enabled, `quest_exit` is null until the native return path captures its continuation. Once captured it contains `code`, `outcome`, `handoff: "held"` and `rewards_granted`. The code comes from the typed continuation's captured result; unknown continuation types fail instead of inferring an exit from Stage state or a nonempty callback.

| Native code | Outcome | Required path |
| --- | --- | --- |
| 0 | Clear | Boss defeat, reward-box pickup, celebration confirmation, ShowQuestResult, gear upgrades, completion and ReturnTown. |
| 1 | Main-character death | Game-over reason1, defeat notice/retry decision, then ReturnTown after retry is declined. |
| 2 | Time expired | Game-over reason2 at timer0, defeat notice/retry decision, then ReturnTown after retry is declined. |
| 3 | Retreat | Pause, request exit and answer0 to kind99; no ShowQuestResult grants. |

`rewards_granted` reports whether ShowQuestResult actually produced a result. It is separate from the return code. A clear's `quest_result` contains experience, money, crystals and prisoners, every weapon/character/cloth/hat grant list as ordered `[id, count]` pairs, and member result records including `level_up_frame`. Empty grant lists stay explicit; awards remain visible even if capped inventory/recruitment has no net change. Existing inventory, recruitment, wallet, quest-record and member-writeback fields expose permanent in-memory effects; collection totals describe the accumulator before grant conversion.

For a clear, Stage state3 waits for pulse `0x100000` before ShowQuestResult. The following state4 completes the quest and returns with code0. Retreat and failed-quest returns update challenge/defeat records, clean up and finalize members without converting the accumulator into victory rewards. A failed quest can therefore have collected drops while `rewards_granted` remains false. See [result grants](rewards.md#quest-result-grant-showquestresult-0x1164548) and [deferred return](rewards.md#member-finalization-and-deferred-town-return).

### Input and modal gates

Dialogue pulses flush or advance a page; they do not override reveal or event state. Native quest1 progress events can stop movement and open a talk form, so a replay must acknowledge those forms and resume its inputs. Movement disables active attacks; a release and attack selection allow the ordinary Human targeting/shooting path to run. Repeating movement alone is not a clear command.

Boss defeat publishes the victory notice and releases movement. Closing that notice does not pick up the reward box: the front-line character must still reach it. The resulting celebration then needs its own confirmation pulse. These steps remain distinct from acknowledging the result notice and from the retained town continuation.

Stage pause uses pulse33554432 in state1; pulse134217728 requests retreat in state2. The exit dialog accepts `answer 0` to return; every other signed answer cancels and leaves the mission paused. A defeat notice sits above the retry choice. `answer 0` acknowledges that notice; the following retry answer selects spending/ad/decline. A negative retry answer declines without spending crystals. Early or duplicate explicit answers fail rather than silently closing another form. Window title controls use the same retained modal objects; original form drawing and close timing remain service boundaries.

## Port

`MissionStage` drives the existing Stage.Update, retry, boss and result/return kernels. `MissionHumans` retains the native form identities and explicit modal answers; `MissionResults` owns the actual wallet, inventory, recruitment, quest records and ordered member cursors. Reporting the captured continuation and all grant arrays adds observations without changing the game-rule pipeline.

The return callback remains `QuestTownContinuation{result, host}`. Cleanup and `MemberSystem.OnFinish` run before capture; the callback's later form switch and event31 dispatch stay held. No town progression callback, save envelope or secure-storage write is synthesized. This boundary belongs to S2/S3 and does not prevent a selected fresh mission from reaching its native terminal state.

The next logical step stops before consuming further replay actions. Bounded runs finish early; the unbounded window also closes and prints its final JSON when it observes the held return instead of retaining an obsolete result title in a paused window. Host pause and single stepping retain their separate controls.

## Evidence

The following replays use native fresh entry and seed42 defaults. Files remain local under `research/exports/mission-endings/`.

| Replay | Ordinary actions | Observed terminal state |
| --- | --- | --- |
| `retreat-intro.replay` | Intro pulses30..180 every5 ticks; pause200, exit201, answer0 at202. | Held ReturnTown3 at tick202; wallet100/100,107 RNG draws and no reward grant. |
| `timeout.replay` | Acknowledge the intro, idle until expiry, acknowledge defeat at7400 and decline retry at7401. | Held ReturnTown2 at tick7402; timer0, wallet100/100, two shots,131 RNG draws and total damage7. |
| `advance-fight.replay` | Starting at200, repeat a180-tick cycle: forward keys at its start, keys0 and attack0 after60 ticks; confirmation pulses every5 ticks through7000. | Natural boss defeat, victory acknowledgement, box pickup and result confirmation; held ReturnTown0 at tick2380 from an8000-frame request. Fifteen shots,384 RNG draws, total damage56, wallet660/200, completion1 and hero level1/EXP29 with HP28/MP22. |
| `defeat.replay` (quest75) | Advance/fight actions through400, then decline retry with answer-1 at400. | HP0, Human state5 and held ReturnTown1 at tick401; one shot,227 RNG draws, total damage3, wallet100/100 and no grants. |

The first three scenarios select quest1; quest75 deliberately exercises death with the same fresh level1 hero. This does not establish that the original unlock flow permits that selection.

Run each selected replay through the headless host with a sufficient upper bound; the retained transition stops the mission before that bound:

```powershell
build/desktop/saga_mission.exe --data extracted/full --quest 1 --headless --frames 8000 --replay research/exports/mission-endings/retreat-intro.replay
build/desktop/saga_mission.exe --data extracted/full --quest 1 --headless --frames 8000 --replay research/exports/mission-endings/timeout.replay
build/desktop/saga_mission.exe --data extracted/full --quest 1 --headless --frames 8000 --replay research/exports/mission-endings/advance-fight.replay
build/desktop/saga_mission.exe --data extracted/full --quest 75 --headless --frames 8000 --replay research/exports/mission-endings/defeat.replay
```

Local `research/exports/town-mission-entry/mission-{retreat-intro,timeout,advance-fight,defeat}.json` retain the actual exits, grant lists and member fields under schema2. Compared with preserved41b996e reports, all fields match except quest_detail_count1, the schema and the two digests/byte counts. These port runs establish reached endings without claiming Android equivalence, completed town return or persistence.

| Replay | Final schema2 logic hash |
| --- | --- |
| `retreat-intro.replay` | `c1629bd810c9472d` |
| `timeout.replay` | `933f5c3c6e033def` |
| `advance-fight.replay` | `60d614e814444ca6` |
| `defeat.replay` | `bd81dad661e6a9e9` |

The540-frame regression retains byte-identical pixels from the preserved combat capture and CPU/GPU logic equality; see [replay checkpoints](integration-replay.md). Stage-result presentation remains a declared service boundary. The S2 town host delivers the same continuation to its retained town form and event31 consumer, refreshes progression and permits another quest; completed_missions retains the pre-delivery mission snapshot.

An unbounded window run with `retreat-intro.replay` and hidden Windows startup closes normally at202 logic steps, without a frame limit or close input. `retreat-unbounded-final.json` retains the matching exit and logic digest. This exercises the window's terminal branch separately from bounded replay execution.

Windows Release builds pass211 desktop CTest contracts and216 viewer contracts. The mission-profile contract covers native fresh retreat/timeout, null/pending exits, all four return labels, complete clear grants/member fields, stock/recruitment caps and stable held state across repeated/extended budgets. S1 A4 is checked on this evidence; original result-form pixels, town return and saves remain separate work.

## Open questions

- Bind S3 save restoration/persistence while preserving the captured result and cleanup/writeback order.
- Compare original form timing, presentation and runtime behaviour when an Android oracle is available; A3/A6 and the rest of S1 retain their own acceptance gates.
