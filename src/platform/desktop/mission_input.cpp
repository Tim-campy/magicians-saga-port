#include "mission_humans.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <stdexcept>

namespace saga::desktop {
using namespace core;
MissionInput::MissionInput(MissionHumans& owner,MissionInputScenario scenario)
    :owner_(owner),scenario_(std::move(scenario)),humans_{owner.system_,static_cast<HumanFrameHost&>(owner)}{
    const auto attribute=owner.stage_.attack_attribute;
    if(attribute<0||attribute>2||scenario_.last_active< -1||scenario_.last_active>2)throw std::invalid_argument("input requires native attack attributes0..2 and last-active -1..2");
    if(scenario_.buttons.at(static_cast<std::size_t>(attribute)).state!=owner.button_.state)throw std::invalid_argument("input button state conflicts with human_system");
    if(owner.notification_scenario_&&scenario_.held_keys!=owner.held_keys_)throw std::invalid_argument("input held keys conflict with stage_notifications");
    controls_[0].touch={35,0x10011};controls_[1].touch={35,0x10013};
    for(std::int32_t i=0;i<3;++i)controls_.at(static_cast<std::size_t>(i+2)).touch={10,0x230000|i};
    owner.held_keys_=scenario_.held_keys;owner.stage_.last_active_button=scenario_.last_active;owner.stage_.move_front_pushed=scenario_.front_pushed;
    owner.stage_.attack_buttons=std::make_shared<std::vector<AttackButtonState*>>();
    for(auto& button:scenario_.buttons)owner.stage_.attack_buttons->push_back(&button);
    initialize_input(state_,this,*this);
    subscribe_input_touch([this](const InputTouch* e){stage_touch(owner_.stage_,e);}); // Event9: Input before Stage.
} // SAGA_STUB(mission-control-surfaces): stable native IDs/lifetime; SDL supplies keys, no Surface widgets.
EntitySubset* MissionInput::input_subset(const EventIntegers& slots){return owner_.world_.subset(slots);}
void MissionInput::subscribe_input_touch(Receiver f){auto next=receivers_?std::make_shared<std::vector<Receiver>>(*receivers_):std::make_shared<std::vector<Receiver>>();next->push_back(std::move(f));receivers_=std::move(next);}
bool MissionInput::input_key_state(std::int32_t mask){return (owner_.held_keys_&static_cast<std::uint32_t>(mask))!=0;}
InputControl* MissionInput::input_control(std::int32_t group,std::int32_t value){for(auto& c:controls_)if(c.touch.group==group&&c.touch.value==value)return &c.touch;return nullptr;}
void MissionInput::send_input_touch(std::int32_t type,InputControl* c){const auto receivers=receivers_;InputTouch e{type,c};if(receivers)for(const auto& f:*receivers)f(&e);++touches_;}
ComponentSlots* MissionInput::input_hero(){return member_hero(&owner_.world_.player_world());}
StageStartState* MissionInput::input_stage(){return &owner_.stage_;}
std::int32_t MissionInput::input_frames_per_second(){return owner_.runtime_.fps;}
void MissionInput::advance(std::int32_t pass,bool enabled){if(gameplay_needs_update(0,enabled,pass)){update_input(state_,*this);++steps_;}}
void MissionInput::set_keys(std::uint32_t keys){owner_.held_keys_=keys;}
void MissionInput::select_attack(std::int32_t index){if(index<0||index>2)throw std::invalid_argument("attack control index0..2 required");send_input_touch(2,&controls_.at(static_cast<std::size_t>(index+2)).touch);}
void MissionInput::logical_touch(std::int32_t type,std::int32_t group,std::int32_t value){InputControl control{group,value};send_input_touch(type,&control);}
MovementControl* MissionInput::movement_control(std::int32_t group,std::int32_t value){for(auto& c:controls_)if(c.touch.group==group&&c.touch.value==value)return &c;return nullptr;}
void MissionInput::release(MovementControl& control,std::int32_t type,std::int32_t event_id){
    if(event_id!=9)throw std::logic_error("mission control event is unbound");
    for(auto& c:controls_)if(static_cast<MovementControl*>(&c)==&control){send_input_touch(type,&c.touch);return;}
    throw std::invalid_argument("foreign mission movement control");
}
void MissionInput::report(std::ostream& out){
    out<<",\"input_steps\":"<<steps_<<",\"input_frame\":"<<state_.frame<<",\"last_down_frame\":"<<state_.last_down_frame
       <<",\"input_held_keys\":"<<owner_.held_keys_<<",\"input_touches\":"<<touches_<<",\"attack_attribute\":"<<owner_.stage_.attack_attribute
       <<",\"last_active_button\":"<<owner_.stage_.last_active_button<<",\"move_front_pushed\":"<<(owner_.stage_.move_front_pushed?"true":"false")<<",\"attack_buttons\":[";
    bool first=true;for(const auto& b:scenario_.buttons){if(!first)out<<',';first=false;out<<"{\"state\":"<<b.state<<",\"frame\":"<<b.frame<<'}';}out<<']';
}
void MissionHumans::enable_input(MissionInputScenario scenario){if(input_)throw std::invalid_argument("input requires one fresh state");input_=std::make_unique<MissionInput>(*this,std::move(scenario));}
void MissionHumans::advance_input(std::int32_t pass,bool enabled){if(input_)input_->advance(pass,enabled);}
void MissionHumans::set_input_keys(std::uint32_t value){if(!input_)throw std::logic_error("input fresh is required");input_->set_keys(value);}
void MissionHumans::select_input_attack(std::int32_t value){if(!input_)throw std::logic_error("input fresh is required");input_->select_attack(value);}
void MissionHumans::observe_input_touch(std::function<void(const InputTouch*)> receiver){if(!input_)throw std::logic_error("input fresh is required");input_->observe_touch(std::move(receiver));}
void MissionHumans::send_logical_touch(std::int32_t type,std::int32_t group,std::int32_t value){if(!input_)throw std::logic_error("input fresh is required");input_->logical_touch(type,group,value);}
void MissionHumans::write_input_report(std::ostream& out){out<<",\"input_enabled\":"<<(input_?"true":"false");if(input_)input_->report(out);}
}
