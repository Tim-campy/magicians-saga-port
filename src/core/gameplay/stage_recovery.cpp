#include "stage_recovery.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing stage recovery input");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return required(dynamic_cast<T*>(e.get(slot)));}
ContactPosition* position(ComponentSlots& e){return dynamic_cast<ContactPosition*>(e.get(0));}
HumanRecordLink& human(ComponentSlots& e){return component<HumanRecordLink>(e,14);}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
float formation(ComponentSlots* input,MemberFormationHost& host,bool z){
    auto& first=required(member_hero(host.recovery_world()));
    auto& first_position=required(position(first));const float origin=z?first_position.z:first_position.x;
    auto& second=required(member_hero(host.recovery_world()));auto& hero_human=human(second);
    required(input);const auto hero_offset=z?hero_human.formation_z:hero_human.formation_x;
    auto& member=human(*input);const auto member_offset=z?member.formation_z:member.formation_x;
    const float relative=origin-static_cast<float>(hero_offset);
    return relative+static_cast<float>(member_offset);
}
}
bool stage_human_dead(const HumanRecordLink& h){return (static_cast<std::uint32_t>(h.state)&0xfffffffeU)==4U;}
std::int32_t initial_human_invincible_time(std::int32_t fps){return word(static_cast<std::uint32_t>(fps)*3U);}
bool is_retry_removable_effect(ComponentSlots* e){return component<EntitySprite>(required(e),2).animation.clip==100;}
float member_formation_x(ComponentSlots* e,MemberFormationHost& host){return formation(e,host,false);}
float member_formation_z(ComponentSlots* e,MemberFormationHost& host){return formation(e,host,true);}
namespace {
float indexed_formation(std::int32_t index,MemberFormationHost& host,bool z){
    auto& first=required(member_hero(host.recovery_world()));
    const auto origin=z?required(position(first)).z:required(position(first)).x;
    auto& second=required(member_hero(host.recovery_world()));auto& hero_human=human(second);
    const auto offset=z?hero_human.formation_z:hero_human.formation_x;
    const auto grid=z?index/4:index%4;
    const auto distance=word(static_cast<std::uint32_t>(grid)*36U);
    const float relative=origin-static_cast<float>(offset);
    return relative+static_cast<float>(distance);
}
}
float indexed_member_formation_x(std::int32_t i,MemberFormationHost& host){return indexed_formation(i,host,false);}
float indexed_member_formation_z(std::int32_t i,MemberFormationHost& host){return indexed_formation(i,host,true);}
void remove_recovery_modification(ComponentSlots& e,StageRecoveryHost& host){
    auto* previous=e.get(15);e.remove(15);host.release_recovery_modification(previous);
}
void retry_stage(StageStartState& s,StageRecoveryHost& host){
    s.retried=true;reset_stage_timer(s,host.recovery_start_host());
    auto& group=required(host.recovery_effect_subset());
    std::vector<ComponentSlots*> selected;
    {auto cursor=group.cursor();while(const auto e=cursor.next())if(is_retry_removable_effect(*e))selected.push_back(*e);}
    auto& destroy=required(host.recovery_destruction_world());
    for(auto* e:selected)destroy_quest_entity(host.recovery_entity_id(*e),true,destroy);
    required(s.members);auto& hero=required(member_hero(host.recovery_world()));
    auto* destination=position(hero);
    required(s.members);const auto left=stage_formation_left(host.recovery_start_host());
    const auto index=human(hero).formation_index;
    required(destination).x=static_cast<float>(add(left,word(static_cast<std::uint32_t>(index%4)*36U)));
    required(position(hero)).y=0;
    const auto list=required(s.members).members;required(list.get());const auto version=list->version();
    for(std::size_t i=0;;++i){
        if(list->version()!=version)throw std::runtime_error("party changed during retry");
        if(i>=list->entities().size())break;
        auto& e=required(list->entities()[i]);
        if(stage_human_dead(human(e))){human(e).state=0;host.recovery_change_animation(e,36,true);}
        fill_parameter(required(resolve_combat_parameter(e,0)));
        fill_parameter(required(resolve_combat_parameter(e,1)));
        if(character_is_sub(required(host.recovery_character(human(e))))){
            destination=position(e);required(s.members);const auto x=member_formation_x(&e,host);required(destination).x=x;
            required(position(e)).y=0;
            destination=position(e);required(s.members);const auto z=member_formation_z(&e,host);required(destination).z=z;
        }
        auto* effect_world=host.recovery_effect_world();
        const auto x=required(position(e)).x,y=required(position(e)).y,z=required(position(e)).z;
        auto& effects=required(effect_world);create_visual_effect({x,y+10.0F,z},102,-1,false,effects.ids,effects.factory);
        auto* captured=dynamic_cast<HumanRecordLink*>(e.get(14));const auto duration=host.invincible_time();
        required(captured).invincible_time=duration;
        if(e.contains(15))remove_recovery_modification(e,host);
        AnimationModificationRequest request;request.type=3;request.max_frame=host.invincible_time();
        add_animation_modification(e,request,host);
    }
    host.recovery_start_host().play_stage_bgm();
}
void start_stage_boss_battle(StageStartState& s,StageRecoveryHost& host){
    if(s.boss_started)return;
    s.boss_started=true;
    auto& boss=required(host.create_stage_boss(required(s.enemies)));
    AnimationModificationRequest request;request.max_frame=30;request.scale_y=0;
    add_animation_modification(boss,request,host);
    s.min_z_rate=75;
    auto* binding=host.stage_object_world();const auto z=stage_z(s,75,host.recovery_start_host());
    auto& world=required(binding);
    auto& fence=create_z_object({140,0,static_cast<float>(add(z,138))},7,44,world.ids,world.factory);
    request.max_frame=20;add_animation_modification(fence,request,host);s.boss_fence=&fence;
    host.play_boss_bgm();host.play_boss_sound();
}
} // namespace saga::core
