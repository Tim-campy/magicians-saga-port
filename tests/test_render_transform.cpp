#include "core/gameplay/render_transform.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
using namespace saga::core;
void check_at(bool value,int line){if(!value){std::cerr<<"render transform contract line "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Host:RenderTransformHost {
    std::function<void()> underflow_hook;int underflows=0,begins=0,ends=0;float x=0,y=0;
    void matrix_underflow()override{++underflows;if(underflow_hook)underflow_hook();}
    void begin_local_coordinates(float a,float b)override{++begins;x=a;y=b;}
    void end_local_coordinates()override{++ends;}
};
int main(){
    {RenderTransformState s;Host h;float x=4,y=5;check(!begin_render_matrix(s,x,y,h)&&x==4&&y==5&&s.flag_count==0);end_render_matrix(s,h);}
    {RenderTransformState s;Host h;SpriteMatrix a,b;set_scale_matrix(a,2,3,0,0);set_translate_matrix(b,5,7);
     push_graphics_matrix(s,&a);auto entries=s.local;auto pair=entries->at(0);check(s.local_index==0&&pair->size()==2);
     push_graphics_matrix(s,&b);check(s.local!=entries&&s.local->at(0)==pair&&s.local_index==1);
     float x=1,y=2;map_matrix_point(*s.screen,x,y);check(x==12&&y==27); // Prior scale * new translation.
     pop_graphics_matrix(s,h);x=1;y=2;map_matrix_point(*s.screen,x,y);check(x==2&&y==6);
     pop_graphics_matrix(s,h);check(s.local_index==-1&&h.underflows==0);auto retained=s.local;push_graphics_matrix(s,&b);check(s.local==retained);}
    {RenderTransformState s;Host h;SpriteMatrix scale;set_scale_matrix(scale,-1,1,2,0);push_graphics_matrix(s,&scale);
     float x=10,y=20;check(begin_render_matrix(s,x,y,h)&&x==0&&y==0&&s.saved_count==1&&s.flag_count==1);
     float a=1,b=2;map_matrix_point(*s.screen,a,b);check(a==13&&b==22);
     float zero_x=0,zero_y=-0.0F;begin_render_matrix(s,zero_x,zero_y,h);check(s.flag_count==2&&s.saved_count==1);
     end_render_matrix(s,h);check(s.saved_count==1&&s.flag_count==1);end_render_matrix(s,h);check(s.saved_count==0&&s.flag_count==0);
     a=1;b=2;map_matrix_point(*s.screen,a,b);check(a==3&&b==2);}
    {RenderTransformState s;Host h;push_graphics_matrix(s,nullptr);float x=0,y=0;begin_render_matrix(s,x,y,h);end_render_matrix(s,h);
     check(s.saved_count==0&&s.flag_count==1); // Native End's saved_count gate leaves this flag outstanding.
     x=2;y=3;begin_render_matrix(s,x,y,h);end_render_matrix(s,h);check(s.flag_count==1&&s.saved_count==0);}
    {RenderTransformState s;Host h;s.local.reset();fails([&]{push_graphics_matrix(s,nullptr);});check(s.local_index==0);
     s.local_index=-1;s.debug=true;h.underflow_hook=[](){throw std::runtime_error("diagnostic");};s.screen->values->at(0)=88;
     fails([&]{pop_graphics_matrix(s,h);});check(s.local_index==-1&&h.underflows==1&&s.screen->values->at(0)==88);}
    {RenderTransformState s;Host h;push_graphics_matrix(s,nullptr);float x=std::numeric_limits<float>::quiet_NaN(),y=2;
     fails([&]{begin_render_matrix(s,x,y,h);});check(s.saved_count==1&&s.flag_count==1&&y==2);
     end_render_matrix(s,h);check(s.saved_count==0&&s.flag_count==0);}
    {RenderTransformState s;Host h;s.local_coordinate_count=1;float x=4,y=5;begin_render_matrix(s,x,y,h);
     check(h.begins==1&&h.x==4&&h.y==5&&x==0&&y==0);end_render_matrix(s,h);check(h.ends==1&&s.saved_count==0);}
    std::cout<<"render transform contract ok\n";
}
