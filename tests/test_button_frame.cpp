#include "core/gameplay/button_frame.hpp"
#include "core/gameplay/render_frame.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <limits>
using namespace saga::core;
void check_at(bool v,int line){if(!v){std::cerr<<"button contract failed at "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Host final:ButtonFrameHost,ButtonInitializationWorld,ButtonFactoryHost {
    StageQuestForm form_state;QuestCameraForm form{form_state};ButtonFrameState frame;EntityIdState ids;
    std::map<const ComponentSlots*,int> keys;std::set<const ComponentSlots*> dead;
    EntitySubset all{[](auto* e){return e->contains(17);},[this](const auto& e){return keys.at(&e);}};
    EntitySubset group{[](auto* e){return e->contains(17)&&e->contains(0)&&e->contains(2);},[this](const auto& e){return keys.at(&e);}};
    SubsetMembership membership{{&all,&group}};
    std::deque<ComponentSlots> nodes;std::deque<ContactPosition> positions;std::deque<EntitySprite> sprites;std::deque<EntityButton> buttons;
    Receiver receiver;std::vector<std::string> calls;std::vector<ComponentSlots*> destroyed;
    std::vector<std::pair<const InputTouch*,ComponentSlots*>> events;
    std::function<void()> on_x,on_y,on_form,on_subscribe,on_acquire;std::function<void(ComponentSlots&)> on_destroy;
    bool null_form=false,null_subset=false;int creates=0;bool acquired_hover=false;
    Host(){form_state.scale=125;initialize_buttons(frame,this,*this);calls.clear();}
    EntitySubset* button_subset(const EventIntegers& slots)override{check(slots==EventIntegers{17,0,2});calls.push_back("subset");return null_subset?nullptr:&group;}
    void subscribe_button_touch(Receiver f)override{calls.push_back("subscribe9");if(on_subscribe)on_subscribe();receiver=std::move(f);}
    bool button_parent_destroyed(ComponentSlots& e)override{calls.push_back("destroyed?");return dead.count(&e)!=0;}
    float project_button_x(float x,float z)override{calls.push_back("project-x");if(on_x)on_x();return render_isometric_x(x,z);}
    float project_button_y(float x,float z)override{calls.push_back("project-y");if(on_y)on_y();return render_isometric_y(x,z);}
    QuestCameraForm* button_camera_form()override{calls.push_back("form");if(on_form)on_form();return null_form?nullptr:&form;}
    void send_button_event(const InputTouch* event,ComponentSlots* e)override{calls.push_back("event11");events.emplace_back(event,e);}
    bool registered_id(int id)override{for(const auto& entry:keys)if(entry.second==id&&!dead.count(entry.first))return true;return false;}
    ComponentSlots* create(int type,int id)override{check(type==0);nodes.emplace_back(35,&membership);auto* e=&nodes.back();keys[e]=id;++creates;calls.push_back("create");return e;}
    void register_entity(ComponentSlots&,bool temporary)override{check(!temporary);calls.push_back("register");}
    ContactPosition& acquire_position()override{positions.emplace_back();return positions.back();}
    EntitySprite& acquire_sprite()override{sprites.emplace_back();return sprites.back();}
    EntityButton& acquire_button()override{if(on_acquire)on_acquire();buttons.emplace_back();buttons.back().hover=acquired_hover;return buttons.back();}
    EntitySubset* factory_buttons()override{calls.push_back("factory-subset");return &all;}
    void destroy_button(ComponentSlots& e,bool registration)override{
        check(registration);calls.push_back("destroy");destroyed.push_back(&e);if(on_destroy)on_destroy(e);
        if(dead.count(&e))return;for(int i=0;i<35;++i)if(e.contains(i))e.remove(i);dead.insert(&e);
    }
    ComponentSlots& actor(float x,float y,float z){auto& e=*create(0,100+creates);auto& p=acquire_position();p.x=x;p.y=y;p.z=z;e.add(0,&p);return e;}
    ButtonCreation spec(ComponentSlots* parent=nullptr){return {7,9,2,40,3,{10,23,false,false,100,nullptr,0,0,parent,8,-9},false};}
    ComponentSlots& make(ButtonCreation s){return create_button(s,ids,*this);}
    ContactPosition& pos(ComponentSlots& e){return *dynamic_cast<ContactPosition*>(e.get(0));}
    EntityButton& data(ComponentSlots& e){return *dynamic_cast<EntityButton*>(e.get(17));}
};
struct Drawing final:ButtonDrawingHost {
    struct Image {ButtonResourceContext manager;std::int32_t clip,frame,scale,anchor;float x,y;};
    struct Touch {std::string kind;std::int32_t id;InputRectangle rectangle;ButtonSebContext seb;std::int32_t frame,value;ButtonTouchOption option;};
    std::vector<std::string> calls;std::vector<Image> images;std::vector<Touch> touches;std::function<void(const std::string&)> callback;std::string fail;
    ButtonDrawMetrics metrics{125.9F,150},touch_metrics{125.9F,150};
    ButtonApplicationContext app=this,resource_app=nullptr;ButtonResourceContext manager=this,seb_manager=nullptr;ButtonSebContext seb=this,bounds_seb=nullptr;
    std::int32_t resource_id=0,seb_clip=0,bounds_frame=0;std::array<std::int32_t,4> bounds{{-3,2,11,9}};
    ButtonText text;std::array<float,2> text_position{};std::int32_t text_anchor=0;bool mode=false;
    std::vector<std::array<std::int32_t,3>> modes;
    void step(const std::string& name){calls.push_back(name);if(callback)callback(name);if(fail==name)throw std::runtime_error(name);}
    ButtonDrawMetrics button_draw_metrics()override{step("metrics");return metrics;}
    ButtonDrawMetrics button_touch_metrics()override{step("touch metrics");return touch_metrics;}
    void button_black_color()override{step("black");}
    ButtonApplicationContext capture_button_application()override{step("app");return app;}
    ButtonResourceContext button_resource(ButtonApplicationContext captured,std::int32_t id)override{resource_app=captured;resource_id=id;step("resource");return manager;}
    ButtonSebContext button_seb(ButtonResourceContext captured,std::int32_t clip)override{seb_manager=captured;seb_clip=clip;step("seb");return seb;}
    std::array<std::int32_t,4> button_bounds(ButtonSebContext captured,std::int32_t frame)override{bounds_seb=captured;bounds_frame=frame;step("bounds");return bounds;}
    void image(std::string kind,ButtonResourceContext captured,std::int32_t clip,std::int32_t frame,float x,float y,std::int32_t scale,std::int32_t anchor){
        images.push_back({captured,clip,frame,scale,anchor,x,y});step(kind);
    }
    void button_draw_sprite(ButtonResourceContext m,std::int32_t c,std::int32_t f,float x,float y)override{image("sprite",m,c,f,x,y,100,0);}
    void button_draw_scaled_sprite(ButtonResourceContext m,std::int32_t c,std::int32_t f,std::int32_t x,std::int32_t y,std::int32_t scale)override{image("scaled",m,c,f,static_cast<float>(x),static_cast<float>(y),scale,0);}
    void button_draw_seb_ex(ButtonResourceContext m,std::int32_t c,std::int32_t f,std::int32_t x,std::int32_t y,std::int32_t scale,std::int32_t anchor)override{image("ex",m,c,f,static_cast<float>(x),static_cast<float>(y),scale,anchor);}
    void button_render_mode(std::int32_t m,std::int32_t a,std::int32_t b)override{modes.push_back({m,a,b});step("mode");mode=true;}
    void button_reset_render_mode()override{step("reset");mode=false;}
    void button_draw_text(ButtonText value,float x,float y,std::int32_t anchor)override{text=std::move(value);text_position={x,y};text_anchor=anchor;step("text");}
    void button_add_touch(std::int32_t id,InputRectangle r,std::int32_t value)override{touches.push_back({"rect",id,r,nullptr,0,value,nullptr});step("rect");}
    void button_add_touch_seb(std::int32_t id,std::int32_t x,std::int32_t y,ButtonSebContext s,std::int32_t frame,std::int32_t value,ButtonTouchOption option)override{
        touches.push_back({"touch",id,{x,y,0,0},s,frame,value,option});step("touch");
    }
    void button_add_touch_delay(std::int32_t id,std::int32_t x,std::int32_t y,ButtonSebContext s,std::int32_t frame,std::int32_t value,ButtonTouchOption option)override{
        touches.push_back({"delay",id,{x,y,0,0},s,frame,value,option});step("delay");
    }
};
void drawing_contracts(){
    // Both empty passes require the same context and black color before touching the subset.
    for(bool dynamic:{false,true}){Host h;Drawing d;draw_buttons(h.frame,dynamic,d);check(d.calls==std::vector<std::string>{"metrics","black"});
     d.calls.clear();h.frame.subset=nullptr;fails([&]{draw_buttons(h.frame,dynamic,d);});check(d.calls==std::vector<std::string>{"metrics","black"});}
    for(bool dynamic:{false,true})for(auto scale:{-50,0,100,200})for(bool hover:{false,true}){Host h;Drawing d;auto spec=h.spec();spec.properties.dynamic=dynamic;spec.properties.scale=scale;
     spec.properties.text=std::make_shared<std::u16string>(u"caption");spec.properties.text_offset_x=2;spec.properties.text_offset_y=-1;
     auto& e=h.make(spec);h.data(e).hover=hover;draw_buttons(h.frame,dynamic,d);
     check(d.images.size()==1&&d.images[0].manager==&d&&d.images[0].clip==40&&d.images[0].frame==3&&d.images[0].x==12&&d.images[0].y==2&&d.images[0].scale==scale);
     check(d.resource_id==2&&d.resource_app==&d&&d.seb_manager==&d&&d.bounds_seb==&d&&d.bounds_frame==3);
     check(d.text==spec.properties.text&&d.text_position==std::array<float,2>{19,5}&&d.text_anchor==34&&!d.mode);
     check(d.modes==(scale==100&&hover?std::vector<std::array<std::int32_t,3>>{{1,255,255}}:std::vector<std::array<std::int32_t,3>>{}));
     const auto& t=d.touches.at(0);check(t.id==10&&t.value==23&&!t.option);
     if(dynamic)check(t.kind=="delay"&&t.rectangle.x==22&&t.rectangle.y==3&&t.seb==&d&&t.frame==3);
     else check(t.kind=="rect"&&t.rectangle.x==12&&t.rectangle.y==2&&t.rectangle.width==11*scale/100&&t.rectangle.height==9*scale/100);
     check(d.calls.back()==(dynamic?"delay":"rect"));}
    // Deferred filtering uses Button.dynamic before Invisible21 and does not test parent destruction.
    {Host h;Drawing d;auto& a=h.make(h.spec());auto b=h.spec();b.properties.dynamic=true;h.make(b);Component invisible;a.add(21,&invisible);
     draw_buttons(h.frame,false,d);check(d.images.empty());d.calls.clear();draw_buttons(h.frame,true,d);check(d.images.size()==1&&d.touches[0].kind=="delay");}
    {ComponentSlots broken(35);EntitySubset group{[](auto*){return true;},[](const auto&){return 0;}};group.add(&broken);ButtonFrameState state{&group};Drawing d;
     fails([&]{draw_buttons(state,false,d);});check(d.calls==std::vector<std::string>{"metrics","black"});}
    {Host h;Drawing d;auto& e=h.make(h.spec());h.dead.insert(&e);draw_buttons(h.frame,false,d);check(d.images.size()==1);}
    {Host h;Drawing d;auto s=h.spec();s.properties.text=std::make_shared<std::u16string>();h.make(s);draw_buttons(h.frame,false,d);check(d.text&&d.text->empty());}
    // Hover is reread after the unscaled draw, so callbacks can suppress or introduce the reset.
    for(bool initial:{false,true}){Host h;Drawing d;auto& e=h.make(h.spec());h.data(e).hover=initial;
     d.callback=[&](const std::string& name){if(name=="sprite")h.data(e).hover=!initial;};draw_buttons(h.frame,false,d);
     check(d.mode==initial&&std::count(d.calls.begin(),d.calls.end(),"reset")==(!initial?1:0));}
    {Host h;Drawing d;auto& e=h.make(h.spec());EntityButton replacement;replacement.scale=0;auto text=std::make_shared<std::u16string>(u"late");
     auto* original=&h.data(e);d.callback=[&](const std::string& name){if(name=="sprite"){
         original->text=text;original->text_offset_x=2;original->text_offset_y=-1;original->scale=200;original->id=777;original->value=888;
         e.change(17,&replacement);d.bounds={100,100,100,100};
     }};draw_buttons(h.frame,false,d);check(d.images[0].scale==100&&d.text==text&&d.text_position==std::array<float,2>{19,5});
     check(d.touches[0].id==777&&d.touches[0].value==888&&d.touches[0].rectangle.width==22&&d.touches[0].rectangle.height==18);}
    {Host h;Drawing d;auto spec=h.spec();spec.properties.dynamic=true;auto& e=h.make(spec);int app=0,other_app=0,manager=0,seb=0,other_seb=0;d.app=&app;d.manager=&manager;d.seb=&seb;
     EntitySprite first,second;first.animation={7,44,8};second.animation={1,51,9};
     d.callback=[&](const std::string& name){if(name=="black")d.metrics={1,1};if(name=="resource")e.change(2,&first);
         if(name=="seb"){d.app=&other_app;e.change(2,&second);}if(name=="sprite"){d.seb=&other_seb;second.animation.frame=12;}};
     draw_buttons(h.frame,true,d);check(d.resource_app==&app&&d.resource_id==2&&d.seb_manager==&manager&&d.seb_clip==44&&d.bounds_seb==&seb&&d.bounds_frame==9);
     check(d.images[0].manager==&manager&&d.images[0].clip==51&&d.images[0].frame==9&&d.touches[0].seb==&seb&&d.touches[0].frame==12&&d.touches[0].rectangle.x==22);}
    // Wrapping integer products precede conversion/division; no implicit scale100 repairs are made.
    {Host h;Drawing d;auto spec=h.spec();spec.properties.dynamic=true;h.make(spec);d.metrics={std::numeric_limits<float>::infinity(),1};draw_buttons(h.frame,true,d);
     check(d.touches[0].rectangle.x==-2576980&&d.touches[0].rectangle.y==-429496);}
    {Host h;Drawing d;auto spec=h.spec();spec.properties.dynamic=true;h.make(spec);d.metrics={3.9F,std::numeric_limits<std::int32_t>::max()};draw_buttons(h.frame,true,d);
     check(d.touches[0].rectangle.x==2576980&&d.touches[0].rectangle.y==429496);}
    {Host h;Drawing d;auto spec=h.spec();spec.properties.offset_x=spec.properties.offset_y=0;auto& e=h.make(spec);d.bounds={0,0,10,10};
     h.pos(e).x=2147483648.0F;h.pos(e).y=-std::numeric_limits<float>::infinity();draw_buttons(h.frame,false,d);
     check(d.images[0].x==2147483648.0F&&d.images[0].y==-2147483648.0F&&d.touches[0].rectangle.x==-1&&d.touches[0].rectangle.y==0);}
    {Host h;Drawing d;auto spec=h.spec();spec.properties.scale=std::numeric_limits<std::int32_t>::max();h.make(spec);d.bounds={0,0,30000,9};draw_buttons(h.frame,false,d);
     check(d.touches[0].rectangle.width==-300&&d.touches[0].rectangle.height==21474836);}
    const std::vector<std::string> complete{"metrics","black","app","resource","seb","bounds","mode","sprite","reset","text","rect"};
    for(std::size_t i=0;i<complete.size();++i){Host h;Drawing d;auto spec=h.spec();spec.properties.text=std::make_shared<std::u16string>(u"caption");auto& e=h.make(spec);h.data(e).hover=true;d.fail=complete[i];
     fails([&]{draw_buttons(h.frame,false,d);});check(d.calls==std::vector<std::string>(complete.begin(),complete.begin()+static_cast<std::ptrdiff_t>(i+1)));
     check(d.mode==(i==7||i==8));}
    for(int missing=0;missing<3;++missing){Host h;Drawing d;h.make(h.spec());if(missing==0)d.app=nullptr;if(missing==1)d.manager=nullptr;if(missing==2)d.seb=nullptr;
     fails([&]{draw_buttons(h.frame,false,d);});check(d.calls.back()==(missing==0?"app":missing==1?"resource":"seb"));}
    {Host h;Drawing d;h.make(h.spec());bool added=false;d.callback=[&](const std::string& name){if(name=="sprite"&&!added){added=true;h.make(h.spec());}};
     fails([&]{draw_buttons(h.frame,false,d);});check(d.images.size()==1&&d.touches.size()==1&&d.calls.back()=="rect");}
    {Host h;Drawing d;auto& e=h.make(h.spec());h.data(e).hover=true;d.callback=[&](const std::string& name){if(name=="mode")e.remove(2);};
     fails([&]{draw_buttons(h.frame,false,d);});check(d.calls.back()=="mode"&&d.mode&&d.images.empty()&&d.touches.empty());}
    {Host h;Drawing d;auto spec=h.spec();spec.properties.dynamic=true;auto& e=h.make(spec);d.callback=[&](const std::string& name){if(name=="sprite")e.remove(2);};
     fails([&]{draw_buttons(h.frame,true,d);});check(d.calls.back()=="sprite"&&d.images.size()==1&&d.touches.empty());}
    // The primitive helper anchors by width/height only, then delegates the fresh SEB hit registration.
    for(bool dynamic:{false,true}){Drawing d;int option=0;ButtonDrawRequest r;r.x=100;r.y=80;r.resource=7;r.clip=36;r.frame=1;r.value=0x10011;r.anchor=34;r.dynamic=dynamic;r.option=&option;
     draw_button(r,d);check(d.images.size()==1&&d.images[0].x==95&&d.images[0].y==76&&d.images[0].scale==100&&d.images[0].anchor==17);
     const auto& t=d.touches.at(0);check(t.id==35&&t.value==0x10011&&t.seb==&d&&t.frame==1&&t.option==&option);
     if(dynamic)check(d.calls==std::vector<std::string>{"app","resource","seb","bounds","ex","touch metrics","delay"}&&t.rectangle.x==179&&t.rectangle.y==143);
     else check(d.calls==std::vector<std::string>{"app","resource","seb","bounds","ex","touch"}&&t.rectangle.x==95&&t.rectangle.y==76);}
    {Drawing d;ButtonDrawRequest r;r.id=44;r.x=100;r.y=80;r.resource=7;r.clip=36;r.frame=1;r.value=23;r.dynamic=true;int option=0,other=0,seb=0;r.option=&option;d.seb=&seb;
     d.callback=[&](const std::string& name){if(name=="app"){r.id=r.x=r.clip=r.frame=r.value=999;r.option=&other;}if(name=="ex"){d.touch_metrics={200,50};d.seb=&other;}};
     draw_button(r,d);check(d.images[0].clip==36&&d.images[0].frame==1&&d.touches[0].id==44&&d.touches[0].value==23&&d.touches[0].option==&option&&d.touches[0].seb==&seb);
     check(d.touches[0].rectangle.x==100&&d.touches[0].rectangle.y==80);}
    for(auto anchor:{0,2,4,32,64,34,68}){Drawing d;ButtonDrawRequest r;r.x=100;r.y=80;r.anchor=anchor;draw_button(r,d);
     check(d.images[0].x==100-((anchor&2)?5:(anchor&4)?11:0)&&d.images[0].y==80-((anchor&32)?4:(anchor&64)?9:0));}
    {Drawing d;d.bounds={100,100,-32768,-1};ButtonDrawRequest r;r.anchor=34;draw_button(r,d);
     check(d.images[0].x==16384&&d.images[0].y==0&&d.touches[0].rectangle.x==16384);}
    {Drawing d;d.bounds={0,0,11,9};ButtonDrawRequest r;r.x=std::numeric_limits<std::int32_t>::min();r.y=r.x;r.anchor=68;draw_button(r,d);
     check(d.touches[0].rectangle.x==2147483637&&d.touches[0].rectangle.y==2147483639);}
    for(const auto& stop:std::vector<std::string>{"app","resource","seb","bounds","ex","touch metrics","delay"}){Drawing d;ButtonDrawRequest r;r.dynamic=true;d.fail=stop;
     fails([&]{draw_button(r,d);});check(d.calls.back()==stop);}
    {Drawing d;d.touch_metrics={std::numeric_limits<float>::infinity(),1};ButtonDrawRequest r;r.dynamic=true;draw_button(r,d);
     check(d.touches[0].rectangle.x==0&&d.touches[0].rectangle.y==0);r.x=1;d.calls.clear();draw_button(r,d);check(d.touches.back().rectangle.x==std::numeric_limits<std::int32_t>::min());}
    {ButtonDrawingHost h;fails([&]{h.button_draw_metrics();});fails([&]{h.button_touch_metrics();});fails([&]{h.button_black_color();});}
}
int main()try{
    drawing_contracts();
    {Host h;h.acquired_hover=true;auto spec=h.spec();auto text=std::make_shared<std::u16string>(u"sample");spec.properties.text=text;
     auto& e=h.make(spec);check(h.group.size()==1&&h.all.size()==1&&h.ids.counter==1);
     check(h.pos(e).x==7&&h.pos(e).y==9&&h.pos(e).z==0&&!h.pos(e).parent);
     check(!e.contains(1)&&!e.contains(4)&&!e.contains(5)&&!e.contains(7));
     auto& sprite=*dynamic_cast<EntitySprite*>(e.get(2));check(sprite.animation.resource==2&&sprite.animation.clip==40&&sprite.animation.frame==3&&*sprite.layers==EventIntegers{-1});
     check(h.data(e).hover&&h.data(e).text==text&&h.data(e).offset_x==8&&h.data(e).offset_y==-9);}
    {Host h;auto& first=h.make(h.spec());auto& second=h.make(h.spec());auto s=h.spec();s.unique_value=true;s.properties.id=999;
     auto& replacement=h.make(s);check(h.destroyed==std::vector<ComponentSlots*>{&first}&&h.group.size()==2);
     check(h.data(second).id==10&&h.data(replacement).id==999);
     check(h.calls[h.calls.size()-3]=="destroy"&&h.calls[h.calls.size()-2]=="create"&&h.calls.back()=="register");}
    {Host h;h.make(h.spec());auto s=h.spec();s.unique_value=true;
     h.on_destroy=[&](auto&){s.x=500;s.properties.id=999;s.properties.value=888;};
     auto& e=create_button(s,h.ids,h);check(h.pos(e).x==7&&h.data(e).id==10&&h.data(e).value==23);}
    {Host h;auto& e=h.actor(0,0,0);auto p=h.spec().properties;
     h.on_acquire=[&]{p.id=999;p.scale=777;};add_button(e,p,h);check(h.data(e).id==10&&h.data(e).scale==100);}
    {Host h;auto& parent=h.actor(100.5F,10.25F,40.25F);auto& e=h.make(h.spec(&parent));h.form.x=3;h.form.y=5;h.calls.clear();
     update_buttons(h.frame,h);check(h.pos(e).x==71.5625F&&h.pos(e).y==68.90625F);
     check(h.calls==std::vector<std::string>{"destroyed?","project-x","project-y","form","form"});
     h.data(e).dynamic=true;update_buttons(h.frame,h);check(h.pos(e).x==57.25F&&h.pos(e).y==55.125F);
     // Button.offsetX/Y and scale are drawing inputs, not added by this frame pass.
     h.data(e).offset_x=1000;h.data(e).scale=1;update_buttons(h.frame,h);check(h.pos(e).x==57.25F);
     h.dead.insert(&parent);h.pos(e).x=999;update_buttons(h.frame,h);check(h.pos(e).x==999&&h.destroyed.empty());
     h.dead.clear();parent.remove(0);update_buttons(h.frame,h);check(h.pos(e).x==999);}
    {Host h;auto& parent=h.actor(100,10,40);auto& e=h.make(h.spec(&parent));h.on_y=[](){throw std::runtime_error("projection");};
     h.data(e).auto_destroy=true;
     fails([&]{update_buttons(h.frame,h);});check(h.pos(e).x==60&&h.pos(e).y==9&&h.destroyed.empty());h.on_y={};h.null_form=true;
     fails([&]{update_buttons(h.frame,h);});check(h.pos(e).x==60&&h.pos(e).y==60&&h.destroyed.empty());}
    {Host h;auto& parent=h.actor(100,10,40);auto& e=h.make(h.spec(&parent));h.on_form=[&]{h.data(e).dynamic=true;};
     update_buttons(h.frame,h);check(h.pos(e).x==60&&h.pos(e).y==60);}
    {Host h;auto& parent=h.actor(100,10,40);auto& e=h.make(h.spec(&parent));int calls=0;
     h.on_form=[&]{if(++calls==1)h.pos(e).y=999;else{h.pos(e).x=1000;h.pos(e).y=80;}};
     update_buttons(h.frame,h);check(calls==2&&h.pos(e).x==75&&h.pos(e).y==100);}
    {Host h;auto s=h.spec();s.properties.auto_destroy=true;auto& a=h.make(s);auto& b=h.make(s);auto& c=h.make(s);
     h.on_destroy=[&](auto&){if(h.destroyed.size()==1){h.data(b).auto_destroy=false;h.make(s);}};
     update_buttons(h.frame,h);check(h.destroyed==std::vector<ComponentSlots*>{&a,&b,&c});check(h.group.size()==1);}
    {Host h;auto& a=h.make(h.spec());auto& b=h.make(h.spec());InputControl c{10,23};InputTouch event{13,&c};h.receiver(&event);
     check(h.data(a).hover&&!h.data(b).hover&&h.events.back().first==&event&&h.events.back().second==&a);
     event.type=2;h.receiver(&event);check(!h.data(a).hover);c.group=99;h.receiver(&event);check(!h.events.back().second);
     const auto delivered=h.events.size();fails([&]{h.receiver(nullptr);});event.component=nullptr;fails([&]{h.receiver(&event);});check(h.events.size()==delivered);}
    {Host h;h.null_subset=true;initialize_buttons(h.frame,&h,h);check(!h.frame.subset&&h.receiver);fails([&]{update_buttons(h.frame,h);});
     fails([&]{initialize_buttons(h.frame,nullptr,h);});}
    check(gameplay_needs_update(26,true,0)&&!gameplay_needs_update(26,true,1)&&!gameplay_needs_update(26,false,0));
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
