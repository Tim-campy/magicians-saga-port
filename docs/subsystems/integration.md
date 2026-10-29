# Integration

How far the ported modules run together on the PC, per slice of `docs/ROADMAP.md`, and which stubs and provisional inputs the hosts use. Game rules stay in their subsystem docs; this page tracks wiring.

## Hosts

| Slice | Host | Run | State |
| --- | --- | --- | --- |
| S1 headless mission | `saga_mission` (always built) | see [setup](../SETUP.md#headless-mission-host) | Shares the strict scenario loader and optional Human -> Projectile -> Move -> Emitter -> Modifier/Animation/Render/Garbage steps with the GPU preview. No PNG decoding or SDL; selected entities, not a quest/map bootstrap. |
| S1 playable mission | `MissionEntities`/`MissionScene` (`src/platform/desktop/mission_*.cpp`), driven by `queue_capture --interactive` (viewer build) | see [assets](assets.md#table-backed-scene-window) | Real tables drive shots, terminal damage, enemy defeat, pending drops, trails and landing-debris cleanup. No player input, enemy AI, collision pair pass, drop collection or stage end. |
| S2 town ↔ mission | none | — | `select_*` modules ported and tested in isolation; no host. |
| S3 boot, title, save | none | — | First form unknown; save envelope not decoded. |
| S4 dialogue, audio | `event_trace` (text only, no window) | see [events](events.md) | Text pipeline ported in isolation; no font backend, no audio. |

## Reachable from the hosts

Keep this list current: a ported module that no host calls is integration debt. Group by host; one line per module or chain.

- `MissionScene`: gameplay catalog, human/enemy/effect factories, dynamic subsets, MoveSystem, Emitter, Modifier and GarbageSystem Init/NeedsUpdate/Update (simulate profile), entity destruction/manager deletion, render frame/draw, SEB sprite evaluation, animation pass, BitmapMeshFrame/GL.
- `saga_mission` and `queue_capture`: MissionProfile owns the same archive managers, saved inputs, factories and scene step. The capture frontend supplies the PNG decoder; an empty decoder keeps the host independent of SDL and pixel preparation.
- `MissionHumans`: actual Human Init/event23+29 subscriptions and full frame dispatcher, total-stat and saved-level projection, state/formation/animation kernels, stage Z limits, FPS attack/recovery intervals and seeded recovery/celebration RNG. Shoot, bullet factory/launch, contact effects, Projectile Init/Update and ordered impact receivers use the concrete owner. Explicit reward profiles bind Enemy Defeat, saved defeat counters, pending EXP/treasure drops and ordinary-enemy landing fragments; boss completion and stage death remain reached-path errors.
- Not reachable from any host yet (tested only): enemy frame/full Init, collision pair dispatch, area damage, drop collection and stage rewards, stage start/update/recovery/return, quest camera, cell system, event catalog/conditions/execution/dispatch, SelectForm passes, text modules, culture/collation/execution-context modules (deferred dependency work).

### Selected movement pass (S1 A3)

The simulate row enables optional Human -> Projectile -> Move -> Emitter -> Modifier -> Animation -> Render -> Garbage, a subset of the native ordering, without placeholder callbacks for the other systems. MissionScene initializes the live And(0,1) group through initialize_movement; advance_movement uses factory-index11's actual gate and move_subset. No game rule is duplicated. Creation-order positions and movement counts are printed by bounded runs. Real enemies retain the velocity calculated by create_enemy from parameter5 and their record/creation flags. The hero retains its factory's zero velocity.

The complete 26-system schedule is still blocking. Ordinary bullets execute event23 in Human/Enemy/Projectile order with actual impact visuals, targeted numeric damage and emitter-tagged entity destruction. Emitter15 binds trail particles and falling-enemy debris to the same resource/factory/lifetime owner. Full Enemy Init/frame, pairwise collision dispatch, area damage delivery, drop collection and mission termination remain unbound. Entity destruction is bound as described below; component pool reuse remains separate. Cell migration/culling and full camera/visibility remain open. The selected scene uses its fixed visible group even when an entity leaves the viewport.

Local demonstration: `build/viewer/queue_capture.exe research/exports/mission-data-scene/movement.txt 600 220 --run-frames 12`. The profile is the animation scene with simulate 0; tick_ms remains an explicit diagnostic timing input. Human0 remains at (0,0,0); enemy2 moves from (150,0,0) to (150,0,11.5068016) after 12 binary32 integrations. A native capture emits six quads through five textures; movement-loop.json and movement-12.bmp remain local. No Android golden replay is claimed.

### Entity destruction and effect expiry (S1 A3)

MissionEntities owns the destroyed flag and stable opaque lifetime handles independently of native entity IDs (temporary entities may share native ID -1). Destroy delegates to destroy_quest_entity and delete_entity_from_manager: ascending live component removal -> disposal of component notifications -> optional registry removal -> destroyed flag store. Duplicate destruction returns; callbacks update the existing subsets before event disposal. Nodes/components remain allocated so captured references stay valid; recycling and full World.Destroy are unbound.

MissionScene initializes the live And(22) subset and uses GarbagePass with factory-index31's gate. Counting finishes before deferred ordered destruction. The frame's render commands survive expiry, matching Render21 before Garbage31. The effect row calls the existing visual-effect factory with resource20, uses the selected SEB's header-derived lifetime and adds no Garbage for a looping effect.

Local demonstration: `build/viewer/queue_capture.exe research/exports/mission-data-scene/effects.txt 600 220 --run-frames 120`. It loads 23_effect alongside the character/enemy/hat/weapon archives, creates clip101 at (250,0,0), and reports its entity destroyed while the two actors remain live. effects-loop.json stays local. Synthetic GPU tests check that the expiring frame is visible and the next frame has no effect pixels; Windows tests check subset/registry removal, idempotent destruction, temporary lifetime identity and remove_registration=false.

### Table-backed Human frame (S1 A2/A3)

MissionHumans binds initialize_humans to the actual world subset/map/stored Stage/Enemy/Member projections and subscribes real Human impact/contact receivers to the world event bus. The services object outlives the callbacks during profile execution. Human updates use ecs_member_total_parameter with current saved character level, the scenario's owned gear-level tables and live shop counts; no slot23-value substitution or missing-level fallback is used. All present gear rows require an explicit level binding. Uniform gear_levels rows are provisional saved-state selections, not recovered Android values.

Player lookup uses the real Player30 group and cache through member_hero. The player row adds the shared marker to the selected human, a scenario operation rather than a complete CreateHero/Member bootstrap. The formation helper's world-lookup interface is narrowed from the larger StageRecoveryHost without changing its two hero lookups or arithmetic. Native animation/state, recovery, position limits and loop counters execute before Move. The supplied StageQuest distance and min/max rates drive stage_z; full Map/Stage startup, member initialization, cell migration and camera/culling remain unbound.

The local walking.txt scenario sets Human0 state1, initial Z speed -1, inactive attack button0, saved gear level1, geometry (312,312,6), quest distance10 and explicit min/max rates0/100. FPS25/System.Random seed42 produce five recovery draws in 120 passes, move the hero from z1000 to z852.819824 and leave Human.frame120/attackFrame49. walking-host.json and walking-window.json match positions, Human counters and RNG consumption after all 120 passes; walking-12.bmp is a visually inspected native capture. These artifacts remain local. The profile's fixed view is chosen to show this position; it is not the recovered quest camera. Human contact/landing receivers create the original effects; stage death remains an explicit error after its native state/effect prefix. Shots use the actual producer and terminal pipeline described below. No mouse/keyboard control or complete combat schedule is claimed.

### Table-backed shots and terminal hits (S1 A2/A3)

MissionHumans calls the existing shoot_human producer: native MP gates/base consumption, first attribute/equipped selection, shared-ID bullet/effect creation, Fire payload, scales/height and attack cut-in. MissionEntities owns stable Projectile/ModifyAnimation storage, shared creation IDs and detached-component bookkeeping; reuse remains separate. Projectile Init runs after Human impact and the actual Enemy impact receiver is registered; completion sends event23 in Human -> Enemy -> Projectile order. Ordinary bullets have Emitter31, so the final receiver destroys them through the real world lifetime chain; finish_projectile tolerates the removed slot27.

Terminal Enemy impact calls create_impact_effect, then on_projectile_hit if the freshly queried opponent still intersects. EcsDamageHost binds original stats/saved gear/affinities, the shared seeded gameplay RNG and DEBUG. The event15 adapter invokes the established Stage damage-accounting body directly; general typed event routing/other listeners remain open. Nonlethal HP subtraction, repeated impact calls, Projectile/Collision removal and final emitter-tagged deletion preserve the native order. Missing targets at landing skip targeted damage, as the original does. Collision pair iteration and neighboring area-damage delivery are not bound by this terminal-hit path.

Damage's stateful presentation suffix replaces an enemy's current modifier with type4/maxFrame8, then creates the positional effect (clip0/image1) at y+10 before Stage accounting. Modifier18 executes its real horizontal shake and expiry after movement/emission and before animation/render. Floating numeric/critical/miss labels and attack audio are presentation stubs listed below; the modifier/effect mutations are not skipped. Lethal damage requires the explicit rewards scenario described below; without it, the host stops after the native HP subtraction. Stage/boss completion remains an explicit reached-path error.

Local scenario: `build/desktop/saga_mission.exe --scene research/exports/mission-data-scene/shooting.txt --frames 120 --headless --fps 25 --debug 0 --rng system --seed 42`. Selected Human0 starts waiting at z1000; Enemy2 is an explicit stationary level1000 scenario target at z940; the attack button is active. Real tables yield two shots, eight RNG draws, MP19->17 and enemy HP492->489, with totalDamage3 and no live projectiles at the final step. The nonlethal actor/counter evidence is retained locally in shooting-start.json, shooting-host.json and shooting-window.json. This is a terminal-hit scene, not an original quest or a complete battle.

### Table-backed defeat, pending rewards and emission (S1 A3)

The rewards row supplies initial system defeated/remaining counters and an explicit uniform initial per-record saved defeat count. It requires human_system, the decoded Treasure.txt table and the AppData7 quest resource. MissionHumans delegates lethal damage to the existing defeat_enemy: FlyAway, state4/color, saved/system counters, EXP and first-success treasure roll share the same seeded gameplay RNG. MissionEntities calls create_drop with shared entity IDs and actual table/image bindings. Pending drops do not grant StageRewards, character EXP or inventory items.

Emitter15 runs after Move11 and before Modifier18, using live And(31), real bullet records and AppData19/20 clocks. Trails use the existing particle factory and expire through Garbage31. Ordinary-enemy landing creates three flying emitters and the actual type3/maxFrame10 corpse modifier; their motion, emission and deferred removal use the existing ports. Boss landing still fails at its unbound stage notification. No extra RNG stream or replacement trajectory is introduced.

Local demonstration: `build/desktop/saga_mission.exe --scene research/exports/mission-data-scene/rewards.txt --frames 240 --headless --fps 25 --debug 0 --rng system --seed 42`. The explicit scenario selects saved/creation hero level10 and ordinary stationary Enemy2 level1 at z940, with counters0/1/0 and no purchases. Real data yields two shots, totalDamage6 and 24 RNG draws; the enemy is destroyed, its saved defeat count becomes1, and a type0 EXP drop value2 remains at (-5,0,930). The native ordinary-enemy remaining counter has floor1; this result does not establish a stage clear. The shared GPU step produces identical reported actor/projectile/reward observations after 240 passes (`rewards-host.json`, `rewards-window.json`, local only). Visually inspected rewards-90.bmp and rewards-240.bmp show the airborne defeated enemy and the retained EXP icon respectively; the latter emits six commands/quads through five textures from seven archives.

Synthetic test_mission_rewards composes actual shoot/damage/defeat, guaranteed EXP/treasure drops, three landing emitters and final corpse/debris cleanup while checking saved character EXP is unchanged. The saga_mission profile contract additionally covers full animation/render passes, deterministic repeated output, the ordinary-enemy counter floor and failure without explicit reward counters. Limits: no Android comparison, collection/result grants or complete mission schedule.

## Stubs and provisional inputs

| Id | Where | Replaces | Kind | Replace when |
| --- | --- | --- | --- | --- |
| scene-view | `MissionScene` | QuestForm camera/viewport and culling owner | provisional input (fixed view) | quest camera/visible-set owner is bound |
| scene-tick | `queue_capture --interactive` `tick_ms` | `MyConfig.FPS` / IApplication timing | provisional input | FPS and timing policy are resolved |
| scene-progress | profile rows | saved level/EXP/shop purchases | provisional input (level 1, EXP 0, no purchases) | save envelope is decoded |
| scene-materials | profile rows | material/shader identities | provisional input | production constructor/material canonicalization is ported |
| mission-config | runtime row / `saga_mission` CLI | live FPS/DEBUG/RNG/seed selection | provisional inputs; FPS/RNG reach Human frame rules; DEBUG/RNG reach numeric damage | effective Android values and remaining consumers are bound |
| mission-attack-audio | MissionHumans | attack sound output | service stub (counter only) | audio backend is bound |
| mission-damage-labels | MissionHumans | floating miss/numeric/critical labels and their display-table selection | presentation stub; no shake/impact suppression | label/effect renderer is bound |
| mission-damage-diagnostics | numeric damage host | optional diagnostic logging | service stub; diagnostic logging disabled | diagnostic output is bound |
| human-scenario | human_system/player/actor_state/gear_levels rows | quest geometry/bounds, attack button, initial actor state and saved gear levels | provisional scenario inputs, no implicit gear levels | quest/save/bootstrap/input producers are bound |
| reward-scenario | rewards row | initial EnemySystem defeated/remaining counters and uniform per-record saved defeat count | provisional scenario inputs; no implicit reward state | stage entry and decoded save restoration are bound |

### Headless execution limits

The host requires an explicit scene, frame count, FPS, DEBUG, backend name and seed. Only the existing explicit-seed System.Random adaptation is available; unsupported backends and unknown flags fail. Human-enabled profiles consume FPS for attack intervals and recovery cadence, and the seeded RNG for recovery/celebration. DEBUG also reaches the numeric damage gate; effective Android values and remaining consumers are unresolved. Scene runtime rows must agree with CLI configuration. Headless execution runs fixed logic passes without sleeping or multiplying movement by delta time. No complete-state hash, gameplay input replay, quest-table selection or save snapshot is claimed.

The original-data effects profile produces the same actor positions and destroyed flags after 120 passes in the headless host and the GPU frontend: enemy2 ends at z115.068047, effect101 is destroyed, and the hero remains still. host-loop.json and effects-loop.json are local evidence. Synthetic host tests additionally use invalid PNG payloads to prove that headless execution performs no PNG decoding and check strict rows, required simulate configuration, duplicate options and deterministic repeated output. GPU tests continue through the same loader/step.

## Remaining integration contracts

- The 26-system order and NeedsUpdate gates are ported/tested, along with generic Init dispatch and Map/Camera/Human/Enemy/Exp/Parameter/Cell/CellCulling/Emitter/Modifier/Projectile/Move/Collision/Garbage/Animation/Member initialization. The host still needs the remaining update bodies and their shared event/world projections; see [ECS](ecs.md) and the current slice queue.
- Stage Start, map/party/enemy entry, seven-state Update, collection, recovery/boss entry, result grants and town return compose with synthetic services. Production quest/map/save state, full Enemy Init/frame and QuestForm cursor input remain required; see [rewards](rewards.md). MapSystem.LoadMap replaces its grid without destroying previous map-chip entities; the mission-transition cleanup owner is unresolved.
- QuestForm offscreen target/sampler ownership, viewport/culling and enclosing callback cleanup remain separate from the selected scene. Native quad insertion and stream order already reach GPU submission; see [assets](assets.md#gpu-bitmap-submission) and [UI](ui.md#quest-mission-render-callbacks).
- Production material canonicalization, ConvertMesh/disposal and EndMesh aging are unresolved (including the top-form predicate). GetMeshBuilder/GetLayer/Reset/AddQuad drive the desktop frame owner; native MeshRecord.Add capture remains an optional service. See [mesh builders](assets.md#mesh-builder-cache-and-record-production).
- Full native async/cache/locale/bootstrap selection, pack overrides and reached @ atlas consumers remain open. The selected synchronous archive/catalog/SEB/TexId/PNG/OPT route already supplies mission resources; see [data](data.md#selected-mission-catalog) and [assets](assets.md). No optional public-library reconstruction is required by the selected reward scene.
- Mono/.NET collation, Unicode and execution-context work is deferred until a concrete gameplay path requires it. Existing findings stay in their subsystem docs and local evidence; unfinished classification edits are in the maintainer's local Git stash. Consult matching public Mono sources first; AOSP duplicate selection still requires an Android comparison.
