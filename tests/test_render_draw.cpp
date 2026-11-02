#include "render_draw_fixture.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <functional>
using namespace saga::core;
void check_at(bool value,int line){if(!value){std::cerr<<"render drawing contract line "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct NumberImages final:NumberImageHost {
    SpriteArray metric=std::make_shared<SpriteValues>(SpriteValues{0,0,0,0,5,7});
    std::vector<std::array<std::int32_t,3>> queries;
    struct Draw {std::int32_t clip,frame;float x,y;};
    std::vector<Draw> draws;std::vector<std::int32_t> attempted_frames;
    std::function<void()> on_metric;std::function<void(std::int32_t)> on_draw;
    SpriteArray number_sprite(std::int32_t clip,std::int32_t frame,std::int32_t layer)override{
        queries.push_back({clip,frame,layer});if(on_metric)on_metric();return metric;
    }
    void draw_number_frame(std::int32_t clip,std::int32_t frame,float x,float y)override{
        attempted_frames.push_back(frame);if(on_draw)on_draw(frame);draws.push_back({clip,frame,x,y});
    }
};
struct NumberValues final:NumberValueImageHost {
    SpriteArray metric=std::make_shared<SpriteValues>(SpriteValues{0,0,0,0,5,7});
    std::vector<std::string> calls;std::vector<std::array<std::int32_t,3>> queries,colors;
    std::vector<NumberImages::Draw> draws;std::vector<std::array<float,2>> minus;
    std::int32_t maximum=46;float scale=0;bool matrix=false;
    std::function<void(std::int32_t)> on_draw;std::function<void()> on_maximum,on_minus;
    SpriteArray number_sprite(std::int32_t clip,std::int32_t frame,std::int32_t layer)override{
        calls.push_back("metric");queries.push_back({clip,frame,layer});return metric;
    }
    void draw_number_frame(std::int32_t clip,std::int32_t frame,float x,float y)override{
        calls.push_back("draw");if(on_draw)on_draw(frame);draws.push_back({clip,frame,x,y});
    }
    std::int32_t number_max_frame(std::int32_t clip)override{calls.push_back("maximum");check(clip==58);if(on_maximum)on_maximum();return maximum;}
    void push_number_scale(float value)override{calls.push_back("push");scale=value;matrix=true;}
    void pop_number_scale()override{calls.push_back("pop");matrix=false;}
    void number_color(std::int32_t r,std::int32_t g,std::int32_t b)override{calls.push_back("color");colors.push_back({r,g,b});}
    void number_minus_text(float x,float y)override{calls.push_back("minus");if(on_minus)on_minus();minus.push_back({x,y});}
};
int main(){
    {RenderDrawServices h;h.add();h.draw();check(h.state.phase==1&&h.queue.counts->at(0)==1);
     check(h.calls==std::vector<std::string>{"mesh","depth","resource","sprite","draw"});
     const auto& d=h.draws.at(0);check(d.image==&h.first_image&&d.u==3&&d.v==4&&d.source_width==10&&d.source_height==20);
     check(d.x==48&&d.y==55&&d.width==10&&d.height==20&&!d.scaled&&h.depths[0]==std::make_pair(0,123));
     h.draw();check(h.state.phase==2&&h.draws.size()==2);}
    {RenderDrawServices h;auto r=h.add(300007);r->at(render_width)=20;r->at(render_height)=40;r->at(render_u)=8;r->at(render_v)=9;
     r->at(render_alpha)=1200;r->at(render_color)=33;r->at(render_angle)=45;h.supplied->at(8)=1;h.supplied->at(9)=1;h.draw();
     check(h.calls==std::vector<std::string>{"mesh","depth","resource","sprite","render+","blend+","rotate","matrix+","draw","reset","matrix-","blend-","render-"});
     check(h.depths[0]==std::make_pair(2,7)&&h.render_modes[0]==std::make_tuple(1,200,55)&&h.blends[0]==std::make_pair(2,33));
     check(h.rotations[0]==std::array<float,3>{45,2,5}&&h.reversals[0]==3&&h.reverse==0);
     check(h.draws[0].scaled&&h.draws[0].width==20&&h.draws[0].source_width==10&&h.draws[0].u==8);}
    {RenderDrawServices h;auto r=h.add();r->at(render_angle)=30;r->at(render_skew)=99;h.draw();
     check(h.tangent_angles==std::vector<float>{30}&&h.matrices[0]==std::array<float,9>{1,0.25F,0,0,1,0,0,0,1});
     check(h.translations[0]==std::make_pair(0.0F,-1.25F)&&h.rotations.empty()&&h.resets==1&&h.matrix_pops==1);}
    for(auto alpha:{-1000,-999,-1,0,254,255,999,1000,1999,2000,2999,3000,3999,4000,std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max()}){
        RenderDrawServices h;auto r=h.add();r->at(render_alpha)=alpha;h.draw();check(h.render_pops==(alpha==255?0:1));
        if(alpha>=-999&&alpha<=999&&alpha!=255)check(h.render_modes==std::vector<std::tuple<int,int,int>>{{alpha/1000,alpha%1000,0}});
        else if(alpha>=1000&&alpha<=2999)check(h.render_modes==std::vector<std::tuple<int,int,int>>{{alpha/1000,alpha%1000,255-alpha%1000}});
        else if(alpha>=3000&&alpha<=3999)check(h.render_modes==std::vector<std::tuple<int,int,int>>{{1,255,alpha%1000}});
        else check(h.render_modes.empty());
    }
    for(int id=1;id<=23;++id){RenderDrawServices h;auto r=h.add();r->at(render_resource)=id;r->at(render_texture)=1;h.draw();
        const bool selected=id==1||id==2||id==3||id==7||(id>=18&&id<=23);
        check(h.draws[0].image==(selected?&h.second_image:&h.first_image));}
    {RenderDrawServices h;auto r=h.add();r->at(render_texture)=1;r->at(render_image_resource)=7;h.draw();
     check(h.draws[0].image==&h.first_image&&std::count(h.calls.begin(),h.calls.end(),"global")==1);}
    {RenderDrawServices h;auto r=h.add();r->at(render_texture)=99;h.draw();check(h.invalid==std::vector<std::array<int,4>>{{1,99,0,0}});
     check(h.missing==std::vector<std::pair<int,int>>{{1,0}}&&!h.draws[0].image);}
    {RenderDrawServices h;auto r=h.add();r->at(render_texture)=-2;r->at(render_image_resource)=7;h.global_provider=[](int){return nullptr;};h.draw();
     check(h.invalid.size()==1&&h.missing.size()==1&&!h.draws[0].image);}
    {RenderDrawServices h;auto r=h.add();r->at(render_texture)=0;r->at(render_image_resource)=7;h.global_provider=[](int){return nullptr;};
     fails([&]{h.draw();});check(h.invalid.empty()&&h.draws.empty());}
    {RenderDrawServices h;h.add(-1);h.add(150000);h.resource_provider=[](int)->RenderDrawServices::Resource*{return nullptr;};h.draw();
     check(h.depths==std::vector<std::pair<int,int>>{{0,0},{1,0}}&&h.draws.empty());}
    {RenderDrawServices h;h.add();h.supplied->at(1)=-1;h.draw();check(h.draws.empty()&&h.depths.size()==1);}
    {RenderDrawServices h;h.add();h.supplied.reset();h.draw();check(h.draws.empty());}
    {RenderDrawServices h;h.add();h.supplied->at(1)=-8;h.supplied->at(8)=1;fails([&]{h.draw();});check(h.reverse==1&&h.draws.empty());}
    {RenderDrawServices h;h.add();h.reverse=7;h.draw();check(h.reversals[0]==7&&h.reverse==7);}
    {RenderDrawServices h;auto r=h.add();r->at(render_angle)=1;r->at(render_color)=2;r->at(render_alpha)=1000;h.supplied->at(8)=1;
     h.hook=[&](const std::string& call){if(call=="draw"){r->at(render_angle)=0;r->at(render_color)=-1;r->at(render_alpha)=255;h.supplied->at(8)=0;}};
     h.draw();check(h.render_pops==0&&h.blend_pops==0&&h.matrix_pops==0&&h.resets==0&&h.reverse==1);}
    {RenderDrawServices h;auto r=h.add();r->at(render_angle)=1;r->at(render_color)=2;r->at(render_alpha)=1000;h.supplied->at(8)=1;
     h.hook=[](const std::string& call){if(call=="draw")throw std::runtime_error("drawing failure");};fails([&]{h.draw();});
     check(h.reverse==1&&h.render_pops==0&&h.blend_pops==0&&h.matrix_pops==0&&h.calls.back()=="draw");}
    {RenderDrawServices h;h.add();h.add();h.hook=[&](const std::string& call){if(call=="draw")clear_render_queue(h.queue);};h.draw();check(h.draws.size()==1);}
    {RenderDrawServices h;h.add();h.add();bool added=false;h.hook=[&](const std::string& call){if(call=="draw"&&!added){added=true;h.add();}};h.draw();check(h.draws.size()==3);}
    {RenderDrawServices h;h.add(123,1);h.draw();check(h.draws.empty());h.draw(1);check(h.draws.size()==1&&h.state.phase==2);}
    {RenderDrawServices h;h.missing_mesh=true;h.draw();check(h.state.phase==1);h.add();fails([&]{h.draw();});check(h.state.phase==2);}
    {RenderDrawServices h;h.state.phase=std::numeric_limits<std::int32_t>::max();fails([&]{draw_render_queue(h.state,nullptr,0,h);});check(h.state.phase==-8);}
    {RenderDrawServices h;auto r=h.add();h.queue.buffers->at(0)->at(0)=std::make_shared<EventIntegers>(r->begin(),r->begin()+17);h.draw();check(h.draws.size()==1);}
    {RenderDrawServices h;auto r=h.add();r->at(render_angle)=5;h.state.matrix=nullptr;fails([&]{h.draw();});check(h.pushed_matrices.empty());}
    {NumberImages h;const std::u16string text=u"1 /2";check(draw_number_images(h,52,&text,10,20,1,17)==23);
     check(h.queries==std::vector<std::array<std::int32_t,3>>{{52,0,0}}&&h.draws.size()==3);
     check(h.attempted_frames==std::vector<std::int32_t>({1,10,2}));
     check(h.draws[0].x==10&&h.draws[1].x==23&&h.draws[2].x==28);
     for(const auto& d:h.draws)check(d.clip==52&&d.y==20);}
    for(const auto horizontal:{0,2,4,6})for(const auto vertical:{0,32,64,96}){
        NumberImages h;const std::u16string text=u"12";const auto anchor=horizontal|vertical;
        check(draw_number_images(h,19,&text,100,200,2,anchor)==12);
        const auto dx=(horizontal&2)?6:(horizontal&4)?12:0,dy=(vertical&32)?3:(vertical&64)?7:0;
        check(h.draws.size()==2&&h.draws[0].x==100-dx&&h.draws[1].x==107-dx&&h.draws[0].y==200-dy);
    }
    {NumberImages h;const std::u16string text;check(draw_number_images(h,52,&text,0,0,3,68)==-3);
     check(h.queries.size()==1&&h.draws.empty());}
    {NumberImages h;const std::u16string text=u"   ";check(draw_number_images(h,52,&text,0,0,0,17)==15);
     check(h.attempted_frames.empty());}
    {NumberImages h;fails([&]{draw_number_images(h,52,nullptr,0,0,0,17);});check(h.queries.size()==1&&h.draws.empty());}
    {NumberImages h;h.metric.reset();const std::u16string text=u"1";
     fails([&]{draw_number_images(h,52,&text,0,0,0,17);});check(h.queries.size()==1&&h.draws.empty());}
    {NumberImages h;h.metric->resize(5);const std::u16string text=u"";
     fails([&]{draw_number_images(h,52,&text,0,0,0,17);});check(h.queries.size()==1&&h.draws.empty());}
    {NumberImages h;const std::u16string text=u"0-9";draw_number_images(h,52,&text,0,0,0,17);
     check(h.attempted_frames==std::vector<std::int32_t>({0,-3,9}));}
    {NumberImages h;const std::u16string text{char16_t{0xd83d},char16_t{0xde00}};draw_number_images(h,52,&text,0,0,0,17);
     check(h.attempted_frames==std::vector<std::int32_t>({55309,56784}));}
    {NumberImages h;const std::u16string text=u"12";h.on_draw=[&](std::int32_t){h.metric->at(4)=99;h.metric->at(5)=100;};
     check(draw_number_images(h,52,&text,10,20,1,68)==11);check(h.draws[0].x==-1&&h.draws[1].x==5);
     check(h.draws[0].y==13&&h.draws[1].y==13);}
    {NumberImages h;const std::u16string text=u"123";h.on_draw=[](std::int32_t frame){if(frame==2)throw std::runtime_error("fresh font resource missing");};
     fails([&]{draw_number_images(h,52,&text,0,0,0,17);});check(h.draws.size()==1&&h.draws[0].frame==1);
     check(h.attempted_frames==std::vector<std::int32_t>({1,2}));}
    {NumberImages h;const std::u16string text=u"12";h.metric->at(4)=std::numeric_limits<std::int32_t>::max();
     check(draw_number_images(h,52,&text,std::numeric_limits<std::int32_t>::max(),std::numeric_limits<std::int32_t>::max(),1,17)==-1);
     check(h.draws[0].x==2147483648.0F&&h.draws[1].x==-1&&h.draws[0].y==2147483648.0F);}
    {NumberImages h;const std::u16string text=u"12";h.metric->at(4)=1;h.metric->at(5)=0;
     draw_number_images(h,52,&text,16777217,16777217,0,17);
     check(h.draws[0].x==16777216.0F&&h.draws[1].x==16777218.0F&&h.draws[0].y==16777216.0F&&h.draws[1].y==16777216.0F);}
    {NumberImages h;const std::u16string text=u"12";h.metric->at(4)=-5;h.metric->at(5)=-7;
     check(draw_number_images(h,52,&text,0,0,-2,34)==-12);check(h.draws[0].x==6&&h.draws[1].x==-1&&h.draws[0].y==3);}
    {NumberValues h;const std::u16string empty;
     check(draw_number_value_images(h,58,&empty,1234,&empty,100,200,0,2,100,-1,-1)==20);
     check(h.queries==std::vector<std::array<std::int32_t,3>>{{58,0,0}}&&h.scale==1.0F&&!h.matrix&&h.draws.size()==4);
     check(h.calls==std::vector<std::string>({"metric","push","draw","draw","draw","draw","pop"}));
     for(int i=0;i<4;++i)check(h.draws[i].clip==58&&h.draws[i].frame==i+1&&h.draws[i].x==90+5*i&&h.draws[i].y==200);}
    {NumberValues h;const std::u16string header=u"1 ",footer=u"/2";
     check(draw_number_value_images(h,58,&header,30,&footer,100,200,1,17,100,-1,-1)==35);
     check(h.draws.size()==5);const std::array<int,5> frames{1,3,0,-1,2},xs{100,112,118,124,130};
     for(int i=0;i<5;++i)check(h.draws[i].frame==frames[i]&&h.draws[i].x==xs[i]);}
    {NumberValues h;const std::u16string empty;
     check(draw_number_value_images(h,58,&empty,-12,&empty,100,200,0,17,100,-1,-1)==15);
     check(h.calls==std::vector<std::string>({"metric","push","maximum","draw","draw","draw","pop"}));
     check(h.draws[0].frame==45&&h.draws[0].x==100&&h.draws[1].frame==1&&h.draws[2].frame==2&&h.minus.empty());}
    {NumberValues h;h.maximum=45;const std::u16string empty;
     check(draw_number_value_images(h,58,&empty,-1,&empty,100,200,0,17,100,-1,-1)==10);
     check(h.calls==std::vector<std::string>({"metric","push","maximum","color","minus","color","minus","draw","pop"}));
     check(h.colors==std::vector<std::array<std::int32_t,3>>({{255,255,255},{0,0,0}}));
     check(h.minus==std::vector<std::array<float,2>>({{101,201},{100,200}})&&h.draws[0].frame==1&&h.draws[0].x==105);}
    {NumberValues h;const std::u16string empty;check(draw_number_value_images(h,58,&empty,0,&empty,0,0,0,17,100,-1,-1)==5);
     check(h.draws.size()==1&&h.draws[0].frame==0);}
    {NumberValues h;const std::u16string empty;
     check(draw_number_value_images(h,58,&empty,std::numeric_limits<std::int64_t>::max(),&empty,0,0,0,17,100,-1,-1)==95);
     const std::array<int,19> digits{9,2,2,3,3,7,2,0,3,6,8,5,4,7,7,5,8,0,7};
     check(h.draws.size()==digits.size());for(std::size_t i=0;i<digits.size();++i)check(h.draws[i].frame==digits[i]);}
    {NumberValues h;const std::u16string empty;
     check(draw_number_value_images(h,58,&empty,std::numeric_limits<std::int64_t>::min(),&empty,0,0,0,17,100,-1,-1)==10);
     check(h.draws.size()==2&&h.draws[0].frame==45&&h.draws[1].frame==0);}
    {NumberValues h;h.metric.reset();const std::u16string empty;
     check(draw_number_value_images(h,58,&empty,12,&empty,100,200,0,34,50,3,9)==2);
     check(h.scale==0.5F&&h.draws[0].x==99&&h.draws[1].x==100&&h.draws[0].y==198);}
    {NumberValues h;h.metric.reset();const std::u16string empty;
     fails([&]{draw_number_value_images(h,58,&empty,1,&empty,0,0,0,17,100,-1,5);});check(h.calls==std::vector<std::string>({"metric"}));}
    {NumberValues h;h.metric->resize(5);const std::u16string empty;
     fails([&]{draw_number_value_images(h,58,&empty,1,&empty,0,0,0,17,100,5,-1);});check(h.calls==std::vector<std::string>({"metric"}));}
    {NumberValues h;const std::u16string empty;
     fails([&]{draw_number_value_images(h,58,nullptr,1,&empty,0,0,0,17,100,-1,-1);});check(h.calls==std::vector<std::string>({"metric"}));}
    {NumberValues h;const std::u16string empty;
     fails([&]{draw_number_value_images(h,58,&empty,1,nullptr,0,0,0,17,100,-1,-1);});check(!h.matrix&&h.draws.empty());}
    {NumberValues h;const std::u16string empty;h.on_draw=[](std::int32_t frame){if(frame==2)throw std::runtime_error("number draw failed");};
     fails([&]{draw_number_value_images(h,58,&empty,123,&empty,0,0,0,17,100,-1,-1);});
     check(h.matrix&&h.draws.size()==1&&h.draws[0].frame==1&&h.calls.back()=="draw");}
    {NumberValues h;const std::u16string empty;h.maximum=45;h.on_minus=[](){throw std::runtime_error("font output missing");};
     fails([&]{draw_number_value_images(h,58,&empty,-1,&empty,0,0,0,17,100,-1,-1);});
     check(h.matrix&&h.draws.empty()&&h.colors.size()==1&&h.colors[0]==std::array<std::int32_t,3>{255,255,255});}
    {NumberValues h;const std::u16string empty;
     check(draw_number_value_images(h,58,&empty,10,&empty,100,200,-7,4,100,-1,-1)==4294967299LL);
     check(h.draws[0].x==97&&h.draws[1].x==95);}
    {NumberValues h;const std::u16string empty;h.metric->at(4)=std::numeric_limits<std::int32_t>::max();
     check(draw_number_value_images(h,58,&empty,10,&empty,0,0,std::numeric_limits<std::int32_t>::min(),2,2,-1,-1)==2147483648LL);
     check(h.draws[0].x==1073741824.0F&&h.draws[1].x==-1073741824.0F);}
    for(const auto anchor:{0,2,4,6,32,64,96,34,68}){
        NumberValues h;const std::u16string empty;
        check(draw_number_value_images(h,58,&empty,12,&empty,100,200,1,anchor,150,-1,-1)==15);
        const auto dx=(anchor&2)?7:(anchor&4)?15:0,dy=(anchor&32)?5:(anchor&64)?10:0;
        check(h.draws[0].x==100-dx&&h.draws[1].x==108-dx&&h.draws[0].y==200-dy&&h.scale==1.5F);
    }
    {NumberValues h;const std::u16string empty;
     draw_number_value_images(h,58,&empty,12,&empty,16777217,16777217,0,17,100,1,0);
     check(h.draws[0].x==16777216.0F&&h.draws[1].x==16777218.0F&&h.draws[0].y==16777216.0F&&h.draws[1].y==16777216.0F);}
}
