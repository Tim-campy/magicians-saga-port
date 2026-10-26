#pragma once
#include "combat_tables.hpp"
#include "../ecs/component_slots.hpp"

namespace saga::core {
// Portable component projections, not binary layouts. Only recovered fields are included.
struct HumanRecordLink final:Component {std::int32_t character_id=0;}; // HumanComponent +0x18.
struct EnemyRecordLink final:Component {std::int32_t enemy_id=0,level=0;}; // +0x20 / +0x10.
struct EquipmentRecordLinks final:Component {EventInstruction ids;}; // EquipComponent +0x10, nullable.
inline constexpr std::int32_t combat_equipment_slot=8,combat_human_slot=14,
    combat_parameter_slot=23,combat_enemy_slot=24;
struct CombatRecordTables {
    const CharacterProgressionTable& characters;
    const EnemyTable& enemies;
    const EquipmentCombatTable &equipment,&hats,&cloth;
};
// Null table slots return null; invalid IDs, absent components or wrong slot types fault.
const CharacterProgressionRecord* resolve_combat_character(const ComponentSlots&,const CombatRecordTables&);
const EnemyRecord* resolve_combat_enemy(const ComponentSlots&,const CombatRecordTables&);
const EquipmentCombatRecord* resolve_combat_equipment(const ComponentSlots&,EquipmentSlot,const CombatRecordTables&);
// Integer setters only store the ID. Record validity is checked by a later getter.
void set_combat_equipment(ComponentSlots&,EquipmentSlot,std::int32_t id);
}
