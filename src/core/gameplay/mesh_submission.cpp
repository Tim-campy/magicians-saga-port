#include "mesh_submission.hpp"
#include "partition_sort.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing mesh submission input");return *p;}
template<class T>auto& item(const std::shared_ptr<std::vector<T>>& p,std::int32_t i){return required(p.get()).at(static_cast<std::size_t>(i));}
SubmissionRecord& record(SubmissionRecords p,std::int32_t i){return required(item(p,i));}
void remember_material(MeshSubmissionState& s,SubmissionMaterial* material){
    auto& entries=required(s.used_materials.get());
    if(static_cast<std::int64_t>(entries.size())<=s.used_count)entries.push_back(nullptr); // One placeholder, not count+1 repair.
    item(s.used_materials,s.used_count)=material;
    s.used_count=detail::sort_word(static_cast<std::uint32_t>(s.used_count)+1U);
}
}
void init_submission_record(SubmissionRecord& r,SubmissionMaterial* material,SubmissionLayer* layer){
    r.material=material;r.layer=layer; // Retained before either required input is validated.
    const auto index=required(layer).index;
    const auto queue=required(material).queue.absolute;
    r.key=detail::sort_word(static_cast<std::uint32_t>(index)*100000U+static_cast<std::uint32_t>(queue));
}
std::int32_t submission_index_count(SubmissionLayer& layer){
    std::uint32_t total=0;
    for(const auto* group:required(layer.groups.get()).values)total+=static_cast<std::uint32_t>(required(group).index_count);
    return detail::sort_word(total);
}
std::int32_t submission_pivot(SubmissionRecords rows,std::int32_t left,std::int32_t right){
    auto key=[&](std::int32_t i){return record(rows,i).key;};return detail::native_pivot(left,right,key);
}
std::int32_t partition_submissions(SubmissionRecords rows,std::int32_t left,std::int32_t right,std::int32_t pivot){
    auto key=[&](std::int32_t i){return record(rows,i).key;};
    auto swap=[&](std::int32_t i,std::int32_t j){std::swap(item(rows,i),item(rows,j));};
    return detail::native_partition(left,right,pivot,key,swap);
}
void sort_submissions(SubmissionRecords rows,std::int32_t left,std::int32_t right){
    auto key=[&](std::int32_t i){return record(rows,i).key;};
    auto swap=[&](std::int32_t i,std::int32_t j){std::swap(item(rows,i),item(rows,j));};detail::native_quicksort(left,right,key,swap);
}
void submit_mesh_streams(MeshSubmissionState& s,std::int32_t stream_count,MeshSubmissionHost& host){
    s.used_count=0;
    auto previous_layer=std::numeric_limits<std::int32_t>::min();
    while(stream_count>0){
        std::int32_t selected=-1,smallest=0;
        for(std::int32_t stream=0;stream<stream_count;++stream){
            const auto cursor=item(s.cursors,stream),length=item(s.lengths,stream);
            if(cursor>=length)continue;
            const auto key=record(item(s.lists,stream),cursor).key;
            if(selected==-1||key<smallest){selected=stream;smallest=key;}
        }
        if(selected==-1)return;
        auto rows=item(s.lists,selected); // Captured list for this run; later state list replacement does not change it.
        const auto selected_key=record(rows,item(s.cursors,selected)).key;
        while(item(s.cursors,selected)<item(s.lengths,selected)){
            const auto cursor=item(s.cursors,selected);
            if(record(rows,cursor).key!=selected_key)break;
            auto* layer=record(rows,cursor).layer;
            if(submission_index_count(required(layer))>=1){
                if(previous_layer!=std::numeric_limits<std::int32_t>::min()){
                    const auto current=required(record(rows,item(s.cursors,selected)).layer).index;
                    if(previous_layer!=current)host.clear_depth();
                }
                previous_layer=required(record(rows,item(s.cursors,selected)).layer).index;
                auto* material=record(rows,item(s.cursors,selected)).material;
                required(material).unused_frames=0;host.set_material_pass(*material);
                auto groups=required(record(rows,item(s.cursors,selected)).layer).groups;
                auto& list=required(groups.get());const auto revision=list.revision;
                // Draw every group in a nonempty layer, including groups whose own index_count is zero.
                for(std::size_t index=0;;++index){
                    if(list.revision!=revision)throw std::runtime_error("mesh group list changed during enumeration");
                    if(index>=list.values.size())break;
                    auto* group=list.values.at(index);
                    auto& value=required(group);const auto matrix=item(s.matrices,selected);
                    host.draw_mesh(value,matrix);host.after_mesh_draw(*material,value);
                }
                auto* image=material->image;if(image)image->access_check=host.check_count();
                remember_material(s,material);
            }
            auto& live_cursor=item(s.cursors,selected);live_cursor=detail::sort_word(static_cast<std::uint32_t>(live_cursor)+1U);
        }
    }
}
} // namespace saga::core
