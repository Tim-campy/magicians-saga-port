#include "entity_creation.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
ComponentSlots& create_entity(EntityIdState& state,std::int32_t type,bool temporary,EntityCreationHost& host){
    constexpr std::int32_t limit=0x7ffffffd;
    auto id=state.counter;
    const auto bits=static_cast<std::uint32_t>(id)+1U;
    const auto next=static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);
    if(next<limit)state.counter=next;
    else{
        state.counter=limit;
        for(std::int32_t candidate=0;candidate<std::numeric_limits<std::int32_t>::max();++candidate){
            if(!host.registered_id(candidate)){id=candidate;break;}
        }
        // If every tested ID is occupied, native code retains the previous counter as id.
    }
    auto* entity=host.create(type,temporary?-1:id);
    if(!entity)throw std::invalid_argument("entity factory returned null");
    host.register_entity(*entity,temporary);
    return *entity;
}
}
