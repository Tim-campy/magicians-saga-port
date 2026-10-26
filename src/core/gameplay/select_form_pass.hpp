#pragma once
#include "form_push.hpp"
#include "select_clouds.hpp"
#include "select_markers.hpp"
#include "select_event_pass.hpp"
#include "select_prelude.hpp"
#include "select_touch_move.hpp"
namespace saga::core {
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
    // Fresh SecureInt conversion of sysSave integer slot 19, each loop test.
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
// Includes touch movement on ContinueInput; next original instruction is 0x11ec948.
SelectUpdatePrefixResult update_select_form_prefix(SelectPreludeState&,SelectPreludeHost&,
    SelectFormPassState&,SelectFormPassHost&,SelectCloudHost&,SelectMarkerHost&,
    EventDelayQueue&,EventExecutionHost&,EventConditionHost&,SelectTouchMoveState&,SelectTouchMoveHost&);
}
