#pragma once
#include "drop_factory.hpp"
#include "game_random.hpp"

namespace saga::core {
using StageScriptGroups=std::shared_ptr<std::vector<EventScript>>;
// QuestData fields needed at stage entry; arrays retain their managed identity.
// Lengths stay fixed within int32 bounds; callbacks may replace references.
struct StageQuest {
    std::int32_t flags=0;
    std::int32_t distance=0,time_limit=0,money_rate=0;
    std::int32_t prisoner_maximum=0,rescued_prisoners=0;
    std::int32_t reward_treasure=0,cleared_reward_treasure=0;
    std::int32_t boss_id=0,boss_level=0; // QuestData +0x70 / +0x44.
    std::int32_t base_level=0,enemy_interval=0,enemy_maximum=0,enemy_total_limit=0;
    std::uint8_t completed=0;
    StageScriptGroups events,fixed_treasure;
    StageScriptGroups normal_enemies,fixed_enemies; // +0x60 / +0x68, first group used.
};
struct StagePlacementState {std::shared_ptr<StageQuest> quest;};
struct StageGeometry {std::int32_t step_x=0,step_z=0;}; // MapComponent +0x20/+0x24.
struct DropPlacementWorld {EntityIdState& ids;DropFactoryHost& factory;const TreasureTable& treasures;};
class StagePlacementHost {
public:
    virtual ~StagePlacementHost()=default;
    // Resolve the current BaseSystem.world_ for each call; missing world faults,
    // but its map result may be null until the original caller dereferences it.
    virtual StageGeometry* stage_map()=0;
    virtual DropPlacementWorld* drop_world()=0;
};
std::int32_t stage_quest_height(const StageQuest&); // QuestData.get_heightNum 0x1172d4c.
std::int32_t remaining_stage_prisoners(const StageQuest&); // 0x1172e0c.
// StageSystem.GetZ 0x1157428; two fresh map reads and wrapping signed arithmetic.
std::int32_t stage_z(StagePlacementState&,std::int32_t rate,StagePlacementHost&);
std::int32_t stage_formation_left(StagePlacementHost&);
std::int32_t stage_formation_top(StagePlacementState&,StagePlacementHost&);
void place_stage_money(StagePlacementState&,RandomSource&,StagePlacementHost&);
void place_stage_treasures(StagePlacementState&,RandomSource&,StagePlacementHost&);
void place_stage_prisoner(StagePlacementState&,std::int32_t rate,StagePlacementHost&);
void place_stage_prisoners(StagePlacementState&,RandomSource&,StagePlacementHost&);
// StageSystem.PlaceDrops 0x1165708: money -> treasure -> prisoners.
void place_stage_drops(StagePlacementState&,RandomSource&,StagePlacementHost&);
} // namespace saga::core
