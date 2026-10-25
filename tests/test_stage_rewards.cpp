#include "core/stage_rewards.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>

void check(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    saga::core::ItemStock item{0, false, 0};
    saga::core::add_item_stock(item, 1);
    check(item.discovery_state == 1 && item.is_new && item.stock == 1,
          "first grant discovers item and adds stock");
    item.is_new = false;
    saga::core::add_item_stock(item, 1);
    check(item.stock == 1 && !item.is_new, "duplicate grant caps stock without rediscovery");
    item.discovery_state = 2;
    saga::core::add_item_stock(item, -3);
    check(item.discovery_state == 1 && item.is_new && item.stock == -2,
          "other state invokes Find before negative stock mutation");
    item.stock = std::numeric_limits<std::int32_t>::max();
    saga::core::add_item_stock(item, 1);
    check(item.stock == std::numeric_limits<std::int32_t>::min(), "stock wraps before upper cap");
    item.stock = std::numeric_limits<std::int32_t>::min();
    saga::core::add_item_stock(item, -1);
    check(item.stock == 1, "wrapped positive stock is capped");
    using saga::core::record_collected_drop;
    saga::core::StageRewards rewards{100, 10, 2, {7}};
    record_collected_drop(rewards, 0, 25);
    record_collected_drop(rewards, 1, -3);
    record_collected_drop(rewards, 2, 999);
    record_collected_drop(rewards, 2, -100);
    record_collected_drop(rewards, 3, 7);
    record_collected_drop(rewards, 3, -1);
    check(rewards.experience == 125 && rewards.money == 7, "separate signed totals");
    check(rewards.prisoners == 4, "prisoner adds one regardless of payload");
    check(rewards.treasure_ids == std::vector<std::int32_t>({7,7,-1}),
          "treasure preserves order, duplicate and signed id");
    for (auto type : {-1, 4, std::numeric_limits<int>::max()})
        record_collected_drop(rewards, type, 500);
    check(rewards.experience == 125 && rewards.money == 7 && rewards.prisoners == 4 &&
          rewards.treasure_ids.size() == 3, "unknown types do not mutate accumulator");
    rewards.experience = std::numeric_limits<std::int32_t>::max();
    rewards.money = std::numeric_limits<std::int32_t>::min();
    rewards.prisoners = std::numeric_limits<std::int32_t>::max();
    record_collected_drop(rewards, 0, 1);
    record_collected_drop(rewards, 1, -1);
    record_collected_drop(rewards, 2, 0);
    check(rewards.experience == std::numeric_limits<std::int32_t>::min() &&
          rewards.money == std::numeric_limits<std::int32_t>::max() &&
          rewards.prisoners == std::numeric_limits<std::int32_t>::min(),
          "all numeric totals wrap rather than saturating");
}
