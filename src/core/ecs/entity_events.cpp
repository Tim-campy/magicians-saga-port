#include "entity_events.hpp"
#include <stdexcept>
#include <utility>

namespace saga::core {
template<class F>void EntityEvents::append(Channels<F>& channels,std::int32_t id,F callback){
    if(!callback)throw std::invalid_argument("missing entity event callback");
    auto next=std::make_shared<List<F>>();
    const auto found=channels.find(id);
    if(found!=channels.end())*next=*found->second;
    next->push_back(std::make_shared<F>(std::move(callback)));
    channels[id]=std::move(next);
}
void EntityEvents::subscribe(std::int32_t id,Single callback){append(single_,id,std::move(callback));}
void EntityEvents::subscribe(std::int32_t id,Pair callback){append(pair_,id,std::move(callback));}
void EntityEvents::send(std::int32_t id,ComponentSlots* entity)const{
    const auto found=single_.find(id);if(found==single_.end())return;
    const auto captured=found->second;
    for(const auto& callback:*captured)(*callback)(entity);
}
void EntityEvents::send(std::int32_t id,ComponentSlots* first,ComponentSlots* second)const{
    const auto found=pair_.find(id);if(found==pair_.end())return;
    const auto captured=found->second;
    for(const auto& callback:*captured)(*callback)(first,second);
}
void bind_combat_receivers(EntityEvents& events,const CombatReceivers& r){
    if(!r.stage_dead||!r.human_impact||!r.enemy_impact||!r.enemy_dead||!r.projectile_impact||!r.human_contact||!r.enemy_contact)
        throw std::invalid_argument("incomplete combat receiver bindings");
    events.subscribe(30,r.stage_dead);
    events.subscribe(23,r.human_impact);events.subscribe(29,r.human_contact);
    events.subscribe(23,r.enemy_impact);events.subscribe(29,r.enemy_contact);events.subscribe(30,r.enemy_dead);
    events.subscribe(23,r.projectile_impact);
}
}
