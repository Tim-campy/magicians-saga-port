#include "human_shoot.hpp"
#include "animation_pass.hpp"
#include "projectile_motion.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing human shoot input");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return required(dynamic_cast<T*>(e.get(slot)));}
ContactPosition& position(ComponentSlots& e){return component<ContactPosition>(e,0);}
ProjectilePoint point(ComponentSlots& e){return {position(e).x,position(e).y,position(e).z};}
std::int32_t decrement(std::int32_t a){const auto v=static_cast<std::uint32_t>(a)-1U;
    return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
}
EventInstruction initial_attack_animation_ids(){return std::make_shared<EventIntegers>(EventIntegers{24,24,24,24,24,24,24,48,52,44,24});}
bool human_spell_matches(HumanFrameSystem& s,const BulletRecord* p){const auto& record=required(p);return record.attribute==required(s.stage).attack_attribute;}
bool shoot_human(HumanFrameSystem& s,ComponentSlots* actor,ComponentSlots* target,HumanFrameHost& frame,HumanShootHost& host){
    if(!target||frame.human_target_destroyed(*target))return false;
    if(enemy_is_dead(component<EnemyRecordLink>(*target,24)))return false;
    auto& e=required(actor);const auto* weapon=resolve_combat_equipment(e,EquipmentSlot::weapon,frame.human_records());
    const auto* character=resolve_combat_character(e,frame.human_records());
    BulletRecords selected_table;const BulletRecord* spell=nullptr;
    if((required(character).flags&2)!=0){
        selected_table=host.shoot_bullet_records();
        // Public Enumerable.FirstOrDefault(predicate): first match, no null-row skipping.
        for(auto& row:required(selected_table.get()))if(human_spell_matches(s,row?&*row:nullptr)){spell=&*row;break;}
    }else{
        if(required(weapon).bullet_id!=-1){selected_table=host.shoot_bullet_records();
            auto& row=required(selected_table.get()).at(static_cast<std::size_t>(weapon->bullet_id));spell=row?&*row:nullptr;}
    }
    if(!spell||!human_has_mp(s,&e,spell->attribute,frame))return false;
    use_human_mp(&e,frame);
    if(!human_has_mp(s,&e,spell->attribute,frame)){
        auto* world=host.shoot_effect_world();auto at=point(e);at.y=at.y+50.0F;
        auto& w=required(world);create_effect({at,7,3,0,0,false,false,30,0,nullptr,-1,true},w.ids,w.factory);
    }
    required(s.members);const auto attack=frame.human_parameter(e,3);
    auto* world=host.shoot_bullet_world();auto at=point(e);at.y=at.y+10.0F;
    auto& bullet=create_bullet({spell->id,attack,spell->attribute,&e,at},required(world));
    auto start=point(bullet);start.y=start.y+10.0F;const auto end=point(*target);
    launch_projectile(bullet,start,end,required(weapon).bullet_speed,&e,true,true,100,host.shoot_launch_host());
    required(s.members);const auto attribute=static_cast<std::uint32_t>(spell->attribute)<=2U?frame.human_parameter(e,spell->attribute+10):0;
    const auto scale=easing_float(1.0F,3.0F,98.0F,static_cast<float>(decrement(attribute)),0.0F);
    component<ProjectileModification>(bullet,15).scale_x=scale;
    component<ProjectileModification>(bullet,15).scale_y=scale;
    component<ProjectileModification>(bullet,15).x=scale*20.0F+-20.0F;
    component<ProjectileModification>(bullet,15).y=scale*40.0F+-40.0F;
    component<ProjectileLink>(bullet,27).max_height=weapon->max_height;
    std::int32_t clip=28;
    if((weapon->flags&8)==0){const auto ids=host.shoot_attack_animations();clip=required(ids.get()).at(static_cast<std::size_t>(weapon->type));}
    cut_in_animation(live_animation(e),clip,36);
    if((weapon->flags&8)!=0){auto* w=host.shoot_effect_world();const auto p=point(e);auto& effects=required(w);
        auto& effect=create_visual_effect(p,0,29,false,effects.ids,effects.factory);
        auto& color=required(world).acquire_bullet_color();color.values={{255,255,255,3192}};effect.add(6,&color);
        component<EntityDepth>(effect,4).extra=-10;
    }
    host.play_attack_sound(spell->attribute==1||spell->attribute==2);return true;
}
}
