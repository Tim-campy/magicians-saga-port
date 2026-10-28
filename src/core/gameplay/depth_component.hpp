#pragma once
#include "../ecs/component_slots.hpp"
namespace saga::core {
struct EntityDepth final:Component {std::int32_t value=0,extra=0;}; // Depth +0x10/+0x14.
}
