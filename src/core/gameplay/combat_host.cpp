#include "combat_host.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& require(T* pointer) {
    if(!pointer)throw std::invalid_argument("missing combat component or record");
    return *pointer;
}
class Member final:public MemberParameterHost {
public:
    Member(CombatEntity& entity,CharacterParameterHost& characters):entity_(entity),characters_(characters){}
    std::int32_t base_parameter(std::int32_t index) override {
        return member_base_parameter(require(require(entity_.human).character),index,characters_);
    }
    const EquipmentParameters& equipment(EquipmentSlot slot) override {
        return require(require(entity_.human).equipment.at(static_cast<std::size_t>(slot))).parameters;
    }
private:
    CombatEntity& entity_;
    CharacterParameterHost& characters_;
};
DamageParameter* parameter_slot(CombatEntity& entity,std::size_t index) {
    return require(entity.parameters).at(index);
}
}
CombatEntity& ResolvedDamageHost::entity(CombatSide side) {
    return side==CombatSide::attacker?attacker_:receiver_;
}
CombatKind ResolvedDamageHost::kind(CombatSide side) {
    const auto& value=entity(side);
    return value.human?CombatKind::human:value.enemy?CombatKind::enemy:CombatKind::other;
}
std::int32_t ResolvedDamageHost::parameter(CombatSide side,std::int32_t index) {
    auto& value=entity(side);
    if(value.human){Member member(value,characters_);return member_total_parameter(member,index);}
    if(value.enemy)return enemy_parameter(require(value.enemy->data),index,value.enemy->level,enemy_maxima_);
    return 0;
}
DamageParameter* ResolvedDamageHost::receiver_parameter_zero(){return parameter_slot(receiver_,0);}
ReceiverDefence ResolvedDamageHost::receiver_defence() {
    if(receiver_.human) {
        const auto defence=parameter(CombatSide::receiver,2);
        const auto attribute=require(require(receiver_.human).equipment.at(0)).attribute;
        Member member(receiver_,characters_);
        const auto bonus=member_attribute_bonus(member,attribute);
        return {defence,attribute,bonus};
    }
    if(receiver_.enemy) {
        const auto defence=parameter(CombatSide::receiver,2);
        return {defence,require(require(receiver_.enemy).data).attribute,0};
    }
    return {};
}
std::int32_t ResolvedDamageHost::attacker_parameter_one_rate() {
    return damage_parameter_rate(require(parameter_slot(attacker_,1)));
}
void ResolvedDamageHost::after_damage(const DamageNumbers& numbers) {
    presentation_.effects(receiver_,numbers);
    presentation_.send_damage(receiver_,numbers.amount);
    // Re-read live flags after listeners, as in 0x11507bc..0x11507fc.
    if(config_.debug&&config_.damage_diagnostics)presentation_.diagnostics(numbers);
}
void record_stage_damage(std::int32_t& total,const CombatEntity* receiver,std::int32_t amount) {
    if(!require(receiver).enemy)return; // Presence, independently of human precedence in stat lookup.
    const auto bits=static_cast<std::uint32_t>(total)+static_cast<std::uint32_t>(amount);
    total=static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):
                                  static_cast<std::int64_t>(bits)-0x100000000LL);
}
}
