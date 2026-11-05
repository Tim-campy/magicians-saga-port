#pragma once
#include "animation_resources.hpp"
#include "bullet_factory.hpp"
#include "combat_tables.hpp"
#include "human_factory.hpp"
#include "pass_initialization.hpp"
#include "quest_catalog.hpp"
#include "sprite_matrix.hpp"

namespace saga::core {
// Public catalog/cache checkpoint fields. The visitor owns recursion and aliases.
// Reset states, shared SEB clocks, RenderQueue and PlayerWorldState are provided
// by state_system_fields.hpp; component/base payloads by state_component_fields.hpp.
// ImageAnimationGroup/Values are vector aliases and use the generic sequence route.
template<class Visitor>inline void visit_state_fields(const CharacterProgressionRecord& s,Visitor&& v){
    v("CharacterProgressionRecord",s.id,s.required_experience_rate,s.weapon_id,s.initial_level,s.flags,s.parameters,s.image,s.base_name);
}
template<class Visitor>inline void visit_state_fields(const EquipmentCombatRecord& s,Visitor&& v){
    v("EquipmentCombatRecord",s.id,s.attribute,s.flags,s.parameters,s.range,s.distance,s.burst_count,s.use_mp,
      s.type,s.bullet_id,s.max_height,s.bullet_speed,s.image,s.resource,s.initial_level,s.name);
}
template<class Visitor>inline void visit_state_fields(const MagicShopCombatRecord& s,Visitor&& v){
    v("MagicShopCombatRecord",s.id,s.category,s.value_per_purchase,s.flags);
}
template<class Visitor>inline void visit_state_fields(const EnemyRecord& s,Visitor&& v){
    v("EnemyRecord",s.id,s.name,s.enemy_seb,s.experience,s.treasures,s.voice,s.attribute,s.resource,
      s.image,s.seb,s.frame,s.rank,s.parameters,s.flags);
}
template<class Visitor>inline void visit_state_fields(const BulletImpactData& s,Visitor&& v){
    v("BulletImpactData",s.category,s.attribute,s.effect_image,s.flags);
}
template<class Visitor>inline void visit_state_fields(const BulletRecord& s,Visitor&& v){
    v("BulletRecord",static_cast<const BulletImpactData&>(s),s.id,s.image,s.trail_image,s.resource,s.impact_resource);
}
template<class Visitor>inline void visit_state_fields(const TreasureRecord& s,Visitor&& v){
    v("TreasureRecord",s.id,s.name,s.resource,s.image,s.experience,s.money,s.crystals,s.weapon_id,s.character_id,s.cloth_id,s.hat_id,s.flags);
}
template<class Visitor>inline void visit_state_fields(const LevelTableRecord& s,Visitor&& v){
    v("LevelTableRecord",s.id,s.level,s.experience,s.shop_parameters,s.unlock_money,s.unlock_crystal,s.unlock_web_page,s.unlock_rescue);
}
template<class Visitor>inline void visit_state_fields(const QuestDefinition& s,Visitor&& v){
    v("QuestDefinition",s.id,s.image,s.need_stamina,s.name,s.resource,s.stage,s.discovery_state,s.is_new,s.record,s.stage_detail);
}
template<class Visitor>inline void visit_state_fields(const AreaDefinition& s,Visitor&& v){
    v("AreaDefinition",s.id,s.image,s.flags,s.name,s.stages);
}
template<class Visitor>inline void visit_state_fields(const TownDefinition& s,Visitor&& v){
    v("TownDefinition",s.id,s.first_build,s.flags,s.open_term,s.rock_term);
}
template<class Visitor>inline void visit_state_fields(const PartyCharacter& s,Visitor&& v){
    v("PartyCharacter",s.id,s.flags,s.saved,s.image,s.weapon_id,s.parameters);
}
template<class Visitor>inline void visit_state_fields(const HumanHat& s,Visitor&& v){
    v("HumanHat",s.flags,s.image);
}
template<class Visitor>inline void visit_state_fields(const ShopParameterUpgrade& s,Visitor&& v){
    v("ShopParameterUpgrade",s.value_per_purchase,s.purchases);
}
template<class Visitor>inline void visit_state_fields(const CharacterParameters& s,Visitor&& v){
    v("CharacterParameters",s.values,s.level,s.flags);
}
template<class Visitor>inline void visit_state_fields(const EquipmentParameters& s,Visitor&& v){
    v("EquipmentParameters",s.level,s.values);
}
template<class Visitor>inline void visit_state_fields(const EntityIdState& s,Visitor&& v){
    v("EntityIdState",s.counter);
}
template<class Visitor>inline void visit_state_fields(const ImageDrawingPosition& s,Visitor&& v){
    v("ImageDrawingPosition",s.x,s.y,s.scale_x,s.scale_y,s.angle);
}
template<class Visitor>inline void visit_state_fields(const SpriteMatrix& s,Visitor&& v){
    v("SpriteMatrix",s.values,s.concat_work,s.rotation_work);
}
template<class Visitor>inline void visit_state_fields(const CombatEquipment& s,Visitor&& v){
    v("CombatEquipment",s.parameters,s.attribute);
}
template<class Visitor>inline void visit_state_fields(const CombatHuman& s,Visitor&& v){
    v("CombatHuman",s.character,s.equipment);
}
template<class Visitor>inline void visit_state_fields(const CombatEnemy& s,Visitor&& v){
    v("CombatEnemy",s.data,s.level);
}
template<class Visitor>inline void visit_state_fields(const CombatEntity& s,Visitor&& v){
    v("CombatEntity",s.human,s.enemy,s.parameters);
}
template<class Visitor>inline void visit_state_fields(const CombatConfiguration& s,Visitor&& v){
    v("CombatConfiguration",s.debug,s.damage_diagnostics);
}
template<class Visitor>inline void visit_state_fields(const AttributeAffinityTables& s,Visitor&& v){
    v("AttributeAffinityTables",s.pairs,s.rates);
}
template<class Visitor>inline void visit_state_fields(const CombatRecordTables& s,Visitor&& v){
    v("CombatRecordTables",s.characters,s.enemies,s.equipment,s.hats,s.cloth);
}
template<class Visitor>inline void visit_state_fields(const CombatSavedLevels& s,Visitor&& v){
    v("CombatSavedLevels",s.characters,s.equipment,s.hats,s.cloth);
}
template<class Visitor>inline void visit_state_fields(const MissionSelectionRecords& s,Visitor&& v){
    v("MissionSelectionRecords",s.area,s.quest);
}
template<class Visitor>inline void visit_state_fields(const ResourceCatalogIndex& s,Visitor&& v){
    v("ResourceCatalogIndex",s.ids,s.rows);
}
template<class Resource,class Visitor>inline void visit_state_fields(const ApplicationResourceFields<Resource>& s,Visitor&& v){
    v("ApplicationResourceFields",s.characters,s.common,s.enemies,s.equipment,s.quest,s.residents,s.selection,
      s.chips,s.bullets,s.effects,s.hats,s.weapons,s.gauges);
}
}
