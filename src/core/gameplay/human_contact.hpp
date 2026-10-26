#pragma once
#include "combat_state.hpp"

namespace saga::core {
class HumanContactHost:public HumanStateHost {
public:
    virtual bool destroyed(ComponentSlots&)=0;
    virtual ComponentSlots& hero()=0; // HumanSystem's MemberSystem +0x50 must exist.
    virtual void cut_in_animation(ComponentSlots&,std::int32_t,std::int32_t)=0;
    virtual void contact_effect(std::int32_t,std::int32_t,std::int32_t,float,float,float)=0;
};
// HumanSystem.OnCollide 0x1157be0. Separate event-29 receiver from EnemySystem.OnCollide.
void on_human_contact(ComponentSlots*,ComponentSlots*,const CombatRecordTables&,HumanContactHost&);
}
