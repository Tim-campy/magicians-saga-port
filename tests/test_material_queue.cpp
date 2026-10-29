#include "core/gameplay/material_queue.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace saga::core;
namespace {
void check(bool v){if(!v)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Host:MaterialQueueHost {
    std::function<void()> hook;
    std::int32_t shader_queue()override{if(hook)hook();return 2450;}
};
}
int main(){
    {GraphicsQueueState s;s.value=7;push_graphics_queue(s,12);push_graphics_queue(s,-4);
     check(s.count==2&&s.saved->at(0)==7&&s.saved->at(1)==12);pop_graphics_queue(s);check(s.value==12);
     pop_graphics_queue(s);check(s.value==7&&s.count==0);pop_graphics_queue(s);check(s.count==0);
     push_graphics_queue(s,9);check(s.saved->size()==2);pop_graphics_queue(s);check(s.value==7);}
    {GraphicsQueueState s;s.value=5;s.count=2;
     fails([&]{push_graphics_queue(s,99);});check(s.value==5&&s.count==2&&s.saved->size()==1&&s.saved->at(0)==0);
     s.saved.reset();fails([&]{pop_graphics_queue(s);});check(s.count==2&&s.value==5);}
    {GraphicsQueueState s;s.count=std::numeric_limits<std::int32_t>::min();
     fails([&]{pop_graphics_queue(s);});check(s.count==std::numeric_limits<std::int32_t>::min());}
    check(add_queue_offset(std::numeric_limits<std::int32_t>::max(),1)==std::numeric_limits<std::int32_t>::min());
    {Host h;MaterialQueueState s;set_material_queue_offset(s,3,h);check(s.offset==3&&s.absolute==2453);
     h.hook=[&]{s.offset=-1;};set_material_queue_offset(s,8,h);check(s.absolute==2449);
     h.hook=[]{throw std::runtime_error("shader absent");};fails([&]{set_material_queue_offset(s,12,h);});
     check(s.offset==12&&s.absolute==2449);}
    check(sprite_shader_queue(0x10110)==2000&&sprite_shader_queue(0x10410)==2450&&sprite_shader_queue(0x10210)==3000);
    check(sprite_shader_queue(0x10c10)==2450&&sprite_shader_queue(0x11210)==3000);
    fails([]{sprite_shader_queue(0xe0000);});
    std::cout<<"material queue contract ok\n";
}
