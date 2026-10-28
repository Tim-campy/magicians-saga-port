#include "animation_resources.hpp"
#include <limits>
#include <unordered_set>

namespace saga::core {
namespace {
std::vector<const assets::Seb*> selected_files(const ResourceCatalogIndex& index,const std::vector<const assets::Seb*>& files) {
    std::vector<const assets::Seb*> result(index.rows.size(),nullptr);
    for(std::size_t slot=0;slot<index.rows.size();++slot)
        if(index.rows[slot])result[slot]=files.at(*index.rows[slot]);
    return result;
}
}
ResourceCatalogIndex index_resource_catalog(std::vector<std::int32_t> ids,std::size_t max_slots) {
    if(ids.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("resource catalog row count exceeds int32");
    auto length=static_cast<std::int32_t>(ids.size());
    for(const auto id:ids) {
        const auto next=id==std::numeric_limits<std::int32_t>::max()?std::numeric_limits<std::int32_t>::min():id+1;
        if(next>length)length=next;
    }
    if(static_cast<std::size_t>(length)>max_slots)throw std::length_error("resource catalog slot budget exceeded");
    std::unordered_set<std::int32_t> used(ids.begin(),ids.end());
    std::int32_t candidate=0; // The used set only grows, so the smallest free ID never decreases.
    for(auto& id:ids)if(id==-1) {
        while(used.find(candidate)!=used.end())++candidate;
        id=candidate;used.insert(candidate);
    }
    ResourceCatalogIndex result{std::move(ids),std::vector<std::optional<std::size_t>>(static_cast<std::size_t>(length))};
    for(std::size_t row=0;row<result.ids.size();++row)
        result.rows.at(static_cast<std::size_t>(result.ids[row]))=row;
    return result;
}
AnimationResource* get_animation_resource(const ApplicationAnimationResources& r,std::int32_t id) noexcept {
    switch(id) {
    case 1:return r.characters;case 2:return r.common;case 3:return r.enemies;case 4:return r.equipment;
    case 7:return r.quest;case 8:return r.residents;case 11:return r.selection;
    case 18:return r.chips;case 19:return r.bullets;case 20:return r.effects;
    case 21:return r.hats;case 22:return r.weapons;case 23:return r.gauges;
    default:return nullptr;
    }
}
std::int32_t animation_clip_max_frame(const ApplicationAnimationResources& app,std::int32_t resource,std::int32_t clip) {
    auto* r=get_animation_resource(app,resource);
    if(!r||!r->clips)throw std::invalid_argument("missing effect animation resource");
    auto* value=r->clips->at(static_cast<std::size_t>(clip));
    if(!value)throw std::invalid_argument("missing effect animation clip");
    return value->max_frame;
}
PreparedAnimationResource::PreparedAnimationResource(const std::vector<const assets::Seb*>& files)
    :clocks_(files.size()),sprites_(files.size()),resource_{std::make_shared<std::vector<SharedAnimationClip*>>(files.size(),nullptr)} {
    for(std::size_t i=0;i<files.size();++i)if(files[i]) {
        // _load first calls Dispose (frame=0), then stores the signed header short at +0x1c.
        sprites_[i]=prepare_seb_sprites(*files[i]);
        clocks_[i]=SharedAnimationClip{0,files[i]->header_value,&*sprites_[i]};
        resource_.clips->at(i)=&*clocks_[i];
    }
}
PreparedAnimationResource::PreparedAnimationResource(const ResourceCatalogIndex& index,const std::vector<const assets::Seb*>& files)
    :PreparedAnimationResource(selected_files(index,files)) {}
} // namespace saga::core
