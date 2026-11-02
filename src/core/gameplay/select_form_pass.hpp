#pragma once
#include "form_push.hpp"
#include "select_clouds.hpp"
#include "select_markers.hpp"
#include "select_event_pass.hpp"
#include "select_prelude.hpp"
#include "select_touch_move.hpp"
#include "select_triggers.hpp"
#include "town_bubble.hpp"
#include "render_draw.hpp"
#include <array>
namespace saga::core {
struct SelectBackgroundState {
    const std::int32_t& camera_x;
    const std::int32_t& camera_y;
    const std::int32_t& left_extent;
    const std::int32_t& right_extent;
    const std::int32_t& view_y;
    const std::int32_t* view_x=nullptr; // Null selects the earlier zero-X profile.
};
using SelectBackgroundResource=void*; // Borrowed host drawing owner, captured after the fill.
class SelectBackgroundHost {
public:
    virtual ~SelectBackgroundHost()=default;
    virtual std::array<float,3> background_pixel()=0; // resSelect.images[RESSELECT_IMG_BG].GetPixel(0,0).
    virtual void background_color(std::int32_t r,std::int32_t g,std::int32_t b,std::int32_t a)=0;
    virtual std::int32_t background_width()=0; // SelectForm.GetGameWidth.
    virtual std::int32_t background_height()=0; // SelectForm.GetGameHeight.
    virtual void fill_background(std::int32_t width,std::int32_t height)=0; // At (0,0).
    virtual SelectBackgroundResource background_resource()=0;
    virtual std::int32_t background_view_height()=0; // Raw GameView height, queried per tile.
    virtual std::int32_t background_clip()=0; // Live RESSELECT_SEB_BG.
    virtual void draw_background(SelectBackgroundResource,std::int32_t clip,std::int32_t frame,float x,float y)=0;
};
// SelectForm._Draw 0x11f0850..0x11f0a6c; stops before DrawDecoObject(0).
// ResetTouchList/IsTopForm and all later town/resident/UI layers belong to the caller.
struct SelectBackgroundOrigin {std::int32_t x,y;}; // Captured after the color fill, before tile callbacks.
SelectBackgroundOrigin draw_select_background(const SelectBackgroundState&,SelectBackgroundHost&);
struct SelectSceneryState {
    EventScript clouds=std::make_shared<std::vector<EventInstruction>>(); // +0x108, eight-int rows.
    EventScript birds,leaves; // +0x198/+0x1a0, six-int rows.
    const std::int32_t& left_extent;
    const std::int32_t& right_extent;
};
class SelectSceneryHost:public SelectViewportHost {
public:
    virtual std::int32_t raw_view_height()=0;
    virtual std::int32_t cloud_right_edge()=0; // Live +0x160 + AppData.VIEW_W.
    virtual std::int32_t scenery_clip(std::int32_t slot)=0; // AppData static field offset.
    virtual std::int32_t app_random(std::int32_t bound)=0;
    virtual std::int32_t lib_random(std::int32_t bound)=0;
    virtual std::int32_t lib_random(std::int32_t minimum,std::int32_t maximum)=0;
    virtual SelectBackgroundResource scenery_resource()=0;
    virtual std::array<std::int32_t,4> scenery_bounds(std::int32_t clip,std::int32_t frame)=0;
    virtual void scenery_reverse(std::int32_t)=0;
    virtual void scenery_draw(SelectBackgroundResource,std::int32_t clip,std::int32_t frame,float x,float y,std::int32_t anchor)=0;
    virtual void scenery_draw_indexed(SelectBackgroundResource,std::int32_t clip,std::int32_t frame,float x,float y,std::int32_t layer,std::int32_t anchor)=0;
};
// SelectForm.Init cloud subsequence, before ChangeState(0)/InitDecoObject.
void initialize_select_scenery_clouds(SelectSceneryState&,SelectSceneryHost&);
EventInstruction generate_select_bird(SelectSceneryState&,bool initial,SelectSceneryHost&);
EventInstruction generate_select_leaf(SelectSceneryState&,bool initial,SelectSceneryHost&);
void initialize_select_decorations(SelectSceneryState&,SelectSceneryHost&);
// DrawDecoObject: types0 (birds) and2 (leaves); other types do not enumerate.
void draw_select_decorations(SelectSceneryState&,std::int32_t type,SelectBackgroundOrigin,SelectSceneryHost&);
// _Draw 0x11f0a80..0x11f0e10: two ground passes, then backward cloud drawing.
// Caller runs DrawDecoObject(0) first; floor/buildings/residents are later layers.
void draw_select_terrain(SelectSceneryState&,SelectBackgroundOrigin,SelectSceneryHost&);
class SelectFloorHost:public SelectViewportHost {
public:
    virtual std::int32_t floor_view_height()=0;
    virtual SebSpriteState* floor_sprite()=0; // Fresh resSelect.seb[RESSELECT_SEB_FLOOR].
    virtual RenderImage* floor_image()=0; // Fresh resSelect.images[RESSELECT_IMG_SELECT_FLOOR].
    virtual void floor_draw(SebSpriteState*,RenderImage*,std::int32_t frame,std::int32_t x,std::int32_t y)=0;
    virtual void floor_depth_layer(std::int32_t)=0;
    virtual SelectBackgroundResource gate_resource()=0;
    virtual std::int32_t gate_clip()=0;
    virtual void gate_draw(SelectBackgroundResource,std::int32_t clip,std::int32_t frame,std::int32_t layer,float x,float y)=0;
};
// _Draw 0x11f0e10..0x11f0fe0: floor ends, middle strips, layer10 and gate0/layer0.
void draw_select_floor(const SelectSceneryState&,SelectBackgroundOrigin,SelectFloorHost&);
struct SelectFormPassState {
    std::int32_t state;
    bool captured;
    bool auto_save;
    FormPushState form;
    SelectMarkerState markers;
    SelectEventPassState events;
};
class SelectFormPassHost {
public:
    virtual ~SelectFormPassHost() = default;
    virtual bool capture_input(std::uint32_t mask) = 0;
    virtual void change_state(std::int32_t value) = 0;
    virtual bool soft_label(std::int32_t index) = 0;
    virtual void finish_capture() = 0;
    virtual void capture() = 0;
    virtual void play_bgm() = 0;
    virtual void update_notifications() = 0;
    virtual bool acquire_balance() = 0;
    virtual void set_active_x() = 0;
    virtual void update_decorations() = 0;
    virtual void update_town_humans() = 0;
    // Fresh SecureInt conversion of normal save_ integer slot 19, each loop test (AppData+0x198).
    virtual std::int32_t town_enable_count() = 0;
    virtual bool update_town_enable(std::int32_t index) = 0;
    virtual void check_guide_arrow(bool force) = 0;
};
enum class SelectPassResult { ReturnTrue, ContinueInput };
// Entry after common MyFormBase/image-animation updates (ELF 0x11ebde8).
// ContinueInput resumes original Update at 0x11ec7ec; it is not a completed frame.
SelectPassResult process_select_form_pass(SelectFormPassState&,SelectFormPassHost&,
    SelectCloudHost&,SelectMarkerHost&,EventDelayQueue&,EventExecutionHost&,EventConditionHost&);
struct SelectUpdatePrefixResult {SelectPreludeSnapshot snapshot;SelectPassResult pass;};
// Includes touch movement and triggers on ContinueInput; next instruction is 0x11eca28.
SelectUpdatePrefixResult update_select_form_prefix(SelectPreludeState&,SelectPreludeHost&,
    SelectFormPassState&,SelectFormPassHost&,SelectCloudHost&,SelectMarkerHost&,
    EventDelayQueue&,EventExecutionHost&,EventConditionHost&,SelectTouchMoveState&,SelectTouchMoveHost&,SelectTriggerHost&);
}
