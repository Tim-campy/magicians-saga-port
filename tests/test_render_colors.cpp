#include "core/gameplay/render_colors.hpp"
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
using namespace saga::core;
void check_at(bool value,int line){if(!value){std::cerr<<"render colors contract line "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Host:RenderColorHost {
    RenderColorState state;RenderColorWork work;bool dirty=false,gui=false;int alpha=2;std::vector<std::string> calls;
    std::function<void(const std::string&)> hook;
    Host(){state.cache_dirty=&dirty;}
    void step(const std::string& s){calls.push_back(s);if(hook)hook(s);}
    int image_alpha_type()override{step("alpha");return alpha;}
    bool render_gui_mode(RenderColorState&)override{step("gui");return gui;}
    void push_gui_matrix()override{step("push");}
    void load_gui_pixel_matrix()override{step("load");}
    int screen_width()override{step("width");return 300;}
    int screen_height()override{step("height");return 400;}
    void gui_viewport(float x,float y,float w,float h)override{check(x==0&&y==0&&w==300&&h==400);step("viewport");}
    void pop_gui_matrix()override{step("pop");}
    SpriteRenderPasses passes(RenderColors colors=nullptr){return get_render_passes(state,true,colors?colors:work.white,work,*this);}
};
int main(){
    {Host h;set_render_mode(h.state,1,255,1);check(h.state.destination==0&&h.state.replace&&h.dirty);
     set_render_mode(h.state,2,128,127);check(!h.state.replace);set_render_mode(h.state,0,300,-7);check(h.state.replace&&h.state.source==300&&h.state.destination==-7);}
    {Host h;push_render_mode(h.state,1,64,191);auto row=h.state.renders->at(0);pop_render_mode(h.state);check(h.state.mode==0&&h.state.source==255&&h.state.render_count==0);
     push_render_mode(h.state,2,128,127);check(h.state.renders->at(0)==row);h.state.cache_dirty=nullptr;fails([&]{pop_render_mode(h.state);});check(h.state.render_count==0&&h.state.mode==0);}
    {Host h;set_blend_mode(h.state,2,-2130771968);check(h.state.blend.r==1&&h.state.blend.g==0&&h.state.blend.b==0&&h.state.blend.a==128.0F/255.0F);
     fails([&]{set_blend_mode(h.state,4,0);});check(h.state.blend_mode==2);
     push_blend_mode(h.state,0,0);h.state.cache_dirty=nullptr;fails([&]{pop_blend_mode(h.state);});check(h.state.blend_count==1&&h.state.blend_mode==2);}
    {Host h;h.state.renders->push_back(std::make_shared<std::vector<int>>(2));fails([&]{save_render_mode(h.state);});
     check(h.state.render_count==0&&h.state.renders->at(0)->at(1)==255);
     h.state.blends->push_back(std::make_shared<std::vector<int>>(1));fails([&]{save_blend_mode(h.state);});check(h.state.blend_count==0);}
    {Host h;auto p=h.passes();check(p->size()==1&&p->at(0)->mode==-1&&p->at(0)->rgb_factor==255&&p->at(0)->alpha_factor==255);
     auto out=begin_render_pass(*p->at(0),h);check(out->at(0).r==1&&out->at(0).a==1);end_render_pass(*p->at(0),h);
     set_render_mode(h.state,0,128,0);p=h.passes();out=begin_render_pass(*p->at(0),h);check(out->at(0).r==128.0F/255.0F&&out->at(0).a==1);end_render_pass(*p->at(0),h);}
    {Host h;set_render_mode(h.state,1,80,175);auto p=h.passes();check(p->size()==1&&p->at(0)->rgb_factor==255&&p->at(0)->alpha_factor==80);
     auto out=begin_render_pass(*p->at(0),h);check(out->at(0).a==80.0F/255.0F&&h.state.render_count==1);end_render_pass(*p->at(0),h);check(h.state.render_count==0&&h.state.source==80);}
    {Host h;h.alpha=3;set_render_mode(h.state,1,40,60);auto p=h.passes();check(p->size()==4);
     check(p->at(0)->source==0&&p->at(0)->destination==60&&p->at(0)->alpha_factor==195);
     check(p->at(1)->source==40&&p->at(1)->destination==255&&p->at(1)->alpha_factor==40);
     check(p->at(0)->alpha_test==1&&p->at(1)->alpha_test==1&&p->at(2)->alpha_test==2&&p->at(3)->alpha_test==2);
     check(p->at(0)->colors==p->at(2)->colors&&p->at(0)->output==p->at(3)->output&&p->at(0)->local!=p->at(2)->local);
     auto first=begin_render_pass(*p->at(0),h);check(first->at(0).a==195.0F/255.0F);end_render_pass(*p->at(0),h);
     auto second=begin_render_pass(*p->at(1),h);check(second==first&&first->at(0).a==40.0F/255.0F);end_render_pass(*p->at(1),h);}
    {Host h;set_render_mode(h.state,1,0,300);check(h.passes()->empty());h.alpha=3;check(h.passes()->empty());
     h.state.is_gui=true;h.state.gl_render=false;set_render_mode(h.state,0,255,0);check(h.passes()->size()==1);}
    {Host h;set_blend_mode(h.state,1,-1);set_render_mode(h.state,1,0,100);auto p=h.passes();
     begin_render_pass(*p->at(0),h);check(h.state.blend_mode==0&&h.state.blend_count==1);end_render_pass(*p->at(0),h);check(h.state.blend_mode==1&&h.state.blend_count==0);}
    {Host h;set_blend_mode(h.state,2,-2130771968);auto p=h.passes();auto out=begin_render_pass(*p->at(0),h);
     check(out->at(0).r==128.0F/255.0F&&out->at(0).g==0&&out->at(0).b==0&&out->at(0).a==1);end_render_pass(*p->at(0),h);}
    {Host h;auto many=std::make_shared<std::vector<RenderColor>>(2,RenderColor{0.5F,0.25F,0.75F,0.5F});auto p=h.passes(many);
     check(p->at(0)->colors==many);many->at(0).r=1;auto out=begin_render_pass(*p->at(0),h);check(out->at(0).r==1);end_render_pass(*p->at(0),h);
     auto empty=std::make_shared<std::vector<RenderColor>>();p=h.passes(empty);check(p->at(0)->colors==h.work.empty_fallback);}
    {Host h;int material=1;h.state.forced_material=&material;auto p=h.passes();check(p->size()==1&&p->at(0)->material==&material);
     h.gui=true;h.calls.clear();begin_render_pass(*p->at(0),h);end_render_pass(*p->at(0),h);
     check(h.calls==std::vector<std::string>{"gui","push","load","width","height","viewport","gui","pop"});check(h.state.render_count==0);}
    {Host h;SpriteRenderPass p;p.mode=7;fails([&]{initialize_render_pass(p,&h.state,2,3,4,5,6,nullptr,h.work);});check(p.owner==&h.state&&p.mode==2&&p.source==3&&p.destination==4);
     initialize_render_pass(p,&h.state,-1,255,255,255,255,h.work.white,h.work);p.output->resize(2);fails([&]{begin_render_pass(p,h);});check(p.output->at(0).a==1);}
    {ImageAlphaClassifier c;c.add(1);check(c.finish(1)==0);ImageAlphaClassifier cut;cut.add(0);cut.add(1);check(cut.finish(2)==1);
     ImageAlphaClassifier partial;partial.add(0);partial.add(0.5F);check(partial.finish(2)==2);
     partial.add(1);check(partial.finish(3)==3);ImageAlphaClassifier bounds;bounds.add(0.01F);bounds.add(0.999F);check(bounds.finish(2)==2);
     ImageAlphaClassifier nan;nan.add(std::numeric_limits<float>::quiet_NaN());check(nan.finish(1)==0);}
    {Host h;SpriteShaderState shader=sprite_shader_state(h.state,true,0,1,h.work.white,-1,false);auto key=sprite_shader_key(shader);
     check(key==0x10110&&sprite_pixel_pipeline(key).blend==SpriteBlend::replace&&!sprite_pixel_pipeline(key).modulate);
     shader=sprite_shader_state(h.state,true,3,1,h.work.white,1,false);key=sprite_shader_key(shader);check(key==0x10c10&&sprite_pixel_pipeline(key).alpha_test==SpriteAlphaTest::greater_099);
     set_render_mode(h.state,2,128,127);shader=sprite_shader_state(h.state,true,2,0.5F,h.work.white,-1,false);check(sprite_pixel_pipeline(sprite_shader_key(shader)).blend==SpriteBlend::subtract_alpha);
     set_render_mode(h.state,1,20,255);shader=sprite_shader_state(h.state,true,2,0.5F,h.work.white,-1,false);check(sprite_pixel_pipeline(sprite_shader_key(shader)).blend==SpriteBlend::add);
     shader.stencil_write=true;shader.color_mask=true;shader.depth_only=true;check(sprite_shader_key(shader)==0xe0000);fails([&]{sprite_pixel_pipeline(sprite_shader_key(shader));});}
    std::cout<<"render colors contract ok\n";
}
