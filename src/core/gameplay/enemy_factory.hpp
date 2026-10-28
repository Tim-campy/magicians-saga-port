#pragma once
#include "human_factory.hpp"
#include "enemy_defeat.hpp"

namespace saga::core {
// Managed-array projection: fixed lengths within int32, stable borrowed record
// addresses during calls. Replacing the table reference is supported.
using EnemyRecords=std::shared_ptr<EnemyTable>;
struct EnemyCreation {std::int32_t direction=0,id=0,level=0,flags=0;ProjectilePoint position{};};
class EnemyFactoryHost:public EntityVisualHost,public PlaybackPool,public PartyCollisionPool {
public:
    virtual EnemyRecords enemy_records()=0; // Fresh Data.enemyData reference per query.
    virtual const ParameterMaximumLevels& enemy_maximum_levels()=0;
    virtual EventInstruction enemy_parameter_ids()=0;
    virtual std::int32_t enemy_resource_id(std::string_view)=0;
    virtual EnemyRecordLink& acquire_enemy()=0;
    virtual HumanAI& acquire_enemy_ai()=0; // Same AIComponent/CacheArray as human creation.
    virtual CombatParameterLinks& acquire_enemy_parameters()=0;
    virtual DefeatColor& acquire_enemy_color()=0;
    virtual ContactDirection& acquire_enemy_direction()=0;
};
struct EnemyFactoryWorld {EntityIdState& ids;EnemyFactoryHost& factory;};
EventInstruction initial_enemy_parameter_ids();
ParameterMaximumLevels initial_enemy_parameter_maximum_levels();
bool enemy_parameter_has_maximum(std::int32_t);
float enemy_move_speed(const EnemyRecord&,std::int32_t level,const ParameterMaximumLevels&);
// World.CreateEnemy 0x1133fc0. Invalid IDs return null before allocation; all
// captured/live record reads and optional direction/playback follow the original.
ComponentSlots* create_enemy(EnemyCreation,EntityIdState&,EnemyFactoryHost&);
} // namespace saga::core
