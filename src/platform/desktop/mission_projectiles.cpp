#include "mission_humans.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing mission projectile input");return *p;}
[[noreturn]]void unbound(const char* service){throw std::logic_error(std::string("mission service unbound: ")+service);}
}
void MissionHumans::advance_projectiles(std::int32_t pass,bool enabled){
    if(gameplay_needs_update(10,enabled,pass))projectiles_.pass.value().update(required(projectiles_.subset),*this);
}
void MissionHumans::play_attack_sound(bool){++attack_sounds_;} // SAGA_STUB(mission-attack-audio): no audio output backend.
void MissionHumans::subscribe_impact(EntityEvents::Single receiver,std::optional<EventHashes> hash){world_.events().subscribe(23,std::move(receiver),hash);}
void MissionHumans::send_impact(ComponentSlots& e){world_.events().send(23,&e);}
ComponentSlots* MissionHumans::search_opponent(ComponentSlots& e){return search_enemy_target(enemies_,&e,*this);}
const BulletImpactData* MissionHumans::bullet_data(ComponentSlots& e){
    auto* component=dynamic_cast<BulletComponent*>(e.get(29));return resolve_bullet_record(required(component),world_);
}
void MissionHumans::read_bullet_data(ComponentSlots& e){static_cast<void>(bullet_data(e));}
void MissionHumans::create_bullet_impact(ComponentSlots& e,ComponentSlots& owner){create_impact_effect(&e,&owner,records_,*this);}
void MissionHumans::bullet_hit(ComponentSlots& e,ComponentSlots* target){on_projectile_hit(&e,target,*this);}
ComponentSlots* MissionHumans::create_effect(std::int32_t clip,std::int32_t image,std::int32_t loop,ProjectilePoint at){return &world_.effect(at,clip,image,loop!=0);}
ComponentSlots* MissionHumans::create_emitter(std::int32_t category,std::int32_t type,std::int32_t interval,std::int32_t maximum,std::int32_t argument,ProjectilePoint at){return &world_.emitter({category,type,interval,maximum,argument,at});}
void MissionHumans::boss_defeated(ComponentSlots&){unbound("boss completion");}
void MissionHumans::add_color(ComponentSlots& e,std::int32_t r,std::int32_t g,std::int32_t b,std::int32_t a){
    auto& color=world_.acquire_enemy_color();color.values={{r,g,b,a}};e.add(6,&color);
}
bool MissionHumans::damage(ComponentSlots* attacker,ComponentSlots& target,std::int32_t amount,std::int32_t attribute){
    const CombatConfiguration config{runtime_.debug,false};
    EcsDamageHost host(required(attacker),target,records_,levels_,world_,world_.enemy_maximum_levels(),affinities_,config,*this);
    return apply_combat_damage(amount,attribute,random_,host);
}
void MissionHumans::enable_rewards(MissionRewardScenario scenario){
    if(reward_scenario_||!world_.reward_catalog_ready())throw std::invalid_argument("rewards require one explicit scenario and a loaded Treasure catalog");
    if(enemy_scenario_&&(scenario.defeated!=enemies_.defeated_enemies||scenario.remaining!=enemies_.remaining_enemies))throw std::invalid_argument("reward counters conflict with enemy_population");
    enemies_.defeated_enemies=scenario.defeated;enemies_.remaining_enemies=scenario.remaining;
    for(const auto& row:records_.enemies)if(row)saved_.enemy_defeat_counts.emplace(row->id,scenario.saved_defeat_count);
    reward_scenario_=scenario;
}
void MissionHumans::defeat(ComponentSlots& e){
    if(!reward_scenario_)unbound("enemy defeat requires explicit counters and reward catalogs");
    defeat_enemy(e,records_,enemies_,random_,*this);
}
void MissionHumans::write_reward_report(std::ostream& out){
    out<<",\"rewards_enabled\":"<<(reward_scenario_?"true":"false");
    if(!reward_scenario_&&!enemy_scenario_)return;
    out<<",\"defeated_enemies\":"<<enemies_.defeated_enemies<<",\"remaining_enemies\":"<<enemies_.remaining_enemies;
    if(!reward_scenario_)return;
    out<<",\"saved_defeats\":[";bool first=true;
    for(const auto& entry:saved_.enemy_defeat_counts)if(entry.second!=reward_scenario_->saved_defeat_count){
        if(!first)out<<',';first=false;out<<"{\"enemy_id\":"<<entry.first<<",\"count\":"<<entry.second<<'}';}
    out<<']';
}
void MissionHumans::miss(ComponentSlots&){} // SAGA_STUB(mission-damage-labels): floating miss label only.
void MissionHumans::effects(ComponentSlots& receiver,const DamageNumbers& numbers){present_combat_damage(receiver,numbers,*this);}
void MissionHumans::damage_labels(ComponentSlots&,const DamageNumbers&){++damage_presentations_;} // SAGA_STUB(mission-damage-labels): floating numeric/critical labels only; stateful shake/impact are executed by present_combat_damage.
void MissionHumans::diagnostics(const DamageNumbers&){} // SAGA_STUB(mission-damage-diagnostics): optional diagnostic output.
} // namespace saga::desktop
