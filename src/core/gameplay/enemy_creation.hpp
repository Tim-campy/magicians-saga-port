#pragma once
#include "enemy_factory.hpp"
#include "stage_start.hpp"

namespace saga::core {
class EnemyGaugeService;
struct EnemyCreationSystem:EnemyDefeatCounters {
    std::shared_ptr<StageQuest> quest;
    StageStartState* stage=nullptr;MemberRoster* members=nullptr;
    EnemyGaugeService* gauge=nullptr; // GaugeSystem reference +0x40; drawing is separate.
    EntitySubset* subset=nullptr; // And(24), published by initialize_enemies.
    std::int32_t created_enemies=0,loop_counter=0;
    EventInstruction maximum_by_section,created_by_section; // Both allocated as int32[10] by Init.
};
class EnemyCreationHost {
public:
    virtual ~EnemyCreationHost()=default;
    virtual EnemyFactoryWorld* current_enemy_factory()=0;
    virtual PlayerWorldState* enemy_world()=0;
    virtual StageGeometry* enemy_map()=0;
    virtual StagePlacementHost& enemy_coordinates()=0;
    virtual bool target_destroyed(ComponentSlots&)=0;
    // Read current AppData system-save slot through its real SecureInt adapter.
    // There is no default value or bypass for the unresolved save/new-game state.
    virtual std::int32_t system_yugami()=0;
    virtual AnimationModificationPool& enemy_modifications()=0;
};
ComponentSlots* search_enemy_target(EnemyCreationSystem&,ComponentSlots*,EnemyCreationHost&);
ComponentSlots& create_system_enemy(EnemyCreationSystem&,EnemyCreation,EnemyCreationHost&);
ComponentSlots& create_boss_enemy(EnemyCreationSystem&,EnemyCreationHost&);
} // namespace saga::core
