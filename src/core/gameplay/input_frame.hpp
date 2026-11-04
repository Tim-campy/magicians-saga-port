#pragma once
#include "human_frame.hpp"
#include <functional>
#include <array>

namespace saga::core {
// TouchComponent.id/value at +0x18/+0x2c; identity/lifetime belong to the host.
struct InputControl {std::int32_t group=0,value=0;};
struct InputTouch {std::int32_t type=0;InputControl* component=nullptr;};
struct InputRectangle {std::int32_t x=0,y=0,width=0,height=0;};
struct CameraTouchRegion {
    InputRectangle bounds;InputControl control{17,0};
    float drag_x=1.0F,drag_y=1.0F;std::int32_t flags=0;
};
// QuestForm.Draw0x11dde24 and SelectForm.Draw0x11ef4d4 use the same registration.
inline CameraTouchRegion camera_touch_region(InputRectangle view,bool first_touch){
    return {first_touch?InputRectangle{-1000,-1000,2000,2000}:view,{17,0},1.0F,1.0F,first_touch?40:0};
}
// SurfaceBase.CheckHitPointRect 0x1c47c30: half-open edges, wrapping int32 sums.
bool input_hit_rectangle(const InputRectangle&,std::int32_t x,std::int32_t y);
// Selected GameView._addTouch path: margin override, SCREEN=1, origin0/density100.
InputRectangle input_screen_rectangle(InputRectangle,std::array<std::int32_t,4> margins,std::int32_t width,std::int32_t height);
// Equal-priority, mutually colliding registrations; preserves insertion order.
void partition_input_rectangles(InputRectangle& earlier,InputRectangle& later);
struct InputFrameState {
    EntitySubset* subset=nullptr;bool old_up=false,old_down=false;
    std::int32_t frame=0,last_down_frame=0;
};
struct InputHumans {HumanFrameSystem& state;HumanFrameHost& host;};
class InputFrameHost {
public:
    virtual ~InputFrameHost()=default;
    virtual bool input_key_state(std::int32_t mask)=0; // Resolved Canvas.CheckKeyState; platform policy is external.
    virtual InputControl* input_control(std::int32_t group,std::int32_t value)=0;
    virtual void send_input_touch(std::int32_t type,InputControl*)=0; // Capture World, construct event, send9.
    virtual InputHumans* input_humans()=0; // Fresh World.GetSystem<HumanSystem>, nullable until used.
    virtual ComponentSlots* input_hero()=0; // Required MemberSystem then actual global hero query.
    virtual StageStartState* input_stage()=0;
    virtual std::int32_t input_frames_per_second()=0;
};
class InputInitializationWorld {
public:
    virtual ~InputInitializationWorld()=default;
    virtual EntitySubset* input_subset(const EventIntegers&)=0;
    virtual void subscribe_input_touch(std::function<void(const InputTouch*)>)=0;
};
void initialize_input(InputFrameState&,InputInitializationWorld*,InputFrameHost&); // InputSystem.Init 0x115b3cc.
void update_input(InputFrameState&,InputFrameHost&); // InputSystem.Update 0x115b4fc; scheduler owns pass0 gate.
void input_touch(const InputTouch*,InputFrameHost&); // OnTouchEvent 0x115b8cc.
void stage_touch(StageStartState&,const InputTouch*); // StageSystem.OnTouchEvent 0x1166564.
void inactivate_attack_buttons(StageStartState&); // InactiveAttackButton 0x115bbf0.
}
