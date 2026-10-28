#pragma once
#include "animation_pass.hpp"
#include "assets/seb.hpp"
#include <optional>

namespace saga::core {
struct ResourceCatalogIndex {
    std::vector<std::int32_t> ids;
    std::vector<std::optional<std::size_t>> rows; // Indexed slots; duplicate ID keeps the last row.
};
// LoadStart indexing slice 0x1ba72d8..0x1ba7430, after host-side line parsing.
// Missing IDs use -1. max_slots is an input-memory policy, not an original game limit.
ResourceCatalogIndex index_resource_catalog(std::vector<std::int32_t> ids,std::size_t max_slots=1048576);

struct ApplicationAnimationResources {
    AnimationResource *characters=nullptr,*common=nullptr,*enemies=nullptr,*equipment=nullptr;
    AnimationResource *quest=nullptr,*residents=nullptr,*selection=nullptr;
    AnimationResource *chips=nullptr,*bullets=nullptr,*effects=nullptr;
    AnimationResource *hats=nullptr,*weapons=nullptr,*gauges=nullptr;
};
// AppData.GetResourceManager 0x1103e90. Unselected IDs return null, including 0.
AnimationResource* get_animation_resource(const ApplicationAnimationResources&,std::int32_t) noexcept;
std::int32_t animation_clip_max_frame(const ApplicationAnimationResources&,std::int32_t resource,std::int32_t clip);

// Desktop preparation from already selected, decoded files. This is not the
// asynchronous LoadStart orchestration or its language/path selection policy.
// Immutable storage keeps clip addresses stable without an allocation per clock.
class PreparedAnimationResource {
public:
    explicit PreparedAnimationResource(const std::vector<const assets::Seb*>& indexed_files);
    PreparedAnimationResource(const ResourceCatalogIndex&,const std::vector<const assets::Seb*>& source_rows);
    PreparedAnimationResource(const PreparedAnimationResource&)=delete;
    PreparedAnimationResource& operator=(const PreparedAnimationResource&)=delete;
    PreparedAnimationResource(PreparedAnimationResource&&)=delete;
    PreparedAnimationResource& operator=(PreparedAnimationResource&&)=delete;
    AnimationResource& resource(){return resource_;}
private:
    std::vector<std::optional<SharedAnimationClip>> clocks_;
    AnimationResource resource_;
};
} // namespace saga::core
