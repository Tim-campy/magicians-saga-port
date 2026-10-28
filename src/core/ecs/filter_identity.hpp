#pragma once
#include "entity_subset.hpp"

namespace saga::core {
// Filter's concrete-type projection. Lists remain borrowed and mutable.
class FilterIdentity final {
public:
    SubsetFilter lists;
    // Diagnostic is required only for a list with at least 33 entries.
    std::int32_t hash(const std::function<void()>& diagnostic);
    bool equals(FilterIdentity* other,const std::function<void()>& diagnostic);
private:
    std::int32_t cached_=-1; // Filter +0x28; builders/setters do not invalidate it.
};
// Filter.GetComponentTypesArrayHashCode 0x113a774.
std::int32_t component_types_hash(const std::vector<std::int32_t>*,const std::function<void()>& diagnostic);
}
