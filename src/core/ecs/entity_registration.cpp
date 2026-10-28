#include "entity_registration.hpp"
#include <stdexcept>

namespace saga::core {
void ComponentEvents::append(Channel& channel,ComponentObserver& observer){
    auto next=std::make_shared<List>();if(channel)*next=*channel;
    next->push_back(&observer);channel=std::move(next);
}
void ComponentEvents::subscribe_added(ComponentObserver& o){append(added_,o);}
void ComponentEvents::subscribe_removed(ComponentObserver& o){append(removed_,o);}
void ComponentEvents::subscribe_changed(ComponentObserver& o){append(changed_,o);}
void ComponentEvents::added(ComponentSlots& e,std::int32_t i,Component* c){
    const auto captured=added_;if(captured)for(auto* o:*captured)o->added(e,i,c);
}
void ComponentEvents::removed(ComponentSlots& e,std::int32_t i,Component* c){
    const auto captured=removed_;if(captured)for(auto* o:*captured)o->removed(e,i,c);
}
void ComponentEvents::changed(ComponentSlots& e,std::int32_t i,Component* old,Component* c){
    const auto captured=changed_;if(captured)for(auto* o:*captured)o->changed(e,i,old,c);
}
void ComponentEvents::dispose(){added_.reset();removed_.reset();changed_.reset();}
void EntityRegistry::add(EntityRegistration* entity,bool temporary){
    if(!entity||!entity->components)throw std::invalid_argument("missing registration entity");
    if(!temporary){
        ++version_; // Native TryInsert increments before checking duplicate keys.
        if(entries_.find(entity->id)!=entries_.end())throw std::invalid_argument("duplicate registered entity id");
        const auto index=free_==no_slot?slots_.size():free_;
        const bool appended=free_==no_slot;
        if(appended)slots_.push_back({nullptr,no_slot});
        try{entries_.emplace(entity->id,index);}
        catch(...){if(appended)slots_.pop_back();throw;}
        if(appended)slots_[index].entity=entity->components;
        else{free_=slots_[index].next_free;slots_[index]={entity->components,no_slot};}
    }
    if(!entity->events)throw std::invalid_argument("missing component subscription service");
    entity->events->subscribe_added(observer_);
    entity->events->subscribe_removed(observer_);
    entity->events->subscribe_changed(observer_);
}
ComponentSlots* EntityRegistry::find(std::int32_t id)const{
    const auto entry=entries_.find(id);return entry==entries_.end()?nullptr:slots_[entry->second].entity;
}
void EntityRegistry::erase(std::int32_t id){
    const auto entry=entries_.find(id);if(entry==entries_.end())return;
    const auto index=entry->second;entries_.erase(entry);
    slots_[index]={nullptr,free_};free_=index;++version_;
}
void EntityRegistry::clear(){entries_.clear();slots_.clear();free_=no_slot;++version_;}
std::optional<ComponentSlots*> EntityRegistry::Cursor::next(){
    if(version_!=owner_.version_)throw std::runtime_error("entity registry changed during enumeration");
    while(index_<owner_.slots_.size())if(auto* entity=owner_.slots_[index_++].entity)return entity;
    return std::nullopt;
}
}
