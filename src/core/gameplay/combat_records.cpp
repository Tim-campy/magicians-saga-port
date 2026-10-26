#include "combat_records.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& component(const ComponentSlots& entity,std::int32_t slot) {
    auto* raw=entity.get(slot);
    if(!raw)throw std::invalid_argument("missing combat record component");
    auto* typed=dynamic_cast<T*>(raw);
    if(!typed)throw std::invalid_argument("wrong combat record component type");
    return *typed;
}
template<class T>const T* record(const std::vector<std::optional<T>>& table,std::int32_t id) {
    const auto& value=table.at(static_cast<std::size_t>(id));return value?&*value:nullptr;
}
EventIntegers& equipment_ids(const ComponentSlots& entity) {
    auto& links=component<EquipmentRecordLinks>(entity,combat_equipment_slot);
    if(!links.ids)throw std::invalid_argument("null equipment ID array");
    return *links.ids;
}
}
// HumanComponent.get_data 0x113ffc4 and EnemyComponent.get_data 0x113efb0.
const CharacterProgressionRecord* resolve_combat_character(const ComponentSlots& entity,const CombatRecordTables& tables) {
    return record(tables.characters,component<HumanRecordLink>(entity,combat_human_slot).character_id);
}
const EnemyRecord* resolve_combat_enemy(const ComponentSlots& entity,const CombatRecordTables& tables) {
    return record(tables.enemies,component<EnemyRecordLink>(entity,combat_enemy_slot).enemy_id);
}
// EquipComponent.get_weapon/get_shield/get_hat/get_cloth 0x113f374..0x113f5b0.
const EquipmentCombatRecord* resolve_combat_equipment(const ComponentSlots& entity,EquipmentSlot slot,const CombatRecordTables& tables) {
    const auto id=equipment_ids(entity).at(static_cast<std::size_t>(slot));
    switch(slot) {
    case EquipmentSlot::weapon:case EquipmentSlot::shield:return record(tables.equipment,id);
    case EquipmentSlot::hat:return record(tables.hats,id);
    case EquipmentSlot::cloth:return record(tables.cloth,id);
    }
    throw std::out_of_range("invalid equipment slot");
}
// EquipComponent.SetWeapon(int) 0x113f5c8 / SetShield(int) 0x113f608 and analogous slots.
void set_combat_equipment(ComponentSlots& entity,EquipmentSlot slot,std::int32_t id) {
    equipment_ids(entity).at(static_cast<std::size_t>(slot))=id;
}
}
