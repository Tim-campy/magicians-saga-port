#pragma once
#include "event_execution.hpp"
namespace saga::core {
struct SelectEventPassState {
    std::int32_t loop_counter;
    bool event_check_requested;
};
// State-0 counter update in SelectForm.Update. Separate from the reached event
// block: earlier UI guards and initialization remain the caller's responsibility.
std::int32_t advance_select_loop_counter(std::int32_t value);
// Called only after the original dialog/state/balance gates and town updates.
// Does NOT advance the loop counter, run an entire form update, or push a form.
EventObject process_select_event_pass(SelectEventPassState&,EventDelayQueue&,
                                     EventExecutionHost&,EventConditionHost&);
}
