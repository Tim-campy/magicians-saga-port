#include "world_teardown.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
EntityRegistry& require(EntityRegistry* registry){
    if(!registry)throw std::invalid_argument("missing entity destruction registry");
    return *registry;
}
}
void destroy_all_entities(RegistryDestruction& state,RegisteredEntityDestruction& host){
    {
        auto cursor=require(state.registry).values();
        while(auto entity=cursor.next())if(*entity)host.destroy(**entity,false);
    }
    require(state.registry).clear();
}
void destroy_world(WorldTeardownHost& host){
    host.clear_visible_entities();
    host.destroy_registered_entities();
    host.remove_systems();
    host.remove_event_handlers();
}
}
