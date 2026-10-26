#include "projectile_hit.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& require(T* value){if(!value)throw std::invalid_argument("missing projectile hit input");return *value;}
template<class T>T& component(ComponentSlots& entity,std::int32_t slot) {
    return require(dynamic_cast<T*>(entity.get(slot)));
}
template<class T>void remove(ComponentSlots& entity,std::int32_t slot,ProjectileHitHost& host) {
    auto* captured=&component<T>(entity,slot);
    entity.remove(slot);
    host.return_component(slot,captured);
}
}
void on_projectile_hit(ComponentSlots* projectile,ComponentSlots* receiver,ProjectileHitHost& host) {
    auto& target=require(receiver);
    if(!target.contains(combat_enemy_slot))return;
    auto& source=require(projectile);
    if(!source.contains(projectile_slot))return;
    if(!source.contains(projectile_collision_slot))return;
    const auto amount=component<ProjectileDamage>(source,projectile_damage_slot).value;
    const auto attribute=component<ProjectileDamage>(source,projectile_damage_slot).attribute;
    auto* attacker=component<ProjectileLink>(source,projectile_slot).parent;
    if(host.damage(attacker,target,amount,attribute))host.defeat(target);
    // Re-read after damage/defeat callbacks. A removed Projectile component faults here.
    host.impact(source,component<ProjectileLink>(source,projectile_slot).parent);
    if(source.contains(projectile_slot))remove<ProjectileLink>(source,projectile_slot,host);
    if(source.contains(projectile_collision_slot))remove<ProjectileCollision>(source,projectile_collision_slot,host);
    component<ProjectileSpeed>(source,projectile_speed_slot).x=0.0F;
    component<ProjectileSpeed>(source,projectile_speed_slot).y=0.0F;
    component<ProjectileSpeed>(source,projectile_speed_slot).z=0.0F;
}
}
