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
    Actor(){entity.add(26,&effect);entity.add(0,&position);entity.add(2,&sprite);}
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
}
int main(){
    drawing_contracts();
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
