#pragma once
#include "combat_host.hpp"
#include "progression_tables.hpp"

namespace saga::core {
// Combat projections: all tokens are consumed, presentation/economy fields are not retained.
struct MagicShopCombatRecord {std::int32_t id=0,category=0,value_per_purchase=0,flags=0;};
struct EquipmentCombatRecord {std::int32_t id=0,attribute=0,flags=0;EventScript parameters;std::int32_t range=0;
    std::int32_t distance=0,burst_count=0,use_mp=0;}; // EquipmentData +0x70/+0x74/+0x7c.
using MagicShopCombatTable=std::vector<std::optional<MagicShopCombatRecord>>;
using EquipmentCombatTable=std::vector<std::optional<EquipmentCombatRecord>>;
enum class EquipmentTableKind { weapon_or_shield,cloth_or_hat };
MagicShopCombatTable load_magic_shop_combat_table(std::string_view,bool normalize_crlf,const EventIntegerParser&);
EquipmentCombatTable load_equipment_combat_table(std::string_view,EquipmentTableKind,bool normalize_crlf,const EventIntegerParser&);
// Saved state is required explicitly; table initial_level is not the saved character level.
CharacterParameters bind_character_parameters(const CharacterProgressionRecord&,std::int32_t saved_level);
CombatEquipment bind_equipment_parameters(const EquipmentCombatRecord&,std::int32_t saved_level);
ShopParameterUpgrade bind_shop_upgrade(const MagicShopCombatRecord&,std::int32_t purchase_count);
}
