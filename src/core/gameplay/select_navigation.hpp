#pragma once
#include "select_form_pass.hpp"
#include <functional>
namespace saga::core {
class SelectNavigationHost {
public:
    virtual ~SelectNavigationHost() = default;
    virtual bool pressed(std::uint32_t mask) = 0;
    virtual void play_button_sound() = 0; // RESSOUND_SE_BUTTON.
    // Retain the callback until the transition's original callback point.
    virtual void make_transition(std::int32_t type,bool option,std::uint32_t argb,std::function<void()> callback) = 0;
    virtual EventObject room_form() = 0; // Requires AppData; frmRoom_ itself can be null.
    virtual EventObject make_shop(std::int32_t type) = 0;
};
enum class SelectNavigationResult { ReturnTrue, ContinueBuildingButtons };
void complete_select_room_transition(SelectFormPassState&,SelectNavigationHost&);
// Entry 0x11eca28, after prefix ContinueInput; no-button continuation 0x11ecb5c.
// Shared owners model the deferred delegate retaining its original form/context.
SelectNavigationResult process_select_navigation(std::shared_ptr<SelectFormPassState>,std::shared_ptr<SelectNavigationHost>);
struct SelectUpdateBindings {
    SelectPreludeState& prelude;
    SelectPreludeHost& prelude_host;
    std::shared_ptr<SelectFormPassState> state;
    SelectFormPassHost& pass_host;
    SelectCloudHost& clouds;
    SelectMarkerHost& markers;
    EventDelayQueue& queue;
    EventExecutionHost& execution;
    EventConditionHost& conditions;
    SelectTouchMoveState& movement;
    SelectTouchMoveHost& movement_host;
    SelectTriggerHost& triggers;
    std::shared_ptr<SelectNavigationHost> navigation;
};
struct SelectNavigationUpdate {SelectPreludeSnapshot snapshot;SelectNavigationResult result;};
// Composed entry through the two initial buttons; early prefix returns skip input.
SelectNavigationUpdate update_select_through_navigation(SelectUpdateBindings&);
}
