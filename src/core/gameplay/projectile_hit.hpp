#pragma once
#include "ecs_combat.hpp"

namespace saga::core {
inline constexpr std::int32_t projectile_speed_slot=1,projectile_slot=27,
    projectile_collision_slot=32,projectile_damage_slot=33;
struct ProjectileLink final:Component {ComponentSlots* parent=nullptr;}; // +0x38.
struct ProjectileDamage final:Component {std::int32_t value=0,attribute=0;}; // +0x10/+0x14.
struct ProjectileSpeed final:Component {float x=0,y=0,z=0;}; // +0x10/+0x14/+0x18.
struct ProjectileCollision final:Component {}; // Presence only; collision shape remains external.
class ProjectileHitHost {
public:
    virtual ~ProjectileHitHost()=default;
    virtual bool damage(ComponentSlots* attacker,ComponentSlots& receiver,std::int32_t,std::int32_t)=0;
    virtual void defeat(ComponentSlots& receiver)=0;
    virtual void impact(ComponentSlots& projectile,ComponentSlots* current_parent)=0;
    // Original generated removers push captured objects after ECS removal notifications.
    virtual void return_component(std::int32_t slot,Component*)=0;
};
// EnemySystem.OnBulletHit 0x114f4ac. Collision detection itself is not implemented here.
void on_projectile_hit(ComponentSlots* projectile,ComponentSlots* receiver,ProjectileHitHost&);
}
