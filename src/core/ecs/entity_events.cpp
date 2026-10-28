#include "entity_events.hpp"
#include <stdexcept>
#include <utility>
#include <algorithm>

namespace saga::core {
template<class F>void EntityEvents::append(Channels<F>& channels,Tracking<F>& tracking,std::int32_t id,F callback,std::optional<EventHashes> hashes){
    if(!callback)throw std::invalid_argument("missing entity event callback");
    auto next=std::make_shared<List<F>>();
    const auto found=channels.find(id);
    if(found!=channels.end())*next=*found->second;
    auto wrapper=std::make_shared<F>(std::move(callback));
    next->push_back(wrapper);
    channels[id]=std::move(next);
    tracking.records.push_back({id,hashes,std::move(wrapper)});++tracking.version;
}
void EntityEvents::subscribe(std::int32_t id,Single callback,std::optional<EventHashes> hashes){append(single_,single_tracking_,id,std::move(callback),hashes);}
void EntityEvents::subscribe(std::int32_t id,Pair callback,std::optional<EventHashes> hashes){append(pair_,pair_tracking_,id,std::move(callback),hashes);}
template<class F>bool EntityEvents::remove(Channels<F>& channels,Tracking<F>& tracking,std::int32_t id,std::int32_t hash){
    const auto channel=channels.find(id);if(channel==channels.end())return false;
    for(auto at=tracking.records.begin();at!=tracking.records.end();++at){
        if(at->event!=id)continue;
        if(!at->hashes)throw std::logic_error("event delegate hashes have not been bound");
        if(at->hashes->original!=hash)continue;
        auto next=std::make_shared<List<F>>(*channel->second);
        const auto wrapper=std::find(next->begin(),next->end(),at->wrapper);
        if(wrapper!=next->end())next->erase(wrapper); // Each Add creates a unique wrapper.
        channel->second=std::move(next); // Preserve the dictionary key even when empty.
        tracking.records.erase(at);++tracking.version;
        if(removal_log)removal_log(id);
        return true;
    }
    return false;
}
bool EntityEvents::unsubscribe_single(std::int32_t id,std::int32_t hash){return remove(single_,single_tracking_,id,hash);}
bool EntityEvents::unsubscribe_pair(std::int32_t id,std::int32_t hash){return remove(pair_,pair_tracking_,id,hash);}
template<class F>void EntityEvents::remove_tracking(Channels<F>& channels,Tracking<F>& tracking){
    const auto version=tracking.version;
    for(std::size_t index=0;;++index){
        if(version!=tracking.version)throw std::runtime_error("event tracking list changed during cleanup");
        if(index>=tracking.records.size())break;
        const auto record=tracking.records[index];
        if(!record.hashes)throw std::logic_error("event wrapper hash has not been bound");
        remove(channels,tracking,record.event,record.hashes->wrapper);
    }
    tracking.records.clear();++tracking.version;
}
void EntityEvents::remove_all(){
    remove_tracking(single_,single_tracking_);
    remove_tracking(pair_,pair_tracking_);
    single_.clear();pair_.clear();
}
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
    const auto hash=[&](std::size_t i)->std::optional<EventHashes>{return r.hashes?std::optional<EventHashes>((*r.hashes)[i]):std::nullopt;};
    events.subscribe(30,r.stage_dead,hash(0));
    events.subscribe(23,r.human_impact,hash(1));events.subscribe(29,r.human_contact,hash(2));
    events.subscribe(23,r.enemy_impact,hash(3));events.subscribe(29,r.enemy_contact,hash(4));events.subscribe(30,r.enemy_dead,hash(5));
    events.subscribe(23,r.projectile_impact,hash(6));
}
}
