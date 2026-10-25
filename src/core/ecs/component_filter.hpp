#pragma once
#include "component_slots.hpp"

namespace saga::core {
// Predicate only: no Filter hash/identity, builder, cache or subset membership.
// Null and empty criteria are skipped. Borrowed lists are read on each call.
bool matches_component_filter(
    const ComponentSlots* entity,
    const std::vector<std::int32_t>* all,
    const std::vector<std::int32_t>* any,
    const std::vector<std::int32_t>* none);
}
