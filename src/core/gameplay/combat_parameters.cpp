#include "combat_parameters.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t wrap(std::uint32_t bits) {
    return static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):
                                    static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::int32_t add(std::int32_t a,std::int32_t b) {
    return wrap(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));
}
}
// EquipmentData.GetParamValue 0x116f204, ClothData 0x116e594, HatData 0x11719d0.
std::int32_t equipment_parameter(const EquipmentParameters& data,std::int32_t index) {
    if(!data.values)throw std::invalid_argument("null equipment parameter array");
    const auto& row=data.values->at(static_cast<std::size_t>(index));
    if(!row)throw std::invalid_argument("null equipment parameter row");
    const auto first=row->at(0),last=row->at(1);
    return easing_zero_amplitude(first,last,99,wrap(static_cast<std::uint32_t>(data.level)-1U));
}
// MemberSystem.GetTotalParamValue(Entity,index) 0x1156424.
std::int32_t member_total_parameter(MemberParameterHost& host,std::int32_t index) {
    const auto base=host.base_parameter(index);
    const auto weapon=equipment_parameter(host.equipment(EquipmentSlot::weapon),index);
    const auto shield=equipment_parameter(host.equipment(EquipmentSlot::shield),index);
    const auto hat=equipment_parameter(host.equipment(EquipmentSlot::hat),index);
    const auto cloth=equipment_parameter(host.equipment(EquipmentSlot::cloth),index);
    return add(add(add(add(base,weapon),shield),hat),cloth);
}
// MemberSystem.GetAttributeValue(Entity,attribute) 0x1157410.
std::int32_t member_attribute_bonus(MemberParameterHost& host,std::int32_t attribute) {
    if(static_cast<std::uint32_t>(attribute)>2U)return 0;
    return member_total_parameter(host,attribute+10);
}
// Parameter.get_rate 0x1178c84. The early guard uses base maximum alone.
std::int32_t damage_parameter_rate(const DamageParameter& parameter) {
    if(parameter.maximum==0)return 0;
    const auto numerator=wrap(static_cast<std::uint32_t>(add(parameter.value,parameter.extra))*100U);
    const auto denominator=add(parameter.maximum,parameter.extra_maximum);
    // ARM64 SDIV semantics, including overflow and zero divisor.
    const auto quotient=denominator==0?0:
        numerator==std::numeric_limits<std::int32_t>::min()&&denominator==-1?numerator:
        numerator/denominator;
    return std::clamp(quotient,0,100);
}
}
