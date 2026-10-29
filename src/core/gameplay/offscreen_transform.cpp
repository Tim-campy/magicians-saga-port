#include "offscreen_transform.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(const std::shared_ptr<T>& p){if(!p)throw std::invalid_argument("missing offscreen matrices");return *p;}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
OffscreenMatrix make_transform(OffscreenTransforms& s,float x,float y,float depth,float sx,float sy,OffscreenTransformHost& host){
    const auto density=static_cast<float>(s.effective_density);
    const float tx=density*x,ty=(-y)*density,tz=-depth;
    return host.translation_scale(tx,ty,tz,sx,sy,1);
}
}
void set_static_offscreen_transform(OffscreenTransforms& s,OffscreenMatrix matrix){
    s.static_base=matrix; // Retained even if the required array is absent.
    auto& entries=required(s.statics);
    std::fill(entries.begin(),entries.end(),matrix);
}
void set_static_offscreen_transform(OffscreenTransforms& s,float x,float y,float depth,float sx,float sy,OffscreenTransformHost& host){
    const auto matrix=make_transform(s,x,y,depth,sx,sy,host);
    set_static_offscreen_transform(s,matrix);
}
void set_multi_static_offscreen_transform(OffscreenTransforms& s,std::int32_t id,OffscreenMatrix matrix,OffscreenTransformHost& host){
    auto entries=s.statics;
    if(static_cast<std::int64_t>(required(entries).size())<=id){
        entries=host.allocate_statics(word(static_cast<std::uint32_t>(id)+1U));
        auto& fresh=required(entries);
        std::fill(fresh.begin(),fresh.end(),s.static_base);
        const auto& previous=required(s.statics);
        if(previous.size()>fresh.size())throw std::out_of_range("offscreen matrix copy");
        std::copy(previous.begin(),previous.end(),fresh.begin());
        s.statics=entries; // Publish only after fill and copy; before indexed assignment.
    }
    required(entries).at(static_cast<std::size_t>(id))=matrix;
}
void set_multi_static_offscreen_transform(OffscreenTransforms& s,std::int32_t id,std::int32_t x,std::int32_t y,std::int32_t depth,float sx,float sy,OffscreenTransformHost& host){
    const auto density=static_cast<std::uint32_t>(s.effective_density);
    const auto tx=word(density*static_cast<std::uint32_t>(x));
    const auto ty=word(0U-density*static_cast<std::uint32_t>(y));
    const auto tz=word(0U-static_cast<std::uint32_t>(depth));
    const auto matrix=host.translation_scale(static_cast<float>(tx),static_cast<float>(ty),static_cast<float>(tz),sx,sy,1);
    set_multi_static_offscreen_transform(s,id,matrix,host);
}
void set_dynamic_offscreen_transform(OffscreenTransforms& s,OffscreenMatrix matrix){s.dynamic=matrix;}
void set_dynamic_offscreen_transform(OffscreenTransforms& s,float x,float y,float depth,float sx,float sy,OffscreenTransformHost& host){
    const auto matrix=make_transform(s,x,y,depth,sx,sy,host);
    set_dynamic_offscreen_transform(s,matrix);
}
OffscreenCameraPlacement offscreen_camera_placement(std::int32_t width,std::int32_t height,float previous_z){
    const auto x=static_cast<float>(width/2),y=static_cast<float>(height/2);
    return {y,x,y,previous_z}; // Signed division truncates toward zero; preserve the camera's Z.
}
} // namespace saga::core
