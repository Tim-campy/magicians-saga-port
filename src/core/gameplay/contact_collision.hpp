#pragma once
#include "projectile_hit.hpp"

namespace saga::core {
inline constexpr std::int32_t contact_position_slot=0,contact_direction_slot=12,contact_bullet_slot=29;
struct ContactPosition final:Component {
    float x=0,y=0,z=0; // +0x10/+0x14/+0x18.
    float offset_x=0,offset_y=0,offset_z=0; // +0x1c/+0x20/+0x24.
    ComponentSlots* parent=nullptr; // +0x28; independent of Projectile.parent.
};
struct ContactDirection final:Component {std::int32_t direction=0;bool locked=false;};
class ContactCollisionHost:public ProjectileHitHost {
public:
    virtual bool collision_blocked(ComponentSlots&)=0; // Entity byte +0x2c, read only in Damage-component route.
    virtual void cut_in_animation(ComponentSlots&,std::int32_t,std::int32_t)=0;
    virtual void change_animation(ComponentSlots&,std::int32_t,std::int32_t)=0;
    virtual void effect(std::int32_t,std::int32_t,std::int32_t,float,float,float)=0;
    virtual void fly_away(ComponentSlots&,std::int32_t,std::int32_t,std::int32_t,std::int32_t,bool)=0;
    virtual void human_state(ComponentSlots&,std::int32_t)=0;
    virtual void enemy_state(ComponentSlots&,std::int32_t)=0;
    virtual std::int32_t progress_rate(float depth)=0;
    virtual ComponentSlots& hero()=0;
    virtual void contact_sound()=0; // Read AppData static +0x254 when invoked.
};
// EnemySystem.OnCollide 0x114f604; collision detection and presentation services remain external.
void on_contact_collision(ComponentSlots* first,ComponentSlots* second,const CombatRecordTables&,
                          const ParameterMaximumLevels&,ContactCollisionHost&);
}
