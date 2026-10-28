#include "enemy_creation.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing enemy creation input");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return required(dynamic_cast<T*>(e.get(slot)));}
EnemyRecordLink& enemy(ComponentSlots& e){return component<EnemyRecordLink>(e,24);}
ContactPosition& position(ComponentSlots& e){return component<ContactPosition>(e,0);}
bool alive(ComponentSlots& e){return (static_cast<std::uint32_t>(component<HumanRecordLink>(e,14).state)&0xfffffffeU)!=4U;}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
}
ComponentSlots* search_enemy_target(EnemyCreationSystem& s,ComponentSlots* input,EnemyCreationHost& host){
    auto& e=required(input);auto* previous=enemy(e).opponent;
    if(previous&&!host.target_destroyed(*previous)){
        auto& candidate=required(enemy(e).opponent);
        if(alive(candidate))return enemy(e).opponent;
    }
    const bool main_target=(enemy(e).flags&1)!=0;
    auto& members=required(s.members);
    if(main_target)return member_hero(host.enemy_world());
    const auto list=members.members;required(list.get());const auto version=list->version();
    ComponentSlots* result=nullptr;float nearest=std::numeric_limits<float>::max();
    for(std::size_t i=0;;++i){
        if(version!=list->version())throw std::runtime_error("party changed during enemy target search");
        if(i>=list->entities().size())break;
        auto& member=required(list->entities()[i]);if(!alive(member))continue;
        const auto mx=position(member).x,ex=position(e).x,mz=position(member).z,ez=position(e).z;
        const float dx=mx-ex,dz=mz-ez;const float xx=dx*dx,zz=dz*dz,distance=xx+zz;
        if(distance<nearest){result=&member;nearest=distance;}
    }
    return result;
}
ComponentSlots& create_system_enemy(EnemyCreationSystem& s,EnemyCreation args,EnemyCreationHost& host){
    auto& world=required(host.current_enemy_factory());
    auto& e=required(create_enemy(args,world.ids,world.factory));
    auto* captured=dynamic_cast<EnemyRecordLink*>(e.get(24));
    auto* target=search_enemy_target(s,&e,host);required(captured).opponent=target;
    if(host.system_yugami()!=10000){auto& value=enemy(e);value.level=word(static_cast<std::uint32_t>(value.level)*50U);}
    return e;
}
ComponentSlots& create_boss_enemy(EnemyCreationSystem& s,EnemyCreationHost& host){
    const auto id=required(s.quest.get()).boss_id;
    world_player_entity(required(host.enemy_world())); // Cache side effect; null result is allowed here.
    const auto x=required(host.enemy_map()).step_x;
    const auto z=stage_z(required(s.stage),98,host.enemy_coordinates());
    const auto level=required(s.quest.get()).boss_level;
    auto& boss=create_system_enemy(s,{2,id,level,1,{static_cast<float>(x),0,static_cast<float>(z)}},host);
    required(s.members);auto& hero=required(member_hero(host.enemy_world()));const auto hero_x=position(hero).x;
    const auto extra_z=stage_z(required(s.stage),96,host.enemy_coordinates());
    auto& extra=create_system_enemy(s,{-1,1,1,0,{hero_x,0,static_cast<float>(extra_z)}},host);
    AnimationModificationRequest request;request.type=9;request.angle=30;request.loop=true;
    add_animation_modification(extra,request,host.enemy_modifications());
    return boss;
}
} // namespace saga::core
