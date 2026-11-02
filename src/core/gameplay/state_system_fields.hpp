#pragma once
#include "animation_pass.hpp"
#include "bubble_frame.hpp"
#include "button_frame.hpp"
#include "cell_system.hpp"
#include "combat_tables.hpp"
#include "cursor_frame.hpp"
#include "effect_frame.hpp"
#include "event_execution.hpp"
#include "event_progression.hpp"
#include "form_push.hpp"
#include "gauge_frame.hpp"
#include "human_frame.hpp"
#include "info_frame.hpp"
#include "input_frame.hpp"
#include "member_selection.hpp"
#include "modification_frame.hpp"
#include "parameter_experience.hpp"
#include "pass_initialization.hpp"
#include "presentation_forms.hpp"
#include "quest_camera.hpp"
#include "quest_frame.hpp"
#include "quest_rewards.hpp"
#include "render_frame.hpp"
#include "text_blocks.hpp"
#include "text_fit.hpp"
#include "text_font.hpp"
#include "text_reveal.hpp"

namespace saga::core {
// Explicit, shallow state schema. Each visitor receives a type tag followed by
// fields in declaration order, including borrowed bindings and shared references.
// The writer owns recursion, alias IDs, callback tags and canonical encoding.
// No gameplay queries, virtual service calls or container mutation occur here.
// Private containers/passes require separate const snapshots in their owners.
// EventCatalog is a vector alias: visit its rows with the writer's vector route,
// then use EventConditionRecord below; wrapping it in itself would recurse.

template<class Visitor> void visit_state_fields(const StageQuest& s,Visitor&& v){
    v("StageQuest",s.flags,s.distance,s.time_limit,s.money_rate,s.prisoner_maximum,
      s.rescued_prisoners,s.reward_treasure,s.cleared_reward_treasure,s.boss_id,s.boss_level,
      s.base_level,s.enemy_interval,s.enemy_maximum,s.enemy_total_limit,s.completed,
      s.events,s.fixed_treasure,s.normal_enemies,s.fixed_enemies,s.chip_images);
}
template<class Visitor> void visit_state_fields(const StagePlacementState& s,Visitor&& v){
    v("StagePlacementState",s.quest);
}
template<class Visitor> void visit_state_fields(const AttackButtonState& s,Visitor&& v){
    v("AttackButtonState",s.state,s.frame);
}
template<class Visitor> void visit_state_fields(const StageHumanStart& s,Visitor&& v){
    v("StageHumanStart",s.transitions);
}
template<class Visitor> void visit_state_fields(const StageQuestForm& s,Visitor&& v){
    v("StageQuestForm",s.scale,s.fade_frame);
}
template<class Visitor> void visit_state_fields(const StageRewards& s,Visitor&& v){
    v("StageRewards",s.experience,s.money,s.prisoners,s.treasure_ids);
}
template<class Visitor> void visit_state_fields(const DropCollectionState& s,Visitor&& v){
    v("DropCollectionState",s.rewards,s.boss_defeated,s.reward_box_picked_up,s.collecting);
}
template<class Visitor> void visit_state_fields(const StageStartState& s,Visitor&& v){
    v("StageStartState",static_cast<const StagePlacementState&>(s),s.area,s.stage,s.state,
      s.remaining_time,s.loop_counter,s.total_damage,s.frame,s.min_z_rate,s.max_z_rate,
      s.last_active_button,s.boss_started,s.move_front_pushed,s.retried,s.attack_attribute,
      s.collection,s.humans,s.members,s.enemies,s.attack_buttons,s.event_states,s.dialog,
      s.boss_fence,s.rewards());
}
template<class Visitor> void visit_state_fields(const StageForm&,Visitor&& v){v("StageForm");}
template<class Visitor> void visit_state_fields(const StageDialog& s,Visitor&& v){
    v("StageDialog",static_cast<const StageForm&>(s),s.kind,s.answer,s.reason);
}
template<class Visitor> void visit_state_fields(const VictoryForm&,Visitor&& v){v("VictoryForm");}
template<class Visitor> void visit_state_fields(const MovementControl&,Visitor&& v){v("MovementControl");}
template<class Visitor> void visit_state_fields(const PartyMembers& s,Visitor&& v){
    v("PartyMembers",s.entities(),s.version());
}
template<class Visitor> void visit_state_fields(const MemberRoster& s,Visitor&& v){
    v("MemberRoster",s.formation,s.members,s.sub_members);
}
template<class Visitor> void visit_state_fields(const PlayerWorldState& s,Visitor&& v){
    v("PlayerWorldState",s.cached_player,s.groups,s.loaded);
}
template<class Visitor> void visit_state_fields(const HumanFrameSystem& s,Visitor&& v){
    v("HumanFrameSystem",static_cast<const StageHumanStart&>(s),s.subset,s.map,
      s.loop_counter,s.stage,s.enemies,s.members);
}
template<class Visitor> void visit_state_fields(const EnemyDefeatCounters& s,Visitor&& v){
    v("EnemyDefeatCounters",s.defeated_enemies,s.remaining_enemies);
}
template<class Visitor> void visit_state_fields(const EnemyCreationSystem& s,Visitor&& v){
    v("EnemyCreationSystem",static_cast<const EnemyDefeatCounters&>(s),s.quest,s.stage,
      s.members,s.gauge,s.subset,s.created_enemies,s.loop_counter,s.maximum_by_section,s.created_by_section);
}
template<class Visitor> void visit_state_fields(const RandomConfiguration& s,Visitor&& v){
    v("RandomConfiguration",s.logic,s.fixed_seed);
}
template<class Visitor> void visit_state_fields(const WeightedLotteryState& s,Visitor&& v){
    v("WeightedLotteryState",s.table);
}
template<class Visitor> void visit_state_fields(const TownInitializationState& s,Visitor&& v){
    v("TownInitializationState",s.building_list,s.build_enable);
}
template<class Visitor> void visit_state_fields(const ProjectileSystemState& s,Visitor&& v){
    v("ProjectileSystemState",s.subset,s.pass);
}
template<class Visitor> void visit_state_fields(const CollisionSystemState& s,Visitor&& v){
    v("CollisionSystemState",s.subset,s.pass);
}
template<class Visitor> void visit_state_fields(const BattleSystemState& s,Visitor&& v){
    v("BattleSystemState",s.humans,s.monsters);
}
template<class Visitor> void visit_state_fields(const DebugVisualizationState& s,Visitor&& v){
    v("DebugVisualizationState",s.subset,s.flags);
}
template<class Visitor> void visit_state_fields(const DebugPathVisualization& s,Visitor&& v){
    v("DebugPathVisualization",s.visualize,s.wait_milliseconds);
}
template<class Visitor> void visit_state_fields(const SharedAnimationClip& s,Visitor&& v){
    v("SharedAnimationClip",s.frame,s.max_frame,s.sprites);
}
template<class Visitor> void visit_state_fields(const AnimationResource& s,Visitor&& v){
    v("AnimationResource",s.clips);
}
template<class Visitor> void visit_state_fields(const SharedAnimationGates& s,Visitor&& v){
    v("SharedAnimationGates",s.automatic,s.application_update);
}
template<class Visitor> void visit_state_fields(const AnimationSystemState& s,Visitor&& v){
    v("AnimationSystemState",s.subset,s.resources);
}
template<class Visitor> void visit_state_fields(const RenderSystemState& s,Visitor&& v){
    v("RenderSystemState",s.map,s.layers);
}
template<class Visitor> void visit_state_fields(const RenderQueue& s,Visitor&& v){
    v("RenderQueue",s.counts,s.buffers);
}
template<class Visitor> void visit_state_fields(const EffectSystemState& s,Visitor&& v){
    v("EffectSystemState",s.subset,s.expired);
}
template<class Visitor> void visit_state_fields(const ModificationSystemState& s,Visitor&& v){
    v("ModificationSystemState",s.subset,s.expired);
}
template<class Visitor> void visit_state_fields(const EmitterSystemState& s,Visitor&& v){
    v("EmitterSystemState",s.subset,s.expired,s.loop_counter);
}
template<class Visitor> void visit_state_fields(const ParameterExperienceSystem& s,Visitor&& v){
    v("ParameterExperienceSystem",s.queues);
}
template<class Visitor> void visit_state_fields(const GaugeDrawState& s,Visitor&& v){
    v("GaugeDrawState",s.alpha);
}
template<class Visitor> void visit_state_fields(const GaugeFrameState& s,Visitor&& v){
    v("GaugeFrameState",s.subset,s.pending);
}
template<class Visitor> void visit_state_fields(const BubbleFrameState& s,Visitor&& v){
    v("BubbleFrameState",s.subset,s.seen,s.pending);
}
template<class Visitor> void visit_state_fields(const ButtonFrameState& s,Visitor&& v){
    v("ButtonFrameState",s.subset);
}
template<class Visitor> void visit_state_fields(const PopupBar& s,Visitor&& v){
    v("PopupBar",s.id,s.x,s.y,s.bottom,s.frame,s.state_frame,s.state);
}
template<class Visitor> void visit_state_fields(const SlideBar& s,Visitor&& v){
    v("SlideBar",s.id,s.x,s.y,s.frame,s.state_frame,s.state);
}
template<class Visitor> void visit_state_fields(const InfoState& s,Visitor&& v){
    v("InfoState",s.slides,s.popups,s.emphasize_frame,s.emphasized,s.offset_bottom,s.dialog);
}
template<class Visitor> void visit_state_fields(const CursorPoint& s,Visitor&& v){
    v("CursorPoint",s.x,s.y);
}
template<class Visitor> void visit_state_fields(const CursorFocusEffect& s,Visitor&& v){
    v("CursorFocusEffect",s.entity_arg);
}
template<class Visitor> void visit_state_fields(const CursorState& s,Visitor&& v){
    v("CursorState",s.last_position,s.focus,s.tracking,s.focus_wait,s.catch_human);
}
template<class Visitor> void visit_state_fields(const QuestCameraSystem& s,Visitor&& v){
    v("QuestCameraSystem",s.camera,s.dirty,s.dragging,s.cursor_revision,s.pinch_scale,s.history,s.scale_range);
}
template<class Visitor> void visit_state_fields(const QuestCameraForm& s,Visitor&& v){
    v("QuestCameraForm",static_cast<const StageQuestForm&>(s.state),s.world,s.view,s.x,s.y,s.update_static_render,
      s.minimum_scale,s.pinch_value,s.form_scale_range);
}
template<class Visitor> void visit_state_fields(const CellSystemState& s,Visitor&& v){
    v("CellSystemState",s.subset,s.map,s.changed);
}
template<class Visitor> void visit_state_fields(const CellIndexState& s,Visitor&& v){
    v("CellIndexState",s.map,s.buckets);
}
template<class Visitor> void visit_state_fields(const InputFrameState& s,Visitor&& v){
    v("InputFrameState",s.subset,s.old_up,s.old_down,s.frame,s.last_down_frame);
}
template<class Visitor> void visit_state_fields(const InputControl& s,Visitor&& v){
    v("InputControl",s.group,s.value);
}
template<class Visitor> void visit_state_fields(const InputTouch& s,Visitor&& v){
    v("InputTouch",s.type,s.component);
}
template<class Visitor> void visit_state_fields(const QuestFrameState& s,Visitor&& v){
    v("QuestFrameState",s.loop_counter,s.frame,s.pinch_frame,s.pinching,s.dialog,s.fade_type,s.fade_state);
}
template<class Visitor> void visit_state_fields(const QuestRecord& s,Visitor&& v){
    v("QuestRecord",s.best_experience,s.best_money,s.challenges,s.clears,s.defeated,s.rescued,s.best_damage);
}
template<class Visitor> void visit_state_fields(const ItemStock& s,Visitor&& v){
    v("ItemStock",s.discovery_state,s.is_new,s.stock);
}
template<class Visitor> void visit_state_fields(const RecruitmentState& s,Visitor&& v){
    v("RecruitmentState",s.discovery_state,s.is_new,s.recruited);
}
template<class Visitor> void visit_state_fields(const SavedCharacterProgression& s,Visitor&& v){
    v("SavedCharacterProgression",s.level,s.experience);
}
template<class Visitor> void visit_state_fields(const CharacterNewGameState& s,Visitor&& v){
    v("CharacterNewGameState",s.name,s.status_definition,s.status,s.witch_experience,
      s.discovery_state,s.level,s.experience,s.living_address,s.recruited,s.sorties,s.equipped_cane,s.is_new);
}
template<class Visitor> void visit_state_fields(const ShopNewGameState& s,Visitor&& v){
    v("ShopNewGameState",s.discovery_state,s.purchases,s.is_new);
}
template<class Visitor> void visit_state_fields(const EquipmentNewGameState& s,Visitor&& v){
    v("EquipmentNewGameState",s.level,s.inventory);
}
template<class Visitor> void visit_state_fields(const QuestCharacterResult& s,Visitor&& v){
    v("QuestCharacterResult",s.character,s.levels_gained,s.added_experience,s.display_level,s.display_experience,s.level_up_frame);
}
template<class Visitor> void visit_state_fields(const QuestRewardResult& s,Visitor&& v){
    v("QuestRewardResult",s.experience,s.money,s.crystals,s.prisoners,s.weapons,s.characters,s.clothes,s.hats,s.members);
}
template<class Visitor> void visit_state_fields(const EventContext& s,Visitor&& v){
    v("EventContext",s.args,s.argi);
}
template<class Visitor> void visit_state_fields(const DelayedEvent& s,Visitor&& v){
    v("DelayedEvent",s.script_id,s.line,s.wait,s.hour,s.talk_character,s.context);
}
template<class Visitor> void visit_state_fields(const EventConditionRecord& s,Visitor&& v){
    v("EventConditionRecord",s.execution_count,s.terms,s.id,s.name,s.timing,s.effects);
}
template<class Visitor> void visit_state_fields(const DecodedEventExecutionState& s,Visitor&& v){
    v("DecodedEventExecutionState",s.catalog,s.normal_save_present,s.normal_save_strings,
      s.quest_data_count,s.print_enabled,s.diagnostic);
}
template<class Visitor> void visit_state_fields(const DiscoveryState& s,Visitor&& v){
    v("DiscoveryState",s.state,s.is_new);
}
template<class Visitor> void visit_state_fields(const EventProgression& s,Visitor&& v){
    v("EventProgression",s.money,s.crystals,s.crystal_subtraction_mode,s.areas,s.quests);
}
template<class Visitor> void visit_state_fields(const QuestConditionRecord& s,Visitor&& v){
    v("QuestConditionRecord",s.challenges,s.clears,s.rescued);
}
template<class Visitor> void visit_state_fields(const QuestConditionState& s,Visitor&& v){
    v("QuestConditionState",s.flags,s.completed,s.record);
}
template<class Visitor> void visit_state_fields(const CharacterConditionState& s,Visitor&& v){
    v("CharacterConditionState",s.sub,s.flags);
}
template<class Visitor> void visit_state_fields(const DecodedEventConditionState& s,Visitor&& v){
    v("DecodedEventConditionState",s.context_present,s.context_character_count,s.quests,
      s.area_states,s.characters,s.town_flags,s.buildings,s.money,s.crystals,s.stamina,
      s.high_score,s.hero_level,s.town_capacity,s.debug_enabled,s.debug_rescue_count);
}
template<class Visitor> void visit_state_fields(const FormPushState& s,Visitor&& v){
    v("FormPushState",s.identity,s.manager,s.dialog);
}
template<class Visitor> void visit_state_fields(const InitialFormSelection& s,Visitor&& v){
    v("InitialFormSelection",s.type,s.x,s.y,s.selection);
}
template<class Visitor> void visit_state_fields(const TalkDefinition& s,Visitor&& v){
    v("TalkDefinition",s.id,s.character_index,s.texts);
}
template<class Visitor> void visit_state_fields(const PreparedTalkForm& s,Visitor&& v){
    v("PreparedTalkForm",s.initial,s.character_index,s.texts,s.text_index,s.next_index,s.arguments);
}
template<class Visitor> void visit_state_fields(const TextRevealState& s,Visitor&& v){
    v("TextRevealState",s.length,s.position,s.speed,s.begin_time,s.time_offset,s.last_frame);
}
template<class Visitor> void visit_state_fields(const TextFontState& s,Visitor&& v){
    v("TextFontState",s.requested_size,s.size,s.use_preset,s.adjusted_size,s.scale,s.font_scale);
}
template<class Visitor> void visit_state_fields(const TextFitState& s,Visitor&& v){
    v("TextFitState",s.width,s.height,s.text_width,s.auto_scale,s.multiline,s.font_size_adjusted);
}
template<class Visitor> void visit_state_fields(const TextBlockConfig& s,Visitor&& v){
    v("TextBlockConfig",s.width,s.height,s.line_space,s.font_size,s.color,s.alignment,s.japanese_style);
}
template<class Visitor> void visit_state_fields(const LayoutTextBlock& s,Visitor&& v){
    v("LayoutTextBlock",s.text,s.x,s.y,s.width,s.font_size,s.color,s.source_begin,s.source_length);
}
template<class Visitor> void visit_state_fields(const TextBlockLayout& s,Visitor&& v){
    v("TextBlockLayout",s.processed_source,s.blocks,s.text_width);
}
} // namespace saga::core
