#include "combat_tables.hpp"
#include "table_catalog.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t shop_word(std::uint32_t bits){
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):-1-static_cast<std::int32_t>(0xffffffffU-bits);
}
std::int32_t shop_group(std::int32_t category){
    if(category>=0&&category<=1)return 0;
    if(category>=2&&category<=4)return 1;
    if(category>=5&&category<=7)return 2;
    return -1;
}
template<class T>const T& shop_required(const T* value){
    if(!value)throw std::invalid_argument("missing magic shop price data");return *value;
}
}
MagicShopCombatTable load_magic_shop_combat_table(std::string_view text,bool normalize,const EventIntegerParser& parser) {
    // MagicShopData.Load 0x1171f2c: id/name/category/basePrice/upValue/explain/flag.
    return detail::load_table_records<MagicShopCombatRecord>(text,normalize,parser,[](detail::TableTokens& t){
        MagicShopCombatRecord r;r.id=t.integer();r.name=t.take();r.category=t.integer();r.base_price=t.integer();
        r.value_per_purchase=t.integer();r.explanation=t.take();r.flags=t.integer();return r;
    });
}
std::int32_t magic_shop_grouped_count(const MagicShopCombatRecord& record,MagicShopPriceHost& host){
    const auto group=shop_group(record.category);
    if(group<0)return std::numeric_limits<std::int32_t>::max();
    const auto first=group==0?0:group==1?2:5;
    const auto end=group==0?2:group==1?5:8;
    std::int32_t total=0;
    for(auto slot=first;slot<end;++slot){
        const auto value=shop_required(host.saved_purchases(slot));
        total=shop_word(static_cast<std::uint32_t>(total)+static_cast<std::uint32_t>(value));
    }
    return total;
}
std::int32_t magic_shop_price(const MagicShopCombatRecord& record,MagicShopPriceHost& host){
    // Grouping callbacks run before the fresh category read used for the column.
    const auto count=magic_shop_grouped_count(record,host);
    const auto group=shop_group(record.category);
    if(group<0)return std::numeric_limits<std::int32_t>::max();
    const auto rate=shop_required(host.price_record(count)).shop_parameters.at(static_cast<std::size_t>(group));
    const auto base=record.base_price; // Re-read only after the selected Table row.
    const auto product=shop_word(static_cast<std::uint32_t>(base)*static_cast<std::uint32_t>(rate));
    return product/100;
}
std::int32_t magic_shop_parameter_key(std::int32_t category){
    if(category>=0&&category<=4)return category;
    return category>=5&&category<=7?category+5:-1;
}
void add_magic_shop_purchases(std::int32_t& purchases,std::int32_t amount){
    purchases=shop_word(static_cast<std::uint32_t>(purchases)+static_cast<std::uint32_t>(amount));
}
EquipmentCombatTable load_equipment_combat_table(std::string_view text,EquipmentTableKind kind,
                                                 bool normalize,const EventIntegerParser& parser) {
    return detail::load_table_records<EquipmentCombatRecord>(text,normalize,parser,[kind](detail::TableTokens& t){
        EquipmentCombatRecord r;r.id=t.integer();r.name=t.take();
        if(kind==EquipmentTableKind::weapon_or_shield) {
            // EquipmentData.Load 0x116f428, status_ +0x88; attribute is fixed token 9.
            t.integer();r.type=t.integer();r.unlock_level=t.integer();t.integer();
            r.resource=t.take();r.image=t.integer();r.bullet_id=t.integer();r.attribute=t.integer();
            t.integer();t.integer();r.buy_price=t.integer();r.max_height=t.integer();r.bullet_speed=t.integer();
            r.distance=t.integer();r.burst_count=t.integer();
            r.range=t.integer(); // EquipmentData.range +0x78, fixed token 17.
            r.use_mp=t.integer();r.initial_level=t.integer();t.integer();
        } else {
            // ClothData.Load 0x116e704 and HatData.Load 0x1171734 share token framing.
            r.unlock_level=t.integer();r.resource=t.take();r.image=t.integer();t.integer();r.buy_price=t.integer();r.initial_level=t.integer();t.integer();
        }
        r.parameters=t.script();r.explanation=t.take();r.flags=t.integer();return r;
    });
}
void append_equipment_shop_entries(std::vector<std::int32_t>& output,const EquipmentCombatTable& records,
    std::uint32_t mask,const std::function<bool(const EquipmentCombatRecord&)>& have){
    struct Ranked {const EquipmentCombatRecord* record;std::int32_t key;};
    std::vector<Ranked> ordered;ordered.reserve(records.size());
    for(const auto& row:records)if(row)ordered.push_back({&*row,row->unlock_level});
    std::stable_sort(ordered.begin(),ordered.end(),[](const auto& a,const auto& b){return a.key<b.key;});
    for(const auto& entry:ordered){const auto* row=entry.record;
        if((static_cast<std::uint32_t>(row->flags)&mask)!=0&&!have(*row))output.push_back(row->id);
    }
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
