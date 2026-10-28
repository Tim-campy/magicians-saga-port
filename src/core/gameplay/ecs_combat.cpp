#include "ecs_combat.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& require(T* value){if(!value)throw std::invalid_argument("missing combat data");return *value;}
template<class T>T& component(const ComponentSlots& entity,std::int32_t slot) {
    return require(dynamic_cast<T*>(entity.get(slot)));
}
class Member final:public MemberParameterHost {
public:
    Member(ComponentSlots& entity,const CombatRecordTables& records,const CombatSavedLevels& levels,
           CharacterParameterHost& characters):entity_(entity),records_(records),levels_(levels),characters_(characters){}
    std::int32_t base_parameter(std::int32_t key) override {
        const auto& record=require(resolve_combat_character(entity_,records_));
        const auto id=component<HumanRecordLink>(entity_,combat_human_slot).character_id;
        const auto value=bind_character_parameters(record,levels_.characters.at(static_cast<std::size_t>(id)));
        return member_base_parameter(value,key,characters_);
    }
    const EquipmentParameters& equipment(EquipmentSlot slot) override {
        const auto& record=require(resolve_combat_equipment(entity_,slot,records_));
        const auto id=require(component<EquipmentRecordLinks>(entity_,combat_equipment_slot).ids.get()).at(static_cast<std::size_t>(slot));
        const auto& levels=slot==EquipmentSlot::hat?levels_.hats:slot==EquipmentSlot::cloth?levels_.cloth:levels_.equipment;
        scratch_=bind_equipment_parameters(record,levels.at(static_cast<std::size_t>(id))).parameters;
        return scratch_;
    }
private:
    ComponentSlots& entity_;
    const CombatRecordTables& records_;
    const CombatSavedLevels& levels_;
    CharacterParameterHost& characters_;
    EquipmentParameters scratch_;
};
}
ComponentSlots& EcsDamageHost::entity(CombatSide side){return side==CombatSide::attacker?attacker_:receiver_;}
CombatKind EcsDamageHost::kind(CombatSide side) {
    const auto& value=entity(side);
    return value.contains(combat_human_slot)?CombatKind::human:value.contains(combat_enemy_slot)?CombatKind::enemy:CombatKind::other;
}
std::int32_t EcsDamageHost::parameter(CombatSide side,std::int32_t key) {
    auto& value=entity(side);
    if(value.contains(combat_human_slot)){Member member(value,records_,levels_,characters_);return member_total_parameter(member,key);}
    if(value.contains(combat_enemy_slot)) {
        const auto& record=require(resolve_combat_enemy(value,records_));
        return enemy_parameter(record,key,component<EnemyRecordLink>(value,combat_enemy_slot).level,enemy_maxima_);
    }
    return 0;
}
DamageParameter* resolve_combat_parameter(const ComponentSlots& entity,std::int32_t key) {
    return require(component<CombatParameterLinks>(entity,combat_parameter_slot).values).at(key);
}
std::int32_t ecs_member_total_parameter(ComponentSlots& e,std::int32_t key,const CombatRecordTables& records,
                                      const CombatSavedLevels& levels,CharacterParameterHost& characters){
    Member member(e,records,levels,characters);return member_total_parameter(member,key);
}
DamageParameter* EcsDamageHost::receiver_parameter_zero(){return resolve_combat_parameter(receiver_,0);}
std::int32_t EcsDamageHost::attacker_parameter_one_rate(){return damage_parameter_rate(require(resolve_combat_parameter(attacker_,1)));}
ReceiverDefence EcsDamageHost::receiver_defence() {
    if(receiver_.contains(combat_human_slot)) {
        const auto defence=parameter(CombatSide::receiver,2);
        const auto attribute=require(resolve_combat_equipment(receiver_,EquipmentSlot::weapon,records_)).attribute;
        Member member(receiver_,records_,levels_,characters_);
        return {defence,attribute,member_attribute_bonus(member,attribute)};
    }
    if(receiver_.contains(combat_enemy_slot)) {
        const auto defence=parameter(CombatSide::receiver,2);
        return {defence,require(resolve_combat_enemy(receiver_,records_)).attribute,0};
    }
    return {};
}
void EcsDamageHost::after_damage(const DamageNumbers& numbers) {
    presentation_.effects(receiver_,numbers);
    presentation_.send_damage(receiver_,numbers.amount);
    if(config_.debug&&config_.damage_diagnostics)presentation_.diagnostics(numbers);
}
void record_ecs_stage_damage(std::int32_t& total,const ComponentSlots& receiver,std::int32_t amount) {
    if(!receiver.contains(combat_enemy_slot))return;
    const auto bits=static_cast<std::uint32_t>(total)+static_cast<std::uint32_t>(amount);
    total=static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);
}
}
