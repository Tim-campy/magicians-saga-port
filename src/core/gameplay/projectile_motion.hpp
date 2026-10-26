#pragma once
#include "projectile_launch.hpp"

namespace saga::core {
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
// Run after the complete group pass, for each queued entity. No hasProjectile guard.
void finish_projectile(ComponentSlots&,ProjectileMotionHost&);
}
