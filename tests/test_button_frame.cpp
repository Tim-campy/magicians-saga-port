#include "core/gameplay/button_frame.hpp"
#include "core/gameplay/render_frame.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <set>
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
int main()try{
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
