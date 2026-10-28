#include "member_selection.hpp"
#include "contact_collision.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
ComponentSlots* world_player_entity(PlayerWorldState& state){
    if(!state.cached_player){
        if(!state.groups)throw std::invalid_argument("missing player entity manager");
        auto& group=state.groups->get(std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{30}));
        state.cached_player=group.first_entity();
    }
    return state.cached_player;
}
ComponentSlots* member_hero(PlayerWorldState* world){
    if(!world)throw std::invalid_argument("missing member world");
    return world_player_entity(*world);
}
namespace {
ContactPosition& position(ComponentSlots* entity){
    if(!entity)throw std::invalid_argument("null party member");
    auto* value=dynamic_cast<ContactPosition*>(entity->get(0));
    if(!value)throw std::invalid_argument("missing member position");
    return *value;
}
}
ComponentSlots* front_line_human(const std::vector<ComponentSlots*>* members){
    if(!members)throw std::invalid_argument("missing member list");
    float smallest=std::numeric_limits<float>::max();
    ComponentSlots* result=nullptr;
    for(auto* entity:*members){
        if(position(entity).z<smallest){
            smallest=position(entity).z;
            result=entity;
        }
    }
    return result;
}
} // namespace saga::core
