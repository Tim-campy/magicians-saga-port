#include "quest_frame.hpp"
#include "select_event_pass.hpp"

namespace saga::core {
namespace {
std::int32_t add_one(std::int32_t value){
    const auto bits=static_cast<std::uint32_t>(value)+1U;
    return static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);
}
}
bool update_quest_frame(QuestFrameState& state,std::int32_t& fade_frame,QuestFrameHost& host){
    host.base_update();
    if(host.flag_check(0x100U))host.flag_off(0x100U);
    if(state.loop_counter==0)host.reset_image_clocks();
    host.advance_image_clocks();
    state.loop_counter=advance_select_loop_counter(state.loop_counter);state.frame=state.loop_counter;
    if(state.dialog)state.dialog.reset();
    const auto passes=host.config_debug_controls()&&host.key_state(0x10U)?3:1;
    if(host.config_debug_controls()&&(host.key_pulse(0x20U)||host.engine_key_down(0x35)))host.debug_show_result();
    if(host.config_debug_controls()&&(host.key_pulse(0x40U)||host.engine_key_down(0x36))){host.debug_complete_quest();host.queue_debug_return();}
    if(host.config_debug_controls()&&(host.key_pulse(0x80U)||host.engine_key_down(0x37)))host.push_debug_defeat(1);
    if(host.config_debug_controls()&&(host.key_pulse(0x100U)||host.engine_key_down(0x38)))host.push_debug_victory(65,true);
    for(std::int32_t pass=0;pass<passes;++pass){
        if(pass!=0){if(!host.active())continue;if(!host.is_top_form())return false;}
        host.update_world(pass);
    }
    if(!state.pinching&&state.pinch_frame<=39)state.pinch_frame=add_one(state.pinch_frame);
    const auto fade_type=state.fade_type;
    if(fade_type!=-1){
        fade_frame=add_one(fade_frame);
        if(fade_frame>=21){
            fade_frame=0;
            if(fade_type==1){state.fade_type=2;host.change_state(state.fade_state);state.fade_state=-1;}
            else{state.fade_type=-1;return false;}
        }
    }
    return true;
}
}
