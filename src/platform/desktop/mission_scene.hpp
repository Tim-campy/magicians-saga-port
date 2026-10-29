#pragma once
#include "mission_entities.hpp"
#include "core/gameplay/render_frame.hpp"
#include "core/gameplay/render_draw.hpp"
#include "core/gameplay/motion_pass.hpp"
#include "core/gameplay/modification_frame.hpp"
#include "core/gameplay/pass_initialization.hpp"
#include "core/gameplay/parameter_experience.hpp"
#include "core/gameplay/effect_frame.hpp"
#include "core/gameplay/gauge_drawing.hpp"
#include <ostream>

namespace saga::desktop {
class MissionSpatial;
using MissionApplication=core::ApplicationResourceFields<core::RenderDrawResource>;
struct MissionView {
    std::int32_t width,height,scale,camera_x,camera_y,margin_x,margin_y,extra_depth_rate;
};
struct MissionRenderFault {std::int32_t entity;std::string message;};
// Selected entity group and fixed supplied view. Full mission culling/camera/input remain host bindings.
class MissionScene final:private core::RenderInitializationWorld,private core::RenderFrameHost,
    private core::QuestCameraHost,private core::QuestViewportView,private core::GarbageUpdateHost,private core::ModificationFrameHost,
    private core::EmitterFrameHost,private core::CollisionPassHost,
    private core::ParameterInitializationWorld,private core::ParameterExperienceInitializationWorld,
    private core::EffectInitializationWorld,private core::EffectFrameHost {
public:
    MissionScene(MissionEntities&,MissionApplication&,core::EntitySubset& visible,MissionView&,core::MapComponent*);
    MissionScene(const MissionScene&)=delete;
    MissionScene& operator=(const MissionScene&)=delete;
    void advance_animations(core::SharedAnimationGates);
    // Selected MoveSystem pass; actor/projectile/emitter updates are advanced separately.
    void advance_movement(std::int32_t pass,bool enabled=true);
    void advance_rotation(std::int32_t pass,bool enabled=true);
    // Native index31 runs after RenderSystem; commands already produced remain in this frame.
    void advance_garbage(std::int32_t pass,bool enabled=true);
    void advance_modifications(std::int32_t pass,bool enabled=true);
    void advance_emitters(std::int32_t pass,bool enabled=true);
    void advance_collisions(std::int32_t pass,bool enabled=true);
    void advance_parameter_experience(std::int32_t pass=0,bool enabled=true);
    void advance_battle(std::int32_t pass=0,bool enabled=true);
    void advance_parameters(std::int32_t pass=0,bool enabled=true);
    void advance_effects(std::int32_t pass=0,bool enabled=true);
    // Typed event21 boundary; no reward-orb or character-EXP producer is inferred.
    void send_parameter_experience(core::ComponentSlots*,std::int32_t key);
    void write_pass_report(std::ostream&);
    void bind_spatial(MissionSpatial&);
    core::QuestCameraHost& gauge_camera_host();
    void draw_enemy_gauges(const std::function<void(const core::ParameterGaugeRequest&)>&);
    std::uint64_t collision_events()const{return collision_events_;}
    std::uint64_t area_collision_events()const{return area_collision_events_;}
    core::RenderQueue& render();
    const std::vector<MissionRenderFault>& faults()const{return faults_;}
private:
    void subscribe_dropoff(core::EntityEvents::Single,std::optional<core::EventHashes>)override;
    void subscribe_parameter_experience(core::ParameterExperienceInitializationWorld::Receiver,std::optional<core::EventHashes>)override;
    core::EntitySubset* effect_subset(const core::EventIntegers& all,const core::EventIntegers& none)override{return world_.subset(all,none);}
    bool effect_parent_destroyed(core::ComponentSlots& e)override{return world_.destroyed(e);}
    void destroy_effect_entity(core::ComponentSlots& e)override{world_.destroy(e,true);}
    std::unique_ptr<core::CollisionCursor> open_collision_cursor()override;
    void send_collision(std::int32_t,core::ComponentSlots&,core::ComponentSlots&)override;
    core::BulletRecords bullet_records()override{return world_.bullet_records();}
    core::ProjectileModification& acquire_modification()override{return world_.acquire_modification();}
    core::EntityPlayback& acquire_playback()override{return world_.acquire_playback();}
    core::ParticleFactoryHost* emitter_particle_world()override{return &world_;}
    core::DropEffectWorld* emitter_effect_world()override{return &world_.effect_world();}
    core::DefeatColor& acquire_emitter_color()override{return world_.acquire_enemy_color();}
    void destroy_emitter_entity(core::ComponentSlots& e)override{world_.destroy(e,true);}
    void destroy_modified_entity(core::ComponentSlots& e)override{world_.destroy(e,true);}
    void return_modification(core::Component* p)override{world_.return_component(15,p);}
    std::unique_ptr<core::GarbageCursor> open_garbage_cursor()override;
    core::QuestEntityLifetime& entity_lifetime(core::QuestMemberId key)override{return world_.entity_lifetime(key);}
    void delete_from_manager(core::QuestMemberId key,bool remove)override{world_.delete_from_manager(key,remove);}
    core::MapComponent* render_map()override{return map_;}
    std::unique_ptr<core::RenderCursor> visible_render_cursor()override;
    core::AnimationResource* render_resource(core::RenderQueue&,std::int32_t)override;
    core::EventInstruction render_sprite(core::SharedAnimationClip&,std::int32_t,std::int32_t)override;
    std::int32_t render_layer_count(core::SharedAnimationClip&)override;
    std::int32_t render_extra_depth_rate()override{return view_.extra_depth_rate;}
    core::RenderQueue* current_render_queue()override{return &queue_;}
    void render_error(core::ComponentSlots&,const std::exception&)override;
    core::QuestCameraForm* current_camera_form()override;
    core::CameraFormation* current_camera_members()override;
    std::int32_t camera_view_width()override{return view_.width;}
    std::int32_t camera_touch_count()override;
    std::int32_t quest_view_x()override{return view_.margin_x;}
    std::int32_t quest_view_y()override{return view_.margin_y;}
    core::EventInstruction camera_saved_integers()override;
    std::int32_t game_width()override{return view_.width;}
    std::int32_t game_height()override{return view_.height;}
    MissionEntities& world_;MissionApplication& app_;core::EntitySubset& visible_;MissionView& view_;core::MapComponent* map_;
    core::StageQuestForm form_state_;core::QuestCameraForm form_;
    core::RenderSystemState renderer_;core::AnimationSystemState animations_;core::SebStaticWork sprite_work_;
    core::EntitySubset *movers_=nullptr,*rotating_=nullptr,*garbage_entities_=nullptr;core::GarbagePass garbage_pass_;
    core::EntitySubset* gauge_enemies_=nullptr;
    core::ModificationSystemState modifications_;
    core::EmitterSystemState emitters_;
    core::CollisionSystemState collisions_;std::uint64_t collision_events_=0,area_collision_events_=0;
    core::ParameterExperienceSystem experience_;
    core::BattleSystemState battle_;
    core::ParameterExperienceInitializationWorld::Receiver experience_receiver_;
    core::EffectSystemState effects_;
    std::uint64_t experience_steps_=0,parameter_steps_=0,effect_steps_=0,battle_steps_=0;
    std::uint64_t rotation_steps_=0;
    core::RenderQueue queue_=core::make_render_queue();std::vector<MissionRenderFault> faults_;
    MissionSpatial* spatial_=nullptr;
};
} // namespace saga::desktop
