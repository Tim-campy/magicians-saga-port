#pragma once
#include "combat_damage.hpp"
#include "event_flow.hpp"

namespace saga::core {
struct EquipmentParameters {
    std::int32_t level=1;
    EventScript values;
};
enum class EquipmentSlot { weapon,shield,hat,cloth };
// Base character growth and witch/shop bonuses remain explicit until their data is bound.
class MemberParameterHost {
public:
    virtual ~MemberParameterHost()=default;
    virtual std::int32_t base_parameter(std::int32_t index)=0;
    // Resolve each slot afresh, in weapon/shield/hat/cloth order; missing records must fault.
    virtual const EquipmentParameters& equipment(EquipmentSlot)=0;
};
std::int32_t equipment_parameter(const EquipmentParameters&,std::int32_t index);
std::int32_t member_total_parameter(MemberParameterHost&,std::int32_t index);
std::int32_t member_attribute_bonus(MemberParameterHost&,std::int32_t attribute);
std::int32_t damage_parameter_rate(const DamageParameter&);
struct CharacterParameters {
    EventScript values;
    std::int32_t level=1;
    std::uint8_t flags=0;
};
struct ShopParameterUpgrade {std::int32_t value_per_purchase=0,purchases=0;};
class CharacterParameterHost {
public:
    virtual ~CharacterParameterHost()=default;
    virtual std::int32_t maximum_level(std::int32_t parameter)=0;
    virtual const ShopParameterUpgrade& shop_upgrade(std::int32_t id)=0;
};
std::int32_t character_parameter(const CharacterParameters&,std::int32_t index,
                                 std::int32_t level,CharacterParameterHost&);
std::int32_t shop_id_for_parameter(std::int32_t index);
std::int32_t shop_parameter_addition(const ShopParameterUpgrade&);
// Use as MemberParameterHost::base_parameter; maximum tables and shop records remain explicit.
std::int32_t member_base_parameter(const CharacterParameters&,std::int32_t index,
                                   CharacterParameterHost&);
std::int32_t member_base_parameter_at_level(const CharacterParameters&,std::int32_t index,
                                            std::int32_t level,CharacterParameterHost&);
}
