#include "stage_rewards.hpp"

namespace saga::core {
namespace {
std::int32_t add32(std::int32_t a, std::int32_t b) {
    const auto bits = static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b);
    return static_cast<std::int32_t>(bits < 0x80000000U ? static_cast<std::int64_t>(bits)
        : static_cast<std::int64_t>(bits) - 0x100000000LL);
}
}
void record_collected_drop(StageRewards& rewards, std::int32_t type,
                           std::int32_t value) {
    switch (type) {
    case 0: rewards.experience = add32(rewards.experience, value); break;
    case 1: rewards.money = add32(rewards.money, value); break;
    case 2: rewards.prisoners = add32(rewards.prisoners, 1); break;
    case 3: rewards.treasure_ids.push_back(value); break;
    default: break;
    }
}
void find_item_stock(ItemStock& item,bool is_new) noexcept {
    item.discovery_state=1;item.is_new=is_new;
}
void add_item_stock(ItemStock& item, std::int32_t amount) {
    if (item.discovery_state != 1) {
        item.discovery_state = 1;
        item.is_new = true;
    }
    const auto candidate = add32(item.stock, amount);
    item.stock = candidate > 1 ? 1 : candidate;
}
}
