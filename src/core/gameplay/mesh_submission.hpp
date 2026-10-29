#pragma once
#include "material_queue.hpp"
#include "offscreen_transform.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace saga::core {
struct MeshGeometry;
struct SubmissionImage {std::int32_t access_check=0;}; // Image +0xc4, asset-use check counter, not a frame timestamp.
struct SubmissionMaterial {
    MaterialQueueState queue;
    std::int32_t unused_frames=0; // Material.NotAccessFrame +0x38.
    SubmissionImage* image=nullptr; // +0x18, borrowed.
    bool uniform_color=false; // Material.UntiColor +0x3c; builder vertex colors use its inverse.
};
struct SubmissionGroup {
    std::int32_t index_count=0; // Mesh group +0x40; actual GPU mesh/storage belongs to host.
    std::int32_t vertex_count=0,head=0; // +0x20/+0x5c; Reset does not release buffers or clear change.
    bool change=false; // +0x68.
    std::shared_ptr<MeshGeometry> geometry{}; // Fixed per-group identity; original positions/UV/colors/index fields.
};
// Standard vector storage plus the public List enumeration revision contract.
// The owning builder increments revision for every list mutation, including replacing an entry.
struct SubmissionGroupList {std::vector<SubmissionGroup*> values;std::uint32_t revision=0;};
using SubmissionGroups=std::shared_ptr<SubmissionGroupList>;
struct SubmissionLayer {
    std::int32_t index=0;SubmissionGroups groups; // Layer +0x10/+0x18.
    std::int32_t current_group=0; // +0x20; notably retained by MeshBuilder.Reset.
};
struct SubmissionRecord {
    SubmissionMaterial* material=nullptr;SubmissionLayer* layer=nullptr;
    std::int32_t key=0; // LayerMesh.Queue, not the material's raw render queue.
};
using SubmissionRecords=std::shared_ptr<std::vector<SubmissionRecord*>>;
void init_submission_record(SubmissionRecord&,SubmissionMaterial*,SubmissionLayer*); // LayerMesh.Init 0x1ca808c.
std::int32_t submission_index_count(SubmissionLayer&); // Layer.GetIndexCount 0x1ca56f8.
std::int32_t submission_pivot(SubmissionRecords,std::int32_t left,std::int32_t right); // 0x1ca8980.
std::int32_t partition_submissions(SubmissionRecords,std::int32_t left,std::int32_t right,std::int32_t pivot); // 0x1ca8a68.
void sort_submissions(SubmissionRecords,std::int32_t left,std::int32_t right); // 0x1ca80c0.

struct MeshSubmissionState {
    std::shared_ptr<std::vector<std::int32_t>> cursors,lengths; // Offscreen +0x100/+0x108.
    std::shared_ptr<std::vector<SubmissionRecords>> lists; // +0x110; ready lists, already natively sorted.
    OffscreenMatrices matrices; // +0xf0; ready matrices, one per stream.
    std::int32_t used_count=0;
    std::shared_ptr<std::vector<SubmissionMaterial*>> used_materials; // +0x118/+0x120, retained across frames.
};
class MeshSubmissionHost {
public:
    virtual ~MeshSubmissionHost()=default;
    virtual void clear_depth()=0; // Clear depth only to public normalized 1, preserving color/active viewport.
    virtual void set_material_pass(SubmissionMaterial&)=0; // Reached Material.SetPass; no bool gate added here.
    virtual void draw_mesh(SubmissionGroup&,OffscreenMatrix)=0; // Public immediate mesh draw.
    virtual void after_mesh_draw(SubmissionMaterial&,SubmissionGroup&)=0; // Live Log.IsLoggingDrawCall/IsLoggingMesh services.
    virtual std::int32_t check_count()=0; // Config.checkCount_, queried only when material.image != null.
};
// OnPostRender's ready-stream submission slice (0x1caba54..0x1cac1a4).
// Caller has prepared lists/cursors/lengths/matrices and captures stream_count.
// Earlier callbacks, profiler/outer managed catch, target lifecycle and final frame cleanup are not ported here.
// Failures propagate with the native completed prefix and current cursor retained.
void submit_mesh_streams(MeshSubmissionState&,std::int32_t stream_count,MeshSubmissionHost&);
} // namespace saga::core
