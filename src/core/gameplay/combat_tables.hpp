#pragma once
#include "combat_host.hpp"
#include "progression_tables.hpp"
#include "stage_rewards.hpp"

namespace saga::core {
// Gameplay projections: all tokens are consumed; unrelated economy fields are not retained.
struct MagicShopCombatRecord {std::int32_t id=0,category=0,value_per_purchase=0,flags=0;};
struct ShopNewGameState {std::int32_t discovery_state=0,purchases=0;bool is_new=false;};
// MagicShopData.NewGame 0x1172014; cctor initially stores mask1 at static+0.
void reset_shop_for_new_game(ShopNewGameState&,const MagicShopCombatRecord&,std::int32_t find_mask,const std::function<void(bool)>& find);
void reset_shop_for_new_game(ShopNewGameState&,const MagicShopCombatRecord&,std::int32_t find_mask);
struct EquipmentCombatRecord {std::int32_t id=0,attribute=0,flags=0;EventScript parameters;std::int32_t range=0;
    std::int32_t distance=0,burst_count=0,use_mp=0; // EquipmentData +0x70/+0x74/+0x7c.
    std::int32_t type=0,bullet_id=-1,max_height=0,bullet_speed=0; // +0x3c/+0x54/+0x68/+0x6c.
    std::int32_t image=0;std::string resource;
    std::int32_t initial_level=0;}; // Equipment +0x80, cloth +0x4c, hat +0x44.
struct EquipmentNewGameState {
    std::int32_t level=0;
    ItemStock inventory{0,false,0}; // Fresh constructors explicitly clear discovery/new.
};
// EquipmentData.NewGame 0x116f0c8, ClothData.NewGame 0x116e458,
// HatData.NewGame 0x1171894: same value contract at different field offsets.
// The hook models virtual Find; flags/initial_level are re-read after it returns.
void reset_equipment_for_new_game(EquipmentNewGameState&,const EquipmentCombatRecord&,const std::function<void(bool)>& find);
void reset_equipment_for_new_game(EquipmentNewGameState&,const EquipmentCombatRecord&);
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
