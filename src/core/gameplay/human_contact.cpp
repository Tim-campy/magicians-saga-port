#include "human_contact.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& require(T* p){if(!p)throw std::invalid_argument("missing human contact input");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return require(dynamic_cast<T*>(e.get(slot)));}
}
void on_human_contact(ComponentSlots* first,ComponentSlots* second,const CombatRecordTables& tables,HumanContactHost& host){
    auto& a=require(first);
    auto* human=a.contains(combat_human_slot)?&a:
        require(second).contains(combat_human_slot)?second:nullptr;
    if(a.contains(projectile_damage_slot)||require(second).contains(projectile_damage_slot))return;
    if(!human||host.destroyed(*human))return;
    const auto state=component<HumanRecordLink>(*human,combat_human_slot).state;
    if((static_cast<std::uint32_t>(state)&0xfffffffeU)==4U)return;
    auto* enemy=a.contains(combat_enemy_slot)?&a:second->contains(combat_enemy_slot)?second:nullptr;
    if(!enemy)return;
    if((require(resolve_combat_enemy(*enemy,tables)).flags&1)!=0)return;
    float distance=10.0F;
    if((component<EnemyRecordLink>(*enemy,combat_enemy_slot).flags&1)==0){
        distance=3.0F;
        if((require(resolve_combat_character(*human,tables)).flags&4)!=0){
            const auto z=component<ContactPosition>(*human,contact_position_slot).z;
            auto& hero=host.hero();
            if(z>component<ContactPosition>(hero,contact_position_slot).z)distance=0.5F;
        }
    }
    auto& position=component<ContactPosition>(*human,contact_position_slot);
    position.z=position.z+distance;
    component<ProjectileSpeed>(*human,projectile_speed_slot).z=distance;
    host.cut_in_animation(*human,4,36);
    component<ContactDirection>(*human,contact_direction_slot).locked=true;
    const auto x=component<ContactPosition>(*human,contact_position_slot).x;
    const auto y=component<ContactPosition>(*human,contact_position_slot).y;
    const auto z=component<ContactPosition>(*human,contact_position_slot).z;
    host.contact_effect(0,2,0,x,y+10.0F,z-12.0F);
    change_human_state(*human,3,host);
}
}
