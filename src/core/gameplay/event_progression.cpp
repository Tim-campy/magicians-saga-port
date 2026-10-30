#include "event_progression.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t signed32(std::uint32_t bits) {
    return bits <= 0x7fffffffU ? static_cast<std::int32_t>(bits)
        : -1 - static_cast<std::int32_t>(0xffffffffU - bits);
}
std::int64_t signed64(std::uint64_t bits) {
    return bits <= 0x7fffffffffffffffULL ? static_cast<std::int64_t>(bits)
        : -1 - static_cast<std::int64_t>(0xffffffffffffffffULL - bits);
}
std::int64_t add(std::int64_t value, std::int32_t amount) {
    return signed64(static_cast<std::uint64_t>(value) + static_cast<std::uint64_t>(amount));
}
std::int64_t subtract(std::int64_t value, std::int32_t amount) {
    const auto candidate = signed64(static_cast<std::uint64_t>(value) - static_cast<std::uint64_t>(amount));
    return candidate < 0 ? 0 : candidate;
}
DiscoveryState& entry(std::vector<std::shared_ptr<DiscoveryState>>& entries, std::int32_t id) {
    const auto value = entries.at(static_cast<std::size_t>(id));
    if (!value) throw std::invalid_argument("null discovery record");
    return *value;
}
}

EventIntegers resolve_event_arguments(const EventIntegers& row,
                                     const std::shared_ptr<EventIntegers>& arguments) {
    auto resolved = row;
    constexpr auto first = std::numeric_limits<std::int32_t>::min();
    for (std::size_t i = 1; i < resolved.size(); ++i) {
        const auto value = resolved[i];
        if (value > first + 99) continue;
        if (!arguments) throw std::invalid_argument("null event arguments for sentinel");
        const auto index = static_cast<std::uint32_t>(value) ^ 0x80000000U;
        resolved[i] = arguments->at(index);
    }
    return resolved;
}

std::int64_t add_decoded_currency(std::int64_t value,std::int32_t amount){return add(value,amount);}
bool apply_event_progression(const EventIntegers& row, EventProgression& state) {
    switch (row.at(0)) {
    case 1: {
        const auto amount = row.at(1);
        state.money = amount >= 0 ? add(state.money, amount)
            : subtract(state.money, signed32(0U - static_cast<std::uint32_t>(amount)));
        return true;
    }
    case 2: {
        const auto amount = row.at(1);
        if (amount >= 0) state.crystals = add(state.crystals, amount);
        else {
            auto cost = 0U - static_cast<std::uint32_t>(amount);
            if (state.crystal_subtraction_mode != 10000) cost *= 10000U;
            state.crystals = subtract(state.crystals, signed32(cost));
        }
        return true;
    }
    case 4: {
        auto& area = entry(state.areas, row.at(1));
        // isFound == (state == 1); AreaData inherits BaseData.Find38052.
        if (area.state != 1) { area.state = 1; area.is_new = true; }
        return true;
    }
    case 15: {
        auto& quest = entry(state.quests, row.at(1));
        // QuestData.Find38509 ignores true and changes only state zero.
        find_quest_discovery(quest.state,quest.is_new,true);
        return true;
    }
    default: return false;
    }
}
}
