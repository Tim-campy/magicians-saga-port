#include "core/gameplay/mesh_depth.hpp"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace saga::core;
namespace {
void check_at(bool value,int line){if(!value){std::cerr<<"mesh depth line "<<line<<'\n';std::abort();}}
#define check(x) check_at((x),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
std::uint32_t bits(float value){std::uint32_t result;std::memcpy(&result,&value,4);return result;}
struct Host:MeshDepthHost {
    float factor=1;bool force_live=false;std::vector<std::string> calls;std::function<void(const std::string&)> hook;
    void event(const std::string& s){calls.push_back(s);if(hook)hook(s);}
    bool surface_is_null(MeshDepthSurface* p)override{event("null");return !force_live&&!p;}
    float low_depth_scaling()override{event("scale");return factor;}
};
}
int main()try{
    Host h;MeshDepthState s;check(mesh_depth_min()==-749999&&mesh_depth_max()==750000);
    check(mesh_depth_back(s,h)==-750000&&h.calls.empty());
    begin_mesh_depth_frame(s);check(s.front==1498976&&s.current==1498976&&s.layer==0);
    set_mesh_depth_layer(s,-9);check(mesh_depth_layer(s)==-9);
    set_mesh_depth(s,100,2,h);check(s.current==749900&&s.front==1498976&&s.layer==-9);
    set_mesh_depth(s,200,0,h);check(s.current==749800&&s.front==749900);
    set_mesh_depth_front(s);check(s.current==749800);s.current=750000;set_mesh_depth_front(s);check(s.current==749900);
    begin_mesh_depth_frame(s);set_mesh_depth(s,100,1,h);check(s.current==1499900&&h.calls.empty());
    // No clamp before the wrapping subtraction; front tracks the previous current value.
    s.front=50;s.current=100;set_mesh_depth(s,800000,0,h);check(s.front==50&&s.current==-50000);
    set_mesh_depth(s,0,0,h);check(s.front==-50000&&s.current==750000);
    set_mesh_depth(s,std::numeric_limits<int>::min(),0,h);check(s.current==-2146733696.0F);
    // Low precision queries fresh surface state and current global scale.
    MeshDepthSurface surface;s.low_depth=true;s.surface=&surface;h.calls.clear();
    set_mesh_depth(s,3,0,h);check(s.current==749997&&h.calls==std::vector<std::string>{"null"});
    surface.use_low_depth_scaling=true;h.factor=0.5F;set_mesh_depth(s,3,0,h);check(s.current==749999);
    check(mesh_depth_back(s,h)==-1500000);
    s.surface=nullptr;h.hook=[&](const auto& event){if(event=="scale")h.factor=h.calls.size()<3?0.5F:2.0F;};h.calls.clear();
    set_mesh_depth(s,100,1,h);check(s.current==3749800);
    check(h.calls==std::vector<std::string>({"null","scale","null","scale"}));h.hook={};
    h.factor=std::numeric_limits<float>::quiet_NaN();check(mesh_depth_back(s,h)==0);set_mesh_depth(s,1,0,h);check(s.current==750000);
    h.factor=-0.0F;check(mesh_depth_back(s,h)==std::numeric_limits<int>::min());
    h.factor=-1e-20F;check(mesh_depth_back(s,h)==std::numeric_limits<int>::max());
    h.factor=std::numeric_limits<float>::infinity();set_mesh_depth(s,1,0,h);check(s.current==-2146733696.0F);
    // A nonnull Unity object result rereads surface after the query; failure precedes depth writes.
    h.force_live=true;s.surface=&surface;s.current=123;s.front=456;
    h.hook=[&](const auto& event){if(event=="null")s.surface=nullptr;};fails([&]{set_mesh_depth(s,1,0,h);});
    check(s.current==123&&s.front==456);h.hook={};h.force_live=false;
    h.hook=[](const auto& event){if(event=="scale")throw std::runtime_error("scale");};
    fails([&]{set_mesh_depth(s,2,0,h);});check(s.current==123&&s.front==456);h.hook={};
    // Comparison predicates skip unordered and preserve signed zero.
    s.low_depth=false;s.current=std::numeric_limits<float>::quiet_NaN();s.front=-0.0F;
    set_mesh_depth(s,0,0,h);check(bits(s.front)==0x80000000U);
    s.front=std::numeric_limits<float>::quiet_NaN();s.current=3;set_mesh_depth_front(s);check(s.current==3);
    MeshBuilderDepth builder{&s};configure_mesh_builder_depth(builder,24,false);check(builder.scale==1.5F&&builder.step==0.1575F);
    configure_mesh_builder_depth(builder,16,false);check(builder.scale==1024&&bits(builder.step)==0x42d70a3dU);
    configure_mesh_builder_depth(builder,24,true);check(builder.scale==1024&&bits(builder.step)==0x42d70a3dU);
    builder.step=0.5F;builder.total=0;s.current=100;begin_mesh_quad_depth(builder,true);
    for(int i=0;i<4;++i)check(append_mesh_vertex_depth(builder,0)==99.5F);
    check(s.current==99.5F&&builder.total==398);begin_mesh_quad_depth(builder,false);check(s.current==99.5F);
    builder.scale=1.00000011920928955078125F;s.current=-1.0000002384185791015625F;
    check(append_mesh_vertex_depth(builder,1.00000011920928955078125F)==0); // Fused multiply-add would differ.
    builder.manager=nullptr;begin_mesh_quad_depth(builder,false);fails([&]{begin_mesh_quad_depth(builder,true);});
    fails([&]{append_mesh_vertex_depth(builder,0);});
    std::cout<<"mesh depth contract ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
