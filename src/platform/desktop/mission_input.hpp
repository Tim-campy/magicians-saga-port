#pragma once
#include "core/gameplay/input_frame.hpp"
#include "core/gameplay/boss_notification.hpp"
#include <array>
#include <ostream>

namespace saga::desktop {
class MissionHumans;
struct MissionInputScenario {
    std::uint32_t held_keys=0;std::int32_t last_active=0;bool front_pushed=false;
    std::array<core::AttackButtonState,3> buttons;
};
class MissionInput final:private core::InputFrameHost,private core::InputInitializationWorld {
public:
    MissionInput(MissionHumans&,MissionInputScenario,bool defer_init=false);
    void initialize();
    void apply_scenario();
    void advance(std::int32_t pass=0,bool enabled=true);
    void set_keys(std::uint32_t);
    void set_movement(bool up,bool down){set_keys((keyboard_keys_&~0xa0000U)|(up?0x20000U:0U)|(down?0x80000U:0U));}
    void set_fast_forward(bool on){set_keys((keyboard_keys_&~0x10U)|(on?0x10U:0U));}
    void pointer_key(std::int32_t type,std::uint32_t mask);
    bool keyboard_holds(std::uint32_t mask)const{return (keyboard_keys_&mask)!=0;}
    void release_keys(std::uint32_t mask);
    void clear_keys(){keyboard_keys_=pointer_keys_=0;set_keys(0);}
    core::InputControl* control(std::int32_t group,std::int32_t value){for(auto& c:controls_)if(c.touch.group==group&&c.touch.value==value)return &c.touch;return nullptr;}
    void bind_movement_widgets(bool front,bool back){widget_lookup_=true;widget_active_={front,back};}
    void select_attack(std::int32_t);
    void observe_touch(std::function<void(const core::InputTouch*)> receiver){subscribe_input_touch(std::move(receiver));}
    void logical_touch(std::int32_t type,std::int32_t group,std::int32_t value);
    void dispatch(const core::InputTouch*);
    core::MovementControl* movement_control(std::int32_t group,std::int32_t value);
    void release(core::MovementControl&,std::int32_t type,std::int32_t event_id);
    void report(std::ostream&);
private:
    struct Control:core::MovementControl {core::InputControl touch;};
    core::EntitySubset* input_subset(const core::EventIntegers& slots)override;
    void subscribe_input_touch(std::function<void(const core::InputTouch*)>)override;
    bool input_key_state(std::int32_t mask)override;
    core::InputControl* input_control(std::int32_t,std::int32_t)override;
    void send_input_touch(std::int32_t,core::InputControl*)override;
    core::InputHumans* input_humans()override{return &humans_;}
    core::ComponentSlots* input_hero()override;
    core::StageStartState* input_stage()override;
    std::int32_t input_frames_per_second()override;
    MissionHumans& owner_;MissionInputScenario scenario_;core::InputFrameState state_;
    core::InputHumans humans_;std::array<Control,5> controls_;
    using Receiver=std::function<void(const core::InputTouch*)>;
    std::shared_ptr<const std::vector<Receiver>> receivers_;
    std::uint64_t steps_=0,touches_=0;
    std::uint32_t keyboard_keys_=0,pointer_keys_=0;
    bool widget_lookup_=false;std::array<bool,2> widget_active_{};
};
}
