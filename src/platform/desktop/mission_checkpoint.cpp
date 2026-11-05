#include "mission_checkpoint.hpp"
#include "mission_profile.hpp"
#include "mission_input.hpp"
#include "mission_stage.hpp"
#include "mission_results.hpp"
#include "core/ecs/state_archive.hpp"
#include "core/gameplay/state_component_fields.hpp"
#include "core/gameplay/state_system_fields.hpp"
#include "core/gameplay/state_data_fields.hpp"
#include "core/gameplay/state_archive_access.hpp"
#include "core/gameplay/character_parameter_set.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <type_traits>
#define XXH_INLINE_ALL
#include <xxhash.h>

namespace saga::desktop {
using namespace core;
namespace {
std::uint64_t digest(const void* data,std::size_t bytes){return XXH64(data,bytes,0);}
std::string hex(std::uint64_t value){std::ostringstream out;out<<std::hex<<std::setfill('0')<<std::setw(16)<<value;return out.str();}
template<class>constexpr bool unsupported_state=false;
struct FieldProbe {template<class...T>void operator()(const char*,const T&...)const;};
template<class T,class=void>struct HasStateFields:std::false_type{};
template<class T>struct HasStateFields<T,std::void_t<decltype(visit_state_fields(std::declval<const T&>(),FieldProbe{}))>>:std::true_type{};
}

// Owns one deterministic graph traversal. Every persistent object is anchored by
// schema type and first traversal order; allocation addresses never enter bytes.
struct MissionCheckpoint {
    const MissionProfile& p;StateArchive archive;
    std::set<std::uint64_t> expanded;
    std::map<const void*,std::function<void()>> opaque;
    std::map<const void*,std::string> services;
    std::size_t logic_end=0;
    bool presentation=false;

    explicit MissionCheckpoint(const MissionProfile& profile):p(profile){bind_objects();}
    template<class...T>void fields(const T&...x){(value(x),...);}
    template<class T,class F>void object(const char* tag,const T& x,F&& write){
        const auto id=archive.reference(tag,&x);const bool first=expanded.insert(id).second;archive.number(first);if(first)write();
    }
    template<class T>void value(const T& x){
        if constexpr(std::is_arithmetic_v<T>)archive.number(x);
        else if constexpr(std::is_enum_v<T>)archive.number(static_cast<std::int32_t>(x));
        else if constexpr(HasStateFields<T>::value)visit_state_fields(x,[&](const char* tag,const auto&...field){object(tag,x,[&]{fields(field...);});});
        else static_assert(unsupported_state<T>,"checkpoint requires an explicit field schema");
    }
    template<class T>void value(T* x){
        archive.number(x!=nullptr);if(!x)return;
        if constexpr(std::is_polymorphic_v<T>&&std::is_abstract_v<T>)binding(dynamic_cast<const void*>(x));
        else value(*x);
    }
    template<class T>void value(const std::shared_ptr<T>& x){value(x.get());}
    template<class T>void value(const std::unique_ptr<T>& x){value(x.get());}
    template<class T>void value(const std::optional<T>& x){archive.number(x.has_value());if(x)value(*x);}
    template<class A,class B>void value(const std::pair<A,B>& x){fields(x.first,x.second);}
    template<class T,std::size_t N>void value(const std::array<T,N>& x){sequence("array",x);}
    template<class T>void value(const std::vector<T>& x){sequence("vector",x);}
    template<class T>void value(const std::deque<T>& x){sequence("deque",x);}
    template<class K,class V>void value(const std::map<K,V>& x){sequence("map",x);}
    template<class K,class V>void value(const std::unordered_map<K,V>& x){
        object("unordered-map",x,[&]{std::vector<K> keys;keys.reserve(x.size());for(const auto& entry:x)keys.push_back(entry.first);
            std::sort(keys.begin(),keys.end());archive.count(keys.size());for(const auto& key:keys)fields(key,x.at(key));});
    }
    template<class T>void sequence(const char* name,const T& x){object(name,x,[&]{archive.count(x.size());for(const auto& item:x)value(item);});}
    void value(const std::string& x){object("utf8",x,[&]{archive.text(x);});}
    void value(const std::u16string& x){object("utf16",x,[&]{archive.text(x);});}
    void value(const std::filesystem::path&){archive.tag("external-file-path");} // Contents are fingerprinted, never workstation paths.
    void value(const void* x){archive.reference("private-entry",x);} // Only private-view entry/next identities, with separately emitted fields.
    void value(void* x){value(static_cast<const void*>(x));}
    void value(const EventObject& x){
        archive.number(x!=nullptr);if(!x)return;const auto found=opaque.find(x.get());
        if(found==opaque.end())throw std::logic_error("checkpoint has an untyped opaque game payload");found->second();
    }
    template<class R,class...A>void value(const std::function<R(A...)>& x){
        // Fixed service slots are named by their containing schema. Multicast ECS
        // callbacks additionally carry per-receiver native semantic IDs in tracking.
        object("fixed-callback",x,[&]{archive.number(static_cast<bool>(x));});
    }
    void binding(const void* x){
        const auto found=services.find(x);if(found==services.end())throw std::logic_error("checkpoint has an unknown service binding");
        archive.tag("binding");archive.text(found->second);
    }
    template<class T>void service(const T* x,const char* name){if(x)services.emplace(x,name);}
    void bind_objects(){
        service(&p,"profile");service(p.entity_world.get(),"entities");service(p.mission.get(),"scene");service(p.humans.get(),"humans");
        service(p.spatial.get(),"spatial");service(p.events_.get(),"events");
        if(p.humans){const auto& h=*p.humans;service(h.stage_loop_.get(),"stage");service(h.results_.get(),"results");service(h.input_.get(),"input");
            service(h.default_enemy_entry_.get(),"default-enemy-entry");service(&h.random_,"random");
            if(h.stage_loop_)service(h.stage_loop_->enemy_entry_.get(),"stage-enemy-entry");
            if(h.stage_.area)opaque.emplace(h.stage_.area.get(),[this,&h]{value(*static_cast<const AreaDefinition*>(h.stage_.area.get()));});
        }
        if(p.spatial)service(&p.spatial->formation_,"camera-formation");
        if(p.mission)service(&p.mission->gauge_service_,"gauge-service");
        for(std::size_t i=0;i<p.initializer_owners_.size();++i)if(p.initializer_owners_[i])services.emplace(p.initializer_owners_[i].get(),"initializer-"+std::to_string(i));
        if(p.events_){const auto& e=*p.events_;
            if(e.root_.identity)opaque.emplace(e.root_.identity.get(),[this,&e]{object("root-form",*static_cast<const int*>(e.root_.identity.get()),[&]{archive.number(*static_cast<const int*>(e.root_.identity.get()));});});
            if(e.execution_.normal_save_strings)for(const auto& text:*e.execution_.normal_save_strings)if(text){const auto* ptr=static_cast<const std::u16string*>(text.get());opaque.emplace(ptr,[this,ptr]{value(*ptr);});}
            for(const auto& form:e.forms_)if(form){const auto* ptr=form.get();opaque.emplace(ptr,[this,ptr]{value(*ptr);});}
            for(const auto& form:{e.creating_,e.updating_})if(form){const auto* ptr=form.get();opaque.emplace(ptr,[this,ptr]{value(*ptr);});}
            if(e.root_.dialog){const auto* ptr=static_cast<const MissionEvents::Talk*>(e.root_.dialog.get());opaque.emplace(ptr,[this,ptr]{value(*ptr);});}
        }
    }
    template<class T>void private_fields(const char* tag,const T& x){object(tag,x,[&]{StateArchiveAccess::visit(x,[&](const char* part,const auto&...field){archive.tag(part);fields(field...);});});}
    void value(const EntityRegistry& x){private_fields("entity-registry",x);}
    void value(const EntitySubset& x){private_fields("entity-subset",x);}
    void value(const DynamicSubsets& x){private_fields("dynamic-subsets",x);}
    void value(const ComponentEvents& x){private_fields("component-events",x);}
    void value(const EntityEvents& x){private_fields("entity-events",x);}
    void value(const FilterIdentity& x){private_fields("filter-identity",x);}
    void value(const ComponentGroupFilter& x){private_fields("component-group-filter",x);}
    void value(const CharacterParameterSet& x){private_fields("parameter-set",x);}
    void value(const LegacySystemRandom& x){private_fields("system-random",x);}
    void value(const ProjectilePass& x){private_fields("projectile-pass",x);}
    void value(const CollisionPass& x){private_fields("collision-pass",x);}
    void value(const GarbagePass& x){private_fields("garbage-pass",x);}
    void value(const InitList& x){private_fields("initializers",x);}
    void value(const EnemyGaugeService& x){binding(&x);}
    void value(const EnemyStageEntry& x){binding(&x);}
    void value(const EmitterExpiryQueue& x){object("expiry-queue",x,[&]{fields(x.version());archive.count(x.size());for(std::size_t i=0;i<x.size();++i)value(x.at(i));});}
    void value(const SubsetFilter& x){private_fields("subset-filter",x);}
    void value(const EventHashes& x){private_fields("event-hashes",x);}
    template<class T>void value(const InfoBars<T>& x){private_fields("info-bars",x);}
    void value(const ComponentSlots* x){archive.tag("entity-ref");archive.number(x!=nullptr);if(x)archive.count(p.entity_world->handle(*x));}
    void value(ComponentSlots* x){value(static_cast<const ComponentSlots*>(x));}
    void value(const ComponentSlots& x){
        object("component-slots",x,[&]{StateArchiveAccess::visit(x,[&](const char* part,const auto&...field){archive.tag(part);fields(field...);});});
    }
    void value(const GroupFilter& x){
        const auto* known=dynamic_cast<const ComponentGroupFilter*>(&x);if(!known)throw std::logic_error("checkpoint has an unknown subset filter");value(*known);
    }
    void value(GroupFilter* x){archive.number(x!=nullptr);if(x)value(*x);}
    void value(const GroupFilter* x){archive.number(x!=nullptr);if(x)value(*x);}
    void value(const ComponentObserver* x){
        archive.number(x!=nullptr);if(!x)return;
        if(const auto* n=dynamic_cast<const MissionSpatial::Notifications*>(x)){object("cell-notifications",*n,[&]{fields(n->group,n->previous,n->events);});return;}
        if(const auto* n=dynamic_cast<const MissionSpatial::Observer*>(x)){object("cell-observer",*n,[&]{fields(n->adding,n->callback);});return;}
        if(const auto* n=dynamic_cast<const ComponentEvents*>(x)){value(*n);return;}
        if(const auto* n=dynamic_cast<const DynamicSubsets*>(x)){value(*n);return;}
        if(const auto* n=dynamic_cast<const MissionEntities*>(x)){binding(n);return;}
        throw std::logic_error("checkpoint has an unknown component observer");
    }
    void value(ComponentObserver* x){value(static_cast<const ComponentObserver*>(x));}
    void value(const ComponentObserver& x){value(&x);}
    void value(const ComponentEvents* x){archive.number(x!=nullptr);if(x)value(*x);}
    void value(const Component* x){
        archive.number(x!=nullptr);if(!x)return;
#define SAGA_COMPONENT(T) if(const auto* typed=dynamic_cast<const T*>(x)){value(*typed);return;}
        SAGA_COMPONENT(ContactPosition) SAGA_COMPONENT(ProjectileSpeed) SAGA_COMPONENT(EntitySprite)
        SAGA_COMPONENT(QuestCameraComponent) SAGA_COMPONENT(EntityDepth) SAGA_COMPONENT(CellComponent)
        SAGA_COMPONENT(DefeatColor) SAGA_COMPONENT(EntityImage) SAGA_COMPONENT(EquipmentRecordLinks)
        SAGA_COMPONENT(MapChipComponent) SAGA_COMPONENT(EntityPlayback) SAGA_COMPONENT(MapComponent)
        SAGA_COMPONENT(ContactDirection) SAGA_COMPONENT(HumanRecordLink) SAGA_COMPONENT(ProjectileModification)
        SAGA_COMPONENT(EntityButton) SAGA_COMPONENT(EntityBubble) SAGA_COMPONENT(EntityGauge)
        SAGA_COMPONENT(HumanAI) SAGA_COMPONENT(GarbageLifetime) SAGA_COMPONENT(CombatParameterLinks)
        SAGA_COMPONENT(EnemyRecordLink) SAGA_COMPONENT(EffectComponent) SAGA_COMPONENT(ProjectileLink)
        SAGA_COMPONENT(BulletComponent) SAGA_COMPONENT(EmitterComponent) SAGA_COMPONENT(ProjectileCollision)
        SAGA_COMPONENT(ProjectileDamage) SAGA_COMPONENT(DropComponent)
#undef SAGA_COMPONENT
        if(typeid(*x)!=typeid(Component))throw std::logic_error("checkpoint has an unknown component type");
        object("component-marker",*x,[]{});
    }
    void value(Component* x){value(static_cast<const Component*>(x));}
    void value(const Component& x){value(&x);}

    void value(const ResourceArchive& x){object("archive-input",x,[&]{const auto& bytes=x.source_bytes();archive.count(bytes.size());archive.number(digest(bytes.data(),bytes.size()));fields(x.selected_profile().language_folder,x.selected_profile().dpi);});}
    void value(const RenderDrawResource& x){object("draw-resource",x,[&]{value(x.animations);});}
    void value(const AnimationResource& x){object("animation-resource",x,[&]{value(x.clips);});}
    void value(const SharedAnimationClip& x){object("animation-clip",x,[&]{fields(x.frame,x.max_frame);archive.reference("seb-sprites",x.sprites);});}
    void value(const SebSpriteState& x){object("seb-sprites",x,[&]{fields(x.groups,x.frames,x.packed,x.extended,x.reverse,x.cache,x.bounds,x.extended_bounds,x.draw_offset,x.work,x.flip);});}
    void value(const SebInstanceWork& x){object("seb-instance-work",x,[&]{fields(x.sprites,x.list);});}
    void value(const SebStaticWork& x){object("seb-static-work",x,[&]{fields(x.local,x.lists,x.left,x.right);});}

    void value(const MissionEntities& x){object("mission-entities",x,[&]{
        fields(x.slots_,x.ids_.counter,x.map_,x.player_world_,x.registry_,x.groups_,x.gameplay_events_);
        archive.count(x.nodes_.size());for(const auto& node:x.nodes_){fields(node.id);archive.count(node.handle);fields(node.lifetime,node.events,node.components);}
        archive.count(x.handles_.size());for(const auto* node:x.handles_)archive.count(node?node->handle:std::numeric_limits<std::size_t>::max());
        fields(x.player_marker_,x.sync_marker_,x.map_component_,x.positions_,x.speeds_,x.sprites_,x.depths_,x.cells_,x.images_,x.playback_,
            x.humans_,x.parameters_,x.equipment_,x.directions_,x.ai_,x.inputs_,x.collisions_,x.enemies_,x.colors_,x.bullets_,x.emitters_,x.damages_,
            x.effects_,x.garbage_,x.projectiles_,x.modifications_,x.buttons_,x.bubbles_,x.gauges_,x.map_chips_,x.cameras_,x.drops_,x.retired_,
            x.bubble_offsets_,x.gauge_offsets_,x.party_rows_,x.party_,x.hat_rows_,x.hats_,x.maxima_,x.enemy_maxima_,x.parameter_ids_,x.enemy_parameter_ids_,x.shop_);
        fields(x.bullet_world_.ids.counter,x.effect_world_.ids.counter,x.enemy_world_.ids.counter,x.map_world_.ids.counter);
    });}
    void value(const MissionScene& x){object("mission-scene",x,[&]{
        fields(x.map_,x.spatial_,x.form_state_,x.form_,x.renderer_,x.animations_,x.movers_,x.rotating_,x.garbage_entities_,x.garbage_pass_,
            x.modifications_,x.emitters_,x.collisions_,x.collision_events_,x.area_collision_events_,x.experience_,x.battle_,x.bubbles_,
            x.debug_visualization_,x.debug_path_,x.gauges_,x.gauge_steps_,x.gauge_destroy_calls_,x.bubble_steps_,x.bubble_destroy_calls_,x.debug_steps_,
            x.buttons_,x.button_touch_receiver_,x.info_,x.info_click_,x.info_town_,x.info_paused_,x.info_steps_,x.info_clicks_,x.info_button_events_,x.info_popup_disposals_,
            x.scenario_popups_,x.scenario_slides_,x.button_steps_,x.button_events_,x.button_removals_,x.last_button_target_,x.last_button_event_type_,
            x.experience_receiver_,x.effects_,x.experience_steps_,x.parameter_steps_,x.effect_steps_,x.battle_steps_,x.rotation_steps_);
        archive.number(x.auxiliary_buttons_!=nullptr);if(x.auxiliary_buttons_){archive.reference("auxiliary-buttons",x.auxiliary_buttons_.get());archive.count(x.auxiliary_buttons_->size());for(const auto& r:*x.auxiliary_buttons_)fields(r.factory,r.callback);}
    });}
    void value(const MissionHumans& x){object("mission-humans",x,[&]{
        fields(x.runtime_,x.random_.source,x.random_.draws,x.characters_,x.equipment_,x.hats_,x.cloth_,x.stage_,x.button_,x.members_,x.quest_form_,x.enemies_,x.system_,
            x.deferred_init_,x.native_state_,x.table_quest_,x.damage_labels_enabled_,x.gauge_service_,x.default_enemy_entry_,x.projectiles_,x.attack_clips_,x.affinities_,x.shots_,x.attack_sounds_,x.damage_presentations_,
            x.reward_scenario_,x.enemy_scenario_,x.lottery_,x.notification_scenario_,x.held_keys_,x.stage_forms_,x.retry_invincibility_time_,x.stage_modal_wait_frames_,
            x.notification_events_,x.collection_enabled_,x.collection_steps_,x.collection_sounds_,x.collection_jingles_,x.results_,x.stage_loop_,x.input_);
        archive.count(x.stage_modals_.size());for(const auto& modal:x.stage_modals_)fields(modal.form,modal.victory,modal.kind);
        value(x.victory_forms_);archive.count(x.stage_forms_.size());for(const auto& form:x.stage_forms_){const auto found=x.form_names_.find(form.get());if(found==x.form_names_.end())throw std::logic_error("checkpoint stage form is unnamed");fields(form,found->second);}
        fields(x.damage_receiver_,x.damage_bindings_,x.stage_event_executor_);archive.number(x.placement_world_.has_value());
    });}
    void value(const MissionInput& x){object("mission-input",x,[&]{
        fields(x.scenario_,x.state_,x.keyboard_keys_,x.pointer_keys_,x.widget_lookup_,x.widget_active_,x.steps_,x.touches_,x.receivers_);
        archive.count(x.controls_.size());for(const auto& control:x.controls_)fields(static_cast<const MovementControl&>(control),control.touch);
    });}
    void value(const MissionStage& x){object("mission-stage",x,[&]{
        fields(x.scenario_,x.enemy_entry_,x.map_loader_,x.boss_creation_calls_,x.steps_,x.pulse_,x.pending_pulse_,x.frame_pulse_,x.answer_received_,x.modal_wait_steps_,x.dialog_answers_,
            x.started_,x.inherited_formation_,x.paused_,x.resume_,x.pause_);
    });}
    void value(const MissionResults& x){object("mission-results",x,[&]{
        fields(x.state_,x.inventory_,x.recruits_,x.granting_,x.treasure_index_,x.grant_cursor_,x.finish_cursors_,x.result_);
        archive.number(static_cast<bool>(x.transition_));if(x.transition_){const auto* captured=x.transition_.target<QuestTownContinuation>();
            if(!captured)throw std::logic_error("checkpoint has an unknown return continuation");fields(captured->result,captured->host);}
    });}
    void value(const MissionResults::Cursor& x){object("result-cursor",x,[&]{fields(x.list,x.version);archive.count(x.index);});}
    void value(const MissionSpatial& x){object("mission-spatial",x,[&]{
        fields(x.selected_quest_,x.form_,x.camera_,x.map_system_,x.cells_,x.index_,x.visible_,x.save_,x.scratch_,x.cell_receivers_,
            x.camera_steps_,x.cell_steps_,x.visibility_steps_,x.changes_,x.visible_count_,x.cursor_,x.point_receivers_,x.focus_effects_,x.old_cursor_cell_,x.new_cursor_cell_,
            x.cursor_steps_,x.cursor_changes_,x.cursor_touches_,x.cursor_clicks_,x.touch_forward_,x.touch_debug_,x.touch_priority_,x.scale_receivers_,
            x.form_touches_,x.scale_events_,x.last_scale_event_,x.observed_,x.notifications_.events);
        archive.count(x.observers_.size());for(const auto& observer:x.observers_)fields(observer.adding,observer.callback);
    });}
    void value(const MissionEvents& x){
        if(x.application_push_||x.numeric_effect_)throw std::logic_error("mission checkpoint does not cover town event bindings");
        object("mission-events",x,[&]{fields(x.scenario_,x.talks_,x.execution_,x.delays_,x.forms_,x.creating_,x.updating_,x.root_,
        x.ticks_,x.opened_,x.closed_,x.flushes_,x.pages_,x.now_,x.total_frames_,x.pulse_,x.held_,x.executed_,x.lifecycle_,x.layout_measurements_,x.normal_save_);
        archive.count(x.layout_samples_.size());for(const auto& sample:x.layout_samples_){fields(sample.talk,sample.page,sample.length,sample.height);archive.count(sample.blocks);}
    });}
    void value(const MissionEvents::Talk& x){object("mission-talk",x,[&]{
        fields(x.id,x.state,x.frame,x.finish_frame,x.prepared,x.reveal,x.layout);
        if(x.prepared.initial.type==4){
            if(!x.event_notice)throw std::logic_error("event form checkpoint requires notice state");
            archive.tag("event-notice");fields(x.event_notice->initial,x.event_notice->event_ids,x.event_notice->texts,x.event_notice->animation,x.event_page,x.event_frame,x.event_resource_ready);
        }else if(x.event_notice)throw std::logic_error("notice payload on non-event form");
    });}
    void value(const MissionTextLayout& x){object("mission-text-layout",x,[&]{fields(x.scenario_,x.font_,x.fit_,x.blocks_,x.text_,x.presets_,x.measurements_,x.length_);});}

#define SAGA_STATE(T,...) void value(const T& x){object(#T,x,[&]{fields(__VA_ARGS__);});}
    SAGA_STATE(MissionRuntimeConfig,x.fps,x.debug,x.seed,x.backend)
    SAGA_STATE(MissionProgress,x.characters,x.shop_purchases,x.equipment_levels,x.hat_levels,x.cloth_levels,x.enemy_defeat_counts,x.sortie_counts)
    SAGA_STATE(GameplayCatalog,x.characters,x.equipment,x.cloth,x.hats,x.shop,x.enemies,x.bullets,x.treasures,x.levels,x.quests,x.areas,x.towns,x.members)
    SAGA_STATE(MissionView,x.width,x.height,x.scale,x.camera_x,x.camera_y,x.margin_x,x.margin_y,x.extra_depth_rate)
    SAGA_STATE(MissionRewardScenario,x.defeated,x.remaining,x.saved_defeat_count)
    SAGA_STATE(MissionEnemyScenario,x.yugami,x.maximum,x.total,x.interval,x.level,x.created,x.defeated,x.remaining,x.loop_counter,x.section_limit,x.section_created,x.boss_defeated,x.pool)
    SAGA_STATE(MissionNotificationScenario,x.state,x.frame,x.retried,x.quest_flags,x.quest_completed,x.held_keys,x.retry_answer)
    SAGA_STATE(MissionInputScenario,x.held_keys,x.last_active,x.front_pushed,x.buttons)
    SAGA_STATE(MissionStageScenario,x.remaining_time,x.fade_frame,x.boss_started,x.events,x.pulses,x.boss,x.reset_timer)
    SAGA_STATE(MissionBossScenario,x.id,x.level)
    SAGA_STATE(MissionResultScenario,x.money,x.crystals,x.rescued,x.record,x.quest_record,x.inventory,x.recruits,x.inventory_records,x.recruit_records)
    SAGA_STATE(MissionEventScenario,x.save_string0,x.short_words,x.held_advance,x.print,x.new_game_context,x.page_lengths,x.font)
    SAGA_STATE(MissionPopupScenario,x.id,x.category,x.type,x.priority,x.single,x.height,x.duration)
    SAGA_STATE(MissionSlideScenario,x.id,x.width,x.height,x.duration)
    SAGA_STATE(MissionProfile::SavedMissionSelection,x.area8,x.quest9)
    SAGA_STATE(MissionProfile::QuestFrameConfig,x.debug_controls,x.orientation)
    SAGA_STATE(MissionProfile::InitialMissionContext,x.yugami,x.orientation,x.event_arguments)
    SAGA_STATE(BitmapCamera,x.near_clip,x.far_clip,x.z)
    SAGA_STATE(QuestEntityLifetime,x.destroyed)
    SAGA_STATE(GaugePoint,x.x,x.y)
    SAGA_STATE(InputRectangle,x.x,x.y,x.width,x.height)
    SAGA_STATE(GaugeCreation,x.parent,x.type,x.from,x.to,x.maximum,x.image,x.frame_max,x.flags,x.option,x.option2)
    SAGA_STATE(MissionMapSystem,x.map)
    SAGA_STATE(OffscreenMatrix,x.values)
#undef SAGA_STATE
    void value(const MissionFontScenario& x){object("font-input",x,[&]{
        fields(x.requested_scale,x.load_flags,x.kerning_mode,x.application_ratio,x.layout_scale,x.japanese,x.japanese_style);
        std::ifstream input(x.path,std::ios::binary);if(!input)throw std::runtime_error("checkpoint cannot fingerprint its selected font");
        XXH64_state_t state;XXH64_reset(&state,0);std::array<char,16384> buffer{};std::uint64_t length=0;
        while(input){input.read(buffer.data(),static_cast<std::streamsize>(buffer.size()));const auto count=input.gcount();
            if(count>0){XXH64_update(&state,buffer.data(),static_cast<std::size_t>(count));length+=static_cast<std::uint64_t>(count);}}
        if(!input.eof())throw std::runtime_error("checkpoint font read failed");archive.number(length);archive.number(static_cast<std::uint64_t>(XXH64_digest(&state)));
    });}
    void value(const MissionInput::ReceiverSlot& x){object("touch-receiver",x,[&]{if(x.callback&&x.semantic_id.empty())throw std::logic_error("checkpoint requires semantic touch receiver binding");fields(x.semantic_id,x.callback);});}
    void value(const StageForm& x){
        if(const auto* dialog=dynamic_cast<const StageDialog*>(&x)){value(*dialog);return;}
        if(typeid(x)!=typeid(StageForm))throw std::logic_error("checkpoint has an unknown stage modal type");object("stage-form",x,[]{});
    }
    void value(const StageForm* x){archive.number(x!=nullptr);if(x)value(*x);}
    void value(StageForm* x){value(static_cast<const StageForm*>(x));}
    void value(const PopupBar& x){object("popup",x,[&]{fields(x.id,x.x,x.y,x.bottom,x.frame,x.state_frame,x.state,mission_popup_payload(x));});}
    void value(const SlideBar& x){object("slide",x,[&]{fields(x.id,x.x,x.y,x.frame,x.state_frame,x.state,mission_slide_payload(x));});}
    void value(PopupBar* x){archive.number(x!=nullptr);if(x)value(static_cast<const PopupBar&>(*x));}
    void value(const PopupBar* x){archive.number(x!=nullptr);if(x)value(*x);}
    void value(SlideBar* x){archive.number(x!=nullptr);if(x)value(static_cast<const SlideBar&>(*x));}
    void value(const SlideBar* x){archive.number(x!=nullptr);if(x)value(*x);}

    void write(){
        archive.tag("magicians-saga-mission-checkpoint-v2");
        archive.tag("inputs");fields(p.tables,p.catalog,p.progress,p.application,p.global,p.runtime,p.selected_quest,p.geometry,p.view,p.has_view,p.has_slots,p.has_animation,p.slots,
            p.frames,p.automatic,p.application_update,p.tick_ms,p.simulate,p.collisions,p.enemy_gauges,p.human_hud,p.stage_mp_hud,p.damage_labels,p.resource_names,
            p.instance_view_scale_,p.base_view_scale_,p.debug_requested_flags_,p.quest_frame_,p.initial_context_,p.saved_mission_selection_,p.awaiting_quest_,p.campaign_system_save_);
        archive.count(p.owners.size());for(const auto& owner:p.owners){fields(owner->archive,owner->image_clocks);value(owner->manager.drawing());}
        archive.tag("entities");fields(p.entity_world,p.created);
        archive.tag("systems");fields(p.mission,p.humans,p.spatial,p.events_);
        archive.tag("form-and-input");fields(p.logic_steps,p.movement_steps,p.selected_count,p.quest_frame_state_,p.parent_update_counter_,p.image_clocks_,
            p.world_update_calls_,p.image_clock_resets_,p.image_clock_updates_,p.last_world_passes_,p.quest_frame_result_,p.debug_modal_,p.application_orientation_,
            p.construct_systems_,p.constructing_systems_,p.construction_complete_,p.construction_pause_,p.construction_order_,p.original_world_entry_,p.world_entry_order_,
            p.native_stage_entry_,p.offscreen_camera_,p.viewport_changes_,p.initial_character_records_,p.initial_shop_records_,p.initialized_loaded_,p.initialized_order_,
            p.initialization_complete_,p.menu_visible_,p.menu_shapes_,p.submenu_touches_,p.menu_touches_,p.saved_town_count_,p.town_count_writes_,p.fresh_town_initialized_,p.fresh_hero_,
            p.gear_new_game_,p.new_game_save_,p.new_game_quest_counts_,p.character_shop_new_game_mask_,p.new_game_gear_counts_,p.configured_,p.enabled_,p.halted_,p.last_updates_,
            p.pause_requests_,p.map_pointer_captured_,p.map_pointer_x_,p.map_pointer_y_,p.map_pointer_events_,p.map_pointer_rejected_,p.map_pointer_cancellations_,
            p.move_pointer_events_,p.move_pointer_cancellations_,p.attack_point_events_,p.attack_point_rejected_,p.held_move_control_,p.held_move_key_,p.replay_bound_,p.replay_applied_,p.replay_modal_ignored_);
        archive.count(p.replay_.size());for(const auto& actions:p.replay_){fields(actions.first);archive.count(actions.second.size());for(const auto& action:actions.second)fields(action.kind,action.value,action.x,action.y,action.z,action.dx,action.dy);}
        archive.count(p.attack_regions_.size());for(const auto& region:p.attack_regions_)fields(region.bounds,region.control);
        archive.count(p.move_regions_.size());for(const auto& region:p.move_regions_)fields(region.bounds,region.control,region.key,region.frame);
        archive.count(p.system_states_.size());for(const auto& state:p.system_states_)fields(state.enabled,state.manager,state.on_enabled,state.on_disabled,state.on_pause);
        fields(p.menu_orientation_,p.menu_button_receiver_,p.town_return_,p.initializers_,p.initialization_.paused,p.town_state_,p.initialization_world_.state,p.gauge_creations_,p.gauge_scratch_);
        logic_end=archive.data().size();presentation=true;
        archive.tag("presentation");fields(p.world,p.has_world,p.projection,p.has_projection,p.has_gpu_depth,p.depth_bits,p.depth_step,p.queue);
        if(p.mission)fields(p.mission->queue_,p.mission->sprite_work_,p.mission->effect_screen_work_,p.mission->dynamic_text_color_,p.mission->dynamic_font_scopes_);
        for(const auto& owner:p.owners){auto* resource=owner->manager.drawing().animations;if(resource&&resource->clips)for(const auto* clip:*resource->clips)if(clip&&clip->sprites)value(*clip->sprites);}
        fields(p.hud_sprite_work_,p.drawing_color_,p.drawing_fonts_,p.drawing_translations_,p.debug_parameter_ids_,p.debug_parameter_names_,p.debug_human_states_,p.debug_enemy_states_,p.debug_texts_);
    }
};

MissionStateDigest mission_state_digest(const MissionProfile& profile){
    MissionCheckpoint writer(profile);writer.write();const auto& bytes=writer.archive.data();
    return {hex(digest(bytes.data(),bytes.size())),hex(digest(bytes.data(),writer.logic_end)),static_cast<std::uint64_t>(bytes.size()),static_cast<std::uint64_t>(writer.logic_end)};
}
std::vector<std::uint8_t> mission_state_bytes(const MissionProfile& profile){MissionCheckpoint writer(profile);writer.write();return writer.archive.data();}
}
