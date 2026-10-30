#include "combat_tables.hpp"
#include "table_catalog.hpp"

namespace saga::core {
MagicShopCombatTable load_magic_shop_combat_table(std::string_view text,bool normalize,const EventIntegerParser& parser) {
    // MagicShopData.Load 0x1171f2c: id/name/category/basePrice/upValue/explain/flag.
    return detail::load_table_records<MagicShopCombatRecord>(text,normalize,parser,[](detail::TableTokens& t){
        MagicShopCombatRecord r;r.id=t.integer();t.take();r.category=t.integer();t.integer();
        r.value_per_purchase=t.integer();t.take();r.flags=t.integer();return r;
    });
}
EquipmentCombatTable load_equipment_combat_table(std::string_view text,EquipmentTableKind kind,
                                                 bool normalize,const EventIntegerParser& parser) {
    return detail::load_table_records<EquipmentCombatRecord>(text,normalize,parser,[kind](detail::TableTokens& t){
        EquipmentCombatRecord r;r.id=t.integer();t.take();
        if(kind==EquipmentTableKind::weapon_or_shield) {
            // EquipmentData.Load 0x116f428, status_ +0x88; attribute is fixed token 9.
            t.integer();r.type=t.integer();t.integer();t.integer();
            r.resource=t.take();r.image=t.integer();r.bullet_id=t.integer();r.attribute=t.integer();
            t.integer();t.integer();t.integer();r.max_height=t.integer();r.bullet_speed=t.integer();
            r.distance=t.integer();r.burst_count=t.integer();
            r.range=t.integer(); // EquipmentData.range +0x78, fixed token 17.
            r.use_mp=t.integer();r.initial_level=t.integer();t.integer();
        } else {
            // ClothData.Load 0x116e704 and HatData.Load 0x1171734 share token framing.
            t.integer();r.resource=t.take();r.image=t.integer();t.integer();t.integer();r.initial_level=t.integer();t.integer();
        }
        r.parameters=t.script();t.take();r.flags=t.integer();return r;
    });
}
void reset_shop_for_new_game(ShopNewGameState& state,const MagicShopCombatRecord& record,std::int32_t mask,const std::function<void(bool)>& find){
    if((static_cast<std::uint32_t>(record.flags)&static_cast<std::uint32_t>(mask))!=0)find(true);
    state.purchases=0;
}
void reset_shop_for_new_game(ShopNewGameState& state,const MagicShopCombatRecord& record,std::int32_t mask){
    reset_shop_for_new_game(state,record,mask,[&state](bool fresh){find_data_record(state.discovery_state,state.is_new,fresh);});
}
void reset_equipment_for_new_game(EquipmentNewGameState& state,const EquipmentCombatRecord& record,const std::function<void(bool)>& find){
    if((static_cast<std::uint32_t>(record.flags)&1U)!=0)find(false);
    state.level=record.initial_level;
    state.inventory.stock=static_cast<std::int32_t>((static_cast<std::uint32_t>(record.flags)>>1)&1U);
}
void reset_equipment_for_new_game(EquipmentNewGameState& state,const EquipmentCombatRecord& record){
    reset_equipment_for_new_game(state,record,[&state](bool fresh){find_item_stock(state.inventory,fresh);});
}
CharacterParameters bind_character_parameters(const CharacterProgressionRecord& record,std::int32_t level) {
    return {record.parameters,level,static_cast<std::uint8_t>(record.flags)};
}
CombatEquipment bind_equipment_parameters(const EquipmentCombatRecord& record,std::int32_t level) {
    return {{level,record.parameters},record.attribute};
}
ShopParameterUpgrade bind_shop_upgrade(const MagicShopCombatRecord& record,std::int32_t count) {
    return {record.value_per_purchase,count};
}
}
