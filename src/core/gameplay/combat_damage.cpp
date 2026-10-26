#include "combat_damage.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t signed_bits(std::uint32_t bits) {
    return static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):
                                    static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::int32_t add(std::int32_t a,std::int32_t b) {
    return signed_bits(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));
}
std::int32_t subtract(std::int32_t a,std::int32_t b) {
    return signed_bits(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));
}
std::int32_t percent(std::int32_t a,std::int32_t b) {
    return signed_bits(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b))/100;
}
DamageParameter& require_parameter(DamageParameter* parameter) {
    if(!parameter)throw std::invalid_argument("missing captured receiver parameter zero");
    return *parameter;
}
}
bool apply_combat_damage(std::int32_t attack,std::int32_t attribute,
                         RandomSource& random,DamageHost& host) {
    // Receiver lookup precedes attacker lookup (0x114fd10..0x114fe94).
    const auto evade=host.parameter(CombatSide::receiver,9);
    const bool human=host.kind(CombatSide::attacker)==CombatKind::human;
    const auto accuracy=host.parameter(CombatSide::attacker,7);
    if(!random_hits(random,hit_threshold(human,accuracy,evade))) {
        host.show_miss();
        return false;
    }
    auto* const target=host.receiver_parameter_zero();
    const auto defence=host.receiver_defence();
    const auto critical_parameter=host.parameter(CombatSide::attacker,8);
    const auto variation=random_inclusive(random,95,105);
    const auto& tables=host.affinity_tables();
    const auto affinity=attribute_affinity(tables,attribute,defence.attribute);
    const auto initial=subtract(percent(variation,attack),defence.parameter2/4);
    const auto adjusted=percent(initial,add(defence.bonus,100));
    auto damage=percent(tables.rates.at(static_cast<std::size_t>(affinity)),adjusted);
    const bool critical=random_hits(random,critical_parameter/2);
    if(critical)damage=percent(damage,170);
    else if(host.kind(CombatSide::attacker)==CombatKind::human)
        damage=percent(damage,noncritical_human_damage_factor(host.attacker_parameter_one_rate()));
    if(defence.attribute==attribute)
        damage=percent(damage,matching_attribute_damage_factor(defence.bonus));
    const auto receiver=host.kind(CombatSide::receiver);
    if(receiver==CombatKind::human)damage=std::max(damage,random_inclusive(random,1,3));
    else if(receiver==CombatKind::enemy) {
        const auto lower=affinity<3?3-affinity:1;
        const auto upper=affinity<3?6-2*affinity:1;
        damage=std::max(damage,random_inclusive(random,lower,upper));
    }
    if(!host.debug_enabled()||host.kind(CombatSide::attacker)==CombatKind::human) {
        auto& parameter=require_parameter(target);
        parameter.value=subtract_parameter(parameter.value,parameter.maximum,damage).value;
    }
    host.after_damage({damage,affinity,critical});
    const auto& parameter=require_parameter(target);
    return add(parameter.value,parameter.extra)<1;
}
}
