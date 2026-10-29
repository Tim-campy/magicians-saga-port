#include "core/gameplay/cursor_frame.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
using namespace saga::core;
void check_at(bool v,int line){if(!v){std::cerr<<"cursor contract failed at "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Host final:CursorHost,CursorInitializationWorld {
    CursorState state;MapComponent map;EventInstruction save=std::make_shared<EventIntegers>(19,0);
    EntitySubset humans{[](auto*){return true;},[](const auto&){return 1;}};
    std::vector<std::string> calls;std::vector<std::pair<int,Receiver>> receivers;
    std::vector<std::pair<CursorPointRef,CursorPointRef>> changes;
    CursorFocusEffect effect;bool dead=false,null_effect=false,null_map=false;
    std::function<void()> on_save,on_map,on_change,on_effect,on_subscribe;int save_reads=0;
    Host(){map.step_x=24;map.step_z=24;}
    EventInstruction cursor_save()override{calls.push_back("save");++save_reads;if(on_save)on_save();return save;}
    MapComponent* cursor_map()override{calls.push_back("map");if(on_map)on_map();return null_map?nullptr:&map;}
    EntitySubset* cursor_humans()override{calls.push_back("humans");return &humans;}
    bool cursor_destroyed(ComponentSlots&)override{calls.push_back("destroyed");return dead;}
    void cursor_cell_changed(CursorPointRef old,CursorPointRef next)override{
        calls.push_back("event13");changes.emplace_back(std::move(old),std::move(next));if(on_change)on_change();
    }
    CursorFocusEffect* create_cursor_focus_effect()override{calls.push_back("effect");if(on_effect)on_effect();return null_effect?nullptr:&effect;}
    void subscribe_cursor_point(int event,Receiver f)override{calls.push_back(std::to_string(event));if(on_subscribe)on_subscribe();receivers.emplace_back(event,std::move(f));}
    void init(){initialize_cursor(state,this,*this);calls.clear();save_reads=0;}
};
struct Node {
    ComponentSlots entity{35};ContactPosition position;
    Node(float x,float y,float z){position.x=x;position.y=y;position.z=z;entity.add(0,&position);}
};
CursorPointRef point(int x,int y){return std::make_shared<CursorPoint>(CursorPoint{x,y});}
int main()try{
    {Host h;h.save->at(17)=11;h.save->at(18)=-12;h.state.focus_wait=9;h.state.tracking=true;
     h.on_save=[&]{if(h.save_reads==2){h.save=std::make_shared<EventIntegers>(19,0);h.save->at(18)=45;}};
     initialize_cursor(h.state,&h,h);check(h.state.last_position->x==11&&h.state.last_position->y==45);
     check(h.calls==std::vector<std::string>{"save","save","2","4"});check(h.state.focus_wait==9&&h.state.tracking);
     h.receivers[1].second(nullptr); // Inert click does not validate the point.
     h.on_save={};fails([&]{initialize_cursor(h.state,nullptr,h);});check(h.state.last_position->y==45);}
    {Host h;h.on_subscribe=[&]{if(!h.receivers.empty())throw std::runtime_error("second subscription");};
     fails([&]{initialize_cursor(h.state,&h,h);});check(h.receivers.size()==1&&h.state.last_position);}
    {Host h;h.init();h.null_map=true;update_cursor(h.state,h);check(h.calls==std::vector<std::string>{"save","save"});
     h.state.tracking=true;h.calls.clear();update_cursor(h.state,h);check(!h.state.tracking&&h.calls.size()==2);}
    {Host h;h.init();Node n(0,0,0);h.state.focus=&n.entity;h.state.catch_human=&n.entity;h.state.tracking=true;h.state.focus_wait=7;h.dead=true;
     update_cursor(h.state,h);check(!h.state.tracking&&h.state.focus==&n.entity&&h.state.catch_human==&n.entity&&h.state.focus_wait==7);
     check(h.calls==std::vector<std::string>{"destroyed","save","save"});}
    {Host h;h.init();h.save->at(17)=1;update_cursor(h.state,h);check(h.changes.empty()&&h.state.last_position->x==1);
     h.save->at(17)=48;h.on_change=[&]{h.state.last_position=point(999,999);h.save->at(17)=100;};
     update_cursor(h.state,h);check(h.changes.size()==1&&h.state.last_position->x==48);
     check(h.changes[0].first!=h.changes[0].second&&h.changes[0].first->x==0&&h.changes[0].first->y==0);
     check(h.changes[0].second->x==1&&h.changes[0].second->y==0);}
    {Host h;h.init();h.save->at(17)=48;h.on_change=[](){throw std::runtime_error("event");};
     fails([&]{update_cursor(h.state,h);});check(h.state.last_position->x==0&&h.changes.size()==1);}
    {Host h;h.init();h.save->at(17)=48;h.on_change=[&]{h.state.last_position.reset();};fails([&]{update_cursor(h.state,h);});check(!h.state.last_position);}
    {Host h;h.init();h.save->at(17)=48;h.on_map=[&]{h.state.last_position=point(48,0);};
     update_cursor(h.state,h);check(h.changes.empty());h.on_map=[&]{h.state.last_position.reset();};h.save->at(17)=49;
     fails([&]{update_cursor(h.state,h);});}
    // Integer halves truncate toward zero; wrap precedes the binary32 round trip and SDIV.
    for(const auto p:std::vector<CursorPoint>{{0,0},{48,0},{-48,0},{0,48},{-1,-1},{16777217,0}}){
        const auto result=cursor_cell(24,24,static_cast<float>(p.x),static_cast<float>(p.y));
        if(p.x==0&&p.y==0)check(result.x==0&&result.y==0);
        if(p.x==48)check(result.x==1&&result.y==0);
        if(p.x==-48)check(result.x==0&&result.y==1);
        if(p.y==48)check(result.x==2&&result.y==2);
        if(p.x==-1)check(result.x==0&&result.y==0);
        if(p.x==16777217)check(result.x==349525&&result.y==-349524);
    }
    check(cursor_cell(0,0,123,456).x==0&&cursor_cell(0,0,123,456).y==0);
    check(cursor_cell(-25,-25,38,0).x==0&&cursor_cell(-25,-25,38,0).y==1);
    {Host h;Node a(100.9F,10.9F,40.9F),b(100,10,40);h.humans.add(&a.entity);h.humans.add(&b.entity);
     check(cursor_human_at(60,65,h)==&a.entity); // Iso=(60,80), score center=(60,65); tie retains first.
     for(const auto p:std::vector<CursorPoint>{{40,65},{80,65},{60,45},{60,85}})check(!cursor_human_at(p.x,p.y,h));
     check(cursor_human_at(41,46,h)==&a.entity);h.dead=true;check(cursor_human_at(60,65,h)==&a.entity); // No destroyed/HP/state filter.
     a.entity.remove(0);fails([&]{cursor_human_at(60,65,h);});}
    {Host h;Node a(100,0,0),b(110,0,0);h.humans.add(&a.entity);h.humans.add(&b.entity);
     check(cursor_human_at(109,39,h)==&b.entity);}
    {Host h;h.init();Node a(0,0,0),b(0,0,0);h.humans.add(&a.entity);
     h.state.tracking=true;h.state.catch_human=&b.entity;h.receivers[0].second(point(0,-15));
     check(h.state.focus==&a.entity&&!h.state.tracking&&h.state.focus_wait==6&&h.effect.entity_arg==&a.entity);
     const auto count=h.calls.size();h.state.tracking=true;cursor_touch_down(h.state,point(0,-15),h);
     check(h.state.tracking&&h.calls.size()==count+1); // Same focus only queries the group.
     cursor_touch_down(h.state,point(500,500),h);check(!h.state.focus&&h.state.tracking&&h.state.focus_wait==6&&h.state.catch_human==&b.entity);
     h.on_effect=[&]{h.state.focus=&b.entity;};cursor_touch_down(h.state,point(0,-15),h);check(h.effect.entity_arg==&b.entity);
     h.state.focus=nullptr;h.on_effect={};h.null_effect=true;fails([&]{cursor_touch_down(h.state,point(0,-15),h);});
     check(h.state.focus==&a.entity&&h.state.focus_wait==6&&!h.state.tracking);
     fails([&]{cursor_touch_down(h.state,nullptr,h);});
     start_cursor_catch(h.state,nullptr);check(!h.state.focus&&!h.state.catch_human&&h.state.tracking&&h.state.focus_wait==6);
     update_cursor(h.state,h);check(!h.state.tracking);start_cursor_catch(h.state,&a.entity);
     check(h.state.focus==&a.entity&&h.state.catch_human==&a.entity&&h.state.tracking);}
    check(gameplay_needs_update(24,true,0)&&gameplay_needs_update(24,true,7)&&!gameplay_needs_update(24,false,0));
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
