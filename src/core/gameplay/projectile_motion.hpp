#pragma once
#include "projectile_launch.hpp"

namespace saga::core {
// Shared float Graph.Easing 0x1c5e15c, also used by drop collection.
float easing_float(float start,float end,float duration,float progress,float amplitude);
class ProjectileMotionHost {
public:
    virtual ~ProjectileMotionHost()=default;
    virtual void return_component(std::int32_t slot,Component*)=0;
    virtual void send_impact(ComponentSlots&)=0; // World.Send<Entity>(23), before Projectile removal.
};
// Per-entity normal path of ProjectileSystem.Update 0x1160018.
// Writes y and velocities; horizontal position integration is a separate system.
// True means append to the system's completion queue, preserving group iteration order.
bool advance_projectile(ComponentSlots&,ProjectileMotionHost&);
// Run after the complete group pass. After event 23, the generated remover's
// null-getter guard permits callbacks that already removed/destroyed the entity.
void finish_projectile(ComponentSlots&,ProjectileMotionHost&);
}
