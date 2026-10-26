#include "contact_collision.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& require(T* value){if(!value)throw std::invalid_argument("missing collision input");return *value;}
template<class T>T& component(ComponentSlots& entity,std::int32_t slot){return require(dynamic_cast<T*>(entity.get(slot)));}
std::int32_t bits(std::uint32_t value){return static_cast<std::int32_t>(value<=0x7fffffffU?static_cast<std::int64_t>(value):static_cast<std::int64_t>(value)-0x100000000LL);}
void effect(ComponentSlots& entity,std::int32_t id,float y_offset,float z_offset,ContactCollisionHost& host){
    const auto x=component<ContactPosition>(entity,contact_position_slot).x;
    const auto y=component<ContactPosition>(entity,contact_position_slot).y;
    const auto z=component<ContactPosition>(entity,contact_position_slot).z;
    host.effect(0,id,0,x,y+y_offset,z_offset==0.0F?z:z+z_offset);
}
}
void on_contact_collision(ComponentSlots* first,ComponentSlots* second,const CombatRecordTables& tables,
                          const ParameterMaximumLevels& maxima,ContactCollisionHost& host) {
    auto& a=require(first);
    auto* enemy=a.contains(combat_enemy_slot)?&a:
        require(second).contains(combat_enemy_slot)?second:nullptr;
    if(a.contains(contact_bullet_slot)||require(second).contains(contact_bullet_slot)) {
        const bool first_bullet=a.contains(contact_bullet_slot);
        on_projectile_hit(first_bullet?first:second,first_bullet?second:first,host);return;
    }
    if((a.contains(projectile_damage_slot)&&!a.contains(contact_bullet_slot))||
       (require(second).contains(projectile_damage_slot)&&!second->contains(contact_bullet_slot))) {
        if(!enemy||host.collision_blocked(*enemy))return;
        auto& source=a.contains(projectile_damage_slot)?a:require(second);
        auto& damage=component<ProjectileDamage>(source,projectile_damage_slot);
        const auto attribute=component<ProjectileDamage>(source,projectile_damage_slot).attribute;
        if(host.damage(damage.parent,*enemy,damage.value,attribute))host.defeat(*enemy);
    }
    auto& monster=require(enemy);
    const auto seb=require(resolve_combat_enemy(monster,tables)).enemy_seb;
    const auto shifted=static_cast<std::uint32_t>(seb)<<2;
    host.cut_in_animation(monster,bits(shifted+24U),bits(shifted|2U));
    if(monster.contains(contact_direction_slot))component<ContactDirection>(monster,contact_direction_slot).locked=true;
    effect(monster,2,10.0F,12.0F,host);
    auto* human=a.contains(combat_human_slot)?&a:require(second).contains(combat_human_slot)?second:nullptr;
    if(!human)return;
    const auto state=component<HumanRecordLink>(*human,combat_human_slot).state;
    if((static_cast<std::uint32_t>(state)&0xfffffffeU)==4U)return;
    const auto& record=require(resolve_combat_enemy(monster,tables));
    if((record.flags&1)!=0) {
        host.fly_away(monster,-100,100,-100,-50,true);
        host.enemy_state(monster,4);host.contact_sound();return;
    }
    const auto attack=enemy_parameter(record,3,component<EnemyRecordLink>(monster,combat_enemy_slot).level,maxima);
    if(component<HumanRecordLink>(*human,combat_human_slot).invincible_time<=0&&
       host.damage(&monster,*human,attack,record.attribute)) {
        host.fly_away(*human,-100,100,50,100,false);
        host.change_animation(*human,16,1);
        effect(*human,24,10.0F,0.0F,host);
        host.human_state(*human,4);
    }
    const auto progress=host.progress_rate(component<ContactPosition>(monster,contact_position_slot).z);
    float distance=3.0F;
    if(progress<=79&&(component<EnemyRecordLink>(monster,combat_enemy_slot).flags&1)==0) {
        distance=10.0F;
        if((require(resolve_combat_character(*human,tables)).flags&4)!=0) {
            const auto human_z=component<ContactPosition>(*human,contact_position_slot).z;
            auto& hero=host.hero();
            if(human_z>component<ContactPosition>(hero,contact_position_slot).z)distance=16.0F;
        }
    }
    auto& position=component<ContactPosition>(monster,contact_position_slot);
    position.z=position.z-distance;
    component<ProjectileSpeed>(monster,projectile_speed_slot).z=-distance;
    host.enemy_state(monster,3);host.contact_sound();
}
}
