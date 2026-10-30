#pragma once
#include "event_flow.hpp"
#include <cstdint>

namespace saga::core {
// cfg.MyConfig..cctor 0x1174274: scalar defaults at static+0x50/+0x4.
// This projection does not include Config's independent controls/debug flags or later overrides.
struct InitialGameFrameConfiguration {std::int32_t fps;bool damage_debug;};
inline constexpr InitialGameFrameConfiguration initial_game_frame_configuration{30,false};
struct QuestFrameState {
    std::int32_t loop_counter=0,frame=0,pinch_frame=0;
    bool pinching=false;
    EventObject dialog; // MyFormBase.dialog_ +0xc0; distinct from StageSystem's answer dialog.
    std::int32_t fade_type=0,fade_state=0;
};
class QuestFrameHost {
public:
    virtual ~QuestFrameHost()=default;
    virtual void base_update()=0;
    virtual bool flag_check(std::uint32_t)=0;
    virtual void flag_off(std::uint32_t)=0;
    virtual void reset_image_clocks()=0;
    virtual void advance_image_clocks()=0;
    virtual bool config_debug_controls()=0; // Config static byte +0x10, queried afresh.
    virtual bool key_state(std::uint32_t)=0;
    virtual bool key_pulse(std::uint32_t)=0;
    virtual bool engine_key_down(std::int32_t)=0;
    virtual void debug_show_result()=0;
    virtual void debug_complete_quest()=0;
    virtual void queue_debug_return()=0;
    virtual void push_debug_defeat(std::int32_t)=0;
    virtual void push_debug_victory(std::int32_t,bool)=0;
    virtual bool active()=0;
    virtual bool is_top_form()=0;
    virtual void update_world(std::int32_t pass)=0;
    virtual void change_state(std::int32_t)=0;
};
// QuestForm.Update 0x11daee4. The native bool result is a form-manager result,
// not permission for a desktop host to stop the game. fade_frame aliases Stage's QuestForm.
bool update_quest_frame(QuestFrameState&,std::int32_t& fade_frame,QuestFrameHost&);
}
