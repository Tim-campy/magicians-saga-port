#include "movement.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& component(ComponentSlots& entity,std::int32_t slot){
    auto* p=dynamic_cast<T*>(entity.get(slot));
    if(!p)throw std::invalid_argument("missing movement component");
    return *p;
}
ContactPosition& parent_position(ContactPosition& p){
    if(!p.parent)throw std::invalid_argument("missing position parent");
    return component<ContactPosition>(*p.parent,contact_position_slot);
}
}
void move_entity(ComponentSlots& entity){
    auto& p=component<ContactPosition>(entity,contact_position_slot);
    const auto& v=component<ProjectileSpeed>(entity,projectile_speed_slot);
    p.x=p.x+v.x;p.y=p.y+v.y;p.z=p.z+v.z;
    if(p.parent&&p.parent->contains(contact_position_slot)){
        p.x=parent_position(p).x+p.offset_x;
        p.y=parent_position(p).y+p.offset_y;
        p.z=parent_position(p).z+p.offset_z;
    }
}
}
