#pragma once
#include "form_push.hpp"
#include "select_clouds.hpp"
#include "select_markers.hpp"
#include "select_event_pass.hpp"
#include "select_prelude.hpp"
#include "select_touch_move.hpp"
#include "select_triggers.hpp"
#include <array>
namespace saga::core {
struct SelectBackgroundState {
    const std::int32_t& camera_x;
    const std::int32_t& camera_y;
    const std::int32_t& left_extent;
    const std::int32_t& right_extent;
    const std::int32_t& view_y;
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
void draw_select_background(const SelectBackgroundState&,SelectBackgroundHost&);
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
