#include "select_form_pass.hpp"
#include "entity_visual.hpp"
#include <stdexcept>
namespace saga::core {
namespace {
std::int32_t background_add(std::int32_t a,std::int32_t b){
    const auto bits=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):-1-static_cast<std::int32_t>(0xffffffffU-bits);
}
}
void draw_select_background(const SelectBackgroundState& state,SelectBackgroundHost& host){
    const auto pixel=host.background_pixel();
    host.background_color(factory_cell_coordinate(pixel[0]*255.0F,1),factory_cell_coordinate(pixel[1]*255.0F,1),factory_cell_coordinate(pixel[2]*255.0F,1),255);
    const auto width=host.background_width();const auto height=host.background_height();
    host.fill_background(width,height);
    const auto base_y=background_add(state.camera_y,state.view_y);
    auto x=state.left_extent;
    SelectBackgroundResource resource=nullptr;
    for(;;){
        const auto limit=state.right_extent;
        resource=host.background_resource();
        if(x>=limit)break;
        const auto view_height=host.background_view_height();
        if(!resource)throw std::invalid_argument("missing select background resource");
        const auto clip=host.background_clip();
        const auto y=background_add(background_add(base_y,-125),view_height);
        host.draw_background(resource,clip,0,static_cast<float>(x),static_cast<float>(y));
        x=background_add(x,240);
    }
    const auto parallax_x=background_add(state.camera_x/30,30);
    if(!resource)throw std::invalid_argument("missing select background resource");
    const auto clip=host.background_clip();
    host.draw_background(resource,clip,1,static_cast<float>(parallax_x),static_cast<float>(background_add(base_y,20)));
}
SelectUpdatePrefixResult update_select_form_prefix(SelectPreludeState& prelude,SelectPreludeHost& prelude_host,
    SelectFormPassState& state,SelectFormPassHost& host,SelectCloudHost& clouds,SelectMarkerHost& markers,
    EventDelayQueue& queue,EventExecutionHost& execution,EventConditionHost& conditions,
    SelectTouchMoveState& movement,SelectTouchMoveHost& movement_host,SelectTriggerHost& triggers) {
    const auto snapshot=update_select_prelude(prelude,state.events.loop_counter,prelude_host);
    const auto pass=process_select_form_pass(state,host,clouds,markers,queue,execution,conditions);
    if(pass==SelectPassResult::ContinueInput) {
        update_select_touch_move(movement,movement_host);
        process_select_triggers(triggers,queue,execution);
    }
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
