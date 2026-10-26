#include "component_filter.hpp"

namespace saga::core {
// Filter.Matches37161, ELF 0x113a4dc; docs/archive/reports/39-generic-collections.md.
bool matches_component_filter(
    const ComponentSlots* entity,
    const std::vector<std::int32_t>* all,
    const std::vector<std::int32_t>* any,
    const std::vector<std::int32_t>* none) {
    if (!entity) return false;
    if (all && !all->empty() && !entity->contains_all(*all)) return false;
    if (any && !any->empty() && !entity->contains_any(*any)) return false;
    if (none && !none->empty() && entity->contains_any(*none)) return false;
    return true;
}
}
