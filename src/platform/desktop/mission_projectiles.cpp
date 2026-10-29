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
ComponentSlots* MissionHumans::search_opponent(ComponentSlots&){unbound("enemy retarget");}
const BulletImpactData* MissionHumans::bullet_data(ComponentSlots& e){
    auto* component=dynamic_cast<BulletComponent*>(e.get(29));return resolve_bullet_record(required(component),world_);
}
void MissionHumans::read_bullet_data(ComponentSlots& e){static_cast<void>(bullet_data(e));}
void MissionHumans::create_bullet_impact(ComponentSlots& e,ComponentSlots& owner){create_impact_effect(&e,&owner,records_,*this);}
void MissionHumans::bullet_hit(ComponentSlots& e,ComponentSlots* target){on_projectile_hit(&e,target,*this);}
ComponentSlots* MissionHumans::create_effect(std::int32_t clip,std::int32_t image,std::int32_t loop,ProjectilePoint at){return &world_.effect(at,clip,image,loop!=0);}
ComponentSlots* MissionHumans::create_emitter(std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,ProjectilePoint){unbound("enemy landing debris emitters");}
void MissionHumans::boss_defeated(ComponentSlots&){unbound("boss completion");}
void MissionHumans::add_color(ComponentSlots& e,std::int32_t r,std::int32_t g,std::int32_t b,std::int32_t a){
    auto& color=world_.acquire_enemy_color();color.values={{r,g,b,a}};e.add(6,&color);
}
bool MissionHumans::damage(ComponentSlots* attacker,ComponentSlots& target,std::int32_t amount,std::int32_t attribute){
    const CombatConfiguration config{runtime_.debug,false};
    EcsDamageHost host(required(attacker),target,records_,levels_,world_,world_.enemy_maximum_levels(),affinities_,config,*this);
    return apply_combat_damage(amount,attribute,random_,host);
}
void MissionHumans::defeat(ComponentSlots&){unbound("enemy defeat, saved counts and pending reward/drop factories");}
void MissionHumans::miss(ComponentSlots&){} // SAGA_STUB(mission-damage-labels): floating miss label only.
void MissionHumans::effects(ComponentSlots& receiver,const DamageNumbers& numbers){present_combat_damage(receiver,numbers,*this);}
void MissionHumans::damage_labels(ComponentSlots&,const DamageNumbers&){++damage_presentations_;} // SAGA_STUB(mission-damage-labels): floating numeric/critical labels only; stateful shake/impact are executed by present_combat_damage.
void MissionHumans::diagnostics(const DamageNumbers&){} // SAGA_STUB(mission-damage-diagnostics): optional diagnostic output.
} // namespace saga::desktop
