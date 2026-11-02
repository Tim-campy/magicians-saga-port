#pragma once
#include "bubble_frame.hpp"
#include "bullet_factory.hpp"
#include "button_frame.hpp"
#include "drop_factory.hpp"
#include "effect_factory.hpp"
#include "gauge_frame.hpp"
#include "human_factory.hpp"
#include "mission_map.hpp"
#include "quest_camera.hpp"

namespace saga::core {
// Explicit checkpoint schema for the reached ECS payloads. Reference traversal,
// alias identities and unknown-type rejection belong to the checkpoint visitor.
// Markers require an explicit host binding; there is deliberately no Component overload.
template<class Visitor>inline void visit_state_fields(const MemberExperience& s,Visitor&& v){
    v("MemberExperience",s.level,s.experience);
}
template<class Visitor>inline void visit_state_fields(const DamageParameter& s,Visitor&& v){
    v("DamageParameter",s.value,s.maximum,s.extra,s.extra_maximum,s.id,s.level,s.max_level,
      s.experience,s.max_experience,s.experience_buffer);
}
template<class Visitor>inline void visit_state_fields(const SpriteAnimation& s,Visitor&& v){
    v("SpriteAnimation",s.resource,s.clip,s.frame);
}
template<class Visitor>inline void visit_state_fields(const ImageArrays& s,Visitor&& v){
    v("ImageArrays",s.textures,s.resources,s.u,s.v,s.width,s.height);
}
template<class Visitor>inline void visit_state_fields(const AnimationPlayback& s,Visitor&& v){
    v("AnimationPlayback",s.rate,s.return_clip);
}
template<class Visitor>inline void visit_state_fields(const StageGeometry& s,Visitor&& v){
    v("StageGeometry",s.step_x,s.step_z,s.height);
}
template<class Visitor>inline void visit_state_fields(const ProjectilePoint& s,Visitor&& v){
    v("ProjectilePoint",s.x,s.y,s.z);
}
template<class Visitor>inline void visit_state_fields(const HumanScriptCache& s,Visitor&& v){
    v("HumanScriptCache",s.array,s.count);
}
template<class Visitor>inline void visit_state_fields(const ContactPosition& s,Visitor&& v){
    v("ContactPosition",s.x,s.y,s.z,s.offset_x,s.offset_y,s.offset_z,s.parent);
}
template<class Visitor>inline void visit_state_fields(const ProjectileSpeed& s,Visitor&& v){
    v("ProjectileSpeed",s.x,s.y,s.z);
}
template<class Visitor>inline void visit_state_fields(const EntitySprite& s,Visitor&& v){
    v("EntitySprite",s.animation,s.layers);
}
template<class Visitor>inline void visit_state_fields(const QuestCameraComponent& s,Visitor&& v){
    v("QuestCameraComponent",s.angle,s.scale);
}
template<class Visitor>inline void visit_state_fields(const EntityDepth& s,Visitor&& v){
    v("EntityDepth",s.value,s.extra);
}
template<class Visitor>inline void visit_state_fields(const CellComponent& s,Visitor&& v){
    v("CellComponent",s.xi,s.yi,s.parent,s.children);
}
template<class Visitor>inline void visit_state_fields(const DefeatColor& s,Visitor&& v){
    v("DefeatColor",s.values);
}
template<class Visitor>inline void visit_state_fields(const EntityImage& s,Visitor&& v){
    v("EntityImage",s.arrays);
}
template<class Visitor>inline void visit_state_fields(const EquipmentRecordLinks& s,Visitor&& v){
    v("EquipmentRecordLinks",s.ids);
}
template<class Visitor>inline void visit_state_fields(const MapChipComponent& s,Visitor&& v){
    v("MapChipComponent",s.data_id,s.staff_id,s.visiting_human_ids);
}
template<class Visitor>inline void visit_state_fields(const EntityPlayback& s,Visitor&& v){
    v("EntityPlayback",s.state);
}
template<class Visitor>inline void visit_state_fields(const MapComponent& s,Visitor&& v){
    v("MapComponent",static_cast<const StageGeometry&>(s),s.width,s.size_width,s.size_height,
      s.area_width,s.area_height,s.chips,s.areas);
}
template<class Visitor>inline void visit_state_fields(const ContactDirection& s,Visitor&& v){
    v("ContactDirection",s.direction,s.locked);
}
template<class Visitor>inline void visit_state_fields(const HumanRecordLink& s,Visitor&& v){
    v("HumanRecordLink",static_cast<const MemberExperience&>(s),s.character_id,s.state,s.invincible_time,
      s.attack_frame,s.frame,s.opponent,s.formation_x,s.formation_z,s.formation_index,s.dash_speed,s.scripts);
}
template<class Visitor>inline void visit_state_fields(const ProjectileModification& s,Visitor&& v){
    v("ProjectileModification",s.type,s.x,s.y,s.z,s.scale_x,s.scale_y,s.angle,s.frame,s.max_frame,
      s.anchor,s.destroy,s.loop,s.alpha);
}
template<class Visitor>inline void visit_state_fields(const EntityButton& s,Visitor&& v){
    v("EntityButton",s.id,s.value,s.dynamic,s.auto_destroy,s.scale,s.text,s.text_offset_x,s.text_offset_y,
      s.parent,s.offset_x,s.offset_y,s.hover);
}
template<class Visitor>inline void visit_state_fields(const EntityBubble& s,Visitor&& v){
    v("EntityBubble",s.data_id,s.type,s.frame,s.frame_max,s.offset_x,s.offset_y,s.offset_z,s.parent);
}
template<class Visitor>inline void visit_state_fields(const EntityGauge& s,Visitor&& v){
    v("EntityGauge",s.type,s.from,s.to,s.maximum,s.frame,s.frame_max,s.offset_x,s.offset_y,s.offset_z,
      s.flags,s.option,s.option2,s.parent);
}
template<class Visitor>inline void visit_state_fields(const HumanAI& s,Visitor&& v){
    v("HumanAI",s.scripts,s.state);
}
template<class Visitor>inline void visit_state_fields(const GarbageLifetime& s,Visitor&& v){
    v("GarbageLifetime",s.frames);
}
template<class Visitor>inline void visit_state_fields(const CombatParameterLinks& s,Visitor&& v){
    v("CombatParameterLinks",s.values,s.storage,s.extras);
}
template<class Visitor>inline void visit_state_fields(const EnemyRecordLink& s,Visitor&& v){
    v("EnemyRecordLink",s.enemy_id,s.level,s.flags,s.state,s.frame,s.opponent);
}
template<class Visitor>inline void visit_state_fields(const EffectComponent& s,Visitor&& v){
    v("EffectComponent",s.type,s.value,s.frame,s.max_frame,s.depth,s.parent);
}
template<class Visitor>inline void visit_state_fields(const ProjectileLink& s,Visitor&& v){
    v("ProjectileLink",s.parent,s.start,s.end,s.speed,s.max_height,s.frame,s.max_frame);
}
template<class Visitor>inline void visit_state_fields(const BulletComponent& s,Visitor&& v){
    v("BulletComponent",s.data_id);
}
template<class Visitor>inline void visit_state_fields(const EmitterComponent& s,Visitor&& v){
    v("EmitterComponent",s.category,s.type,s.interval,s.frame,s.maximum,s.argument);
}
template<class Visitor>inline void visit_state_fields(const ProjectileCollision& s,Visitor&& v){
    v("ProjectileCollision",s.radius,s.offset_x,s.offset_y,s.offset_z,s.visited);
}
template<class Visitor>inline void visit_state_fields(const ProjectileDamage& s,Visitor&& v){
    v("ProjectileDamage",s.value,s.attribute,s.parent);
}
template<class Visitor>inline void visit_state_fields(const DropComponent& s,Visitor&& v){
    v("DropComponent",s.type,s.value,s.x0,s.z0,s.frame);
}
}
