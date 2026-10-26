#pragma once
#include "ecs_combat.hpp"

namespace saga::core {
inline constexpr std::int32_t projectile_speed_slot=1,projectile_slot=27,
    projectile_collision_slot=32,projectile_damage_slot=33;
struct ProjectilePoint {float x=0,y=0,z=0;};
struct ProjectileLink final:Component {
    ComponentSlots* parent=nullptr; // +0x38.
    ProjectilePoint start,end; // +0x10/+0x1c.
    std::int32_t speed=0,max_height=0,frame=0,max_frame=0; // +0x28/+0x2c/+0x30/+0x34.
};
struct ProjectileDamage final:Component {std::int32_t value=0,attribute=0;ComponentSlots* parent=nullptr;}; // +0x10/+0x14/+0x18.
struct ProjectileSpeed final:Component {float x=0,y=0,z=0;}; // +0x10/+0x14/+0x18.
struct ProjectileCollision final:Component {
    std::int32_t radius=0,offset_x=0,offset_y=0,offset_z=0; // +0x10/+0x14/+0x18/+0x1c.
    // visited +0x20 belongs to the collision-system traversal, not CheckHit.
};
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
