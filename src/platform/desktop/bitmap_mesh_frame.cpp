#include "bitmap_mesh_frame.hpp"
#include <stdexcept>

namespace saga::desktop {
using namespace core;
SubmissionMaterial& BitmapMaterials::resolve(std::size_t texture,std::uint32_t shader,std::int32_t queue){
    const auto base=sprite_shader_queue(shader);
    const auto key=std::make_tuple(texture,shader,queue);
    const auto found=materials_.find(key);if(found!=materials_.end())return found->second;
    auto& value=materials_[key];value.queue={add_queue_offset(queue,-base),queue};
    value.uniform_color=(shader&0x10000U)!=0;value.image=&images_[texture];return value;
}
BitmapMeshFrame::BitmapMeshFrame(BitmapBackend& backend,MeshDepthState& depth,MeshDepthHost& queries,BitmapMeshSettings settings,Materials materials)
    :backend_(backend),depth_(depth),queries_(queries),settings_(std::move(settings)),resolve_(std::move(materials)),
     cache_{std::make_shared<std::map<SubmissionMaterial*,MeshBuilderState*>>(),std::make_shared<MeshBuilderOrder>()},
     ready_{std::make_shared<std::vector<SubmissionRecord*>>()},source_(positions(4)){
    if(!resolve_||!settings_.depth_step)throw std::invalid_argument("bitmap frame needs material/depth providers");
    drawing_.matrix=&backend_.matrix();
    backend_.set_depth_host(this);
}
BitmapMeshFrame::~BitmapMeshFrame(){backend_.set_depth_host(nullptr);}
void BitmapMeshFrame::begin_frame(){
    for(auto& b:builders_)reset_mesh_builder(*b);
    begin_mesh_depth_frame(depth_);backend_.clear_quads();for(auto& entry:working_)entry.second.clear();
}
void BitmapMeshFrame::draw(RenderQueue& queue,std::int32_t type){
    drawing_.queue=&queue;draw_render_queue(drawing_,&backend_.graphics(),type,backend_.resources());
}
void BitmapMeshFrame::finish(){update_mesh_records(cache_,ready_,*this);}
bool BitmapMeshFrame::depth_step_enabled(){return settings_.depth_step();}
MeshBuilderState& BitmapMeshFrame::builder(std::size_t texture,std::uint32_t shader,std::int32_t queue){
    auto& material=resolve_(texture,shader,queue);if(material.queue.absolute!=queue)throw std::invalid_argument("material queue does not match bitmap pass");
    auto result=shaders_.emplace(&material,shader);
    if(!result.second&&result.first->second!=shader)throw std::invalid_argument("material identity has conflicting shaders");
    auto* b=get_mesh_builder(cache_,&material,*this);if(!b)throw std::invalid_argument("cached null mesh builder");return *b;
}
MeshBuilderDepth& BitmapMeshFrame::depth_builder(std::size_t t,std::uint32_t s,std::int32_t q){return builder(t,s,q).depth;}
void BitmapMeshFrame::emit_quad(BitmapQuad& q){
    auto& b=builder(q.texture,q.shader_key,q.render_queue);
    for(std::size_t i=0;i<4;++i){const auto& p=q.points[i];source_->at(i)={p.x,p.y,p.z};}
    auto* group=add_mesh_quad(b,source_,q.uv.left,1-q.uv.top,q.uv.right-q.uv.left,q.uv.top-q.uv.bottom,
                              {q.color.r,q.color.g,q.color.b,q.color.a},*this);
    q.native_uv.emplace();
    for(std::size_t i=0;i<4;++i){const auto index=static_cast<std::size_t>(group->head)+i;const auto& p=group->geometry->positions->at(index);
        q.points[i]={p.x,p.y,p.z};q.native_uv->at(i)=group->geometry->uv->at(index);}
    q.native_depth=true;working_[group].push_back(q);
}
MeshBuilderState* BitmapMeshFrame::create_builder(MeshBuilderCache&,bool color,SubmissionMaterial* material){
    auto b=std::make_unique<MeshBuilderState>();b->depth.manager=&depth_;b->vertex_color=color;b->material=material;
    b->layers=std::make_shared<MeshBuilderLayers>();b->layers->set(0,create_layer(*b,0));
    configure_mesh_builder_depth(b->depth,settings_.depth_bits,settings_.null_surface);reset_mesh_builder(*b);
    auto* value=b.get();builders_.push_back(std::move(b));return value;
}
SubmissionLayer* BitmapMeshFrame::create_layer(MeshBuilderState& b,std::int32_t index){
    auto l=std::make_unique<SubmissionLayer>();l->index=index;l->groups=std::make_shared<SubmissionGroupList>();
    l->groups->values.push_back(create_group(b));auto* value=l.get();layers_.push_back(std::move(l));return value;
}
SubmissionGroup* BitmapMeshFrame::create_group(MeshBuilderState&){
    auto g=std::make_unique<SubmissionGroup>();g->geometry=std::make_shared<MeshGeometry>();
    g->geometry->positions=positions(0);g->geometry->uv=uv(0);g->geometry->colors=colors(0);g->geometry->indices=indices(0);
    auto* value=g.get();uploaded_[value];working_[value];groups_.push_back(std::move(g));return value;
}
void BitmapMeshFrame::record_indices(MeshDepthState&,SubmissionGroup&,std::int32_t,std::int32_t){} // This adapter has no optional MeshRecord capture.
SubmissionRecord* BitmapMeshFrame::create_record(){auto r=std::make_unique<SubmissionRecord>();auto* value=r.get();records_.push_back(std::move(r));return value;}
void BitmapMeshFrame::prepare_group(MeshBuilderState&,SubmissionGroup& group){uploaded_.at(&group)=working_.at(&group);}
} // namespace saga::desktop
