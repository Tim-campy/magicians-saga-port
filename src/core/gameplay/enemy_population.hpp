#pragma once
#include "enemy_creation.hpp"

namespace saga::core {
class EnemyPopulationHost:public EnemyCreationHost {
public:
    // Capture this current-world service before snapshotting the stored group.
    virtual QuestEntityHost* population_destruction_world()=0;
    virtual QuestMemberId population_entity_id(ComponentSlots&)=0;
    virtual RandomSource& population_random()=0;
    // The application's shared Random static scratch, not a per-enemy table.
    virtual WeightedLotteryState& population_lottery()=0;
    virtual EnemyRecords population_records()=0;
};
void destroy_all_population_enemies(EnemyCreationSystem&,EnemyPopulationHost&);
void reset_enemy_population(EnemyCreationSystem&,EnemyPopulationHost&);
void create_fixed_enemies(EnemyCreationSystem&,EnemyPopulationHost&);
void start_enemy_population(EnemyCreationSystem&,std::shared_ptr<StageQuest>,EnemyPopulationHost&);
std::int32_t enemy_spawn_weight(const EventInstruction&);
ComponentSlots* create_normal_enemy(EnemyCreationSystem&,EnemyPopulationHost&);
bool enemy_counts_towards_population(ComponentSlots*,const EnemyTable&);
// Prefix of EnemySystem.Update 0x114c774..0x114c9ec only. Returns the captured
// group for its later per-enemy pass, which is not implemented by this function.
EntitySubset& update_enemy_population(EnemyCreationSystem&,EnemyPopulationHost&);
class EnemyStageEntry final:public StageEnemyStart {
public:
    EnemyStageEntry(EnemyCreationSystem& state,EnemyPopulationHost& host):state_(state),host_(host){}
    void start_enemies(std::shared_ptr<StageQuest> quest)override{start_enemy_population(state_,std::move(quest),host_);}
private:
    EnemyCreationSystem& state_;EnemyPopulationHost& host_;
};
} // namespace saga::core
