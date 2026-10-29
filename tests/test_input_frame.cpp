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
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
