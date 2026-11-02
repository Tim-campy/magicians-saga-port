#include "core/gameplay/effect_frame.hpp"
#include <cstdlib>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

using namespace saga::core;
namespace {
void check(bool value){if(!value)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Actor {
    ComponentSlots entity{35};EffectComponent effect;ContactPosition position;EntitySprite sprite;
    Actor(){sprite.animation={20,52,0};entity.add(26,&effect);entity.add(0,&position);entity.add(2,&sprite);}
};
struct Host final:EffectInitializationWorld,EffectFrameHost {
    EffectSystemState state;EntitySubset group{[](ComponentSlots* e){return e&&e->contains(26)&&!e->contains(4);},[](const ComponentSlots&){return 0;}};
    std::vector<ComponentSlots*> destroyed;std::function<void(ComponentSlots&)> on_parent,on_destroy;
    bool dead=false,null_subset=false,fail_subset=false;
    Host(){initialize_effects(state,this);}
    EntitySubset* effect_subset(const EventIntegers& all,const EventIntegers& none)override{
        check(all==EventIntegers{26}&&none==EventIntegers{4});if(fail_subset)throw std::runtime_error("subset");return null_subset?nullptr:&group;
    }
    bool effect_parent_destroyed(ComponentSlots& e)override{if(on_parent)on_parent(e);return dead;}
    void destroy_effect_entity(ComponentSlots& e)override{destroyed.push_back(&e);if(on_destroy)on_destroy(e);}
};
struct Drawing final:EffectDrawingHost {
    std::vector<std::string> calls;std::vector<ComponentSlots*> actors;std::function<void(const std::string&)> callback;
    std::string fail;GaugePoint point{40,50};ContactPosition* projected=nullptr;
    EffectNumberContext context=this,captured=nullptr;std::optional<std::array<std::int64_t,9>> number;
    EffectResourceContext manager=this,drawn_manager=nullptr;EffectNumberContext resource_application=nullptr;
    std::int32_t resource_id=0,sprite_clip=0,sprite_frame=0,formatted_time=0;float sprite_x=0,sprite_y=0;bool mode=false;
    std::vector<std::array<std::int32_t,2>> modes;
    std::vector<std::int32_t> notice_frames{3,3,2,0};
    std::vector<EffectText> notice_texts{std::make_shared<const std::u16string>(u"zero"),std::make_shared<const std::u16string>(u"one"),
        std::make_shared<const std::u16string>(u"two"),std::make_shared<const std::u16string>(u"three")};
    EffectText drawn_text;std::optional<std::array<std::int32_t,3>> text_args;
    std::int32_t frame_notice=-1,text_notice=-1,sprite_anchor=0,font_depth=0;bool white=false;
    void step(const std::string& name){calls.push_back(name);if(callback)callback(name);if(fail==name)throw std::runtime_error(name);}
    void handler(const std::string& name,ComponentSlots& e){actors.push_back(&e);step(name);}
    void effect_draw_normal(ComponentSlots& e)override{handler("normal",e);}
    void effect_draw_damage(ComponentSlots& e)override{handler("damage",e);}
    void effect_draw_time(ComponentSlots& e)override{handler("time",e);}
    void effect_draw_jump(ComponentSlots& e)override{handler("jump",e);}
    void effect_draw_string_background(ComponentSlots& e)override{handler("string",e);}
    GaugePoint& effect_screen_position(ContactPosition* p)override{projected=p;step("screen");return point;}
    EffectNumberContext capture_effect_numbers()override{step("capture");return context;}
    void effect_draw_number(EffectNumberContext app,std::int32_t clip,std::int64_t value,std::int32_t x,std::int32_t y,
        std::int32_t pad,std::int32_t anchor,std::int32_t scale,std::int32_t width,std::int32_t height)override{
        captured=app;number=std::array<std::int64_t,9>{clip,value,x,y,pad,anchor,scale,width,height};step("number");
    }
    EffectResourceContext capture_effect_resource(EffectNumberContext app,std::int32_t resource)override{
        resource_application=app;resource_id=resource;step("resource");return manager;
    }
    void effect_draw_sprite(EffectResourceContext resource,std::int32_t clip,std::int32_t frame,float x,float y)override{
        drawn_manager=resource;sprite_clip=clip;sprite_frame=frame;sprite_x=x;sprite_y=y;step("sprite");
    }
    void effect_render_mode(std::int32_t kind,std::int32_t alpha)override{modes.push_back({kind,alpha});step("mode");mode=true;}
    void effect_reset_render_mode()override{step("reset");mode=false;}
    void effect_format_time(std::int32_t value)override{formatted_time=value;step("format");}
    std::int32_t effect_notice_frame(std::int32_t notice)override{frame_notice=notice;step("notice frame");return notice_frames.at(static_cast<std::size_t>(notice));}
    EffectText effect_notice_text(std::int32_t notice)override{text_notice=notice;step("notice text");return notice_texts.at(static_cast<std::size_t>(notice));}
    void effect_draw_anchored_sprite(EffectResourceContext resource,std::int32_t clip,std::int32_t frame,float x,float y,std::int32_t anchor)override{
        drawn_manager=resource;sprite_clip=clip;sprite_frame=frame;sprite_x=x;sprite_y=y;sprite_anchor=anchor;step("background");
    }
    void effect_push_font(std::int32_t size)override{check(size==10);step("font push");++font_depth;}
    void effect_white_text_color()override{step("white");white=true;}
    void effect_draw_text(EffectText text,std::int32_t x,std::int32_t y,std::int32_t anchor)override{
        drawn_text=std::move(text);text_args=std::array<std::int32_t,3>{x,y,anchor};step("text");
    }
    void effect_pop_font()override{step("font pop");--font_depth;}
};
struct Projection final:EffectProjectionHost {
    GaugePoint work{},camera{};std::array<GaugePoint*,3> points{{&work,&work,&work}};
    std::vector<std::string> calls;std::function<void(const std::string&)> callback;std::string fail;std::size_t reads=0;
    std::optional<float> x,y;
    void step(const std::string& name){calls.push_back(name);if(callback)callback(name);if(name==fail)throw std::runtime_error(name);}
    GaugePoint* effect_screen_work()override{const auto index=reads++;step("work"+std::to_string(index+1));return points.at(index);}
    float effect_projection_x(float a,float b)override{step("x");return x?*x:a-b;}
    float effect_projection_y(float a,float b)override{step("y");return y?*y:(a+b)*0.5F;}
    void validate_effect_draw_context()override{step("context");}
    GaugePoint effect_camera_offset()override{step("camera");return camera;}
};
void drawing_contracts(){
    const std::array<std::vector<std::string>,8> expected{{{"normal"},{"damage"},{"time"},{"screen","capture","number"},{},{"jump"},{},{"string"}}};
    for(std::int32_t type=0;type<=7;++type){Host h;Actor a;Drawing d;a.effect.type=type;a.effect.value=-12;a.sprite.animation.clip=52;h.group.add(&a.entity);
     draw_effects(h.state,d);check(d.calls==expected.at(static_cast<std::size_t>(type))&&a.effect.frame==0);
     if(type==3)check(d.number&&*d.number==std::array<std::int64_t,9>{52,-12,40,50,0,2,100,-1,-1}&&d.captured==&d&&d.projected==&a.position);
     else if(type!=4&&type!=6)check(d.actors==std::vector<ComponentSlots*>{&a.entity});}
    for(auto type:{-1,std::numeric_limits<std::int32_t>::min(),8,std::numeric_limits<std::int32_t>::max()}){
     Host h;Actor a;Drawing d;a.effect.type=type;a.entity.remove(0);a.entity.remove(2);h.group.add(&a.entity);draw_effects(h.state,d);check(d.calls.empty());}
    {Host h;Drawing d;draw_effects(h.state,d);check(d.calls.empty());h.state.subset=nullptr;fails([&]{draw_effects(h.state,d);});check(d.calls.empty());}
    {Host h;Actor a;Drawing d;h.group.add(&a.entity);a.entity.remove(26);fails([&]{draw_effects(h.state,d);});check(d.calls.empty());}
    {Host h;Actor a,b;Drawing d;a.effect.type=0;b.effect.type=7;h.group.add(&a.entity);h.group.add(&b.entity);draw_effects(h.state,d);
     check(d.calls==std::vector<std::string>{"normal","string"}&&d.actors==std::vector<ComponentSlots*>{&a.entity,&b.entity});}
    {Host h;Actor a,b;Drawing d;h.group.add(&a.entity);d.callback=[&](const std::string& name){if(name=="normal")h.group.add(&b.entity);};
     fails([&]{draw_effects(h.state,d);});check(d.calls==std::vector<std::string>{"normal"});}
    {Host h;Actor a,b;Drawing d;b.effect.type=1;h.group.add(&a.entity);h.group.add(&b.entity);d.fail="damage";
     fails([&]{draw_effects(h.state,d);});check(d.calls==std::vector<std::string>{"normal","damage"});}
    // Number captures the old Effect and Position, projects first, then captures AppData before fresh Seb.
    {Actor a;Drawing d;EffectComponent replacement;replacement.value=999;EntitySprite sprite;sprite.animation.clip=71;a.effect.value=42;
     d.callback=[&](const std::string& name){if(name=="capture"){a.entity.change(26,&replacement);a.entity.change(2,&sprite);a.effect.value=-31;d.point={8,9};}};
     draw_number_effect(&a.entity,d);check(d.number&&*d.number==std::array<std::int64_t,9>{71,-31,8,9,0,2,100,-1,-1});}
    for(auto value:{std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max()}){
     Actor a;Drawing d;a.effect.value=value;draw_number_effect(&a.entity,d);check(d.number->at(1)==static_cast<std::int64_t>(value));}
    for(const auto& stop:std::vector<std::string>{"screen","capture","number"}){Actor a;Drawing d;d.fail=stop;fails([&]{draw_number_effect(&a.entity,d);});
     check(d.calls.back()==stop&&d.calls.size()==(stop=="screen"?1U:stop=="capture"?2U:3U));}
    {Drawing d;fails([&]{draw_number_effect(nullptr,d);});check(d.calls.empty());}
    for(auto missing:{2,26}){Actor a;Drawing d;a.entity.remove(missing);fails([&]{draw_number_effect(&a.entity,d);});
     check(d.calls==std::vector<std::string>{"screen","capture"}&&!d.number);}
    {Actor a;Drawing d;a.entity.remove(0);draw_number_effect(&a.entity,d);check(!d.projected);} // The projection provider validates nullable Position.
    {Actor a;Drawing d;d.context=nullptr;fails([&]{draw_number_effect(&a.entity,d);});check(d.calls==std::vector<std::string>{"screen","capture"}&&!d.number);}
    // EffectSystem.ToScreenPos uses fresh shared-point identities for X, Y and final camera subtraction.
    {Projection p;ContactPosition position;position.x=10;position.z=4;position.y=3;p.camera={2,1};
     auto& result=effect_to_screen_position(&position,p);check(&result==&p.work&&result.x==4&&result.y==3);
     check(p.calls==std::vector<std::string>{"work1","x","work2","y","work3","context","camera"});}
    {Projection p;GaugePoint a{1,2},b{3,4},c{100,200},unused;p.points={&a,&b,&c};p.camera={7,9};ContactPosition position;position.x=10;position.z=4;
     p.callback=[&](const std::string& name){if(name=="x")p.points[0]=&unused;if(name=="y"){p.points[1]=&unused;position.y=2;}};
     auto& result=effect_to_screen_position(&position,p);check(&result==&c&&a.x==6&&b.y==5&&c.x==93&&c.y==191&&unused.x==0&&unused.y==0);}
    {Projection p;fails([&]{effect_to_screen_position(nullptr,p);});check(p.calls==std::vector<std::string>{"work1"});}
    for(std::size_t index=0;index<3;++index){Projection p;ContactPosition position;p.points[index]=nullptr;
     fails([&]{effect_to_screen_position(&position,p);});check(p.calls.back()==(index==0?"x":index==1?"y":"work3"));}
    for(const auto& stop:std::vector<std::string>{"x","y","context","camera"}){Projection p;ContactPosition position;position.x=8;position.z=2;p.fail=stop;
     fails([&]{effect_to_screen_position(&position,p);});check(p.calls.back()==stop);
     if(stop!="x")check(p.work.x==6);if(stop=="context"||stop=="camera")check(p.work.y==5);}
    const std::array<float,7> floats{{std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN(),2147483648.0F,-2147483648.0F,7.9F,-7.9F}};
    const std::array<std::int32_t,7> integers{{std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::min(),0,
        std::numeric_limits<std::int32_t>::max(),std::numeric_limits<std::int32_t>::min(),7,-7}};
    for(std::size_t i=0;i<floats.size();++i){Projection p;ContactPosition position;p.x=floats[i];p.y=floats[i];
     auto& result=effect_to_screen_position(&position,p);check(result.x==integers[i]&&result.y==integers[i]);}
    {Projection p;ContactPosition position;p.x=2147483648.0F;p.y=-2147483648.0F;p.camera={std::numeric_limits<std::int32_t>::min(),1};
     auto& result=effect_to_screen_position(&position,p);check(result.x==-1&&result.y==std::numeric_limits<std::int32_t>::max());}
    {Projection p;ContactPosition position;position.y=-1;p.y=16777216.0F;effect_to_screen_position(&position,p);check(p.work.y==16777216);}
}
void visual_handler_contracts(){
    const auto low=std::numeric_limits<std::int32_t>::min(),high=std::numeric_limits<std::int32_t>::max();
    {Actor a;Drawing d;a.effect.depth=true;a.entity.remove(0);a.entity.remove(2);draw_normal_effect(&a.entity,d);check(d.calls.empty());}
    {Actor a;Drawing d;a.sprite.animation={20,4,99};a.effect.frame=-3;draw_normal_effect(&a.entity,d);
     check(d.calls==std::vector<std::string>{"screen","capture","resource","sprite"}&&d.resource_id==20&&d.resource_application==&d);
     check(d.drawn_manager==&d&&d.sprite_clip==4&&d.sprite_frame==-3&&d.sprite_x==40&&d.sprite_y==50);}
    {Actor a;Drawing d;EntitySprite first,second;EffectComponent replacement;int context=0,new_context=0,manager=0;
     first.animation={7,3,0};second.animation={20,44,0};replacement.frame=9;d.context=&context;d.manager=&manager;
     d.callback=[&](const std::string& name){if(name=="capture")a.entity.change(2,&first);
         if(name=="resource"){d.context=&new_context;a.entity.change(2,&second);a.entity.change(26,&replacement);d.point={1,2};}};
     draw_normal_effect(&a.entity,d);check(d.resource_application==&context&&d.resource_id==7&&d.drawn_manager==&manager);
     check(d.sprite_clip==44&&d.sprite_frame==9&&d.sprite_x==1&&d.sprite_y==2);}
    {Actor a;Drawing d;d.point={high,low};draw_normal_effect(&a.entity,d);check(d.sprite_x==2147483648.0F&&d.sprite_y==-2147483648.0F);}
    for(const auto& failure:std::vector<std::string>{"screen","capture","resource","sprite"}){
     Actor a;Drawing d;d.fail=failure;fails([&]{draw_normal_effect(&a.entity,d);});check(d.calls.back()==failure);}
    {Actor a;Drawing d;d.manager=nullptr;fails([&]{draw_normal_effect(&a.entity,d);});check(d.calls==std::vector<std::string>{"screen","capture","resource"});}
    {Actor a;Drawing d;d.context=nullptr;fails([&]{draw_normal_effect(&a.entity,d);});check(d.calls==std::vector<std::string>{"screen","capture"});}
    for(auto slot:{2,26}){Actor a;Drawing d;a.entity.remove(slot);fails([&]{draw_normal_effect(&a.entity,d);});
     check(d.calls==(slot==26?std::vector<std::string>{}:std::vector<std::string>{"screen","capture"}));}
    const std::array<std::int32_t,10> scales{{50,141,191,200,165,135,115,105,100,100}};
    const std::array<std::int32_t,10> bounce{{-12,10,-8,6,-4,2,0,0,0,0}};
    for(std::int32_t frame=0;frame<10;++frame){Actor a;Drawing d;a.effect.frame=frame;a.effect.max_frame=100;a.effect.value=-12;a.sprite.animation.clip=52;
     draw_damage_number_effect(&a.entity,d);check(d.calls==std::vector<std::string>{"screen","capture","number"});
     check(*d.number==std::array<std::int64_t,9>{52,-12,40,50+bounce[static_cast<std::size_t>(frame)],0,2,scales[static_cast<std::size_t>(frame)],-1,-1});}
    for(const auto row:std::array<std::array<std::int32_t,3>,3>{{{{-1,9,64}},{{low,-2147483638,38}},{{high,-2147483639,50}}}}){
     Actor a;Drawing d;a.effect.frame=row[0];a.effect.max_frame=row[1];draw_damage_number_effect(&a.entity,d);
     check(d.calls==std::vector<std::string>{"screen","capture","number"}&&d.number->at(3)==row[2]&&d.number->at(6)==(row[0]<3?50:100));}
    for(const auto row:std::array<std::array<std::int32_t,3>,12>{{{{-1,-1,-1}},{{34,-1,-1}},{{35,10,12}},{{37,10,12}},{{38,15,17}},{{40,15,17}},
        {{41,-1,-1}},{{79,-1,-1}},{{80,8,9}},{{82,8,9}},{{83,-1,-1}},{{high,-1,-1}}}}){
     Actor a;Drawing d;a.effect.max_frame=100;a.sprite.animation.clip=row[0];draw_damage_number_effect(&a.entity,d);
     check(d.number->at(7)==row[1]&&d.number->at(8)==row[2]);}
    for(const auto row:std::array<std::array<std::int32_t,2>,4>{{{{18,-1}},{{17,204}},{{8,0}},{{7,0}}}}){Actor a;Drawing d;a.effect.frame=8;a.effect.max_frame=row[0];
     draw_damage_number_effect(&a.entity,d);check(d.mode==false);
     if(row[1]<0)check(d.modes.empty()&&d.calls==std::vector<std::string>{"screen","capture","number"});
     else check(d.modes==std::vector<std::array<std::int32_t,2>>{{1,row[1]}}&&d.calls==std::vector<std::string>{"screen","mode","capture","number","reset"});}
    for(const auto row:std::array<std::array<std::int32_t,2>,2>{{{{high,low}},{{low,high}}}}){Actor a;Drawing d;a.effect.frame=row[0];a.effect.max_frame=row[1];
     draw_damage_number_effect(&a.entity,d);check(d.modes==std::vector<std::array<std::int32_t,2>>{{1,0}}&&d.calls.back()=="reset");}
    {Actor a;Drawing d;EffectComponent replacement;replacement.max_frame=100;replacement.value=999;EntitySprite sprite;sprite.animation={2,80,0};
     ProjectileModification modification;modification.scale_x=0.5F;a.entity.add(15,&modification);a.effect.frame=0;a.effect.max_frame=9;a.sprite.animation.clip=35;
     d.callback=[&](const std::string& name){if(name=="mode"){a.effect.frame=3;a.entity.change(26,&replacement);d.point={99,88};}
         if(name=="capture"){a.entity.change(2,&sprite);a.effect.value=-77;}};
     draw_damage_number_effect(&a.entity,d);check(*d.number==std::array<std::int64_t,9>{80,-77,40,38,0,2,100,10,12}&&d.calls.back()=="reset");}
    const std::array<float,5> factors{{0.5F,-0.5F,std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}};
    const std::array<std::int32_t,5> modified{{25,-25,low,low,0}};
    for(std::size_t i=0;i<factors.size();++i){Actor a;Drawing d;ProjectileModification m;m.scale_x=factors[i];a.entity.add(15,&m);a.effect.max_frame=100;
     draw_damage_number_effect(&a.entity,d);check(d.number->at(6)==modified[i]);}
    {Actor a;Drawing d;d.point={high,low};a.effect.max_frame=100;draw_damage_number_effect(&a.entity,d);check(d.number->at(2)==high&&d.number->at(3)==2147483636);}
    for(const auto& failure:std::vector<std::string>{"screen","mode","capture","number","reset"}){Actor a;Drawing d;a.effect.max_frame=9;d.fail=failure;
     fails([&]{draw_damage_number_effect(&a.entity,d);});check(d.calls.back()==failure);
     check(d.mode==(failure=="capture"||failure=="number"||failure=="reset"));}
    {Actor a;Drawing d;a.effect.max_frame=9;a.entity.remove(2);fails([&]{draw_damage_number_effect(&a.entity,d);});
     check(d.calls==std::vector<std::string>{"screen","mode"}&&d.mode);}
    {Actor a;Drawing d;a.entity.remove(26);fails([&]{draw_damage_number_effect(&a.entity,d);});check(d.calls==std::vector<std::string>{"screen"});}
    {Actor a;Drawing d;a.effect.max_frame=9;d.context=nullptr;fails([&]{draw_damage_number_effect(&a.entity,d);});
     check(d.calls==std::vector<std::string>{"screen","mode","capture"}&&d.mode);}
    // Dead-output handlers retain their formatting/projection/style order without inventing glyphs.
    {Actor a;Drawing d;ContactPosition replacement;a.effect.value=low;a.entity.remove(2);
     d.callback=[&](const std::string& name){if(name=="format")a.entity.change(0,&replacement);};draw_time_effect(&a.entity,d);
     check(d.formatted_time==low&&d.projected==&replacement&&d.calls==std::vector<std::string>{"format","screen"}&&!d.number);}
    for(const auto& failure:std::vector<std::string>{"format","screen"}){Actor a;Drawing d;d.fail=failure;fails([&]{draw_time_effect(&a.entity,d);});check(d.calls.back()==failure);}
    for(auto remaining:{-1,0,9,10}){Actor a;Drawing d;a.effect.frame=8;a.effect.max_frame=8+remaining;a.entity.remove(2);
     draw_jump_number_effect(&a.entity,d);check(!d.number&&!d.mode);
     check(d.calls==(remaining<=9?std::vector<std::string>{"mode","screen","reset"}:std::vector<std::string>{"screen"}));}
    {Actor a;Drawing d;a.effect.max_frame=9;d.callback=[&](const std::string& name){if(name=="mode")a.effect.max_frame=100;};
     draw_jump_number_effect(&a.entity,d);check(d.calls.back()=="reset");}
    {Actor a;Drawing d;a.effect.max_frame=9;d.fail="screen";fails([&]{draw_jump_number_effect(&a.entity,d);});
     check(d.calls==std::vector<std::string>{"mode","screen"}&&d.mode);}
    {Drawing d;fails([&]{d.EffectDrawingHost::capture_effect_resource(&d,20);});fails([&]{d.EffectDrawingHost::effect_draw_sprite(&d,0,0,0,0);});
     fails([&]{d.EffectDrawingHost::effect_render_mode(1,255);});fails([&]{d.EffectDrawingHost::effect_reset_render_mode();});d.EffectDrawingHost::effect_format_time(42);}
}
void notice_contracts(){
    const std::vector<std::string> order{"screen","mode","capture","resource","notice frame","background","notice text","font push","white","text","font pop","reset"};
    const std::array<std::int32_t,6> displacement{{0,6,12,16,18,20}};
    const std::array<std::int32_t,21> alpha{{0,3,24,75,156,255,255,255,255,255,255,255,255,255,255,255,165,91,40,12,0}};
    for(std::int32_t notice=0;notice<4;++notice)for(std::int32_t frame=0;frame<=20;++frame){Actor a;Drawing d;a.effect.value=notice;a.effect.frame=frame;a.entity.remove(2);
     const auto text=d.notice_texts[static_cast<std::size_t>(notice)];draw_string_background_effect(&a.entity,d);
     const auto offset=displacement[static_cast<std::size_t>(frame<5?frame:5)]*(notice==1||notice==2?-1:1);
     check(d.calls==order&&d.point.y==50+offset&&d.modes==std::vector<std::array<std::int32_t,2>>{{1,alpha[static_cast<std::size_t>(frame)]}});
     check(d.resource_id==7&&d.resource_application==&d&&d.drawn_manager==&d&&d.sprite_clip==49&&d.sprite_frame==d.notice_frames[static_cast<std::size_t>(notice)]);
     check(d.sprite_x==40&&d.sprite_y==50+offset&&d.sprite_anchor==34&&d.frame_notice==notice&&d.text_notice==notice);
     check(d.drawn_text==text&&d.text_args&&*d.text_args==std::array<std::int32_t,3>{40,51+offset,34}&&d.font_depth==0&&d.white&&!d.mode&&!d.number);
     check(a.effect.frame==frame&&a.effect.value==notice);}
    for(auto frame:{-1,std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max()}){Actor a;Drawing d;a.effect.frame=frame;
     draw_string_background_effect(&a.entity,d);check(d.modes==std::vector<std::array<std::int32_t,2>>{{1,0}}&&d.point.y==(frame<0?50:70));}
    // Manager and background coordinates are captured before frame lookup; later text reads the retained Effect and shared point.
    {Actor a;Drawing d;EffectComponent replacement;replacement.value=0;a.effect.value=1;a.effect.frame=5;int application=0,other_application=0,manager=0;
     const auto original_text=d.notice_texts[2];d.context=&application;d.manager=&manager;
     d.callback=[&](const std::string& name){
         if(name=="mode"){a.effect.value=3;a.entity.change(26,&replacement);d.point={60,70};}
         if(name=="resource"){d.context=&other_application;d.point={80,90};}
         if(name=="notice frame")d.point={100,110};
         if(name=="background"){a.effect.value=2;d.point={120,130};}
         if(name=="font push"){d.notice_texts[2]=std::make_shared<const std::u16string>(u"replacement");d.point={140,150};}
         if(name=="white")d.point={160,170};
     };
     draw_string_background_effect(&a.entity,d);check(d.calls==order&&d.resource_application==&application&&d.drawn_manager==&manager);
     check(d.frame_notice==3&&d.sprite_frame==0&&d.sprite_x==80&&d.sprite_y==90&&d.text_notice==2&&d.drawn_text==original_text);
     check(d.text_args&&*d.text_args==std::array<std::int32_t,3>{160,171,34});}
    {Actor a;Drawing d;d.notice_texts[0].reset();draw_string_background_effect(&a.entity,d);check(!d.drawn_text&&d.calls==order&&!d.mode&&d.font_depth==0);}
    {Actor a;Drawing d;d.point={std::numeric_limits<std::int32_t>::max(),std::numeric_limits<std::int32_t>::max()};draw_string_background_effect(&a.entity,d);
     check(d.sprite_x==2147483648.0F&&d.sprite_y==2147483648.0F&&d.text_args->at(0)==std::numeric_limits<std::int32_t>::max()&&d.text_args->at(1)==std::numeric_limits<std::int32_t>::min());}
    // Every exceptional prefix leaves the current font/mode scope intact, with no synthetic cleanup.
    for(std::size_t i=0;i<order.size();++i){Actor a;Drawing d;d.fail=order[i];fails([&]{draw_string_background_effect(&a.entity,d);});
     check(d.calls==std::vector<std::string>(order.begin(),order.begin()+static_cast<std::ptrdiff_t>(i+1)));
     check(d.mode==(i>=2)&&d.font_depth==(i>=8&&i<=10?1:0));}
    for(auto notice:{-1,4,std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max()}){Actor a;Drawing d;a.effect.value=notice;d.manager=nullptr;
     bool bounds=false;try{draw_string_background_effect(&a.entity,d);}catch(const std::out_of_range&){bounds=true;}
     check(bounds&&d.calls==std::vector<std::string>{"screen","mode","capture","resource","notice frame"}&&d.mode&&d.point.y==50);}
    {Actor a;Drawing d;d.manager=nullptr;fails([&]{draw_string_background_effect(&a.entity,d);});
     check(d.calls==std::vector<std::string>{"screen","mode","capture","resource","notice frame"}&&d.mode);}
    {Actor a;Drawing d;d.notice_texts.clear();fails([&]{draw_string_background_effect(&a.entity,d);});
     check(d.calls.back()=="notice text"&&d.sprite_clip==49&&d.mode&&d.font_depth==0);}
    {Actor a;Drawing d;d.context=nullptr;fails([&]{draw_string_background_effect(&a.entity,d);});check(d.calls==std::vector<std::string>{"screen","mode","capture"}&&d.mode);}
    {Actor a;Drawing d;a.entity.remove(26);fails([&]{draw_string_background_effect(&a.entity,d);});check(d.calls==std::vector<std::string>{"screen"});}
    {Drawing d;fails([&]{draw_string_background_effect(nullptr,d);});check(d.calls.empty());
     fails([&]{d.EffectDrawingHost::effect_notice_frame(0);});fails([&]{d.EffectDrawingHost::effect_notice_text(0);});
     fails([&]{d.EffectDrawingHost::effect_draw_anchored_sprite(&d,49,3,0,0,34);});fails([&]{d.EffectDrawingHost::effect_push_font(10);});
     fails([&]{d.EffectDrawingHost::effect_white_text_color();});fails([&]{d.EffectDrawingHost::effect_draw_text({},0,0,34);});fails([&]{d.EffectDrawingHost::effect_pop_font();});}
}
}
int main(){
    drawing_contracts();
    visual_handler_contracts();
    notice_contracts();
    const auto low=std::numeric_limits<std::int32_t>::min(),high=std::numeric_limits<std::int32_t>::max();
    {Host h;auto old=h.state.expired;h.state.expired->add(nullptr);initialize_effects(h.state,&h);check(h.state.expired!=old&&h.state.expired->size()==0);
     old=h.state.expired;h.fail_subset=true;fails([&]{initialize_effects(h.state,&h);});check(h.state.expired==old&&h.state.subset==&h.group);
     h.fail_subset=false;h.null_subset=true;initialize_effects(h.state,&h);check(!h.state.subset);fails([&]{update_effects(h.state,h);});
     fails([&]{initialize_effects(h.state,nullptr);});}
    {Host h;const auto version=h.state.expired->version();update_effects(h.state,h);check(h.state.expired->version()==version);}
    for(std::int32_t type=-1;type<=9;++type){
        Host h;Actor a;a.effect.type=type;a.effect.frame=4;a.effect.max_frame=5;a.position.y=7;h.group.add(&a.entity);update_effects(h.state,h);
        const auto frame=type==0?0:((type==1||(type>=4&&type<=7))?5:4);
        check(a.effect.frame==frame&&a.position.y==(type==1?8.0F:7.0F));
    }
    {Actor a;a.effect.depth=true;a.effect.frame=high;update_normal_effect(&a.entity);check(a.effect.frame==high);
     a.effect.depth=false;a.effect.max_frame=0;update_normal_effect(&a.entity);check(a.effect.frame==low);
     a.effect.frame=high;a.effect.max_frame=-1;update_normal_effect(&a.entity);check(a.effect.frame==0);
     a.effect.frame=-5;a.effect.max_frame=3;update_normal_effect(&a.entity);check(a.effect.frame==-1);
     a.effect.frame=high;update_level_up_effect(&a.entity);check(a.effect.frame==low);update_time_effect(nullptr);
     fails([&]{update_normal_effect(nullptr);});a.entity.remove(26);fails([&]{update_damage_number_effect(&a.entity);});check(a.position.y==1);}
    {Host h;Actor a,b,parent;a.effect.parent=&parent.entity;b.effect.parent=&parent.entity;a.effect.type=7;b.effect.type=1;
     h.dead=true;h.group.add(&a.entity);h.group.add(&b.entity);h.on_destroy=[&](ComponentSlots& e){check(a.effect.frame==1&&b.effect.frame==1&&b.position.y==1);h.group.remove(&e,26,e.get(26));};
     update_effects(h.state,h);check(h.destroyed==std::vector<ComponentSlots*>({&a.entity,&b.entity})&&h.group.size()==0&&h.state.expired->size()==0);}
    {Host h;Actor a,parent;a.effect.type=7;a.effect.parent=&parent.entity;h.dead=true;h.group.add(&a.entity);
     h.on_parent=[&](ComponentSlots&){a.effect.type=1;};update_effects(h.state,h);check(a.position.y==1&&a.effect.frame==1);}
    {Host h;Actor a,b,parent;a.effect.parent=&parent.entity;b.effect.parent=&parent.entity;h.dead=true;h.group.add(&a.entity);h.group.add(&b.entity);
     b.entity.change(26,nullptr);fails([&]{update_effects(h.state,h);});check(h.state.expired->size()==1&&h.destroyed.empty());
     b.entity.change(26,&b.effect);h.on_destroy=[](ComponentSlots&){throw std::runtime_error("destroy");};fails([&]{update_effects(h.state,h);});
     check(h.state.expired->size()==3&&h.destroyed.size()==1);}
    {Host h;Actor a,parent;a.effect.parent=&parent.entity;h.dead=true;h.group.add(&a.entity);auto old=h.state.expired;
     h.on_destroy=[&](ComponentSlots&){h.state.expired=std::make_shared<EmitterExpiryQueue>();h.state.expired->add(nullptr);};
     update_effects(h.state,h);check(old->size()==1&&h.state.expired!=old&&h.state.expired->size()==0);}
    {Host h;Actor a,parent;a.effect.parent=&parent.entity;h.dead=true;h.group.add(&a.entity);
     h.on_destroy=[&](ComponentSlots&){h.state.expired->add(nullptr);};fails([&]{update_effects(h.state,h);});check(h.state.expired->size()==2);}
}
