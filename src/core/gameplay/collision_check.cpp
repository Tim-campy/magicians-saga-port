#include "collision_check.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& require(T* p){if(!p)throw std::invalid_argument("missing collision-check input");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return require(dynamic_cast<T*>(e.get(slot)));}
ProjectileCollision& collision(ComponentSlots& e){return component<ProjectileCollision>(e,projectile_collision_slot);}
ContactPosition& position(ComponentSlots& e){return component<ContactPosition>(e,contact_position_slot);}
}
bool check_collision(ComponentSlots* first,ComponentSlots* second){
    auto& a=require(first);if(!a.contains(projectile_collision_slot))return false;
    auto& b=require(second);if(!b.contains(projectile_collision_slot))return false;
    const auto radius_a=collision(a).radius,radius_b=collision(b).radius;
    const auto ax=position(a).x;const auto ox=collision(a).offset_x;
    const auto ay=position(a).y;const auto oy=collision(a).offset_y;
    const auto az=position(a).z;const auto oz=collision(a).offset_z;
    const auto bx=position(b).x;const auto px=collision(b).offset_x;
    const auto by=position(b).y;const auto py=collision(b).offset_y;
    const auto bz=position(b).z;const auto pz=collision(b).offset_z;
    const float acx=ax+static_cast<float>(ox),acy=ay+static_cast<float>(oy),acz=az+static_cast<float>(oz);
    const float bcx=bx+static_cast<float>(px),bcy=by+static_cast<float>(py),bcz=bz+static_cast<float>(pz);
    const float dx=acx-bcx,dy=acy-bcy,dz=acz-bcz;
    const float xx=dx*dx,yy=dy*dy,zz=dz*dz;
    const float xy=xx+yy,distance=xy+zz;
    const auto radii=static_cast<std::uint32_t>(radius_a)+static_cast<std::uint32_t>(radius_b);
    const auto square=radii*radii;
    const auto limit=static_cast<std::int32_t>(square<=0x7fffffffU?static_cast<std::int64_t>(square):static_cast<std::int64_t>(square)-0x100000000LL);
    return distance<=static_cast<float>(limit);
}
}
