#include "mesh_builder.hpp"
#include "partition_sort.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing mesh builder input");return *p;}
void unchanged(std::uint32_t revision,std::uint32_t expected){
    if(revision!=expected)throw std::runtime_error("mesh builder collection changed during enumeration");
}
}
void MeshBuilderLayers::set(std::int32_t key,SubmissionLayer* layer){
    const auto found=entries.find(key);
    if(found==entries.end()){entries.emplace(key,layer);value_keys.push_back(key);}
    else found->second=layer;
    ++revision;
}
MeshBuilderState* get_mesh_builder(MeshBuilderCache& cache,SubmissionMaterial* material,MeshBuilderFactory& factory){
    auto dictionary=cache.builders;auto& entries=required(dictionary.get());
    if(!material)throw std::invalid_argument("null mesh material dictionary key");
    const auto found=entries.find(material);if(found!=entries.end())return found->second; // A stored null is still a hit.
    auto* builder=factory.create_builder(cache,!material->uniform_color,material);required(builder);
    entries[material]=builder; // Captured dictionary, before obtaining the fresh sorted-list field.
    auto sorted=cache.sorted;auto& list=required(sorted.get()).values;const auto queue=material->queue.absolute;
    std::size_t index=0;
    while(index<list.size()&&queue>required(list.at(index).first).queue.absolute)++index;
    list.insert(list.begin()+static_cast<std::ptrdiff_t>(index),{material,builder}); // New equal queues precede older entries.
    ++sorted->revision;
    return builder;
}
SubmissionLayer* get_mesh_layer(MeshBuilderState& builder,MeshBuilderFactory& factory){
    const auto index=required(builder.depth.manager).layer;auto directory=builder.layers;
    const auto& entries=required(directory.get()).entries;const auto found=entries.find(index);
    if(found!=entries.end())return found->second;
    const auto captured_index=required(builder.depth.manager).layer;
    auto* layer=factory.create_layer(builder,captured_index);required(layer);
    const auto fresh_index=required(builder.depth.manager).layer;
    required(builder.layers.get()).set(fresh_index,layer);return layer;
}
void reset_mesh_builder(MeshBuilderState& builder){
    auto directory=builder.layers;const auto& layers=required(directory.get()).value_keys;
    for(auto key:layers){auto* layer=directory->entries.at(key);auto groups=required(layer).groups;
        for(auto* group:required(groups.get()).values){auto& value=required(group);value.vertex_count=0;value.index_count=0;value.head=0;}
    }
    builder.depth.total=0;
}
void update_mesh_records(MeshBuilderCache& cache,MeshRecordList& state,MeshRecordHost& host){
    state.active_count=0;
    auto sorted=cache.sorted;auto& order=required(sorted.get());const auto order_revision=order.revision;
    auto min_layer=std::numeric_limits<std::int32_t>::max(),max_layer=std::numeric_limits<std::int32_t>::min();
    for(std::size_t i=0;;++i){
        unchanged(order.revision,order_revision);if(i>=order.values.size())break;
        const auto pair=order.values.at(i);auto& builder=required(pair.second);
        auto directory=builder.layers;auto& layers=required(directory.get());const auto layers_revision=layers.revision;
        for(std::size_t j=0;;++j){
            unchanged(layers.revision,layers_revision);if(j>=layers.value_keys.size())break;
            auto* layer=layers.entries.at(layers.value_keys.at(j));
            if(submission_index_count(required(layer))<1)continue;
            min_layer=std::min(min_layer,layer->index);max_layer=std::max(max_layer,layer->index);
            auto rows=state.rows;auto& retained=required(rows.get());
            if(static_cast<std::int64_t>(retained.size())<=state.active_count)retained.push_back(host.create_record());
            if(host.has_surface()){
                auto groups=layer->groups;auto& list=required(groups.get());const auto revision=list.revision;
                for(std::size_t k=0;;++k){
                    unchanged(list.revision,revision);if(k>=list.values.size())break;
                    auto& group=required(list.values.at(k));if(group.index_count>=1)host.prepare_group(builder,group);
                }
            }
            // Public graphics callbacks may replace the manager's record list or active count.
            auto fresh_rows=state.rows;
            auto& record=required(required(fresh_rows.get()).at(static_cast<std::size_t>(state.active_count)));
            init_submission_record(record,pair.first,layer);
            state.active_count=detail::sort_word(static_cast<std::uint32_t>(state.active_count)+1U);
        }
    }
    // Same-layer material order must survive composite-key overflow; do not unconditionally sort.
    if(min_layer!=max_layer&&state.active_count>=1)sort_submissions(state.rows,0,state.active_count-1);
}
} // namespace saga::core
