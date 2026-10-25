#pragma once
#include "event_flow.hpp"

namespace saga::core {
struct DiscoveryState {
    std::int32_t state;
    bool is_new;
};
struct EventProgression {
    std::int64_t money;
    std::int64_t crystals;
    // Decoded SecureInt selected by AppData.SYSSAVEI_YUGAMI. No assumed default.
    std::int32_t crystal_subtraction_mode;
    std::vector<std::shared_ptr<DiscoveryState>> areas;
    std::vector<std::shared_ptr<DiscoveryState>> quests;
};

// Unit-dispatch preprocessing, AppData.ExecuteScript34379 VA0x110cc4c:
// clone the row, substitute INT_MIN..INT_MIN+99 in arguments (not opcode).
EventIntegers resolve_event_arguments(const EventIntegers& row,
                                     const std::shared_ptr<EventIntegers>& arguments);

// Effects for opcodes1/2/4/15 on decoded state, after argument substitution.
// True means handled; false explicitly leaves remaining opcodes to the host.
// This is not SecureProperty serialization, UI notification or a full dispatcher.
bool apply_event_progression(const EventIntegers& row, EventProgression& state);
}
