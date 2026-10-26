#include "select_form_pass.hpp"
namespace saga::core {
SelectUpdatePrefixResult update_select_form_prefix(SelectPreludeState& prelude,SelectPreludeHost& prelude_host,
    SelectFormPassState& state,SelectFormPassHost& host,SelectCloudHost& clouds,SelectMarkerHost& markers,
    EventDelayQueue& queue,EventExecutionHost& execution,EventConditionHost& conditions) {
    const auto snapshot=update_select_prelude(prelude,state.events.loop_counter,prelude_host);
    const auto pass=process_select_form_pass(state,host,clouds,markers,queue,execution,conditions);
    return {snapshot,pass};
}
// SelectForm.Update, ELF 0x11ebde8..0x11ec7e8, with returned-form Push at 0x11ecb30.
SelectPassResult process_select_form_pass(SelectFormPassState& state,SelectFormPassHost& host,
    SelectCloudHost& clouds,SelectMarkerHost& markers,EventDelayQueue& queue,
    EventExecutionHost& execution,EventConditionHost& conditions) {
    if (consume_dialog_update_gate(state.form)) return SelectPassResult::ReturnTrue;
    if (state.state==4) {
        host.capture();
        return SelectPassResult::ReturnTrue;
    }
    if (state.state==3) {
        if (state.captured) return SelectPassResult::ReturnTrue;
        if (host.capture_input(0x100000U)) {
            host.change_state(4); // Host owns ChangeState's ordered labels and final state write.
            return SelectPassResult::ReturnTrue;
        }
        if (host.soft_label(3)) host.finish_capture();
        host.play_bgm();
        return SelectPassResult::ReturnTrue;
    }
    if (state.state!=0) {
        host.play_bgm();
        return SelectPassResult::ReturnTrue;
    }
    state.events.loop_counter=advance_select_loop_counter(state.events.loop_counter);
    update_select_clouds(state.events.loop_counter,clouds);
    refresh_select_markers(state.markers,markers);
    host.update_notifications();
    if (host.acquire_balance()) return SelectPassResult::ReturnTrue;
    host.set_active_x();
    host.update_decorations();
    host.update_town_humans();
    if (state.events.loop_counter<=2) {
        for (std::int32_t index=0;index<host.town_enable_count();++index) {
            if (host.update_town_enable(index)) {
                // ExecEvent(int) defaults both contexts; its result is discarded here.
                execute_indexed_event(1,{},queue,execution);
            }
        }
        host.check_guide_arrow(true);
    }
    const auto form=process_select_event_pass(state.events,queue,execution,conditions);
    if (form) {
        push_select_form(state.form,form,state.auto_save,state.events.event_check_requested);
        return SelectPassResult::ReturnTrue;
    }
    return SelectPassResult::ContinueInput;
}
}
