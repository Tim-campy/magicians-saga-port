#include "input_frame.hpp"
#include <limits>
#include <algorithm>
#include <stdexcept>

namespace saga::core {
SelectAreaTouchRegion select_area_touch_region(std::int32_t index,bool selected,bool locked,
    std::int32_t x,std::int32_t y,std::int32_t width,std::int32_t height){
    const auto value=selected?28U:static_cast<std::uint32_t>(index)|0xc0000U;
    const auto signed_value=value<=0x7fffffffU?static_cast<std::int32_t>(value):-1-static_cast<std::int32_t>(0xffffffffU-value);
    const std::array<std::int32_t,4> margins=locked?std::array<std::int32_t,4>{5,5,20,5}:std::array<std::int32_t,4>{15,15,50,5};
    return {input_screen_rectangle({x,y,selected?29:129,selected?37:82},margins,width,height),{selected?4:29,signed_value}};
}
SelectAreaTouchRegion select_resident_gift_touch_region(std::int32_t x,std::int32_t y,
    std::int32_t width,std::int32_t height){
    // DrawTownHuman0x11f3d18..0x11f3d60 through MyFormBase.AddTouchListExScale.
    // The reached desktop profile has scale100; preserve the selector in value.
    return {input_screen_rectangle({x,y,32,32},{10,10,10,10},width,height),{4,0x10019}};
}
SelectAreaTouchRegion select_building_touch_region(std::int32_t index,bool selected,
    std::int32_t x,std::int32_t y,std::int32_t width,std::int32_t height){
    const auto value=static_cast<std::uint32_t>(index)|0x220000U;
    const auto encoded=value<=0x7fffffffU?static_cast<std::int32_t>(value):-1-static_cast<std::int32_t>(0xffffffffU-value);
    return {input_screen_rectangle({x,y,80,90},{-10,-10,-10,selected?25:-10},width,height),{selected?4:10,selected?27:encoded}};
}
void select_widget_touch(const InputTouch* event,SelectWidgetHost& host){
    if(!event||!event->component)throw std::invalid_argument("missing town widget event/control");
    const auto& control=*event->component;
    auto encoded=static_cast<std::uint32_t>(control.value);
    const auto set=[&]{const auto flags=encoded&0xffff0000U;const auto low=encoded&0xffffU;
        if(flags)host.select_value(flags,low<=0x7fffU?static_cast<std::int32_t>(low):static_cast<std::int32_t>(low)-0x10000);};
    if(control.group==10){
        // GameView.OnTouchList0x10fd17c: reached option flags9 has no bit16.
        if(event->type!=1&&event->type!=2)return;
        set();if(event->type==1)host.click_key(0x100000U);return;
    }
    if(control.group==29){
        if(event->type!=1&&event->type!=2)return;
        set();host.click_key(event->type==1?0x100000U:0x80000U);return;
    }
    if(control.group!=4)throw std::invalid_argument("unsupported town widget group");
    if(encoded==0)encoded=20;
    if(event->type!=5)return;
    if(encoded&0xffff0000U)set();else host.click_key(1U<<(encoded&31U));
}
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing input frame value");return *p;}
template<class T>T& field(ComponentSlots& e,int slot){return required(dynamic_cast<T*>(e.get(slot)));}
std::int32_t add(std::int32_t a,std::int32_t b){const auto v=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t sub(std::int32_t a,std::int32_t b){return add(a,add(~b,1));}
bool dead(HumanRecordLink& h){return (static_cast<std::uint32_t>(h.state)&0xfffffffeU)==4U;}
bool movement(const InputControl& c){return c.group==35&&(static_cast<std::uint32_t>(c.value)&0xffff0000U)==0x10000U&&((c.value&0xffff)|2)==19;}
}
bool input_hit_rectangle(const InputRectangle& r,std::int32_t x,std::int32_t y){
    if(r.x>x||r.y>y)return false;
    return add(r.x,r.width)>x&&add(r.y,r.height)>y;
}
InputRectangle input_screen_rectangle(InputRectangle r,std::array<std::int32_t,4> m,std::int32_t width,std::int32_t height){
    const auto left=std::max(sub(r.x,m[0]),0),top=std::max(sub(r.y,m[2]),0);
    const auto right=std::min(add(add(r.x,r.width),m[1]),width),bottom=std::min(add(add(r.y,r.height),m[3]),height);
    return {left,top,sub(right,left),sub(bottom,top)};
}
std::array<std::int32_t,2> construction_menu_origin(std::int32_t width,std::int32_t height){
    if(width<=240&&height<=240)return {0,0};
    return {add(width,-240)/2,add(height,-240)/2};
}
SelectAreaTouchRegion construction_row_touch_region(std::int32_t index,std::int32_t width,std::int32_t height){
    const auto origin=construction_menu_origin(width,height);
    return {input_screen_rectangle({add(origin[0],add(106*(index&1),16)),add(origin[1],add((index>>1)*54,14)),104,52},{0,0,0,0},width,height),{10,0x20000|index}};
}
void partition_input_rectangles(InputRectangle& a,InputRectangle& b){
    // GameView._addTouch 0x10fa2bc..0x10fa3d4, after mask/priority checks.
    auto vertical=sub(add(a.y,a.height),b.y);if(vertical<0)return;
    auto horizontal=sub(add(a.x,a.width),b.x);if(horizontal<0)return;
    if(add(b.x,b.width)<a.x||add(b.y,b.height)<a.y)return;
    if(b.y<a.y)vertical=sub(add(b.y,b.height),a.y);
    if(b.x<a.x)horizontal=sub(add(b.x,b.width),a.x);
    const bool split_x=horizontal<vertical&&(b.x!=a.x||a.width!=b.width);
    const bool same_vertical=b.y==a.y&&a.height==b.height;
    if(split_x||same_vertical){
        auto& first=b.x<=a.x?b:a;auto& last=b.x<=a.x?a:b;
        const auto half=horizontal/2,rest=sub(horizontal,half);
        first.width=sub(first.width,half);last.x=add(last.x,rest);last.width=sub(last.width,rest);
    }else{
        auto& first=b.y<=a.y?b:a;auto& last=b.y<=a.y?a:b;
        const auto half=vertical/2,rest=sub(vertical,half);
        first.height=sub(first.height,half);last.y=add(last.y,rest);last.height=sub(last.height,rest);
    }
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
