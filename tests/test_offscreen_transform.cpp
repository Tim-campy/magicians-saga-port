#include "core/gameplay/offscreen_transform.hpp"
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace saga::core;
namespace {
void check_at(bool value,int line){if(!value){std::cerr<<"offscreen transform line "<<line<<'\n';std::abort();}}
#define check(x) check_at((x),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
std::uint32_t bits(float v){std::uint32_t result;std::memcpy(&result,&v,4);return result;}
OffscreenMatrix marker(float value){OffscreenMatrix m;m.values.fill(value);return m;}
bool equal(const OffscreenMatrix& a,const OffscreenMatrix& b){return a.values==b.values;}
struct Host:OffscreenTransformHost {
    std::array<float,6> args{};int transforms=0,allocations=0;std::int32_t last_count=0;
    std::function<void()> transform_hook,allocation_hook;
    OffscreenMatrix translation_scale(float x,float y,float z,float sx,float sy,float sz)override{
        args={x,y,z,sx,sy,sz};++transforms;if(transform_hook)transform_hook();
        // Public identity-rotation TRS fixture; no Unity implementation is copied.
        OffscreenMatrix m;m.values[0]=sx;m.values[5]=sy;m.values[10]=sz;m.values[15]=1;
        m.values[12]=x;m.values[13]=y;m.values[14]=z;return m;
    }
    OffscreenMatrices allocate_statics(std::int32_t count)override{
        ++allocations;last_count=count;if(allocation_hook)allocation_hook();
        if(count<0||count>100)throw std::length_error("fixture allocation limit");
        return std::make_shared<std::vector<OffscreenMatrix>>(static_cast<std::size_t>(count));
    }
};
OffscreenTransforms surface(){OffscreenTransforms s;s.effective_density=3;s.statics=std::make_shared<std::vector<OffscreenMatrix>>(2);return s;}
}
int main()try{
    Host h;
    // Static transform is density-scaled translation, negative Y/depth and identity rotation.
    {auto s=surface();const auto array=s.statics;set_static_offscreen_transform(s,2.5F,-4,7,2,0.5F,h);
        check((h.args==std::array<float,6>{7.5F,12,-7,2,0.5F,1}));check(s.statics==array);
        check(equal(s.static_base,array->at(0))&&equal(s.static_base,array->at(1)));
        check(s.static_base.values[14]==-7&&s.dynamic.values[15]==0);
        // Value overload snapshots its input even when that input aliases the destination array.
        array->at(1)=marker(9);set_static_offscreen_transform(s,array->at(1));
        check(equal(s.static_base,marker(9))&&equal(array->at(0),marker(9))&&equal(array->at(1),marker(9)));}
    // Dynamic transforms leave the static base, shared storage and entries untouched.
    {auto s=surface();set_static_offscreen_transform(s,marker(11));const auto array=s.statics;
        set_dynamic_offscreen_transform(s,-2,5,-8,-1,4,h);
        check((h.args==std::array<float,6>{-6,-15,8,-1,4,1}));
        check(equal(s.static_base,marker(11))&&equal(array->at(0),marker(11))&&s.statics==array);
        set_dynamic_offscreen_transform(s,marker(12));check(equal(s.dynamic,marker(12)));}
    // In-range updates retain array identity. Growth fills gaps from base and preserves overrides.
    {auto s=surface();set_static_offscreen_transform(s,marker(1));const auto old=s.statics;
        set_multi_static_offscreen_transform(s,1,marker(2),h);check(s.statics==old);
        const auto count=h.allocations;set_multi_static_offscreen_transform(s,4,marker(3),h);
        check(h.allocations==count+1&&h.last_count==5&&s.statics!=old&&s.statics->size()==5);
        check(equal(s.statics->at(0),marker(1))&&equal(s.statics->at(1),marker(2)));
        check(equal(s.statics->at(2),marker(1))&&equal(s.statics->at(3),marker(1))&&equal(s.statics->at(4),marker(3)));
        check(old->size()==2&&equal(old->at(1),marker(2))&&equal(s.static_base,marker(1)));
        set_static_offscreen_transform(s,marker(4));for(const auto& m:*s.statics)check(equal(m,marker(4)));}
    // Empty storage grows too; negative IDs fail without allocating or changing the base.
    {auto s=surface();s.statics->clear();set_static_offscreen_transform(s,marker(5));
        set_multi_static_offscreen_transform(s,2,marker(6),h);
        check(s.statics->size()==3&&equal(s.statics->at(1),marker(5)));
        const auto array=s.statics;const auto allocations=h.allocations;
        fails([&]{set_multi_static_offscreen_transform(s,-1,marker(7),h);});
        check(s.statics==array&&h.allocations==allocations&&equal(s.static_base,marker(5)));
        fails([&]{set_multi_static_offscreen_transform(s,std::numeric_limits<std::int32_t>::max(),marker(7),h);});
        check(h.last_count==std::numeric_limits<std::int32_t>::min()&&s.statics==array);}
    // Integer overload wraps before conversion; floating overload converts density first.
    {auto s=surface();s.effective_density=16777217;
        set_multi_static_offscreen_transform(s,0,3,-3,4,1,1,h);
        check(h.args[0]==50331652.0F&&h.args[1]==50331652.0F&&h.args[2]==-4);
        set_dynamic_offscreen_transform(s,3,-3,4,1,1,h);check(h.args[0]==50331648.0F&&h.args[1]==50331648.0F);
        s.effective_density=3;
        set_multi_static_offscreen_transform(s,0,std::numeric_limits<std::int32_t>::max(),std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::min(),1,1,h);
        check(h.args[0]==2147483648.0F&&h.args[1]==-2147483648.0F&&h.args[2]==-2147483648.0F);}
    // Floating signed zero/nonfinite inputs reach the public math boundary unchanged by clamping.
    {auto s=surface();set_dynamic_offscreen_transform(s,-0.0F,0,0,1,1,h);
        check(bits(h.args[0])==0x80000000U&&bits(h.args[1])==0x80000000U&&bits(h.args[2])==0x80000000U);
        set_multi_static_offscreen_transform(s,0,0,0,0,1,1,h);
        check(bits(h.args[0])==0&&bits(h.args[1])==0&&bits(h.args[2])==0);
        set_dynamic_offscreen_transform(s,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN(),-std::numeric_limits<float>::infinity(),1,1,h);
        check(std::isinf(h.args[0])&&std::isnan(h.args[1])&&std::isinf(h.args[2])&&!std::signbit(h.args[2]));}
    // Preserve partial writes and allocator failure boundaries.
    {OffscreenTransforms s;fails([&]{set_static_offscreen_transform(s,marker(10));});check(equal(s.static_base,marker(10)));
        const auto calls=h.transforms;fails([&]{set_multi_static_offscreen_transform(s,0,1,2,3,1,1,h);});
        check(h.transforms==calls+1&&!s.statics); // TRS happens before the missing-array fault.
        s=surface();const auto old=s.statics;h.allocation_hook=[](){throw std::bad_alloc();};
        fails([&]{set_multi_static_offscreen_transform(s,2,marker(3),h);});check(s.statics==old);h.allocation_hook={};
        s.static_base=marker(7);s.dynamic=marker(8);h.transform_hook=[](){throw std::runtime_error("math");};
        fails([&]{set_static_offscreen_transform(s,1,2,3,1,1,h);});
        fails([&]{set_dynamic_offscreen_transform(s,1,2,3,1,1,h);});
        check(equal(s.static_base,marker(7))&&equal(s.dynamic,marker(8)));h.transform_hook={};}
    // Camera sizing truncates signed integer halves, and never substitutes a Z default.
    {auto p=offscreen_camera_placement(481,721,-1);check(p.half_height==360&&p.x==240&&p.y==360&&p.z==-1);
        p=offscreen_camera_placement(-481,-721,-10);check(p.half_height==-360&&p.x==-240&&p.y==-360&&p.z==-10);
        p=offscreen_camera_placement(1,-1,-0.0F);check(p.x==0&&p.y==0&&bits(p.z)==0x80000000U);
        p=offscreen_camera_placement(std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max(),19);
        check(p.x==-1073741824.0F&&p.y==1073741824.0F&&p.z==19);}
    std::cout<<"offscreen transform contract ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
