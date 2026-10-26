#include "combat_state.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& component(ComponentSlots& e,std::int32_t slot){
    auto* value=dynamic_cast<T*>(e.get(slot));
    if(!value)throw std::invalid_argument("missing state transition component");
    return *value;
}
}
void change_enemy_state(ComponentSlots& e,std::int32_t state){
    component<EnemyRecordLink>(e,combat_enemy_slot).state=state;
    component<EnemyRecordLink>(e,combat_enemy_slot).frame=0;
}
void before_human_state(ComponentSlots& e,std::int32_t old_state,std::int32_t new_state,HumanStateHost& host){
    if(old_state==1&&new_state==2){
        component<ContactDirection>(e,contact_direction_slot).direction=0;
        host.change_animation(e,36,true);
    }
}
void after_human_state(ComponentSlots& e,std::int32_t old_state,std::int32_t new_state,HumanStateHost& host){
    if(old_state==1&&new_state==0)component<ProjectileSpeed>(e,projectile_speed_slot).z=0.0F;
    if(new_state==0){
        component<ContactDirection>(e,contact_direction_slot).direction=0;
        if(!host.playing_cut_in(e))host.change_animation(e,36,true);
    }
    if(old_state==2&&new_state!=2){
        const auto frame=component<HumanRecordLink>(e,combat_human_slot).attack_frame;
        const auto interval=host.attack_interval(e);
        if(frame>interval)component<HumanRecordLink>(e,combat_human_slot).attack_frame=0;
    }
}
void change_human_state(ComponentSlots& e,std::int32_t state,HumanStateHost& host){
    const auto old_state=component<HumanRecordLink>(e,combat_human_slot).state;
    before_human_state(e,old_state,state,host);
    component<HumanRecordLink>(e,combat_human_slot).state=state;
    component<HumanRecordLink>(e,combat_human_slot).frame=0;
    after_human_state(e,old_state,state,host);
}
}
