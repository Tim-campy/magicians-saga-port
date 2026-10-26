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
}
