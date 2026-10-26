#pragma once
#include "../ecs/entity_subset.hpp"
#include "projectile_motion.hpp"

namespace saga::core {
// MoveSystem.Update 0x115faac over its live And({0,1}) group.
void move_subset(EntitySubset&);
// ProjectileSystem.Update 0x1160018 over its live And({27,0,1}) group.
// The scheduler owns enabled gating and invokes movement after completion events.
class ProjectilePass {
public:
    void update(EntitySubset&,ProjectileMotionHost&);
private:
    // Native system +0x28: retained after failure, cleared only on successful delivery.
    std::vector<ComponentSlots*> completed_;
    std::uint32_t version_=0;
};
}
