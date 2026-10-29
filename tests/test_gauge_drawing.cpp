#include "core/gameplay/gauge_drawing.hpp"
#include "core/gameplay/ecs_combat.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace saga::core;
#define CHECK(x) do{if(!(x)){std::cerr<<"failed line "<<__LINE__<<'\n';std::abort();}}while(false)
struct Graphics final:RenderGraphics {
    std::vector<RenderImageDraw> draws;std::function<void()> callback;
    RenderDepth* mesh_manager()override{return nullptr;}
    void push_render_mode(std::int32_t,std::int32_t,std::int32_t)override{std::abort();}
    void pop_render_mode()override{std::abort();}void push_blend_mode(std::int32_t,std::int32_t)override{std::abort();}
    void pop_blend_mode()override{std::abort();}void push_matrix(RenderMatrix&)override{std::abort();}void pop_matrix()override{std::abort();}
    void draw_image(const RenderImageDraw& r)override{draws.push_back(r);if(callback)callback();}
};
struct Host final:GaugeDrawingHost {
    PixelImage image;std::vector<std::string> calls;int offset=4;bool transparent=false,null_result=false;
    std::int32_t view_width=200,view_height=100;
    Host(){image.width=100;image.height=20;}
    PixelImage* gauge_image(std::int32_t id)override{CHECK(id==1);calls.push_back("image");return &image;}
    std::int32_t gauge_view_width()override{calls.push_back("width");return view_width;}
    std::int32_t gauge_view_height()override{calls.push_back("height");return view_height;}
    std::shared_ptr<std::vector<float>> gauge_alpha(PixelImage&,std::int32_t x,std::int32_t y,std::int32_t w,std::int32_t h,std::shared_ptr<std::vector<float>> a)override{
        CHECK(x==0&&y==15&&w==30&&h==2);calls.push_back("alpha");
        if(null_result)return nullptr;
        for(std::size_t i=0;i<60;++i)a->at(i)=!transparent&&i==static_cast<std::size_t>(offset)?1.0F:0.0F;
        return a;
    }
};
template<class F>void fails(F f){bool error=false;try{f();}catch(const std::exception&){error=true;}CHECK(error);}
int main(){
    CHECK(anchor_gauge(10,20,9,7,2|32).x==6&&anchor_gauge(10,20,9,7,2|32).y==17);
    CHECK(anchor_gauge(10,20,-9,-7,2|4|32|64).x==14&&anchor_gauge(10,20,-9,-7,2|32).y==23);
    CHECK(anchor_gauge(0,0,9,7,4|64).x==-9&&anchor_gauge(0,0,9,7,4|64).y==-7);
    GaugeDrawState state;Host h;Graphics g;
    draw_gauge(state,&g,10,20,1,50,50,h);
    CHECK(g.draws.size()==2&&g.draws[0].width==50&&g.draws[0].height==10&&g.draws[0].source_width==100);
    CHECK(g.draws[1].x==12&&g.draws[1].u==2&&g.draws[1].v==10&&g.draws[1].source_width==48&&g.draws[1].width==24);
    CHECK(g.draws[1].image==&h.image&&g.draws[1].scaled);
    h.transparent=true;state.alpha->at(90)=0.5F;CHECK(gauge_image_offset(state,&h.image,h)==90); // Prefix overwrite retains suffix.
    state.alpha->at(90)=0;CHECK(gauge_image_offset(state,&h.image,h)==128);
    state.alpha->at(80)=std::numeric_limits<float>::quiet_NaN();CHECK(gauge_image_offset(state,&h.image,h)==80);
    h.null_result=true;fails([&]{gauge_image_offset(state,&h.image,h);});CHECK(!state.alpha);
    state=GaugeDrawState{};h.null_result=false;h.transparent=false;h.offset=0;g.draws.clear();
    draw_gauge(state,&g,0,0,1,1,1,h);CHECK(g.draws[1].width==1); // Positive rate keeps a minimum destination pixel.
    g.draws.clear();draw_gauge(state,&g,0,0,1,0,1,h);CHECK(g.draws[1].width==0);
    g.draws.clear();g.callback=[&]{if(g.draws.size()==1)throw std::runtime_error("draw");};
    fails([&]{draw_gauge(state,&g,0,0,1,100,100,h);});CHECK(g.draws.size()==1&&state.alpha->at(0)==1);g.callback=nullptr;
    ComponentSlots e(35);CombatParameterLinks links;DamageParameter p;p.value=50;p.maximum=100;
    DamageParameterMap values{{0,&p}};links.values=&values;e.add(23,&links);
    ParameterGaugeRequest r{&e,100,30,1,0,2,false};g.draws.clear();h.calls.clear();
    draw_parameter_gauge(state,&g,r,h);CHECK(g.draws.size()==2&&g.draws[0].x==50&&g.draws[0].width==60);
    CHECK((h.calls==std::vector<std::string>{"image","width","height","image","alpha"}));
    p.extra=50;p.extra_maximum=100;g.draws.clear();draw_parameter_gauge(state,&g,r,h);
    CHECK(g.draws[0].width==70&&g.draws[1].width==35);p.extra=0;p.extra_maximum=0;
    p.value=0;h.calls.clear();g.draws.clear();draw_parameter_gauge(state,nullptr,r,h);CHECK(h.calls.empty());
    r.force=true;draw_parameter_gauge(state,&g,r,h);CHECK(g.draws.size()==2&&g.draws[1].width==0);
    r.force=false;p.value=1;r.x=251;h.calls.clear();draw_parameter_gauge(state,nullptr,r,h);
    CHECK((h.calls==std::vector<std::string>{"image","width","height"}));
    r.x=250;fails([&]{draw_parameter_gauge(state,nullptr,r,h);}); // Touching view edge passes culling, then requires graphics.
    r.parameter=1;h.calls.clear();draw_parameter_gauge(state,nullptr,r,h);CHECK(h.calls.empty());
    values[0]=nullptr;r.parameter=0;r.force=false;
    fails([&]{draw_parameter_gauge(state,nullptr,r,h);});CHECK(h.calls.empty());
    r.force=true;r.x=251;draw_parameter_gauge(state,nullptr,r,h); // Culled force path never dereferences null captured parameter.
    fails([&]{draw_parameter_gauge(state,&g,{},h);});
    struct EnemyHost final:EnemyGaugeDrawHost {
        EnemyRecord data;int count=5,projections=0;std::vector<ParameterGaugeRequest> requests;
        const EnemyRecord* gauge_enemy_record(ComponentSlots&)override{return &data;}
        std::int32_t gauge_enemy_defeats(const EnemyRecord&)override{return count;}
        GaugePoint gauge_screen_position(const ContactPosition* p)override{CHECK(!p);++projections;return {12,std::numeric_limits<std::int32_t>::max()};}
        std::int32_t enemy_gauge_palette(std::int32_t a)override{return std::array<int,5>{{1,5,6,1,1}}.at(static_cast<std::size_t>(a));}
        void parameter_gauge(const ParameterGaugeRequest& request)override{requests.push_back(request);}
    } enemy;
    const EventIntegers all{};SubsetFilter filter{&all};EntitySubset group(filter,[](const auto&){return 1;});group.add(&e);
    draw_enemy_gauges(group,enemy);CHECK(enemy.requests.empty()&&enemy.projections==0);
    enemy.count=6;enemy.data.attribute=1;draw_enemy_gauges(group,enemy);
    CHECK(enemy.requests.size()==1&&enemy.requests[0].image==5&&enemy.requests[0].parameter==0&&enemy.requests[0].anchor==2&&!enemy.requests[0].force);
    CHECK(enemy.requests[0].y==std::numeric_limits<std::int32_t>::min()+2);
    enemy.data.flags=1;draw_enemy_gauges(group,enemy);CHECK(enemy.requests.size()==1);
    enemy.data.flags=0;enemy.data.attribute=-1;fails([&]{draw_enemy_gauges(group,enemy);});CHECK(enemy.projections==2);
}
