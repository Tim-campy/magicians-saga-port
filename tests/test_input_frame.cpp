#include "human_frame_fixture.hpp"
#include "core/gameplay/input_frame.hpp"
#include <cmath>

struct Inputs final:InputFrameHost,InputInitializationWorld {
    HumanFixture t;InputFrameState state;InputHumans humans{t.host.system,t.host};
    InputControl up{35,0x10011},down{35,0x10013};InputControl* down_result=&down;
    bool held_up=false,held_down=false,missing_up=false,missing_humans=false,missing_hero=false;
    std::vector<std::string> calls;std::vector<std::function<void(const InputTouch*)>> receivers;
    std::function<void()> on_send,on_hero;
    Inputs(){initialize_input(state,this,*this);receivers.push_back([this](const auto* e){stage_touch(t.f.state,e);});calls.clear();t.flat_parameters();}
    EntitySubset* input_subset(const EventIntegers& slots)override{check(slots==EventIntegers{28,0,1});return &t.f.groups.get(std::make_shared<ComponentGroupFilter>(slots));}
    void subscribe_input_touch(std::function<void(const InputTouch*)> f)override{receivers.push_back(std::move(f));}
    bool input_key_state(std::int32_t mask)override{calls.push_back("key:"+std::to_string(mask));return mask==0x20000?held_up:held_down;}
    InputControl* input_control(std::int32_t group,std::int32_t id)override{check(group==35);calls.push_back("control:"+std::to_string(id));return id==0x10011?(missing_up?nullptr:&up):down_result;}
    void send_input_touch(std::int32_t type,InputControl* c)override{calls.push_back("touch:"+std::to_string(type)+":"+std::to_string(c->value));if(on_send)on_send();InputTouch e{type,c};for(const auto& f:receivers)f(&e);}
    InputHumans* input_humans()override{calls.push_back("humans");return missing_humans?nullptr:&humans;}
    ComponentSlots* input_hero()override{if(on_hero)on_hero();return missing_hero?nullptr:&t.hero();}
    StageStartState* input_stage()override{return &t.f.state;}
    std::int32_t input_frames_per_second()override{calls.push_back("fps");return t.f.fps;}
    void update(){update_input(state,*this);}
};
int main()try{
    {const InputRectangle r{-3,4,10,20};check(input_hit_rectangle(r,-3,4)&&input_hit_rectangle(r,6,23));
     check(!input_hit_rectangle(r,-4,4)&&!input_hit_rectangle(r,7,4)&&!input_hit_rectangle(r,-3,24));
     check(!input_hit_rectangle({0,0,0,4},0,0)&&!input_hit_rectangle({0,0,4,0},0,0));
     check(!input_hit_rectangle({std::numeric_limits<std::int32_t>::max(),0,1,1},std::numeric_limits<std::int32_t>::max(),0));
     check(input_hit_rectangle({std::numeric_limits<std::int32_t>::min()+2,0,-10,1},0,0));} // Native wrapping can make a negative width hit.
    {Inputs h;h.state.old_up=true;h.state.frame=42;h.state.last_down_frame=9;initialize_input(h.state,&h,h);
     check(h.state.old_up&&h.state.frame==42&&h.state.last_down_frame==-1);fails([&]{initialize_input(h.state,nullptr,h);});}
    {Inputs h;h.held_up=h.held_down=true;h.on_send=[&]{h.down_result=nullptr;};h.update();
     check(h.calls[0]=="key:131072"&&h.calls[1]=="key:524288"&&h.calls[2]=="control:65553"&&h.calls[3]=="control:65555");
     check(h.calls[4]=="touch:0:65553");check(std::find(h.calls.begin(),h.calls.end(),"touch:0:65555")!=h.calls.end());
     check(h.state.old_up&&h.state.old_down&&h.state.frame==1);}
    {Inputs h;h.held_up=true;h.update();for(int i=0;i<80;++i)h.update();check(field<HumanRecordLink>(h.t.hero(),14).dash_speed==0);
     h.held_up=false;h.update();h.held_up=true;h.update();for(int i=0;i<80;++i)h.update();
     check(field<HumanRecordLink>(h.t.hero(),14).dash_speed==2.0F);
     h.held_up=false;h.held_down=true;h.update();check(field<HumanRecordLink>(h.t.hero(),14).dash_speed==0);}
    {Inputs h;h.missing_up=true;h.held_up=true;h.update();h.missing_up=false;h.calls.clear();h.update();
     check(std::none_of(h.calls.begin(),h.calls.end(),[](const auto& s){return s.rfind("touch:",0)==0;}));}
    {Inputs h;h.state.frame=std::numeric_limits<std::int32_t>::max();h.update();check(h.state.frame==-1);}
    {Inputs h;h.held_up=h.state.old_up=true;h.state.last_down_frame=std::numeric_limits<std::int32_t>::max();h.state.frame=1;
     h.update();check(field<HumanRecordLink>(h.t.hero(),14).dash_speed==0.1F);} // Threshold wraps before signed comparison.
    {Inputs h;h.state.old_up=true;h.held_up=false;h.missing_humans=true;
     // Release's speed-zero prefix survives its later missing HumanSystem failure.
     field<HumanRecordLink>(h.t.hero(),14).state=1;field<ProjectileSpeed>(h.t.hero(),1).z=5;
     fails([&]{h.update();});check(field<ProjectileSpeed>(h.t.hero(),1).z==0&&h.state.old_up&&h.state.frame==0);}
    for(int kind:{0,2,13}){Inputs h;field<HumanRecordLink>(h.t.hero(),14).state=1;InputTouch e{kind,&h.up};input_touch(&e,h);
     check(field<ProjectileSpeed>(h.t.hero(),1).z<0);}
    for(int kind:{1,3,6,10}){Inputs h;field<HumanRecordLink>(h.t.hero(),14).state=1;field<ProjectileSpeed>(h.t.hero(),1).z=7;
     InputTouch e{kind,&h.up};input_touch(&e,h);check(field<HumanRecordLink>(h.t.hero(),14).state==0&&field<ProjectileSpeed>(h.t.hero(),1).z==0);}
    {Inputs h;field<HumanRecordLink>(h.t.hero(),14).state=1;InputTouch e{0,&h.up};h.on_hero=[&]{h.up.value=0x10013;e.type=1;};
     input_touch(&e,h);check(field<ProjectileSpeed>(h.t.hero(),1).z<0);}
    {Inputs h;InputTouch e{99,&h.up};h.missing_hero=true;fails([&]{input_touch(&e,h);});e.component=nullptr;fails([&]{input_touch(&e,h);});}
    {Inputs h;auto& hero=field<HumanRecordLink>(h.t.hero(),14);auto& other=field<HumanRecordLink>(h.t.sub(),14);
     hero.dash_speed=std::numeric_limits<float>::quiet_NaN();other.state=4;other.dash_speed=1;
     accelerate_humans(h.t.host.system);check(std::isnan(hero.dash_speed)&&other.dash_speed==1);
     reset_human_speeds(h.t.host.system);check(hero.dash_speed==0&&other.dash_speed==0);}
    {StageStartState s;AttackButtonState a,b,c;a.state=1;c.state=2;
     s.attack_buttons=std::make_shared<std::vector<AttackButtonState*>>(std::initializer_list<AttackButtonState*>{&a,&b,&c});
     InputControl select{10,0x230001};InputTouch e{2,&select};stage_touch(s,&e);
     check(a.state==0&&b.state==1&&c.state==2&&s.attack_attribute==1&&s.last_active_button==1);
     inactivate_attack_buttons(s);check(b.state==0&&c.state==2);
     InputControl move{35,0x10011};e={99,&move};stage_touch(s,&e);check(s.move_front_pushed&&b.state==0);
     e.type=1;stage_touch(s,&e);check(b.state==1&&s.attack_attribute==1);
     select.value=0x230004;e={2,&select};fails([&]{stage_touch(s,&e);});}
    {auto a=input_screen_rectangle({556,460,60,60},{10,0,0,0},600,640);
     auto b=input_screen_rectangle({540,515,60,60},{10,0,0,0},600,640);
     partition_input_rectangles(a,b);
     auto c=input_screen_rectangle({556,570,60,60},{10,0,0,0},600,640);
     partition_input_rectangles(a,c);partition_input_rectangles(b,c);
     check(a.x==546&&a.width==54&&a.y==460&&a.height==58);
     check(b.x==530&&b.width==70&&b.y==518&&b.height==55);
     check(c.y==573&&c.height==57);
     check(!input_hit_rectangle(a,550,518)&&input_hit_rectangle(b,550,518));
     InputRectangle left{0,0,10,10},right{7,0,10,10};partition_input_rectangles(left,right);
     check(left.width==9&&right.x==9&&right.width==8);
     InputRectangle separate{100,100,10,10};partition_input_rectangles(left,separate);check(left.width==9&&separate.x==100);}
    {InputRectangle earlier{0,0,10,10},later=earlier;partition_input_rectangles(earlier,later);
     check(earlier.x==5&&earlier.width==5&&later.x==0&&later.width==5);}
    {const auto area=select_area_touch_region(2,false,false,100,200,600,640);
     check(area.control.group==29&&area.control.value==0xc0002);
     check(area.bounds.x==85&&area.bounds.y==150&&area.bounds.width==159&&area.bounds.height==137);
     const auto go=select_area_touch_region(2,true,false,150,245,600,640);
     check(go.control.group==4&&go.control.value==28&&go.bounds.x==135&&go.bounds.y==195&&go.bounds.width==59&&go.bounds.height==92);
     const auto locked=select_area_touch_region(3,false,true,100,200,600,640);
     check(locked.bounds.x==95&&locked.bounds.y==180&&locked.bounds.width==139&&locked.bounds.height==107);}
    {const auto gift=select_resident_gift_touch_region(100,200,600,640);
     check(gift.control.group==4&&gift.control.value==0x10019);
     check(gift.bounds.x==90&&gift.bounds.y==190&&gift.bounds.width==52&&gift.bounds.height==52);
     check(input_hit_rectangle(gift.bounds,90,190)&&!input_hit_rectangle(gift.bounds,142,200));
     const auto edge=select_resident_gift_touch_region(-5,630,600,640);
     check(edge.bounds.x==0&&edge.bounds.y==620&&edge.bounds.width==37&&edge.bounds.height==20);}
    {struct Widget final:SelectWidgetHost {
         std::vector<std::int32_t> values;std::vector<std::uint32_t> keys;bool fail=false;
         void select_value(std::uint32_t flags,std::int32_t value)override{check(flags==0xc0000U||flags==0x10000U);values.push_back(value);if(fail)throw std::runtime_error("setter");}
         void click_key(std::uint32_t mask)override{keys.push_back(mask);}
     } widget;
     InputControl area{29,0xcffff};InputTouch event{2,&area};select_widget_touch(&event,widget);
     check(widget.values==std::vector<std::int32_t>({-1})&&widget.keys==std::vector<std::uint32_t>({0x80000U}));
     event.type=1;select_widget_touch(&event,widget);check(widget.keys.back()==0x100000U);
     widget.fail=true;widget.keys.clear();fails([&]{select_widget_touch(&event,widget);});check(widget.keys.empty());
     InputControl go{4,28};event={2,&go};select_widget_touch(&event,widget);check(widget.keys.empty());
     event.type=5;select_widget_touch(&event,widget);check(widget.keys.back()==0x10000000U);
     go.value=0;select_widget_touch(&event,widget);check(widget.keys.back()==0x100000U);
     widget.fail=false;widget.keys.clear();InputControl gift{4,0x10019};event={2,&gift};select_widget_touch(&event,widget);
     check(widget.keys.empty());event.type=5;select_widget_touch(&event,widget);
     check(widget.values.back()==25&&widget.keys.empty());}
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
