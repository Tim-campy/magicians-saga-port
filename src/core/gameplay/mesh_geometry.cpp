#include "mesh_geometry.hpp"
#include "partition_sort.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing mesh geometry input");return *p;}
std::int32_t add(std::int32_t a,std::int32_t b){return detail::sort_word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
template<class T>std::int32_t length(const MeshArray<T>& p){
    const auto n=required(p.get()).size();if(n>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("mesh array exceeds int32");
    return static_cast<std::int32_t>(n);
}
template<class T>MeshArray<T> allocate(std::int32_t count){if(count<0)throw std::length_error("negative mesh array length");return std::make_shared<std::vector<T>>(static_cast<std::size_t>(count));}
template<class T>void copy(MeshArray<T> source,MeshArray<T> target){
    auto& from=required(source.get());auto& to=required(target.get());if(from.size()>to.size())throw std::out_of_range("mesh array copy exceeds target");
    if(source!=target)std::copy(from.begin(),from.end(),to.begin());
}
template<class T>T& item(const MeshArray<T>& p,std::int32_t index){return required(p.get()).at(static_cast<std::size_t>(index));}
MeshGeometry& geometry(SubmissionGroup& g){return required(g.geometry.get());}
}
MeshArray<MeshVertex> MeshArrays::positions(std::int32_t n){return allocate<MeshVertex>(n);}
MeshArray<MeshUv> MeshArrays::uv(std::int32_t n){return allocate<MeshUv>(n);}
MeshArray<MeshColor> MeshArrays::colors(std::int32_t n){return allocate<MeshColor>(n);}
MeshArray<std::int32_t> MeshArrays::indices(std::int32_t n){return allocate<std::int32_t>(n);}
SubmissionGroup* current_mesh_group(SubmissionLayer& layer){return required(layer.groups.get()).values.at(static_cast<std::size_t>(layer.current_group));}
void check_mesh_group(MeshBuilderState& builder,MeshQuadHost& host){
    auto* layer=get_mesh_layer(builder,host);auto& selected=required(layer);
    if(!current_mesh_group(selected))return; // Native first null-group check returns without marking change.
    auto* group=current_mesh_group(selected);auto& g=required(group);
    if(g.vertex_count>59999||g.index_count>=55000){
        auto groups=selected.groups;auto* created=host.create_group(builder);
        required(groups.get()).values.push_back(created);++groups->revision;
        selected.current_group=add(selected.current_group,1);group=created;
    }
    required(group).change=true;
}
void expand_mesh_vertices(MeshBuilderState& builder,std::int32_t count,MeshArrays& arrays){
    auto& group=required(current_mesh_group(required(builder.current_layer)));auto& storage=geometry(group);
    const auto old_length=length(storage.positions),used=group.vertex_count,needed=add(used,count);
    if(needed<=old_length)return;
    const auto size=add(needed,used/2); // Based on USED count, not old array capacity; signed division truncates toward zero.
    auto positions=arrays.positions(size);auto uv=arrays.uv(size);
    copy(storage.positions,positions);copy(storage.uv,uv);storage.positions=positions;storage.uv=uv;
    if(builder.vertex_color){auto colors=arrays.colors(size);copy(storage.colors,colors);storage.colors=colors;}
}
SubmissionGroup* add_mesh_quad(MeshBuilderState& builder,MeshArray<MeshVertex> source,float u,float v,float width,float height,MeshColor color,MeshQuadHost& host){
    check_mesh_group(builder,host);
    begin_mesh_quad_depth(builder.depth,host.depth_step_enabled());
    builder.current_layer=get_mesh_layer(builder,host);
    auto* selected=current_mesh_group(required(builder.current_layer));
    expand_mesh_vertices(builder,4,host);auto& group=required(selected);group.head=group.vertex_count;
    const auto count=length(source);const float right=u+width,top=v+height;
    for(std::int32_t i=0;i<count;++i){
        auto& storage=geometry(group);const auto index=group.vertex_count;
        const auto point=item(source,i);auto& target=item(storage.positions,index);target.x=point.x;target.y=point.y;
        // Store Z before UV/color; DepthTotal and vertexCount advance only after all these writes succeed.
        auto& manager=required(builder.depth.manager);const float product=builder.depth.scale*point.z;target.z=product+manager.current;
        auto& uv=item(storage.uv,index);uv={u,v};if(i==1||i==2)uv[0]=right;if(i<=1)uv[1]=top;
        if(builder.vertex_color)item(storage.colors,index)=color;
        builder.depth.total=builder.depth.total+item(storage.positions,group.vertex_count).z;
        group.vertex_count=add(group.vertex_count,1);
    }
    auto& storage=geometry(group);auto indices=storage.indices;
    if(add(group.index_count,6)>length(indices)){
        const auto size=add(add(group.index_count,group.index_count/2),12);
        auto next=host.indices(size);copy(storage.indices,next);storage.indices=next;indices=next;
    }
    host.record_indices(required(builder.depth.manager),group,group.index_count,6);indices=storage.indices; // Optional record callback may replace it.
    constexpr std::array<std::int32_t,6> offsets{0,1,2,0,2,3};
    for(std::int32_t i=0;i<6;++i)item(indices,add(group.index_count,i))=add(group.head,offsets.at(static_cast<std::size_t>(i)));
    group.index_count=add(group.index_count,6);return selected;
}
} // namespace saga::core
