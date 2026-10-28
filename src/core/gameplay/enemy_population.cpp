#include "enemy_population.hpp"
#include "stage_update.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing enemy population input");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return required(dynamic_cast<T*>(e.get(slot)));}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t multiply(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
std::int32_t coordinate(float z){
    if(std::isnan(z))return 0;
    if(z==std::numeric_limits<float>::infinity()||z<=-2147483648.0F)return std::numeric_limits<std::int32_t>::min();
    if(z>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();return static_cast<std::int32_t>(z);
}
ComponentSlots& hero(EnemyCreationSystem& s,EnemyCreationHost& host){required(s.members);return required(member_hero(host.enemy_world()));}
ContactPosition& position(ComponentSlots& e){return component<ContactPosition>(e,0);}
EventScript first(const StageScriptGroups& groups){return required(groups.get()).at(0);}
void spinning_entry(ComponentSlots& e,EnemyCreationHost& host){
    AnimationModificationRequest request;request.type=9;request.angle=30;request.loop=true;
    add_animation_modification(e,request,host.enemy_modifications());
}
}
void destroy_all_population_enemies(EnemyCreationSystem& s,EnemyPopulationHost& host){
    auto* world=host.population_destruction_world();auto& group=required(s.subset);
    std::vector<ComponentSlots*> snapshot;{auto cursor=group.cursor();while(const auto e=cursor.next())snapshot.push_back(*e);}
    auto& destruction=required(world);
    for(auto* e:snapshot)if(e)destroy_quest_entity(host.population_entity_id(*e),true,destruction);
}
void reset_enemy_population(EnemyCreationSystem& s,EnemyPopulationHost& host){
    s.created_enemies=0;s.defeated_enemies=0;s.loop_counter=0;
    s.remaining_enemies=required(s.quest.get()).enemy_total_limit;
    auto& counts=required(s.created_by_section.get());std::fill(counts.begin(),counts.end(),0);
    destroy_all_population_enemies(s,host);
}
void create_fixed_enemies(EnemyCreationSystem& s,EnemyPopulationHost& host){
    const auto rows=first(required(s.quest.get()).fixed_enemies);auto* map=host.enemy_map();
    for(const auto& pointer:required(rows.get())){
        const auto& row=required(pointer.get());const auto id=row.at(0),rate=row.at(1);
        const auto x=required(map).step_x;auto& stage=required(s.stage);
        const auto z=stage_z(stage,rate,host.enemy_coordinates());const auto level=required(s.quest.get()).base_level;
        create_system_enemy(s,{2,id,level,2,{static_cast<float>(x),0,static_cast<float>(z)}},host);
    }
}
void start_enemy_population(EnemyCreationSystem& s,std::shared_ptr<StageQuest> quest,EnemyPopulationHost& host){
    s.quest=std::move(quest);reset_enemy_population(s,host);create_fixed_enemies(s,host);
    const auto x=position(hero(s,host)).x;
    const auto first_step=required(host.enemy_map()).step_z;
    const auto height=stage_quest_height(required(s.quest.get()));
    const auto second_step=required(host.enemy_map()).step_z;
    const auto z=add(add(multiply(add(height,-2),first_step),-(second_step/2)),-50);
    auto& initial=create_system_enemy(s,{-1,0,1,0,{x,0,static_cast<float>(z)}},host);spinning_entry(initial,host);
    const auto quota=std::max(required(s.quest.get()).enemy_total_limit/10,1);
    const auto limits=s.maximum_by_section;auto& values=required(limits.get());
    const auto length=static_cast<std::int32_t>(values.size());
    for(std::int32_t i=0;i<length;++i)values.at(static_cast<std::size_t>(i))=i<=length-4?quota:99999;
    if(host.system_yugami()!=10000){auto& current=required(s.maximum_by_section.get());for(auto& v:current)v=multiply(v,100);}
}
std::int32_t enemy_spawn_weight(const EventInstruction& row){return required(row.get()).at(1);}
ComponentSlots* create_normal_enemy(EnemyCreationSystem& s,EnemyPopulationHost& host){
    s.created_enemies=add(s.created_enemies,1);
    const auto rows=first(required(s.quest.get()).normal_enemies);std::vector<std::int32_t> weights;
    weights.reserve(required(rows.get()).size());for(const auto& row:*rows)weights.push_back(enemy_spawn_weight(row));
    const auto selected=weighted_lottery(&weights,host.population_lottery(),host.population_random());
    if(selected==-1)return nullptr;
    const auto row=required(rows.get()).at(static_cast<std::size_t>(selected));const auto id=required(row.get()).at(0);
    auto* player=world_player_entity(required(host.enemy_world()));
    const auto column=random_inclusive(host.population_random(),-1,1);
    const auto x=required(host.enemy_map()).step_x;
    const auto player_z=position(required(player)).z;
    const auto step=required(host.enemy_map()).step_z;auto& stage=required(s.stage);
    const auto end=stage_z(stage,100,host.enemy_coordinates());
    const auto z=std::max(add(coordinate(player_z),multiply(step,-2)),end);
    const auto level=required(s.quest.get()).base_level;
    auto& result=create_system_enemy(s,{2,id,level,0,{static_cast<float>(add(x,multiply(column,36))),0,static_cast<float>(z)}},host);
    auto* current_stage=s.stage;const auto live_z=position(required(player)).z;
    const auto progress=std::clamp(stage_progress_rate(required(current_stage),live_z,host.enemy_coordinates()),0,100);
    auto& count=required(s.created_by_section.get()).at(static_cast<std::size_t>(progress/10));count=add(count,1);
    change_enemy_state(result,1);return &result;
}
bool enemy_counts_towards_population(ComponentSlots* e,const EnemyTable& table){
    const auto id=component<EnemyRecordLink>(required(e),24).enemy_id;
    const auto& row=table.at(static_cast<std::size_t>(id));return (required(row?&*row:nullptr).flags&1)==0;
}
EntitySubset& update_enemy_population(EnemyCreationSystem& s,EnemyPopulationHost& host){
    s.loop_counter=add(s.loop_counter,1)%std::numeric_limits<std::int32_t>::max();
    auto& group=required(s.subset);const auto& q=required(s.quest.get());
    auto maximum=q.enemy_maximum,total=q.enemy_total_limit,interval=q.enemy_interval;
    if(host.system_yugami()!=10000){maximum=multiply(maximum,100);total=multiply(total,100);interval=1;}
    auto* stage=s.stage;const auto z=position(hero(s,host)).z;
    const auto progress=stage_progress_rate(required(stage),z,host.enemy_coordinates());
    const auto section=std::clamp(progress/10,0,100);
    if(required(s.stage).collection.boss_defeated||s.remaining_enemies<2||s.created_enemies>=add(total,-2))return group;
    const auto created=required(s.created_by_section.get()).at(static_cast<std::size_t>(section));
    const auto limit=required(s.maximum_by_section.get()).at(static_cast<std::size_t>(section));
    if(created>=limit)return group;
    std::int32_t counted=0;{auto cursor=group.cursor();while(const auto e=cursor.next()){
        const auto records=host.population_records();if(enemy_counts_towards_population(*e,required(records.get())))counted=add(counted,1);
    }}
    if(counted>=maximum)return group;
    // ARM64 SDIV/MSUB remainder: divisor zero leaves the original counter.
    const auto remainder=interval==0?s.loop_counter:(interval==-1&&s.loop_counter==std::numeric_limits<std::int32_t>::min()?0:s.loop_counter%interval);
    if(remainder==0)create_normal_enemy(s,host);return group;
}
} // namespace saga::core
