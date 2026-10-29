#include "mesh_depth.hpp"
#include "entity_visual.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing mesh depth input");return *p;}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
bool scaled(MeshDepthState& s,MeshDepthHost& host){
    if(!s.low_depth)return false;
    auto* surface=s.surface;if(host.surface_is_null(surface))return true;
    return required(s.surface).use_low_depth_scaling; // Fresh field after the equality query.
}
}
std::int32_t mesh_depth_back(MeshDepthState& s,MeshDepthHost& host){
    if(!scaled(s,host))return -750000;
    const float value=-750000.0F/host.low_depth_scaling();return factory_cell_coordinate(value,1);
}
void set_mesh_depth(MeshDepthState& s,std::int32_t depth,std::int32_t relative,MeshDepthHost& host){
    if((static_cast<std::uint32_t>(relative)&1U)!=0)depth=word(static_cast<std::uint32_t>(mesh_depth_back(s,host))+static_cast<std::uint32_t>(depth));
    if(scaled(s,host)){
        const auto factor=host.low_depth_scaling();const float value=factor*static_cast<float>(depth);depth=factory_cell_coordinate(value,1);
    }
    if(s.current<s.front)s.front=s.current; // The previous current value, before the new subtraction.
    s.current=static_cast<float>(word(750000U-static_cast<std::uint32_t>(depth)));
}
void set_mesh_depth_layer(MeshDepthState& s,std::int32_t value){s.layer=value;}
std::int32_t mesh_depth_layer(const MeshDepthState& s){return s.layer;}
std::int32_t mesh_depth_min(){return -749999;}
std::int32_t mesh_depth_max(){return 750000;}
void set_mesh_depth_front(MeshDepthState& s){if(s.front<s.current)s.current=s.front;}
void begin_mesh_depth_frame(MeshDepthState& s){s.front=1498976.0F;s.current=1498976.0F;s.layer=0;}
void configure_mesh_builder_depth(MeshBuilderDepth& s,std::int32_t bits,bool null_surface){
    s.scale=bits<17?1024.0F:1.5F;
    if(null_surface){s.scale=1024.0F;s.step=107.51999664306640625F;}
    else s.step=s.scale*0.105F;
}
void begin_mesh_quad_depth(MeshBuilderDepth& s,bool enabled){if(enabled){auto& manager=required(s.manager);manager.current=manager.current-s.step;}}
float append_mesh_vertex_depth(MeshBuilderDepth& s,float source_z){
    auto& manager=required(s.manager);const float product=s.scale*source_z;const float result=product+manager.current;s.total=s.total+result;return result;
}
} // namespace saga::core
