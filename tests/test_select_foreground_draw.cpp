#include "core/gameplay/select_form_pass.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
namespace {
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
struct Host final:SelectForegroundDrawingHost {
    std::vector<std::string> log;
    std::int32_t height=320,width=600,capacity=1,towns=3,positions=0;
    std::shared_ptr<SelectAreaDrawingRows> areas=std::make_shared<SelectAreaDrawingRows>(1,std::make_shared<SelectAreaDrawingRecord>());
    SharedAnimationClip arrow{0,8,nullptr};
    std::vector<std::array<std::int32_t,5>> draws;
    std::vector<SelectForegroundImage> images;
    std::vector<SelectForegroundTouch> touches;
    std::function<void(const std::string&)> action;
    void call(const std::string& name){log.push_back(name);if(action)action(name);}
    std::int32_t game_width()override{call("width");return width;}
    std::int32_t game_height()override{call("height");return height;}
    std::int32_t foreground_view_height()override{call("raw_height");return 320;}
    std::int32_t foreground_view_width()override{call("raw_width");return 600;}
    std::int32_t foreground_capacity()override{call("capacity");return capacity;}
    std::int32_t foreground_town_count()override{call("towns");return towns;}
    std::array<std::int32_t,2> foreground_obstacle_position()override{call("position");return ++positions%2?std::array<std::int32_t,2>{200,111}:std::array<std::int32_t,2>{999,250};}
    std::shared_ptr<SelectAreaDrawingRows> foreground_areas()override{const auto captured=areas;call("areas");return captured;}
    SelectBackgroundResource foreground_resource()override{call("resource");return this;}
    std::int32_t foreground_clip(std::int32_t field)override{call("clip"+std::to_string(field));
        switch(field){case 0x168:return 1;case 0x170:return 3;case 0x188:return 9;case 0x1cc:return 26;case 0x1d8:return 29;case 0x1dc:return 30;}
        throw std::logic_error("unexpected selector");}
    SharedAnimationClip* foreground_animation(std::int32_t clip)override{check(clip==9,"native selected marker uses clip9");call("animation");return &arrow;}
    void foreground_draw(SelectBackgroundResource,std::int32_t clip,std::int32_t frame,std::int32_t layer,std::int32_t x,std::int32_t y)override{draws.push_back({clip,frame,layer,x,y});call("draw");}
    void foreground_image(SelectBackgroundResource,const SelectForegroundImage& image)override{images.push_back(image);call("image");}
    void foreground_touch(const SelectForegroundTouch& touch)override{touches.push_back(touch);call("touch");}
    void foreground_information(const SelectForegroundDrawingState&,std::int32_t,std::int32_t,SelectBackgroundOrigin)override{call("information");throw std::logic_error("unbound information");}
};
struct Fixture {
    std::int32_t frame=17,selected=-1,information_frame=-1,information_number=0,scale=100,left=-30,right=580;
    EventScript notices=std::make_shared<std::vector<EventInstruction>>();
    SelectForegroundDrawingState state{frame,selected,information_frame,information_number,scale,left,right,notices};Host host;
};
void fresh(){
    Fixture f;draw_select_town_foreground(f.state,{10,53},f.host);
    check(f.host.draws==std::vector<std::array<std::int32_t,5>>{{3,0,1,50,303},{26,0,-1,-20,303},{26,1,-1,590,303}},"foreground gate layer1 followed by left/right boundary frames");
    check(f.host.positions==2&&f.host.images.size()==1,"obstacle coordinates come from separate calls");
    const auto& image=f.host.images.front();check(image.clip==1&&image.image==31&&image.x==200&&image.y==250&&image.frame==17&&image.scale==100&&image.anchor==17&&image.layer==-1,"obstacle uses CHIP1/image31 and native DrawSebImg arguments");
    const auto& t=f.host.touches.front();check(t.kind==10&&t.flags==0x2203e7&&t.rectangle==std::array<std::int32_t,4>{160,160,80,90}&&t.margins==std::array<std::int32_t,4>{-10,-10,-10,-10},"unselected obstacle touch geometry and flags");
    check(f.notices->empty(),"fresh information and guides do not invent notices");
}
void selected_and_live_reads(){
    Fixture f;f.selected=999;
    f.host.action=[&](const auto& name){
        if(name=="clip360")f.frame=99; // Counter captured before CHIP selector callback.
        if(name=="image"){f.frame=49;f.selected=999;}
        if(name=="animation"){f.frame=55;f.scale=75;f.selected=-1;}
    };
    draw_select_town_foreground(f.state,{10,53},f.host);
    check(f.host.images.front().frame==17,"obstacle counter precedes selector access");
    check(*f.notices->at(0)==EventIntegers({11,195,220,9,1,0,0}),"selected marker captures counter after animation owner and keeps selected branch");
    const auto& t=f.host.touches.front();check(t.kind==4&&t.flags==29&&t.rectangle==std::array<std::int32_t,4>{160,130,80,90}&&t.margins==std::array<std::int32_t,4>{-10,-10,-10,25}&&t.scale==75,"selected touch uses live form scale and native margins");
    Fixture gated;gated.information_frame=1;
    bool caught=false;try{draw_select_town_foreground(gated.state,{0,0},gated.host);}catch(const std::logic_error&){caught=true;}
    check(caught&&gated.host.log.back()=="information","unbound Update-produced information fails explicitly before boundary work");
}
void boundary_owners_and_guides(){
    Fixture f;int queries=0;
    auto first=f.host.areas;auto second=std::make_shared<SelectAreaDrawingRows>(2);
    f.host.action=[&](const auto& name){if(name=="areas"&&++queries==1)f.host.areas=second;};
    bool caught=false;try{draw_select_town_foreground(f.state,{0,0},f.host);}catch(const std::out_of_range&){caught=true;}
    check(caught,"last index uses second catalog length against captured first catalog");
    Host h;auto guides=std::make_shared<std::vector<std::uint8_t>>(std::initializer_list<std::uint8_t>{1,1});auto notices=std::make_shared<std::vector<EventInstruction>>();std::int32_t frame=31;
    queue_select_town_guides(guides,frame,300,{10,53},notices,h);
    check(*notices->at(0)==EventIntegers({11,10,193,29,31,0,0}),"left guide takes priority and uses captured terrain floor");
    guides->at(0)=0;queue_select_town_guides(guides,frame,300,{10,53},notices,h);
    check(*notices->at(1)==EventIntegers({11,590,193,30,31,0,1}),"right guide queries raw width and keeps insertion depth");
}
}
int main(){fresh();selected_and_live_reads();boundary_owners_and_guides();}
