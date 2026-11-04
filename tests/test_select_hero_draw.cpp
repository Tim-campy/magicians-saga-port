#include "core/gameplay/select_form_pass.hpp"
#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
namespace {
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
struct Host final:SelectHeroDrawingHost {
    std::vector<std::string> calls;
    ComponentSlots hero{35};
    std::int32_t height=320;
    std::array<std::int32_t,4> marker{};
    std::array<std::int32_t,3> effect{};
    std::array<std::int32_t,5> draw{};
    ComponentSlots* drawn=nullptr;
    std::function<void(const std::string&)> on_call;
    void call(const std::string& name){calls.push_back(name);if(on_call)on_call(name);}
    SelectBackgroundResource hero_marker_resource()override{call("resource");return this;}
    std::int32_t hero_view_height()override{call("height");return height;}
    std::int32_t hero_marker_clip()override{call("clip");return 16;}
    void hero_target_marker(SelectBackgroundResource r,std::int32_t c,std::int32_t f,std::int32_t x,std::int32_t y)override{
        check(r==this,"marker uses captured resource");marker={c,f,x,y};call("marker");}
    void hero_target_effect(std::int32_t x,std::int32_t y,std::int32_t f)override{effect={x,y,f};call("effect");}
    ComponentSlots* town_hero()override{call("hero");return &hero;}
    void hero_draw(ComponentSlots* e,std::int32_t x,std::int32_t y,std::int32_t c,std::int32_t d,std::int32_t f)override{
        drawn=e;draw={x,y,c,d,f};call("draw");}
};
void initial(){
    SelectHeroDrawingState state;std::int32_t frame=0;Host host;
    draw_select_town_hero(state,frame,{300,53},host);
    check(host.calls==std::vector<std::string>{"height","hero","draw"},"initial equal positions skip marker/effect and resolve hero after height");
    check(host.draw==std::array<std::int32_t,5>{340,289,36,1,0}&&host.drawn==&host.hero,"native Init values produce idle clip and world origin");
}
void moving_and_callbacks(){
    SelectHeroDrawingState state{40.75F,2,90.75F};std::int32_t frame=23;Host host;
    host.on_call=[&](const auto& name){
        if(name=="marker")state.target_x=100.9F;
        if(name=="effect"){state.current_x=50.9F;state.target_x=50.9F;}
        if(name=="height"&&host.calls.size()>5){state.current_x=700;state.target_x=800;state.direction=3;frame=30;}
        if(name=="hero"){state.direction=9;frame=99;}
    };
    draw_select_town_hero(state,frame,{10,53},host);
    check(host.marker==std::array<std::int32_t,4>{16,23,100,284},"first target is captured before marker and truncates toward zero");
    check(host.effect==std::array<std::int32_t,3>{110,284,4},"effect rereads target, uses a fresh height and counter/3 remainder8 minus3");
    check(host.draw==std::array<std::int32_t,5>{60,289,36,3,30},"hero captures equality/current before height, direction/frame after height and before hero lookup");
    check(host.calls==std::vector<std::string>{"resource","height","clip","marker","height","effect","height","hero","draw"},"marker/effect/hero preserve native query order");
}
void special_float_and_failures(){
    SelectHeroDrawingState state{-40.75F,1,-40.75F};std::int32_t frame=0;Host negative;
    draw_select_town_hero(state,frame,{0,0},negative);
    check(negative.draw[0]==-40,"negative hero position truncates toward zero");
    state.current_x=state.target_x=std::numeric_limits<float>::infinity();Host infinity;
    draw_select_town_hero(state,frame,{0,0},infinity);
    check(infinity.draw[0]==std::numeric_limits<std::int32_t>::min(),"positive infinity preserves the native int32 override");
    state={0,1,1};Host failed;failed.on_call=[](const auto& name){if(name=="marker")throw std::runtime_error("marker");};
    bool caught=false;try{draw_select_town_hero(state,frame,{0,0},failed);}catch(const std::runtime_error&){caught=true;}
    check(caught&&failed.calls.back()=="marker"&&failed.drawn==nullptr,"marker failure stops before effect and hero");
}
}
int main(){initial();moving_and_callbacks();special_float_and_failures();}
