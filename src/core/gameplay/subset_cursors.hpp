#pragma once
#include "../ecs/entity_subset.hpp"
#include "collision_pass.hpp"
#include "garbage_lifetime.hpp"

namespace saga::core {
std::unique_ptr<CollisionCursor> open_collision_subset(EntitySubset&);
std::unique_ptr<GarbageCursor> open_garbage_subset(EntitySubset&,
    std::function<QuestMemberId(const ComponentSlots&)> identity);
}
