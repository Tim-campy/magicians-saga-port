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
// CharaData.GetParamValue 0x116d810. Read endpoints before the maximum provider.
std::int32_t character_parameter(const CharacterParameters& data,std::int32_t index,
                                 std::int32_t level,CharacterParameterHost& host) {
    if(!data.values)throw std::invalid_argument("null character parameter array");
    const auto& row=data.values->at(static_cast<std::size_t>(index));
    if(!row)throw std::invalid_argument("null character parameter row");
    const auto first=row->at(0),last=row->at(1);
    const auto maximum=host.maximum_level(index);
    auto value=easing_zero_amplitude(first,last,add(maximum,-1),add(level,-1));
    if(static_cast<std::uint32_t>(index)<=1U&&level>maximum) {
        // ELF binary64 constant 0x6274a8 is 0.086. int32 * 0.086 is finite and in range.
        const auto extension=static_cast<std::int32_t>(static_cast<double>(last)*0.086);
        const auto duration=wrap(1200U-static_cast<std::uint32_t>(maximum));
        const auto progress=wrap(static_cast<std::uint32_t>(level)-static_cast<std::uint32_t>(maximum));
        value=add(value,easing_zero_amplitude(0,extension,duration,progress));
    }
    return value;
}
// MagicShopData.GetDataIdForParamKey 0x1172564.
std::int32_t shop_id_for_parameter(std::int32_t index) {
    if(index>=0&&index<=4)return index;
    if(index>=10&&index<=12)return index-5;
    return -1;
}
// MagicShopData.GetAddValue 0x11725c4; no purchase-count clamping.
std::int32_t shop_parameter_addition(const ShopParameterUpgrade& upgrade) {
    return wrap(static_cast<std::uint32_t>(upgrade.value_per_purchase)*
                static_cast<std::uint32_t>(upgrade.purchases));
}
// MemberSystem.GetParamValue(CharaData,index) 0x115dd68 -> overload 0x115dd80.
std::int32_t member_base_parameter(const CharacterParameters& data,std::int32_t index,
                                   CharacterParameterHost& host) {
    return member_base_parameter_at_level(data,index,data.level,host);
}
std::int32_t member_base_parameter_at_level(const CharacterParameters& data,std::int32_t index,
                                            std::int32_t level,CharacterParameterHost& host) {
    const auto value=character_parameter(data,index,level,host);
    if((data.flags&2U)==0)return value; // CharaData.get_isMain 0x116d590.
    const auto id=shop_id_for_parameter(index);
    if(id==-1)return value;
    return add(value,shop_parameter_addition(host.shop_upgrade(id)));
}
}
