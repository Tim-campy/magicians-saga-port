#include "enemy_impact.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& require(T* p){if(!p)throw std::invalid_argument("missing enemy impact input");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return require(dynamic_cast<T*>(e.get(slot)));}
ContactPosition& position(ComponentSlots& e){return component<ContactPosition>(e,contact_position_slot);}
ProjectilePoint point(ComponentSlots& e){return {position(e).x,position(e).y,position(e).z};}
ComponentSlots* opponent(ComponentSlots& e){return component<HumanRecordLink>(e,combat_human_slot).opponent;}
std::int32_t product(std::int32_t a,std::int32_t b){
    const auto v=static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b);
    return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);
}
}
void enemy_projectile_impact(ComponentSlots* entity,const CombatRecordTables& tables,RandomSource& random,EnemyImpactHost& host){
    if(!entity||host.destroyed(*entity))return;
    auto& e=*entity;if(!e.contains(impact_cell_slot))return;
    auto* parent=component<ProjectileLink>(e,projectile_slot).parent;
    if(e.contains(contact_bullet_slot)){
        host.read_bullet_data(e);
        auto& owner=require(parent);
        resolve_combat_equipment(owner,EquipmentSlot::weapon,tables); // Unused result, required lookup.
        host.create_bullet_impact(e,owner);
        if(opponent(owner)&&host.check_hit(e,require(opponent(owner))))host.bullet_hit(e,opponent(owner));
    }
    if(!e.contains(combat_enemy_slot))return;
    auto at=point(e);at.y=at.y+10.0F;
    auto* effect=host.create_effect(0,24,0,at);
    for(int i=0;i<3;++i){
        auto& effect_entity=require(effect);
        auto* emitter=host.create_emitter(0,1,1,-1,0,point(effect_entity));
        const auto start=point(effect_entity);
        const float x=position(effect_entity).x;
        const auto dx=random_inclusive(random,50,150);
        const auto sx=random_hits(random,50)?1:-1; // Random.Choose(50,1,-1).
        const float z=position(effect_entity).z;
        const auto dz=random_inclusive(random,50,150);
        const auto sz=random_hits(random,50)?1:-1;
        const ProjectilePoint end{x+static_cast<float>(product(sx,dx)),0.0F,z+static_cast<float>(product(sz,dz))};
        launch_projectile(require(emitter),start,end,5,nullptr,true,false,50,host);
    }
    if(e.contains(modify_animation_slot)){
        auto* old=&component<ProjectileModification>(e,modify_animation_slot);
        e.remove(modify_animation_slot);host.return_component(modify_animation_slot,old);
    }
    add_projectile_modification(e,3,10,true,false,host);
    if((component<EnemyRecordLink>(e,combat_enemy_slot).flags&1)!=0)host.boss_defeated(e);
}
}
