#pragma once
#include "human_impact.hpp"
#include "projectile_launch.hpp"
#include "collision_check.hpp"

namespace saga::core {
class EnemyImpactHost:public ProjectileLaunchHost {
public:
    virtual bool destroyed(ComponentSlots&)=0;
    virtual void read_bullet_data(ComponentSlots&)=0; // Preserve Bullet.get_data validation even though result is unused.
    virtual void create_bullet_impact(ComponentSlots&,ComponentSlots& parent)=0;
    virtual bool check_hit(ComponentSlots& source,ComponentSlots& target){return check_collision(&source,&target);}
    virtual void bullet_hit(ComponentSlots&,ComponentSlots* target)=0; // Bind existing on_projectile_hit.
    virtual ComponentSlots* create_effect(std::int32_t,std::int32_t,std::int32_t,ProjectilePoint)=0;
    virtual ComponentSlots* create_emitter(std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,ProjectilePoint)=0;
    virtual void boss_defeated(ComponentSlots&)=0; // StageSystem.OnBossDefeat remains separate.
};
// EnemySystem.OnProjectileImpact 0x114e7b4; enemy_initialization binds event 23.
void enemy_projectile_impact(ComponentSlots*,const CombatRecordTables&,RandomSource&,EnemyImpactHost&);
}
