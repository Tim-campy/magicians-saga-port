#include "mission_humans.hpp"
#include "core/gameplay/stage_update.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::desktop {
using namespace core;
void MissionHumans::enable_enemies(MissionEnemyScenario scenario){
    if(enemy_scenario_)throw std::invalid_argument("enemy system requires one explicit scenario");
    if(!native_state_&&reward_scenario_&&(scenario.defeated!=enemies_.defeated_enemies||scenario.remaining!=enemies_.remaining_enemies))throw std::invalid_argument("enemy_population counters conflict with rewards");
    if(!table_quest_){
        auto& quest=*stage_.quest;quest.enemy_maximum=scenario.maximum;quest.enemy_total_limit=scenario.total;
        quest.enemy_interval=scenario.interval;quest.base_level=scenario.level;
        auto rows=std::make_shared<std::vector<EventInstruction>>();
        for(const auto& row:scenario.pool)rows->push_back(std::make_shared<EventIntegers>(EventIntegers{row.first,row.second}));
        quest.normal_enemies=std::make_shared<std::vector<EventScript>>(1,std::move(rows));
    }
    if(!native_state_){
        enemies_.quest=stage_.quest;enemies_.created_enemies=scenario.created;
        enemies_.defeated_enemies=scenario.defeated;enemies_.remaining_enemies=scenario.remaining;enemies_.loop_counter=scenario.loop_counter;
        stage_.collection.boss_defeated=scenario.boss_defeated;
    }
    if(!native_state_&&!deferred_init_){
        std::fill(enemies_.maximum_by_section->begin(),enemies_.maximum_by_section->end(),scenario.section_limit);
        std::fill(enemies_.created_by_section->begin(),enemies_.created_by_section->end(),scenario.section_created);
    }
    enemy_scenario_=std::move(scenario);
}
void MissionHumans::apply_initialized_scenario(){
    if(input_)input_->apply_scenario();
    if(enemy_scenario_&&!native_state_){
        std::fill(enemies_.maximum_by_section->begin(),enemies_.maximum_by_section->end(),enemy_scenario_->section_limit);
        std::fill(enemies_.created_by_section->begin(),enemies_.created_by_section->end(),enemy_scenario_->section_created);
    }
}
void MissionHumans::advance_enemies(std::int32_t pass,bool enabled){
    if(enemy_scenario_&&gameplay_needs_update(5,enabled,pass))update_enemies(enemies_,*this);
}
std::int32_t MissionHumans::system_yugami(){
    if(!enemy_scenario_)throw std::logic_error("mission service unbound: explicit SYSSAVEI_YUGAMI");
    return enemy_scenario_->yugami;
}
void MissionHumans::subscribe_enemy_impact(EntityEvents::Single receiver,std::optional<EventHashes> hash){world_.events().subscribe(23,std::move(receiver),hash);}
void MissionHumans::subscribe_enemy_contact(EntityEvents::Pair receiver,std::optional<EventHashes> hash){world_.events().subscribe(29,std::move(receiver),hash);}
void MissionHumans::subscribe_enemy_human_dead(EntityEvents::Single receiver,std::optional<EventHashes> hash){world_.events().subscribe(30,std::move(receiver),hash);}
void MissionHumans::fly_away(ComponentSlots& e,std::int32_t x0,std::int32_t x1,std::int32_t z0,std::int32_t z1,bool modify){
    core::fly_away(e,x0,x1,z0,z1,modify,random_,static_cast<EnemyImpactHost&>(*this));
}
std::int32_t MissionHumans::progress_rate(float z){return stage_progress_rate(stage_,z,*this);}
void MissionHumans::contact_sound(){} // SAGA_STUB(mission-contact-audio): native contact state/effects still execute.
void MissionHumans::write_enemy_report(std::ostream& out){
    out<<",\"enemy_system_enabled\":"<<(enemy_scenario_?"true":"false");
    if(!enemy_scenario_)return;
    if(native_state_)out<<",\"new_game_enemy_records\":"<<saved_.enemy_defeat_counts.size();
    out<<",\"enemy_created\":"<<enemies_.created_enemies<<",\"enemy_loop_counter\":"<<enemies_.loop_counter<<",\"enemies\":[";
    bool first=true;auto cursor=world_.subset({24,0})->cursor();while(const auto entry=cursor.next()){
        const auto* link=dynamic_cast<EnemyRecordLink*>((*entry)->get(24));const auto* pos=dynamic_cast<ContactPosition*>((*entry)->get(0));
        if(!link||!pos)throw std::invalid_argument("missing diagnostic enemy fields");
        if(!first)out<<',';first=false;
        out<<"{\"id\":"<<world_.id(**entry)<<",\"enemy_id\":"<<link->enemy_id<<",\"level\":"<<link->level
           <<",\"state\":"<<link->state<<",\"frame\":"<<link->frame<<",\"flags\":"<<link->flags
           <<",\"x\":"<<pos->x<<",\"y\":"<<pos->y<<",\"z\":"<<pos->z<<",\"target\":";
        if(link->opponent)out<<world_.id(*link->opponent);else out<<"null";out<<'}';
    }out<<']';
}
} // namespace saga::desktop
