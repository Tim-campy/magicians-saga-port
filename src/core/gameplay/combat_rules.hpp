#pragma once
#include <cstdint>
#include <array>

namespace saga::core {
// Graph.Easing: ordered binary32 arithmetic and binary64 degree-to-radian sine.
// Host libm is used; bitwise agreement with Android libm awaits runtime comparison.
std::int32_t easing(std::int32_t start,std::int32_t end,std::int32_t duration,
                   std::int32_t progress,std::int32_t amplitude);
// Graph.Easing zero-amplitude numeric path; binary32 rounds and wrapping endpoint difference.
std::int32_t easing_zero_amplitude(std::int32_t start,std::int32_t end,
                                   std::int32_t duration,std::int32_t progress);
// EnemySystem.Damage: parameter 7 of attacker and parameter 9 of receiver.
// Inputs are already resolved through MemberSystem/EnemyData. This computes
// the threshold only; it neither consumes nor replaces the original RNG.
std::int32_t hit_threshold(bool attacker_is_human, std::int32_t attacker_param7,
                           std::int32_t receiver_param9);
struct ParameterSubtraction {
    std::int32_t value;
    std::int32_t remainder;
};
// Parameter.Sub: subtracts from the base value. A zero base maximum disables
// clamping; extraValue/extraMaxValue do not participate in this operation.
ParameterSubtraction subtract_parameter(std::int32_t value, std::int32_t maximum,
                                        std::int32_t amount);
struct AttributeAffinityTables {
    std::array<std::array<std::int32_t, 2>, 5> pairs;
    std::array<std::int32_t, 4> rates;
};
// Values written by AppData's static initializer for this APK version.
AttributeAffinityTables initial_attribute_affinity_tables();
// Returns the selector used to index rates. Only attacker indexes pairs;
// receiver is compared as an integer (including sentinel -1).
std::int32_t attribute_affinity(const AttributeAffinityTables& tables,
                              std::int32_t attacker, std::int32_t receiver);
// Numeric factors only: Graph.Easing's two zero-amplitude combat cases.
// Does not reproduce class initialization, effects, or RNG consumption.
std::int32_t noncritical_human_damage_factor(std::int32_t parameter_rate);
std::int32_t matching_attribute_damage_factor(std::int32_t attribute_bonus);
}
