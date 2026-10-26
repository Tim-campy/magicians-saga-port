#pragma once
#include "combat_state.hpp"

namespace saga::core {
inline constexpr std::int32_t impact_cell_slot=5;
class HumanImpactHost:public HumanStateHost {
public:
    virtual bool destroyed(ComponentSlots&)=0; // Entity byte +0x2c.
    virtual void impact_effect(std::int32_t,std::int32_t,std::int32_t,float,float,float)=0;
    virtual void send_human_dead(ComponentSlots&)=0; // World.Send<Entity>(30).
};
class HumanDeathHost {
public:
    virtual ~HumanDeathHost()=default;
    virtual void game_over(std::int32_t reason)=0; // Retry/dialog policy remains external.
};
// HumanSystem.OnProjectileImpact 0x1157a50; null/destroyed entities return.
void human_projectile_impact(ComponentSlots*,HumanImpactHost&);
// StageSystem.OnHumanDead 0x1166514; no null/destroyed/kind guard before record lookup.
void stage_human_dead(ComponentSlots&,const CombatRecordTables&,HumanDeathHost&);
}
