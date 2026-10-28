#pragma once
#include "combat_tables.hpp"
#include "../ecs/component_slots.hpp"

namespace saga::core {
// Portable component projections, not binary layouts. Only recovered fields are included.
struct HumanRecordLink final:Component {
    std::int32_t character_id=0,state=0,invincible_time=0; // +0x18/+0x24/+0x70.
    std::int32_t attack_frame=0,frame=0; // +0x30/+0x68.
    ComponentSlots* opponent=nullptr; // +0x38.
    std::int32_t formation_x=0,formation_z=0,formation_index=0; // +0x28/+0x2c/+0x74.
    std::int32_t level=0,experience=0;float dash_speed=0; // +0x1c/+0x20/+0x78.
    // Opaque script references until the component script interpreter is bound.
    std::shared_ptr<std::vector<std::shared_ptr<void>>> scripts; // +0x10.
};
struct EnemyRecordLink final:Component {
    std::int32_t enemy_id=0,level=0,flags=0; // +0x20/+0x10/+0x14.
    std::int32_t state=0,frame=0; // +0x18/+0x1c.
    ComponentSlots* opponent=nullptr; // +0x28; EnemySystem.OnHumanDead.
};
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
