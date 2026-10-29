#pragma once
#include <cstdint>

namespace saga::core {
struct MeshDepthSurface {bool use_low_depth_scaling=false;}; // Offscreen +0xe8.
struct MeshDepthState {
    MeshDepthSurface* surface=nullptr; // MeshManager +0x30, borrowed.
    bool low_depth=false; // +0x38, graphics depth bits <17 at construction.
    float front=0,current=0; // +0x3c/+0x40; fresh-object values, not BeginMesh state.
    std::int32_t layer=0; // +0x44.
};
class MeshDepthHost {
public:
    virtual ~MeshDepthHost()=default;
    virtual bool surface_is_null(MeshDepthSurface*)=0; // Public Unity Object equality/liveness boundary.
    virtual float low_depth_scaling()=0; // Current Offscreen static +0x18; no assumed value.
};
void set_mesh_depth(MeshDepthState&,std::int32_t depth,std::int32_t relative,MeshDepthHost&); // 0x1ca7c50.
std::int32_t mesh_depth_back(MeshDepthState&,MeshDepthHost&); // 0x1ca7d68.
void set_mesh_depth_layer(MeshDepthState&,std::int32_t); // 0x1ca7e4c.
std::int32_t mesh_depth_layer(const MeshDepthState&); // 0x1ca7e54.
std::int32_t mesh_depth_min(); // 0x1ca7e5c.
std::int32_t mesh_depth_max(); // 0x1ca7e68.
void set_mesh_depth_front(MeshDepthState&); // 0x1ca7e74.
// BeginMesh's final depth stores only, after earlier mesh/record clearing succeeds.
void begin_mesh_depth_frame(MeshDepthState&); // 0x1ca5dec..0x1ca5dfc; not all of BeginMesh.
struct MeshBuilderDepth {
    MeshDepthState* manager=nullptr;
    float scale=0,step=0,total=0; // MeshBuilder +0x40/+0x44/+0x30.
};
// Constructor arithmetic projection after depth-bit and surface-null queries.
// Does not construct layers/materials or implement MeshBuilder.Reset.
void configure_mesh_builder_depth(MeshBuilderDepth&,std::int32_t depth_bits,bool surface_is_null);
// AddQuad's reached depth slices; call once per emitted material pass, then per vertex.
void begin_mesh_quad_depth(MeshBuilderDepth&,bool depth_step_enabled);
float append_mesh_vertex_depth(MeshBuilderDepth&,float source_z);
} // namespace saga::core
