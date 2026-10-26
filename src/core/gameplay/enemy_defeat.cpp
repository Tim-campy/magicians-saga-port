#include "enemy_defeat.hpp"
#include "combat_rules.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& require(T* p){if(!p)throw std::invalid_argument("missing enemy defeat input");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return require(dynamic_cast<T*>(e.get(slot)));}
std::int32_t bits(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
ContactPosition& position(ComponentSlots& e){return component<ContactPosition>(e,contact_position_slot);}
class Rewards final:public EnemyRewardHost {
    ComponentSlots& entity_;const CombatRecordTables& tables_;RandomSource& random_;EnemyDefeatHost& host_;
    const EnemyRecord* initial_;
    void drop(std::int32_t type,std::int32_t value,ProjectilePoint p,std::int32_t scale){
        auto& e=require(host_.create_drop(type,value,p,scale));
        component<DropDepth>(e,drop_depth_slot).extra=-10;
    }
public:
    Rewards(ComponentSlots& e,const CombatRecordTables& tables,RandomSource& random,EnemyDefeatHost& host,const EnemyRecord& initial)
        :entity_(e),tables_(tables),random_(random),host_(host),initial_(&initial){}
    const EnemyRecord& enemy_data()override{
        if(initial_){const auto* p=initial_;initial_=nullptr;return *p;}
        return require(resolve_combat_enemy(entity_,tables_));
    }
    bool hits1000(std::int32_t threshold)override{return random_hits1000(random_,threshold);}
    void spawn_experience_drop(std::int32_t value)override{
        const auto scale=easing(25,50,500,value,-50);
        const auto x=position(entity_).x,y=position(entity_).y,z=position(entity_).z;
        drop(0,value,{x-5.0F,y,z-10.0F},scale);
    }
    void spawn_treasure_drop(std::int32_t id)override{
        const auto x=position(entity_).x,z=position(entity_).z;
        drop(3,id,{x,0.0F,z},25); // World.CreateDrop later replaces treasure scale with 100.
    }
};
}
void add_enemy_defeat_count(std::int32_t& count,std::int32_t increment){
    count=std::max(0,bits(static_cast<std::uint32_t>(count)+static_cast<std::uint32_t>(increment)));
}
void defeat_enemy(ComponentSlots& entity,const CombatRecordTables& tables,EnemyDefeatCounters& counters,RandomSource& random,EnemyDefeatHost& host){
    auto& speed=component<ProjectileSpeed>(entity,projectile_speed_slot);speed.x=0;speed.y=0;speed.z=0;
    fly_away(entity,-100,100,-100,-50,true,random,host);
    change_enemy_state(entity,4);
    component<DefeatColor>(entity,defeat_color_slot).values={{255,0,255,0}};
    auto& count=host.saved_defeat_count(require(resolve_combat_enemy(entity,tables)));
    add_enemy_defeat_count(count,1);
    if((require(resolve_combat_enemy(entity,tables)).flags&1)==0)
        counters.defeated_enemies=bits(static_cast<std::uint32_t>(counters.defeated_enemies)+1U);
    const auto floor=(component<EnemyRecordLink>(entity,combat_enemy_slot).flags&1)!=0?0:1;
    counters.remaining_enemies=std::max(bits(static_cast<std::uint32_t>(counters.remaining_enemies)-1U),floor);
    const auto& initial=require(resolve_combat_enemy(entity,tables));
    const auto level=component<EnemyRecordLink>(entity,combat_enemy_slot).level;
    Rewards rewards(entity,tables,random,host,initial);
    spawn_enemy_rewards(level,rewards);
}
}
