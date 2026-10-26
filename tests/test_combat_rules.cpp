#include "core/gameplay/combat_rules.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
void check(bool ok, const char* why) {
    if (!ok) { std::cerr << why << '\n'; std::exit(1); }
}
int main() {
    // Expected hash from tools/verify_combat_easing.py's struct-rounded Python model.
    std::uint64_t curve_hash=14695981039346656037ULL;
    for(const auto endpoints: {std::array<int,2>{100,1000},std::array<int,2>{1000,100},
                              std::array<int,2>{-1000,50},std::array<int,2>{0,9999999}})
        for(const int duration:{199,999})for(const int amplitude:{-80,-50,0,80})
            for(int progress=-1;progress<=duration+1;++progress){
                curve_hash^=static_cast<std::uint32_t>(saga::core::easing(endpoints[0],endpoints[1],duration,progress,amplitude));
                curve_hash*=1099511628211ULL;
            }
    check(curve_hash==17971640406678543181ULL,"19264 general easing cases match independent static model");
    check(saga::core::easing(0,1000,100,50,-50)==340,"negative amplitude differs from linear interpolation");
    check(saga::core::easing(9,4,0,-1,-50)==9&&saga::core::easing(9,4,0,0,-50)==4,"general easing early guard order");
    // Exhaustive interior values. Integer expectations were independently
    // checked against the instruction-level binary32 model (report 64).
    for (int t = 0; t < 50; ++t)
        check(saga::core::noncritical_human_damage_factor(t) == 10 + t * 90 / 50,
              "human factor interior");
    for (int t = 0; t < 98; ++t)
        check(saga::core::matching_attribute_damage_factor(t + 1) == 99 - t * 29 / 98,
              "matching attribute factor interior");
    for (const int t : {std::numeric_limits<int>::min(), -1})
        check(saga::core::noncritical_human_damage_factor(t) == 10, "human lower clamp");
    for (const int t : {50, 100, std::numeric_limits<int>::max()})
        check(saga::core::noncritical_human_damage_factor(t) == 100, "human upper clamp");
    check(saga::core::matching_attribute_damage_factor(0) == 99, "bonus below domain");
    check(saga::core::matching_attribute_damage_factor(99) == 70, "bonus upper boundary");
    check(saga::core::matching_attribute_damage_factor(std::numeric_limits<int>::min()) == 70,
          "bonus subtraction wraps to positive maximum");
    check(saga::core::matching_attribute_damage_factor(std::numeric_limits<int>::max()) == 70,
          "maximum bonus clamps");
    using saga::core::hit_threshold;
    check(hit_threshold(true, 0, 0) == 90, "human baseline");
    check(hit_threshold(false, 0, 0) == 80, "other baseline");
    check(hit_threshold(true, 200, 0) == 105, "no clamp to 100");
    check(hit_threshold(false, 200, 0) == 95, "upper adjustment");
    check(hit_threshold(true, 0, 200) == 30, "human lower adjustment");
    check(hit_threshold(false, 0, 200) == 20, "other lower adjustment");
    check(hit_threshold(true, 0, -3) == 91, "negative half truncates to zero");
    check(hit_threshold(true, 0, 3) == 89, "positive half truncates");
    check(hit_threshold(true, -60, 0) == 30, "lower boundary");
    check(hit_threshold(true, -59, 0) == 31, "inside lower boundary");
    check(hit_threshold(true, 14, 0) == 104, "inside upper boundary");
    check(hit_threshold(true, 15, 0) == 105, "upper boundary");
    check(hit_threshold(true, std::numeric_limits<std::int32_t>::min(), 2) == 105,
          "32-bit subtraction wraps upward");
    check(hit_threshold(false, std::numeric_limits<std::int32_t>::max(), -2) == 20,
          "32-bit subtraction wraps downward");
    const auto ordinary = saga::core::subtract_parameter(30, 100, 12);
    check(ordinary.value == 18 && ordinary.remainder == 0, "ordinary damage");
    const auto excess = saga::core::subtract_parameter(30, 100, 50);
    check(excess.value == 0 && excess.remainder == -20, "overkill remainder sign");
    const auto unbounded = saga::core::subtract_parameter(30, 0, 50);
    check(unbounded.value == -20 && unbounded.remainder == 0, "zero maximum bypass");
    const auto healing = saga::core::subtract_parameter(90, 100, -30);
    check(healing.value == 100 && healing.remainder == 20, "negative amount and upper limit");
    const auto lower_first = saga::core::subtract_parameter(0, -5, 1);
    check(lower_first.value == 0 && lower_first.remainder == -1, "lower check precedes upper");
    const auto upper = saga::core::subtract_parameter(0, -5, 0);
    check(upper.value == -5 && upper.remainder == 5, "negative maximum retained");
    const auto wrap = saga::core::subtract_parameter(std::numeric_limits<std::int32_t>::min(), 0, 1);
    check(wrap.value == std::numeric_limits<std::int32_t>::max() && wrap.remainder == 0,
          "subtraction uses 32-bit wrap");
    auto tables = saga::core::initial_attribute_affinity_tables();
    const int expected[5][5] = {
        {45,20,90,45,45}, {90,45,20,45,45}, {20,90,45,45,45},
        {60,60,60,60,60}, {60,60,60,60,60}};
    for (int a = 0; a < 5; ++a) for (int r = 0; r < 5; ++r) {
        const auto index = saga::core::attribute_affinity(tables, a, r);
        check(tables.rates.at(static_cast<std::size_t>(index)) == expected[a][r],
              "initial affinity matrix");
    }
    check(saga::core::attribute_affinity(tables, 3, -1) == 0, "match before sentinel fallback");
    check(saga::core::attribute_affinity(tables, 0, 999) == 1, "receiver is not an index");
    tables.pairs[0] = {4, 4};
    check(saga::core::attribute_affinity(tables, 0, 4) == 0, "first match precedes second");
    tables.pairs[0] = {2, -1};
    check(saga::core::attribute_affinity(tables, 0, 4) == 3, "second sentinel fallback");
    for (const int a : {-1, 5}) {
        bool rejected = false;
        try { saga::core::attribute_affinity(tables, a, 0); }
        catch (const std::out_of_range&) { rejected = true; }
        check(rejected, "invalid attacker index");
    }
}
