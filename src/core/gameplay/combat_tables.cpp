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
            for(int i=0;i<4;++i)t.integer();
            t.take();t.integer();t.integer();r.attribute=t.integer();
            for(int i=0;i<5;++i)t.integer();
            r.distance=t.integer();r.burst_count=t.integer();
            r.range=t.integer(); // EquipmentData.range +0x78, fixed token 17.
            r.use_mp=t.integer();for(int i=0;i<2;++i)t.integer();
        } else {
            // ClothData.Load 0x116e704 and HatData.Load 0x1171734 share token framing.
            t.integer();t.take();for(int i=0;i<5;++i)t.integer();
        }
        r.parameters=t.script();t.take();r.flags=t.integer();return r;
    });
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
