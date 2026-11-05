#pragma once
#include "combat_host.hpp"
#include "progression_tables.hpp"
#include "stage_rewards.hpp"

namespace saga::core {
// Retain the original four aggregate fields before the added economy/text fields.
struct MagicShopCombatRecord {
    std::int32_t id=0,category=0,value_per_purchase=0,flags=0;
    std::int32_t base_price=0;
    std::string name{},explanation{};
};
struct ShopNewGameState {std::int32_t discovery_state=0,purchases=0;bool is_new=false;};
// MagicShopData.NewGame 0x1172014; cctor initially stores mask1 at static+0.
void reset_shop_for_new_game(ShopNewGameState&,const MagicShopCombatRecord&,std::int32_t find_mask,const std::function<void(bool)>& find);
void reset_shop_for_new_game(ShopNewGameState&,const MagicShopCombatRecord&,std::int32_t find_mask);
class MagicShopPriceHost {
public:
    virtual ~MagicShopPriceHost()=default;
    // Fresh catalog-indexed saved counter, not a sum of discovered/visible rows.
    // Borrowed values remain valid until the next provider call. Bounds errors
    // propagate; null represents a missing native row and fails in the kernel.
    virtual const std::int32_t* saved_purchases(std::int32_t catalog_slot)=0;
    virtual const LevelTableRecord* price_record(std::int32_t grouped_count)=0; // Data.tableData.
};
// MagicShopData.GetGoupBuyCount0x11722b4 / GetPeice0x1172094.
// Fixed slot groups0..1,2..4,5..7; unknown category returns INT32_MAX.
std::int32_t magic_shop_grouped_count(const MagicShopCombatRecord&,MagicShopPriceHost&);
std::int32_t magic_shop_price(const MagicShopCombatRecord&,MagicShopPriceHost&);
// GetParamKeyForCategory0x1172538; reverse mapping already shop_id_for_parameter.
std::int32_t magic_shop_parameter_key(std::int32_t category);
// AddBuyCount0x11725b4: signed int32 storage, modulo32 addition, no clamp.
void add_magic_shop_purchases(std::int32_t& purchases,std::int32_t amount);
struct EquipmentCombatRecord {std::int32_t id=0,attribute=0,flags=0;EventScript parameters;std::int32_t range=0;
    std::int32_t distance=0,burst_count=0,use_mp=0; // EquipmentData +0x70/+0x74/+0x7c.
    std::int32_t type=0,bullet_id=-1,max_height=0,bullet_speed=0; // +0x3c/+0x54/+0x68/+0x6c.
    std::int32_t image=0;std::string resource;
    std::int32_t initial_level=0; // Equipment +0x80, cloth +0x4c, hat +0x44.
    std::string name; // BaseData name, selected resource-language table token1.
    std::int32_t unlock_level=0,buy_price=0; // Equipment tokens4/12; cloth/hat tokens2/6.
    std::string explanation;}; // Equipment token22; cloth/hat token10.
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
// ShopForm.Reset: materialize nonnull records, stable OrderBy(unlock_level),
// then Check(mask) and !isHave. Append directly, retaining the published prefix
// if have throws. Record addresses must survive callbacks; fields remain live.
void append_equipment_shop_entries(std::vector<std::int32_t>& output,const EquipmentCombatTable&,
    std::uint32_t flag_mask,const std::function<bool(const EquipmentCombatRecord&)>& have);

enum class EquipmentTableKind { weapon_or_shield,cloth_or_hat };
MagicShopCombatTable load_magic_shop_combat_table(std::string_view,bool normalize_crlf,const EventIntegerParser&);
EquipmentCombatTable load_equipment_combat_table(std::string_view,EquipmentTableKind,bool normalize_crlf,const EventIntegerParser&);
// Saved state is required explicitly; table initial_level is not the saved character level.
CharacterParameters bind_character_parameters(const CharacterProgressionRecord&,std::int32_t saved_level);
CombatEquipment bind_equipment_parameters(const EquipmentCombatRecord&,std::int32_t saved_level);
ShopParameterUpgrade bind_shop_upgrade(const MagicShopCombatRecord&,std::int32_t purchase_count);
}
