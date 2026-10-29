#pragma once
#include "human_frame.hpp"
#include <functional>

namespace saga::core {
// TouchComponent.id/value at +0x18/+0x2c; identity/lifetime belong to the host.
struct InputControl {std::int32_t group=0,value=0;};
struct InputTouch {std::int32_t type=0;InputControl* component=nullptr;};
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
