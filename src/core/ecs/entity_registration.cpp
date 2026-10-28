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
    if(!temporary&&!entries_.emplace(entity->id,entity->components).second)
        throw std::invalid_argument("duplicate registered entity id");
    if(!entity->events)throw std::invalid_argument("missing component subscription service");
    entity->events->subscribe_added(observer_);
    entity->events->subscribe_removed(observer_);
    entity->events->subscribe_changed(observer_);
}
ComponentSlots* EntityRegistry::find(std::int32_t id)const{
    const auto entry=entries_.find(id);return entry==entries_.end()?nullptr:entry->second;
}
}
