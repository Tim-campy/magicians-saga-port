#include "human_impact.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& component(ComponentSlots& e,std::int32_t slot){
    auto* p=dynamic_cast<T*>(e.get(slot));
    if(!p)throw std::invalid_argument("missing human impact component");
    return *p;
}
}
void human_projectile_impact(ComponentSlots* entity,HumanImpactHost& host){
    if(!entity||host.destroyed(*entity))return;
    auto& e=*entity;
    if(!e.contains(impact_cell_slot)||!e.contains(combat_human_slot))return;
    if(component<HumanRecordLink>(e,combat_human_slot).state!=4)return;
    host.change_animation(e,12,true);
    const auto x=component<ContactPosition>(e,contact_position_slot).x;
    const auto y=component<ContactPosition>(e,contact_position_slot).y;
    const auto z=component<ContactPosition>(e,contact_position_slot).z;
    host.impact_effect(100,100,1,x,y+10.0F,z);
    change_human_state(e,5,host);
    host.send_human_dead(e);
}
void stage_human_dead(ComponentSlots& e,const CombatRecordTables& tables,HumanDeathHost& host){
    const auto* data=resolve_combat_character(e,tables);
    if(!data)throw std::invalid_argument("missing dead human record");
    if((data->flags&2)!=0)host.game_over(1);
}
}
