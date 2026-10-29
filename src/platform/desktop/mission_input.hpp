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
    MissionInput(MissionHumans&,MissionInputScenario);
    void advance(std::int32_t pass=0,bool enabled=true);
    void set_keys(std::uint32_t);
    void select_attack(std::int32_t);
    void observe_touch(std::function<void(const core::InputTouch*)> receiver){subscribe_input_touch(std::move(receiver));}
    void logical_touch(std::int32_t type,std::int32_t group,std::int32_t value);
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
};
}
