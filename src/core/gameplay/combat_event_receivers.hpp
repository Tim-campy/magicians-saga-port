#pragma once
#include "combat_records.hpp"
#include "../ecs/entity_subset.hpp"

namespace saga::core {
class CombatEventHost {
public:
    virtual ~CombatEventHost()=default;
    virtual bool destroyed(ComponentSlots&)=0;
    virtual void destroy(ComponentSlots&,bool remove)=0;
    // EnemySystem.SearchTarget 0x114da7c; target selection remains explicit.
    virtual ComponentSlots* search_opponent(ComponentSlots&)=0;
};
// ProjectileSystem.OnProjectileImpact 0x1160bd8. Event 23, after Human/Enemy registration.
void projectile_impact_cleanup(ComponentSlots*,CombatEventHost&);
// EnemySystem.OnHumanDead 0x11511cc. Init supplies the live And({24}) group.
void retarget_after_human_death(EntitySubset&,ComponentSlots* dead,CombatEventHost&);
}
