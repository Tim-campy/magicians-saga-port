#pragma once
#include <cstdint>
#include <vector>

namespace saga::core {
struct StageRewards {
    std::int32_t experience;
    std::int32_t money;
    std::int32_t prisoners;
    std::vector<std::int32_t> treasure_ids;
};
// Only the accumulator mutation in StageSystem.OnCollectedDrop (0x1165988).
// Caller must invoke at collection completion, before its effects/cleanup.
// No deduplication, player inventory grant, animation or entity destruction.
void record_collected_drop(StageRewards& rewards, std::int32_t type,
                           std::int32_t value);
struct ItemStock {
    std::int32_t discovery_state;
    bool is_new;
    std::int32_t stock;
};
// BaseData.Find 0x116c790; stock is outside its discovery-state contract.
void find_item_stock(ItemStock&,bool is_new) noexcept;
// EquipmentData/ClothData/HatData.AddStock, including their resolved Find hook.
void add_item_stock(ItemStock& item, std::int32_t amount);
}
