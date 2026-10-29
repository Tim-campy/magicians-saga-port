#pragma once
#include "mesh_builder.hpp"
#include <array>

namespace saga::core {
struct MeshVertex {float x=0,y=0,z=0;};
using MeshUv=std::array<float,2>;
using MeshColor=std::array<float,4>;
template<class T>using MeshArray=std::shared_ptr<std::vector<T>>;
struct MeshGeometry {
    MeshArray<MeshVertex> positions;
    MeshArray<MeshUv> uv;
    MeshArray<MeshColor> colors;
    MeshArray<std::int32_t> indices;
};
// Public managed-array allocation is replaced by standard vectors. Lengths are fixed after allocation.
class MeshArrays {
public:
    virtual ~MeshArrays()=default;
    virtual MeshArray<MeshVertex> positions(std::int32_t);
    virtual MeshArray<MeshUv> uv(std::int32_t);
    virtual MeshArray<MeshColor> colors(std::int32_t);
    virtual MeshArray<std::int32_t> indices(std::int32_t);
};
class MeshQuadHost:public MeshBuilderFactory,public MeshArrays {
public:
    virtual SubmissionGroup* create_group(MeshBuilderState&)=0;
    virtual bool depth_step_enabled()=0;
    // Reached manager MeshRecord.Add(group, indexCount, 6); absent capture is explicitly a no-op.
    // Manager selection and optional recording remain the owner's service.
    virtual void record_indices(MeshDepthState&,SubmissionGroup&,std::int32_t start,std::int32_t count)=0;
};
SubmissionGroup* current_mesh_group(SubmissionLayer&); // Layer.get_Current 0x1ca2834.
void check_mesh_group(MeshBuilderState&,MeshQuadHost&); // CheckAndExpandGroup 0x1ca261c.
void expand_mesh_vertices(MeshBuilderState&,std::int32_t count,MeshArrays&); // CheckAndExpand 0x1ca2a18.
// AddQuad(Vector3[],u,v,width,height,r,g,b,a), 0x1ca2d74. Source length is not forced to four.
// The returned group is the one captured before allocation; caller can bind its completed geometry.
SubmissionGroup* add_mesh_quad(MeshBuilderState&,MeshArray<MeshVertex>,float u,float v,float width,float height,
                              MeshColor,MeshQuadHost&);
} // namespace saga::core
