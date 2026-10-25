#include "combat_rules.hpp"
#include <algorithm>
#include <limits>

namespace saga::core {
namespace {
// Fixed small positive bounds only. Separate volatile stores retain each
// binary32 rounding and prevent contraction into an FMA.
std::int32_t combat_easing(std::int32_t start, std::int32_t end,
                          std::int32_t duration, std::int32_t progress) {
    static_assert(std::numeric_limits<float>::is_iec559 &&
                  std::numeric_limits<float>::digits == 24);
    if (progress < 0) return start;
    if (progress >= duration) return end;
    const volatile float ratio = static_cast<float>(progress) / static_cast<float>(duration);
    const volatile float product = ratio * static_cast<float>(end - start);
    const volatile float result = product + static_cast<float>(start);
    return std::clamp(static_cast<std::int32_t>(result), start, end);
}
std::int32_t subtract32(std::int32_t a, std::int32_t b) {
    const auto bits = static_cast<std::uint32_t>(a) - static_cast<std::uint32_t>(b);
    return static_cast<std::int32_t>(bits < 0x80000000U ? static_cast<std::int64_t>(bits)
        : static_cast<std::int64_t>(bits) - 0x100000000LL);
}
}
std::int32_t hit_threshold(bool human, std::int32_t attack, std::int32_t evade) {
    // C++17 signed division truncates toward zero, matching CINC/ASR here.
    const auto half = evade / 2;
    const auto bits = static_cast<std::uint32_t>(attack) - static_cast<std::uint32_t>(half);
    const auto difference = bits < 0x80000000U ? static_cast<std::int64_t>(bits)
        : static_cast<std::int64_t>(bits) - 0x100000000LL;
    const auto adjustment = std::clamp(static_cast<std::int32_t>(difference), -60, 15);
    return (human ? 90 : 80) + adjustment;
}
ParameterSubtraction subtract_parameter(std::int32_t value, std::int32_t maximum,
                                        std::int32_t amount) {
    const auto candidate = subtract32(value, amount);
    auto result = candidate;
    if (maximum != 0) {
        // Preserve lower-bound early return even for a negative maximum.
        if (result < 0) result = 0;
        else if (result > maximum) result = maximum;
    }
    return {result, subtract32(candidate, result)};
}
AttributeAffinityTables initial_attribute_affinity_tables() {
    return {{{{2, 1}, {0, 2}, {1, 0}, {-1, -1}, {-1, -1}}}, {{90, 45, 20, 60}}};
}
std::int32_t attribute_affinity(const AttributeAffinityTables& tables,
                              std::int32_t attacker, std::int32_t receiver) {
    const auto& pair = tables.pairs.at(static_cast<std::size_t>(attacker));
    if (pair[0] == receiver) return 0;
    if (pair[1] == receiver) return 2;
    return pair[0] == -1 || pair[1] == -1 ? 3 : 1;
}
std::int32_t noncritical_human_damage_factor(std::int32_t rate) {
    return combat_easing(10, 100, 50, rate);
}
std::int32_t matching_attribute_damage_factor(std::int32_t bonus) {
    return 100 - combat_easing(1, 30, 98, subtract32(bonus, 1));
}
}
