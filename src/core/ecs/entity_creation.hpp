#pragma once
#include "component_slots.hpp"

namespace saga::core {
struct EntityIdState {std::int32_t counter=0;}; // EntityManager +0x14; restored by Deserialize.
class EntityCreationHost {
public:
    virtual ~EntityCreationHost()=default;
    virtual bool registered_id(std::int32_t)=0;
    // EntityManager.Create virtual slot 4. Host owns the returned entity.
    virtual ComponentSlots* create(std::int32_t type,std::int32_t id)=0;
    // AddEntity: skip registry insertion when temporary, but retain subscriptions.
    virtual void register_entity(ComponentSlots&,bool temporary)=0;
};
// EntityManager.CreateEntity 0x11375e4. Allocation is committed before factory/registration.
ComponentSlots& create_entity(EntityIdState&,std::int32_t type,bool temporary,EntityCreationHost&);
}
