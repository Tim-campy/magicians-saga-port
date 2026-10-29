#pragma once
#include "bitmap_backend.hpp"
#include "core/gameplay/mesh_geometry.hpp"
#include <tuple>

namespace saga::desktop {
struct BitmapMeshSettings {
    std::int32_t depth_bits;
    bool null_surface;
    std::function<bool()> depth_step;
};
// Ordinary textured MaterialManager.GetMaterial projection: image identity,
// shader name key and queue variant. Special material paths remain rejected by BitmapBackend.
class BitmapMaterials {
public:
    BitmapMaterials()=default;
    BitmapMaterials(const BitmapMaterials&)=delete;
    BitmapMaterials& operator=(const BitmapMaterials&)=delete;
    core::SubmissionMaterial& resolve(std::size_t texture,std::uint32_t shader,std::int32_t absolute_queue);
    std::size_t size()const{return materials_.size();}
private:
    std::map<std::size_t,core::SubmissionImage> images_;
    std::map<std::tuple<std::size_t,std::uint32_t,std::int32_t>,core::SubmissionMaterial> materials_;
};
// Desktop ownership adapter for the recovered mesh kernels. It does not choose scene/camera defaults.
// Material ownership is supplied explicitly, either by BitmapMaterials or a diagnostic profile.
class BitmapMeshFrame final:public BitmapDepthHost,private core::MeshQuadHost,private core::MeshRecordHost {
public:
    using Materials=std::function<core::SubmissionMaterial&(std::size_t,std::uint32_t,std::int32_t)>;
    BitmapMeshFrame(BitmapBackend&,core::MeshDepthState&,core::MeshDepthHost&,BitmapMeshSettings,Materials);
    ~BitmapMeshFrame();
    BitmapMeshFrame(const BitmapMeshFrame&)=delete;
    BitmapMeshFrame& operator=(const BitmapMeshFrame&)=delete;
    // Reset recovered counters and depth stores; full BeginMesh invalidation/record disposal is not inferred.
    void begin_frame();
    void draw(core::RenderQueue&,std::int32_t type);
    void finish(); // UpdateLayerMeshes record-production projection and completed-group uploads.
    const core::MeshRecordList& records()const{return ready_;}
    const std::map<core::SubmissionMaterial*,std::uint32_t>& materials()const{return shaders_;}
    const std::map<core::SubmissionGroup*,std::vector<BitmapQuad>>& uploaded()const{return uploaded_;}
    const core::MeshBuilderCache& cache()const{return cache_;}
    core::MeshDepthState& mesh_depth_state()override{return depth_;}
    core::MeshBuilderDepth& depth_builder(std::size_t,std::uint32_t,std::int32_t)override;
    bool depth_step_enabled()override;
    bool surface_is_null(core::MeshDepthSurface* p)override{return queries_.surface_is_null(p);}
    float low_depth_scaling()override{return queries_.low_depth_scaling();}
    void emit_quad(BitmapQuad&)override;
private:
    core::MeshBuilderState& builder(std::size_t,std::uint32_t,std::int32_t);
    core::MeshBuilderState* create_builder(core::MeshBuilderCache&,bool,core::SubmissionMaterial*)override;
    core::SubmissionLayer* create_layer(core::MeshBuilderState&,std::int32_t)override;
    core::SubmissionGroup* create_group(core::MeshBuilderState&)override;
    void record_indices(core::MeshDepthState&,core::SubmissionGroup&,std::int32_t,std::int32_t)override; // No capture configured by this owner.
    core::SubmissionRecord* create_record()override;
    bool has_surface()override{return !settings_.null_surface;}
    void prepare_group(core::MeshBuilderState&,core::SubmissionGroup&)override;
    BitmapBackend& backend_;core::MeshDepthState& depth_;core::MeshDepthHost& queries_;
    BitmapMeshSettings settings_;Materials resolve_;
    core::MeshBuilderCache cache_;
    core::MeshRecordList ready_;
    core::RenderDrawState drawing_{}; // Retain AppData's draw phase across frames.
    std::vector<std::unique_ptr<core::MeshBuilderState>> builders_;
    std::vector<std::unique_ptr<core::SubmissionLayer>> layers_;
    std::vector<std::unique_ptr<core::SubmissionGroup>> groups_;
    std::vector<std::unique_ptr<core::SubmissionRecord>> records_;
    core::MeshArray<core::MeshVertex> source_;
    std::map<core::SubmissionMaterial*,std::uint32_t> shaders_;
    std::map<core::SubmissionGroup*,std::vector<BitmapQuad>> working_,uploaded_;
};
} // namespace saga::desktop
