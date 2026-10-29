#pragma once
#include "mission_entities.hpp"
#include "core/gameplay/render_frame.hpp"
#include "core/gameplay/render_draw.hpp"
#include "core/gameplay/motion_pass.hpp"

namespace saga::desktop {
using MissionApplication=core::ApplicationResourceFields<core::RenderDrawResource>;
struct MissionView {
    std::int32_t width,height,scale,camera_x,camera_y,margin_x,margin_y,extra_depth_rate;
};
struct MissionRenderFault {std::int32_t entity;std::string message;};
// Selected entity group and fixed supplied view. Full mission culling/camera/input remain host bindings.
class MissionScene final:private core::RenderInitializationWorld,private core::RenderFrameHost,
    private core::QuestCameraHost,private core::QuestViewportView {
public:
    MissionScene(MissionEntities&,MissionApplication&,core::EntitySubset& visible,MissionView&,core::MapComponent*);
    MissionScene(const MissionScene&)=delete;
    MissionScene& operator=(const MissionScene&)=delete;
    void advance_animations(core::SharedAnimationGates);
    // Selected MoveSystem pass only; Human/Enemy AI, collision and cell migration remain unbound.
    void advance_movement(std::int32_t pass,bool enabled=true);
    core::RenderQueue& render();
    const std::vector<MissionRenderFault>& faults()const{return faults_;}
private:
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
    core::EntitySubset* movers_=nullptr;
    core::RenderQueue queue_=core::make_render_queue();std::vector<MissionRenderFault> faults_;
};
} // namespace saga::desktop
