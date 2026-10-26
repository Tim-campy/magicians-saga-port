#include "select_event_pass.hpp"
#include <limits>
namespace saga::core {
// SelectForm.Update 0x11ebe34..0x11ebe60: wrapped increment modulo INT32_MAX.
std::int32_t advance_select_loop_counter(std::int32_t value) {
    const auto next=value==std::numeric_limits<std::int32_t>::max()
        ? std::numeric_limits<std::int32_t>::min() : value+1;
    return next % std::numeric_limits<std::int32_t>::max();
}
// SelectForm.Update 0x11ec578..0x11ec7e8. The later non-null-result path owns Push.
EventObject process_select_event_pass(SelectEventPassState& state,EventDelayQueue& queue,
                                     EventExecutionHost& host,EventConditionHost& conditions) {
    process_counted_event_delays(queue,host);
    // Original multiply/add/rotate test is equivalent to signed % 300 == 0.
    // Read after resumption, since callbacks can change shared application state.
    if (state.loop_counter%300==0) state.event_check_requested=true;
    if (!state.event_check_requested) return nullptr;
    auto form=select_and_execute_events(queue,host,conditions);
    if (!form) state.event_check_requested=false;
    return form;
}
}
