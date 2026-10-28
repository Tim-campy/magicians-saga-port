#include "stage_placement.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing stage placement data");return *p;}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t multiply(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
ComponentSlots* spawn(StagePlacementHost& host,std::int32_t type,std::int32_t value,ProjectilePoint p){
    auto& world=required(host.drop_world());return create_drop(type,value,p,25,world.treasures,world.ids,world.factory);
}
}
std::int32_t stage_quest_height(const StageQuest& q){return add(q.distance,2);}
std::int32_t remaining_stage_prisoners(const StageQuest& q){return std::max(word(static_cast<std::uint32_t>(q.prisoner_maximum)-static_cast<std::uint32_t>(q.rescued_prisoners)),0);}
std::int32_t stage_z(StagePlacementState& state,std::int32_t rate,StagePlacementHost& host){
    const auto height=stage_quest_height(required(state.quest.get()));
    const auto step_z=required(host.stage_map()).step_z;
    const auto second_step=required(host.stage_map()).step_z;
    const auto base=multiply(step_z,add(height,-3));
    // 0x11574d8..0x11574fc uses signed magic 0xae147ae1: divide by -100.
    return add(add(base,-(multiply(base,rate)/100)),second_step/2);
}
std::int32_t stage_formation_left(StagePlacementHost& host){return add(required(host.stage_map()).step_x,-72);}
std::int32_t stage_formation_top(StagePlacementState& state,StagePlacementHost& host){return stage_z(state,0,host);}
void place_stage_money(StagePlacementState& state,RandomSource& random,StagePlacementHost& host){
    auto* map=host.stage_map();
    for(std::int32_t rate=10;rate<=90;++rate){
        const auto chance=required(state.quest.get()).money_rate;
        if(!random_hits(random,chance))continue;
        const auto center=required(map).step_x;
        const auto offset=random_inclusive(random,-2,2);
        const auto z=stage_z(state,rate,host);
        const auto value=random_hits(random,50)?1:15;
        auto& entity=required(spawn(host,1,value,{static_cast<float>(add(center,multiply(offset,24))),0,static_cast<float>(z)}));
        auto& depth=required(dynamic_cast<EntityDepth*>(entity.get(4)));depth.extra=-10;
    }
}
void place_stage_treasures(StagePlacementState& state,RandomSource& random,StagePlacementHost& host){
    auto* map=host.stage_map();
    const auto chance=required(state.quest.get()).completed==0?100:10;
    const auto groups=required(state.quest.get()).fixed_treasure;
    const auto entries=required(groups.get()).at(0);
    required(entries.get());
    for(std::size_t index=0;index<entries->size();++index){
        const auto row=entries->at(index); // Capture before the probability callback.
        if(!random_hits(random,chance))continue;
        const auto treasure=required(row.get()).at(0),rate=row->at(1);
        const auto center=required(map).step_x;
        const auto offset=random_inclusive(random,-2,2);
        const auto z=stage_z(state,rate,host);
        spawn(host,3,treasure,{static_cast<float>(add(center,multiply(offset,24))),0,static_cast<float>(z)});
    }
    const auto first_clear=required(state.quest.get()).completed==0;
    const auto& q=required(state.quest.get());
    const auto treasure=first_clear?q.reward_treasure:q.cleared_reward_treasure;
    const auto center=required(map).step_x;
    const auto z=stage_z(state,100,host);
    spawn(host,3,treasure,{static_cast<float>(center),0,static_cast<float>(z)});
}
void place_stage_prisoner(StagePlacementState& state,std::int32_t rate,StagePlacementHost& host){
    const auto center=required(host.stage_map()).step_x;
    const auto z=stage_z(state,rate,host);
    spawn(host,2,0,{static_cast<float>(add(center,-72)),0,static_cast<float>(z)});
}
void place_stage_prisoners(StagePlacementState& state,RandomSource& random,StagePlacementHost& host){
    (void)host.stage_map(); // Original requires the world but discards its map result.
    const auto count=std::min(remaining_stage_prisoners(required(state.quest.get())),3);
    std::int32_t successes=0;
    for(std::int32_t i=0;i<count;++i){
        if(random_hits(random,50)){place_stage_prisoner(state,10+i*20,host);++successes;}
    }
    if(count>0&&successes==0)place_stage_prisoner(state,add(multiply(random_bounded(random,3),20),10),host);
}
void place_stage_drops(StagePlacementState& state,RandomSource& random,StagePlacementHost& host){
    place_stage_money(state,random,host);place_stage_treasures(state,random,host);place_stage_prisoners(state,random,host);
}
} // namespace saga::core
