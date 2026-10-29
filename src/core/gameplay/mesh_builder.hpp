#pragma once
#include "mesh_depth.hpp"
#include "mesh_submission.hpp"
#include <map>

namespace saga::core {
struct MeshBuilderState;
struct MeshBuilderOrder {
    std::vector<std::pair<SubmissionMaterial*,MeshBuilderState*>> values;
    std::uint32_t revision=0; // Public List enumeration invalidation contract.
};
struct MeshBuilderCache {
    // Material.Equals is reference equality; standard map storage replaces the managed dictionary.
    std::shared_ptr<std::map<SubmissionMaterial*,MeshBuilderState*>> builders;
    std::shared_ptr<MeshBuilderOrder> sorted;
};
struct MeshBuilderLayers {
    std::map<std::int32_t,SubmissionLayer*> entries;
    // Values enumeration is supplied by the directory owner; do not substitute key-sorted order.
    std::vector<std::int32_t> value_keys;
    std::uint32_t revision=0;
    void set(std::int32_t key,SubmissionLayer*);
};
struct MeshBuilderState {
    MeshBuilderDepth depth;
    SubmissionMaterial* material=nullptr;
    std::shared_ptr<MeshBuilderLayers> layers;
    SubmissionLayer* current_layer=nullptr; // MeshBuilder +0x20.
    bool vertex_color=false; // +0x28.
    std::int32_t empty_frames=0; // +0x2c.
};
class MeshBuilderFactory {
public:
    virtual ~MeshBuilderFactory()=default;
    // Required original constructor contracts; allocation/public graphics services belong to the owner.
    virtual MeshBuilderState* create_builder(MeshBuilderCache&,bool vertex_color,SubmissionMaterial*)=0;
    virtual SubmissionLayer* create_layer(MeshBuilderState&,std::int32_t index)=0;
};
MeshBuilderState* get_mesh_builder(MeshBuilderCache&,SubmissionMaterial*,MeshBuilderFactory&); // 0x1ca5aac.
SubmissionLayer* get_mesh_layer(MeshBuilderState&,MeshBuilderFactory&); // 0x1ca274c.
void reset_mesh_builder(MeshBuilderState&); // 0x1ca22a4; retains storage and layer/group selection.

struct MeshRecordList {
    SubmissionRecords rows; // MeshManager +0x50, including retained inactive suffix.
    std::int32_t active_count=0; // +0x58.
};
class MeshRecordHost {
public:
    virtual ~MeshRecordHost()=default;
    virtual SubmissionRecord* create_record()=0;
    virtual bool has_surface()=0; // Reached public Unity object-liveness check, once per nonempty layer.
    // ConvertMesh/public mesh upload, version fallback and RecalculateBounds remain host services.
    // Keep the last uploaded mesh for a zero-count group: it is not converted this frame.
    virtual void prepare_group(MeshBuilderState&,SubmissionGroup&)=0;
};
// UpdateLayerMeshes 0x1ca67d4 list-production projection; public mesh conversion stays explicit.
// Owner increments directory/list revisions for mutations. Callback failures retain the completed prefix.
void update_mesh_records(MeshBuilderCache&,MeshRecordList&,MeshRecordHost&);
} // namespace saga::core
