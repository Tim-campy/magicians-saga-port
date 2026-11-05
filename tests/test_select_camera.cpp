#include "core/gameplay/select_camera.hpp"
#include "core/gameplay/input_frame.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace saga::core;
void check(bool value){if(!value){std::cerr<<"camera contract failure\n";std::exit(1);}}
struct Host final:SelectCameraHost,SelectActiveAreaHost {
    std::vector<std::string> calls;
    std::vector<std::int32_t> widths{320,320},heights{240,240},game_heights{240,240};
    std::size_t wi=0,hi=0,ghi=0;
    std::function<void()> width_hook;
    bool fail_height=false;
    std::shared_ptr<EventIntegers> save=std::make_shared<EventIntegers>(20,0);
    std::shared_ptr<CameraAreas> areas=std::make_shared<CameraAreas>();
    int area_calls=0;
    std::function<void(int)> area_hook;
    std::shared_ptr<EventIntegers> normal_save_integers() override {calls.push_back("save");return save;}
    std::shared_ptr<CameraAreas> camera_areas() override {calls.push_back("areas");++area_calls;if(area_hook)area_hook(area_calls);return areas;}
    std::int32_t game_width() override {calls.push_back("gw");if(width_hook)width_hook();return 320;}
    std::int32_t game_height() override {calls.push_back("gh");if(fail_height)throw std::runtime_error("height");return game_heights.at(ghi++);}
    std::int32_t view_width() override {calls.push_back("vw");return widths.at(wi++);}
    std::int32_t view_height() override {calls.push_back("vh");return heights.at(hi++);}
};
struct Fixture {
    std::int32_t x=-10,y=0;
    SelectCameraState state{false,0,0,x,y,0,1000};
    Host host;
    void move(){move_select_camera(state,host);}
};
template<class Error,class Action>void touch_fails(Action action){
    try{action();check(false);}catch(const Error&){}
}
struct TouchHost final:SelectTouchHost {
    EventInstruction range=std::make_shared<EventIntegers>(EventIntegers{-30,90});
    std::vector<std::string> calls;
    std::function<void()> range_hook,count_hook,position_hook,scale_hook;
    std::int32_t count=1,requested_x=0,requested_y=0,requested_scale=0,base_value=0;
    std::uint32_t base_flags=0;
    EventInstruction active_range()override{calls.push_back("range");if(range_hook)range_hook();return range;}
    std::int32_t touch_count()override{calls.push_back("count");if(count_hook)count_hook();return count;}
    void set_camera_position(std::int32_t x,std::int32_t y)override{calls.push_back("position");requested_x=x;requested_y=y;if(position_hook)position_hook();}
    void set_scale(std::int32_t value)override{calls.push_back("scale");requested_scale=value;if(scale_hook)scale_hook();}
    void base_set_touch_value(std::uint32_t flags,std::int32_t value)override{calls.push_back("base-set");base_flags=flags;base_value=value;}
    std::int32_t base_get_touch_value(std::uint32_t flags,std::int32_t fallback)override{calls.push_back("base-get");base_flags=flags;base_value=fallback;return 123;}
};
struct TouchFixture {
    std::int32_t x=0,y=0,chip=7,area=8,form=0,scale=100;
    SelectCameraState camera{false,12.0F,13.0F,x,y,-100,1000};
    SelectTouchMoveState movement{false,40.0F,40.0F,1,0};
    SelectTouchState touch{camera,movement,chip,area,form,scale};
    TouchHost host;
    InputControl control{17,0};
    SelectFormTouch event{0,50,0,0,0,&control};
    void send(std::int32_t type){on_select_camera_touch(touch,type,&event,host);}
};
struct ReturnPositionHost final:SelectReturnPositionHost {
    EventInstruction save=std::make_shared<EventIntegers>(14,0);
    EventInstruction position=std::make_shared<EventIntegers>(EventIntegers{100,999});
    std::vector<std::string> calls;std::string fail;
    std::function<void(const std::string&)> hook;
    std::int32_t width=301,height=500,surface_height=640,area=-1,scale=-1,x=0,y=0;
    int save_calls=0;
    ReturnPositionHost(){save->at(8)=3;save->at(13)=80;}
    void call(const std::string& name){calls.push_back(name);if(hook)hook(name);if(name==fail)throw std::runtime_error(name);}
    EventInstruction return_saved_integers()override{call("save"+std::to_string(++save_calls));return save;}
    EventInstruction return_area_position(std::int32_t value)override{area=value;call("area");return position;}
    void return_scale(std::int32_t value)override{scale=value;call("scale");}
    std::int32_t return_game_width()override{call("width");return width;}
    std::int32_t return_game_height()override{call("height");return height;}
    std::int32_t return_surface_game_height()override{call("surface_height");return surface_height;}
    void return_camera_position(std::int32_t a,std::int32_t b)override{x=a;y=b;call("camera");}
};
void test_return_position(){
    const std::vector<std::string> order{"save1","area","save2","scale","width","height","surface_height","camera"};
    {SelectTouchMoveState state{true,-17,23,2,9};ReturnPositionHost h;
     set_select_return_position(state,h);
     check(h.calls==order&&h.area==3&&h.scale==80&&h.x==315&&h.y==-180);
     check(state.witch_x==165&&state.drag_distance==165&&state.enabled&&state.direction==2&&state.collision_id==9);}
    {SelectTouchMoveState state{false,-17,23,1,0};ReturnPositionHost h;h.position->resize(1);
     set_select_return_position(state,h);check(state.witch_x==165&&h.x==315);} // Caller requires only cell0.
    {SelectTouchMoveState state{false,-17,23,1,0};ReturnPositionHost h;const auto original=h.save;
     h.hook=[&](const std::string& call){if(call=="area"){h.save=std::make_shared<EventIntegers>(14,0);h.save->at(8)=99;h.save->at(13)=61;}};
     set_select_return_position(state,h);check(h.area==3&&h.scale==61&&original->at(13)==80&&state.witch_x==165);}
    {SelectTouchMoveState state{false,-17,23,1,0};ReturnPositionHost h;
     h.hook=[&](const std::string& call){if(call=="scale")state.witch_x=-123.75F;else if(call=="width")state.witch_x=444;
         else if(call=="camera")state.witch_x=777;};
     set_select_return_position(state,h);check(h.x==273&&state.witch_x==777&&state.drag_distance==777);}
    {SelectTouchMoveState state{false,-17,23,1,0};ReturnPositionHost h;
     h.position->at(0)=std::numeric_limits<std::int32_t>::max()-64;h.width=2;
     h.height=std::numeric_limits<std::int32_t>::max();h.surface_height=std::numeric_limits<std::int32_t>::min();
     set_select_return_position(state,h);
     check(state.witch_x==-2147483648.0F&&h.x==std::numeric_limits<std::int32_t>::min()&&h.y==-41);}
    struct Conversion {float value;std::int32_t absolute;};
    const Conversion conversions[]{{-1.9F,1},{-0.0F,0},{std::numeric_limits<float>::quiet_NaN(),0},
        {std::numeric_limits<float>::infinity(),std::numeric_limits<std::int32_t>::min()},
        {-std::numeric_limits<float>::infinity(),std::numeric_limits<std::int32_t>::min()},
        {2147483648.0F,std::numeric_limits<std::int32_t>::max()},
        {-3.0e10F,std::numeric_limits<std::int32_t>::max()}};
    for(const auto& conversion:conversions){
        SelectTouchMoveState state{false,-17,23,1,0};ReturnPositionHost h;h.width=-3;
        h.hook=[&](const std::string& call){if(call=="scale")state.witch_x=conversion.value;};
        set_select_return_position(state,h);
        const auto expected=conversion.absolute==std::numeric_limits<std::int32_t>::min()?
            std::numeric_limits<std::int32_t>::max():conversion.absolute-1;
        check(h.x==expected);
        if(std::isnan(conversion.value))check(std::isnan(state.drag_distance));
        else check(state.drag_distance==conversion.value&&std::signbit(state.drag_distance)==std::signbit(conversion.value));
    }
    for(const auto& failure:order){
        SelectTouchMoveState state{false,-17,23,1,0};ReturnPositionHost h;h.fail=failure;
        touch_fails<std::runtime_error>([&]{set_select_return_position(state,h);});
        const auto end=std::find(order.begin(),order.end(),failure)+1;
        check(h.calls==std::vector<std::string>(order.begin(),end)&&state.drag_distance==23);
        check(state.witch_x==(end>order.begin()+2?165:-17));
    }
    for(const bool missing:{false,true}){
        SelectTouchMoveState state{false,-17,23,1,0};ReturnPositionHost h;
        if(missing)h.save.reset();else h.save->resize(8);
        if(missing)touch_fails<std::invalid_argument>([&]{set_select_return_position(state,h);});
        else touch_fails<std::out_of_range>([&]{set_select_return_position(state,h);});
        check(h.calls==std::vector<std::string>{"save1"}&&state.witch_x==-17&&state.drag_distance==23);
    }
    for(const bool missing:{false,true}){
        SelectTouchMoveState state{false,-17,23,1,0};ReturnPositionHost h;
        if(missing)h.position.reset();else h.position->clear();
        if(missing)touch_fails<std::invalid_argument>([&]{set_select_return_position(state,h);});
        else touch_fails<std::out_of_range>([&]{set_select_return_position(state,h);});
        check(h.calls==std::vector<std::string>({"save1","area"})&&state.witch_x==-17&&state.drag_distance==23);
    }
    for(const bool missing:{false,true}){
        SelectTouchMoveState state{false,-17,23,1,0};ReturnPositionHost h;
        h.hook=[&](const std::string& call){if(call=="area"){if(missing)h.save.reset();else h.save->resize(13);}};
        if(missing)touch_fails<std::invalid_argument>([&]{set_select_return_position(state,h);});
        else touch_fails<std::out_of_range>([&]{set_select_return_position(state,h);});
        check(h.calls==std::vector<std::string>({"save1","area","save2"})&&state.witch_x==165&&state.drag_distance==23);
    }
    {SelectTouchMoveState state{false,-17,23,1,0};ReturnPositionHost h;h.fail="camera";
     h.hook=[&](const std::string& call){if(call=="camera")state.witch_x=999;};
     touch_fails<std::runtime_error>([&]{set_select_return_position(state,h);});
     check(state.witch_x==999&&state.drag_distance==23);}
}
void test_touch_producers(){
    {TouchFixture f;set_select_touch_value(f.touch,0x2203e7U,999,f.host);check(f.chip==999&&f.area==-1);
     check(get_select_touch_value(f.touch,0x22ffffU,-8,f.host)==999);
     set_select_touch_value(f.touch,0xc0002U,-2,f.host);check(f.area==-2&&f.chip==-1);
     check(get_select_touch_value(f.touch,0xcffffU,4,f.host)==-2&&f.host.calls.empty());
     set_select_touch_value(f.touch,0xdead1234U,45,f.host);check(f.host.base_flags==0xdead0000U&&f.host.base_value==45);
     check(get_select_touch_value(f.touch,0x9911U,56,f.host)==123&&f.host.base_flags==0&&f.host.base_value==56);
     check(f.chip==-1&&f.area==-2);}
    {TouchFixture f;touch_fails<std::invalid_argument>([&]{on_select_form_touch(f.touch,nullptr,f.host);});
     f.event.control=nullptr;touch_fails<std::invalid_argument>([&]{on_select_form_touch(f.touch,&f.event,f.host);});
     touch_fails<std::invalid_argument>([&]{on_select_camera_touch(f.touch,0,nullptr,f.host);});check(!f.camera.dragging&&!f.movement.enabled);}
    {TouchFixture f;f.control.group=18;f.touch.history.reset();f.scale=0;
     check(on_select_form_touch(f.touch,&f.event,f.host)&&f.host.calls.empty()&&!f.movement.enabled);}
    {TouchFixture f;f.touch.history->push_back(std::make_shared<EventIntegers>(2,8));f.touch.build_drag=true;
     f.host.range_hook=[&]{f.control.group=10;};check(!on_select_form_touch(f.touch,&f.event,f.host));
     check(f.touch.history->empty()&&f.camera.dragging&&!f.touch.build_drag&&f.movement.enabled&&f.movement.drag_distance==50.0F);}
    {TouchFixture f;f.form=4;f.touch.history->push_back(nullptr);f.send(0);
     check(f.touch.history->empty()&&f.camera.dragging&&f.movement.enabled&&f.movement.drag_distance==40.0F&&f.host.calls.empty());}
    {TouchFixture f;f.touch.history.reset();f.touch.build_drag=true;touch_fails<std::invalid_argument>([&]{f.send(0);});
     check(!f.camera.dragging&&!f.movement.enabled&&f.touch.build_drag);}
    {TouchFixture f;f.scale=50;f.x=-10;f.event.x=100;f.send(0);check(f.movement.drag_distance==90.0F);}
    {TouchFixture f;f.event.x=-100;f.host.range=std::make_shared<EventIntegers>(EventIntegers{-30});f.send(0);check(f.movement.drag_distance==-30.0F);}
    {TouchFixture f;f.event.x=50;f.host.range=std::make_shared<EventIntegers>(EventIntegers{-30});
     touch_fails<std::out_of_range>([&]{f.send(0);});check(f.camera.dragging&&f.movement.enabled&&f.movement.drag_distance==50.0F);}
    {TouchFixture f;f.host.range_hook=[&]{f.movement.drag_distance=std::numeric_limits<float>::quiet_NaN();};
     f.send(0);check(std::isnan(f.movement.drag_distance));}
    {TouchFixture f;f.camera.dragging=true;f.touch.history->push_back(std::make_shared<EventIntegers>(EventIntegers{5,-8}));
     *f.touch.camera_decimal={1,2,3};f.event.dx=10;f.event.dy=2;f.send(1);
     check(!f.camera.dragging&&(*f.touch.camera_speed==std::array<float,3>{5,0,10})&&(*f.touch.camera_decimal==std::array<float,3>{0,0,0}));
     check(f.camera.velocity_x==12&&f.camera.velocity_y==13);}
    {TouchFixture f;f.camera.dragging=true;f.touch.camera_speed.reset();f.touch.camera_decimal.reset();f.send(1);check(!f.camera.dragging);}
    {TouchFixture f;f.camera.dragging=false;f.touch.history.reset();touch_fails<std::invalid_argument>([&]{f.send(1);});}
    {TouchFixture f;f.camera.dragging=true;f.touch.camera_speed.reset();f.touch.history->push_back(std::make_shared<EventIntegers>(1,0));
     touch_fails<std::out_of_range>([&]{f.send(1);});check(f.camera.dragging);}
    {TouchFixture f;f.camera.dragging=true;f.touch.camera_decimal.reset();f.touch.history->push_back(std::make_shared<EventIntegers>(EventIntegers{1,2}));
     f.event.dx=5;f.event.dy=6;touch_fails<std::invalid_argument>([&]{f.send(1);});
     check(f.camera.dragging&&(*f.touch.camera_speed==std::array<float,3>{4,0,4}));}
    {TouchFixture f;f.send(5);check(f.touch.build_drag);f.touch.build_drag=false;f.touch.history->push_back(nullptr);f.send(5);check(!f.touch.build_drag);}
    {TouchFixture f;f.camera.dragging=true;f.touch.history.reset();f.host.count=2;f.event.dx=100;f.send(8);
     check((f.host.calls==std::vector<std::string>{"count"})&&f.camera.velocity_x==12&&f.chip==7&&f.area==8);}
    {TouchFixture f;f.camera.dragging=true;f.scale=50;f.event.dx=3;f.event.dy=-2;
     auto original=f.touch.history;f.host.count_hook=[&]{f.scale=200;f.x=10;f.y=20;};
     f.host.position_hook=[&]{f.touch.history=std::make_shared<std::vector<EventInstruction>>();};f.send(8);
     check(f.host.requested_x==16&&f.host.requested_y==16&&original->empty()&&f.touch.history->size()==1);
     check((*f.touch.history->at(0)==EventIntegers{6,-4})&&f.camera.velocity_x==6&&f.camera.velocity_y==-4&&f.chip==-1&&f.area==-1);}
    {TouchFixture f;f.event.dx=-100;f.event.dy=-100;f.send(8);check(f.chip==7&&f.area==8&&f.touch.history->size()==1);}
    {TouchFixture f;f.event.dx=5;f.event.dy=5;f.send(8);check(f.chip==7&&f.area==8);
     f.event.dy=6;f.send(8);check(f.chip==-1&&f.area==-1);}
    {TouchFixture f;f.event.dx=std::numeric_limits<std::int32_t>::max();f.send(8);
     check(f.camera.velocity_x==-1&&f.touch.history->at(0)->at(0)==-1&&f.chip==7);}
    {TouchFixture f;f.scale=1;f.event.dx=21474836;f.send(8);
     check(f.camera.velocity_x==2147483648.0F&&f.touch.history->at(0)->at(0)==std::numeric_limits<std::int32_t>::max());}
    {TouchFixture f;for(std::int32_t n=1;n<=5;++n){f.event.dx=n;f.send(8);}
     auto reused=f.touch.history->back();f.event.dx=6;f.event.dy=7;f.send(8);
     check(f.touch.history->size()==5&&f.touch.history->front()==reused&&(*reused==EventIntegers{6,7}));}
    {TouchFixture f;f.touch.history->resize(5);auto row=std::make_shared<EventIntegers>(1,100);f.touch.history->back()=row;
     f.event.dx=7;f.event.dy=8;touch_fails<std::out_of_range>([&]{f.send(8);});
     check(row->at(0)==7&&f.touch.history->size()==5&&f.camera.velocity_x==12&&f.chip==7);}
    {TouchFixture f;f.camera.dragging=true;f.event.dx=2;
     f.host.position_hook=[&]{throw std::runtime_error("position");};touch_fails<std::runtime_error>([&]{f.send(8);});
     check(f.touch.history->empty()&&f.camera.velocity_x==12);}
    for(const auto pair:std::vector<std::array<std::int32_t,2>>{{48,50},{49,50},{50,50},{100,100},{101,100},{190,150}}){
        TouchFixture f;f.touch.pinch_value=pair[0];f.touch.pinch_frame=9;f.touch.build_drag=true;f.send(12);
        check(f.touch.pinch_value==pair[0]&&f.host.requested_scale==pair[1]&&f.touch.pinching&&f.touch.pinch_frame==0&&!f.touch.build_drag);}
    {TouchFixture f;f.touch.pinch_value=std::numeric_limits<std::int32_t>::max();f.event.pinch_delta=1;f.send(12);
     check(f.touch.pinch_value==40&&f.host.requested_scale==50);}
    {TouchFixture f;f.touch.pinch_value=100;f.event.pinch_delta=20;f.touch.pinch_frame=9;f.touch.build_drag=true;
     f.host.scale_hook=[&]{throw std::runtime_error("scale");};touch_fails<std::runtime_error>([&]{f.send(12);});
     check(f.touch.pinch_value==120&&!f.touch.build_drag&&!f.touch.pinching&&f.touch.pinch_frame==9);}
    {TouchFixture f;f.touch.history.reset();f.send(-1);f.send(13);check(f.host.calls.empty());}
}
int main() {
    test_return_position();
    test_touch_producers();
    {Host h;check(select_area_number(h)==-1&&h.area_calls==1);}
    {Host h;for(auto state:{1,0,2,1})h.areas->push_back(std::make_shared<CameraAreaRecord>(CameraAreaRecord{state}));
     check(select_area_number(h)==1&&h.area_calls==9);}
    {Host h;h.areas->push_back(std::make_shared<CameraAreaRecord>(CameraAreaRecord{0}));
     h.area_hook=[&](int n){if(n==2)h.areas->at(0)->state=1;};check(select_area_number(h)==0&&h.area_calls==3);}
    {Host h;h.areas->push_back(nullptr);try{select_area_number(h);check(false);}catch(const std::invalid_argument&){}check(h.area_calls==2);}
    {Host h;h.areas.reset();try{select_area_number(h);check(false);}catch(const std::invalid_argument&){}check(h.area_calls==1);}
    {Fixture f;f.host.save->at(19)=2;f.host.widths={320,330};set_select_active_x(f.state,f.host);check(f.state.right_extent==500 && f.state.left_extent==-459);check((f.host.calls==std::vector<std::string>{"save","vw","areas","vw"}));}
    {Fixture f;f.host.areas->resize(4);f.host.areas->at(1)=std::make_shared<CameraAreaRecord>(CameraAreaRecord{1});f.host.areas->at(3)=std::make_shared<CameraAreaRecord>(CameraAreaRecord{2});set_select_active_x(f.state,f.host);check(f.state.left_extent==-578 && f.host.area_calls==7);}
    {Fixture f;f.host.areas->push_back(std::make_shared<CameraAreaRecord>(CameraAreaRecord{1}));set_select_active_x(f.state,f.host);check(f.state.left_extent==-449);} // Clamp(1,1,0) returns 1.
    {Fixture f;f.host.save->at(19)=std::numeric_limits<std::int32_t>::max();f.host.areas.reset();try{set_select_active_x(f.state,f.host);check(false);}catch(const std::invalid_argument&){}check(f.state.right_extent==230 && f.state.left_extent==0);}
    {Fixture f;f.host.areas->push_back(std::make_shared<CameraAreaRecord>(CameraAreaRecord{1}));f.host.area_hook=[&](int n){if(n==3)f.host.areas->at(0).reset();};try{set_select_active_x(f.state,f.host);check(false);}catch(const std::invalid_argument&){}check(f.state.right_extent==320 && f.state.left_extent==0);}
    check(camera_clamp(5,10,0)==10);
    check(camera_clamp(-20,-10,10)==-10);
    check(camera_clamp(20,-10,10)==10);
    check(scaled_view_dimension(101,200.0F)==50);
    check(scaled_view_dimension(-101,200.0F)==-50);
    check(scaled_view_dimension(1,0.0F)==std::numeric_limits<std::int32_t>::min());
    check(scaled_view_dimension(0,0.0F)==0);
    check(scaled_select_dimension(321,200)==160);
    check(scaled_select_dimension(321,0)==0);
    check(scaled_select_dimension(0x20000000,-1)==std::numeric_limits<std::int32_t>::min());
    {Fixture f;f.state.velocity_x=0.9F;f.state.velocity_y=-0.8F;f.move();check(f.host.calls.empty() && f.state.velocity_x==0.9F && f.state.velocity_y==-0.8F);}
    {Fixture f;f.state.dragging=true;f.state.velocity_x=100.0F;f.move();check(f.host.calls.empty() && f.state.velocity_x==100.0F);}
    {Fixture f;f.state.velocity_x=2.0F;f.state.velocity_y=-0.25F;f.move();check(f.state.velocity_x==1.8F && f.state.velocity_y==0.0F && !std::signbit(f.state.velocity_y));check(f.x==-9 && f.y==0);check((f.host.calls==std::vector<std::string>{"gw","vw","vw","gh","vh","gh","vh"}));}
    {Fixture f;f.state.velocity_x=std::numeric_limits<float>::quiet_NaN();f.move();check(f.host.calls.empty() && std::isnan(f.state.velocity_x));}
    {Fixture f;f.state.velocity_x=std::numeric_limits<float>::infinity();f.move();check(f.x==0 && std::isinf(f.state.velocity_x));} // Wrapped -10 + INT32_MIN is positive, then upper clamp.
    {Fixture f;f.host.widths={320,330};f.host.game_heights={240,260};f.host.heights={240,250};set_select_camera_position(f.state,-999,999,f.host);check(f.x==-670 && f.y==60);}
    {Fixture f;f.host.width_hook=[&]{f.state.right_extent=2000;f.state.left_extent=20;};set_select_camera_position(f.state,-999,0,f.host);check(f.x==-680);} // Right bound captured before the callback.
    {Fixture f;f.host.fail_height=true;f.state.velocity_x=2.0F;try{f.move();check(false);}catch(const std::runtime_error&){}check(f.x==-9 && f.y==0 && f.state.velocity_x==1.8F);}
}
