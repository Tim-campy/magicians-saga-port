#include "input_frame.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing input frame value");return *p;}
template<class T>T& field(ComponentSlots& e,int slot){return required(dynamic_cast<T*>(e.get(slot)));}
std::int32_t add(std::int32_t a,std::int32_t b){const auto v=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
bool dead(HumanRecordLink& h){return (static_cast<std::uint32_t>(h.state)&0xfffffffeU)==4U;}
bool movement(const InputControl& c){return c.group==35&&(static_cast<std::uint32_t>(c.value)&0xffff0000U)==0x10000U&&((c.value&0xffff)|2)==19;}
}
void initialize_input(InputFrameState& s,InputInitializationWorld* world,InputFrameHost& host){
    s.subset=required(world).input_subset({28,0,1});s.last_down_frame=-1;
    world->subscribe_input_touch([&host](const InputTouch* e){input_touch(e,host);});
}
void update_input(InputFrameState& s,InputFrameHost& host){
    const bool up=host.input_key_state(0x20000),down=host.input_key_state(0x80000);
    auto* first=host.input_control(35,0x10011);auto* second=host.input_control(35,0x10013);
    if(first){if(up&&!s.old_up){host.send_input_touch(0,first);s.last_down_frame=s.frame;}
        else if(!up&&s.old_up)host.send_input_touch(1,first);}
    if(second){if(down&&!s.old_down){host.send_input_touch(0,second);s.last_down_frame=s.frame;}
        else if(!down&&s.old_down)host.send_input_touch(1,second);}
    s.old_up=up;s.old_down=down;
    auto* humans=host.input_humans();
    if(up){const auto last=s.last_down_frame;if(last>=1){const auto frame=s.frame;
        const auto threshold=add(last,host.input_frames_per_second());if(frame>=threshold)accelerate_humans(required(humans).state);}}
    else{s.last_down_frame=-1;reset_human_speeds(required(humans).state);}
    s.frame=add(s.frame,1)%std::numeric_limits<std::int32_t>::max();
}
void inactivate_attack_buttons(StageStartState& s){
    const auto buttons=s.attack_buttons;required(buttons.get());
    for(auto* p:*buttons){auto& b=required(p);if(b.state==1)change_attack_button_state(b,0);}
}
void input_touch(const InputTouch* event,InputFrameHost& host){
    const auto& e=required(event);const auto& c=required(e.component);if(!movement(c))return;
    const auto type=e.type,control_value=c.value;
    auto& hero=required(host.input_hero());if(dead(field<HumanRecordLink>(hero,14)))return;
    if(static_cast<std::uint32_t>(type)>13U)return;
    const auto bit=1U<<static_cast<std::uint32_t>(type);
    if((bit&0x44aU)!=0){
        if(field<HumanRecordLink>(hero,14).state==0||field<HumanRecordLink>(hero,14).state==3||field<HumanRecordLink>(hero,14).state==6)return;
        field<ProjectileSpeed>(hero,1).z=0;
        auto& human=required(host.input_humans());change_human_state(hero,0,human.host);return;
    }
    if((bit&0x2005U)==0)return;
    if(field<HumanRecordLink>(hero,14).state!=1&&field<HumanRecordLink>(hero,14).state!=3&&field<HumanRecordLink>(hero,14).state!=6){
        inactivate_attack_buttons(required(host.input_stage()));
        auto& human=required(host.input_humans());change_human_state(hero,1,human.host);
    }
    if(field<HumanRecordLink>(hero,14).state!=1)return;
    auto& human=required(host.input_humans());const float speed=human_move_speed(human.state,&hero,human.host);
    field<ProjectileSpeed>(hero,1).z=(control_value&0xffff)==17?-speed:speed;
}
void stage_touch(StageStartState& s,const InputTouch* event){
    const auto& e=required(event);const auto& c=required(e.component);const auto value=static_cast<std::uint32_t>(c.value);
    if(c.group==10&&e.type==2&&(value&0xffff0000U)==0x230000U){
        auto* selected=&required(required(s.attack_buttons.get()).at(value&0xffffU));if(selected->state!=0)return;
        change_attack_button_state(*selected,1);s.attack_attribute=static_cast<std::int32_t>(value&0xffffU);s.last_active_button=s.attack_attribute;
        const auto buttons=s.attack_buttons;required(buttons.get());
        for(auto* p:*buttons)if(p!=selected){auto& b=required(p);if(b.state==1)change_attack_button_state(b,0);}return;
    }
    if(!movement(c))return;
    if((value&0xffffU)==17)s.move_front_pushed=true;
    if(static_cast<std::uint32_t>(e.type)>10U||( (1U<<static_cast<std::uint32_t>(e.type))&0x44aU)==0||s.last_active_button==-1)return;
    auto& button=required(required(s.attack_buttons.get()).at(static_cast<std::size_t>(s.last_active_button)));
    if(button.state==0){change_attack_button_state(button,1);s.attack_attribute=s.last_active_button;}
}
}
