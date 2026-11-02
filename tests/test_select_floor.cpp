#include "core/gameplay/select_form_pass.hpp"
#include "core/gameplay/render_draw.hpp"
#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
namespace {
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
template<class Exception,class Action>void fails(Action action){bool failed=false;try{action();}catch(const Exception&){failed=true;}check(failed,"expected exception");}
SpriteArray row(std::initializer_list<std::int32_t> values){return std::make_shared<SpriteValues>(values);}
saga::assets::Seb file(std::int16_t key=0,std::uint32_t layers=1){
    saga::assets::Seb source{0,4,{},{}};
    for(std::uint32_t i=0;i<layers;++i){source.groups.push_back({i,1,std::int16_t{0}});
        saga::assets::SebRecord record;record.value_count=10;record.values={key,999,3,4,5,7,2,-3,1,1};source.records.push_back(record);}
    return source;
}
struct Graphics final:RenderGraphics {
    std::vector<RenderImageDraw> draws;
    std::function<void()> after;
    RenderDepth* mesh_manager()override{throw std::logic_error("unexpected depth");}
    void push_render_mode(std::int32_t,std::int32_t,std::int32_t)override{throw std::logic_error("unexpected mode");}
    void pop_render_mode()override{throw std::logic_error("unexpected mode pop");}
    void push_blend_mode(std::int32_t,std::int32_t)override{throw std::logic_error("unexpected blend");}
    void pop_blend_mode()override{throw std::logic_error("unexpected blend pop");}
    void push_matrix(RenderMatrix&)override{throw std::logic_error("unexpected matrix");}
    void pop_matrix()override{throw std::logic_error("unexpected matrix pop");}
    void draw_image(const RenderImageDraw& value)override{draws.push_back(value);if(after)after();}
};
void fixed_image(){
    SebStaticWork work;RenderImage image;Graphics graphics;graphics.reverse=7;
    auto state=prepare_seb_sprites(file(5));
    draw_fixed_image_seb(&graphics,10,20,&state,&image,5,work);
    check(graphics.draws.size()==1,"raw frame5 is not reduced modulo header max4");
    const auto& d=graphics.draws[0];
    check(d.image==&image&&d.x==12&&d.y==17,"explicit image ignores nonnegative TexId999");
    check(d.u==3&&d.v==4&&d.source_width==5&&d.source_height==7&&!d.scaled,"source rectangle is unscaled");
    check(graphics.reverse==7,"sprite reversal flags do not alter Graphics");
    check(state.work->sprites&&state.work->list,"GetSprites uses retained instance work");
    fails<std::invalid_argument>([&]{draw_fixed_image_seb(&graphics,0,0,&state,&image,0,work);});
    fails<std::invalid_argument>([&]{draw_fixed_image_seb(nullptr,0,0,nullptr,nullptr,0,work);});
    auto empty=prepare_seb_sprites(file(0,0));draw_fixed_image_seb(nullptr,0,0,&empty,nullptr,0,work);
    auto cached=prepare_seb_sprites(file());
    cached.cache=std::make_shared<std::vector<SpriteRows>>(1,std::make_shared<std::vector<SpriteArray>>(1,row({0,-99})));
    draw_fixed_image_seb(nullptr,0,0,&cached,nullptr,0,work);
    cached.cache->at(0)->at(0)=row({0,1});
    fails<std::out_of_range>([&]{draw_fixed_image_seb(nullptr,0,0,&cached,nullptr,0,work);});
    cached.cache->at(0)->at(0)=row({0,1,3,4,5,6,7,8});
    fails<std::invalid_argument>([&]{draw_fixed_image_seb(nullptr,0,0,&cached,nullptr,0,work);});
    Graphics null_image;draw_fixed_image_seb(&null_image,0,0,&cached,nullptr,0,work);
    check(null_image.draws.size()==1&&null_image.draws[0].image==nullptr,"null image reaches Graphics service");
    Graphics wrapped;auto normal=prepare_seb_sprites(file());
    draw_fixed_image_seb(&wrapped,std::numeric_limits<std::int32_t>::max(),std::numeric_limits<std::int32_t>::min(),&normal,&image,0,work);
    check(wrapped.draws[0].x==static_cast<float>(std::numeric_limits<std::int32_t>::min()+1),"X wraps before binary32 conversion");
    check(wrapped.draws[0].y==static_cast<float>(std::numeric_limits<std::int32_t>::max()-2),"Y wraps before binary32 conversion");
}
void captured_rows(){
    SebStaticWork work;RenderImage image;Graphics graphics;graphics.reverse=3;
    auto state=prepare_seb_sprites(file(0,2));SpriteRows captured;
    graphics.after=[&]{if(graphics.draws.size()==1){captured=state.work->list;
        state.work->list=std::make_shared<std::vector<SpriteArray>>();
        captured->at(1)=row({0,777,11,12,13,14,15,16,1,1,99,123});}};
    draw_fixed_image_seb(&graphics,100,200,&state,&image,0,work);
    check(graphics.draws.size()==2&&graphics.draws[1].x==115&&graphics.draws[1].y==216,"captured rows survive instance-list replacement");
    check(graphics.draws[1].u==11&&graphics.draws[1].image==&image&&graphics.reverse==3,"later row values are live; color/blend/reverse ignored");
    for(int mode=0;mode<3;++mode){
        auto another=prepare_seb_sprites(file(0,2));Graphics target;
        target.after=[&]{if(target.draws.size()==1){auto& last=another.work->list->at(1);
            if(mode==0)last.reset();else if(mode==1)last=row({0,8});else last=row({0,-8});}};
        if(mode==0)fails<std::invalid_argument>([&]{draw_fixed_image_seb(&target,0,0,&another,&image,0,work);});
        else if(mode==1)fails<std::out_of_range>([&]{draw_fixed_image_seb(&target,0,0,&another,&image,0,work);});
        else draw_fixed_image_seb(&target,0,0,&another,&image,0,work);
        check(target.draws.size()==1,"late row failure or negative skip retains first draw");
    }
}
struct FloorDraw {SebSpriteState* sprite;RenderImage* image;std::int32_t frame,x,y;};
struct GateDraw {SelectBackgroundResource resource;std::int32_t clip,frame,layer;float x,y;};
struct FloorHost final:SelectFloorHost {
    std::vector<std::string> log;
    std::vector<FloorDraw> floors;
    std::vector<GateDraw> gates;
    std::deque<std::int32_t> heights;
    std::function<void(const std::string&)> action;
    SebSpriteState sprite;RenderImage image;int identity=0;
    SelectBackgroundResource resource=&identity;
    SebSpriteState* selected_sprite=&sprite;RenderImage* selected_image=&image;
    std::int32_t width=600,height=200,raw_height=500;
    void call(const std::string& name){log.push_back(name);if(action)action(name);}
    std::int32_t game_width()override{call("W");return width;}
    std::int32_t game_height()override{call("G");return height;}
    std::int32_t floor_view_height()override{call("H");if(heights.empty())return raw_height;const auto value=heights.front();heights.pop_front();return value;}
    SebSpriteState* floor_sprite()override{call("S");return selected_sprite;}
    RenderImage* floor_image()override{call("I");return selected_image;}
    void floor_draw(SebSpriteState* s,RenderImage* i,std::int32_t frame,std::int32_t x,std::int32_t y)override{floors.push_back({s,i,frame,x,y});call("D");}
    void floor_depth_layer(std::int32_t layer)override{call("L"+std::to_string(layer));}
    SelectBackgroundResource gate_resource()override{call("R");return resource;}
    std::int32_t gate_clip()override{call("C");return 3;}
    void gate_draw(SelectBackgroundResource r,std::int32_t clip,std::int32_t frame,std::int32_t layer,float x,float y)override{gates.push_back({r,clip,frame,layer,x,y});call("T");}
};
struct FloorFixture {
    std::int32_t left=0,right=180;
    SelectSceneryState state{nullptr,nullptr,nullptr,left,right};
    FloorHost host;
};
void floor_sequence(){
    FloorFixture f;f.host.heights={500,510,520,530};
    f.host.action=[&](const auto& call){if(call=="D"){f.left=-999;f.right=-999;}};
    draw_select_floor(f.state,{-3,5},f.host);
    check(f.host.floors.size()==3,"captured ends survive callbacks");
    for(std::size_t i=0;i<3;++i){const auto& d=f.host.floors[i];
        check(d.frame==static_cast<std::int32_t>(i)&&d.x==37+static_cast<std::int32_t>(i)*32&&d.y==405+static_cast<std::int32_t>(i)*10,"start/middle/end strip frames and fresh heights");
        check(d.sprite==&f.host.sprite&&d.image==&f.host.image,"explicit sprite/image forwarded");}
    check(f.host.gates.size()==1,"gate follows floor loop");const auto& g=f.host.gates[0];
    check(g.resource==f.host.resource&&g.clip==3&&g.frame==0&&g.layer==0&&g.x==37&&g.y==465,"unanchored gate frame0/layer0 and fresh height");
    check(f.host.log==std::vector<std::string>({"G","W","G","H","S","I","D","G","W","G","H","S","I","D","G","W","G","H","S","I","D","L10","H","R","C","T"}),"floor and gate query order");
    FloorFixture equality;equality.right=176;draw_select_floor(equality.state,{0,0},equality.host);
    check(equality.host.floors.size()==2&&equality.host.floors[0].frame==0&&equality.host.floors[1].frame==1,"end equal to next does not select final frame");
    FloorFixture one;one.right=120;draw_select_floor(one.state,{0,0},one.host);
    check(one.host.floors.size()==1&&one.host.floors[0].frame==2,"short span final frame overrides initial frame");
}
void floor_limits(){
    FloorFixture empty;empty.right=112;empty.host.selected_sprite=nullptr;empty.host.selected_image=nullptr;
    draw_select_floor(empty.state,{0,0},empty.host);
    check(empty.host.floors.empty()&&empty.host.log==std::vector<std::string>({"L10","H","R","C","T"}),"empty floor loop still changes layer and draws gate");
    FloorFixture culled;draw_select_floor(culled.state,{-1000,0},culled.host);
    check(culled.host.floors.empty()&&culled.host.gates[0].x==-960,"gate has no floor culling");
    check(culled.host.log==std::vector<std::string>({"G","G","G","L10","H","R","C","T"}),"left culling skips width and draw-only providers");
    FloorFixture absent;absent.right=112;absent.host.resource=nullptr;
    fails<std::invalid_argument>([&]{draw_select_floor(absent.state,{0,0},absent.host);});
    check(absent.host.log==std::vector<std::string>({"L10","H","R"}),"missing gate resource fails after layer and raw height, before clip");
    FloorFixture captured;captured.right=112;const auto resource=captured.host.resource;
    captured.host.action=[&](const auto& call){if(call=="C")captured.host.resource=nullptr;};
    draw_select_floor(captured.state,{0,0},captured.host);
    check(captured.host.gates[0].resource==resource,"gate retains resource captured before clip query");
    FloorFixture missing;missing.right=120;missing.host.selected_sprite=nullptr;
    draw_select_floor(missing.state,{0,0},missing.host);
    check(missing.host.floors[0].sprite==nullptr&&missing.host.floors[0].image==&missing.host.image,"null Seb is forwarded only after image lookup");
    FloorFixture wrap;const auto min=std::numeric_limits<std::int32_t>::min(),max=std::numeric_limits<std::int32_t>::max();
    wrap.left=max-39;wrap.right=min+73;wrap.host.raw_height=100;
    draw_select_floor(wrap.state,{max,max},wrap.host);
    check(wrap.host.floors.size()==1&&wrap.host.floors[0].frame==2&&wrap.host.floors[0].x==-1&&wrap.host.floors[0].y==max,"floor extents/coordinates wrap as int32");
    check(wrap.host.gates[0].x==static_cast<float>(min+39)&&wrap.host.gates[0].y==static_cast<float>(min+29),"gate wraps integer additions before binary32 conversion");
}
}
int main(){fixed_image();captured_rows();floor_sequence();floor_limits();return 0;}
