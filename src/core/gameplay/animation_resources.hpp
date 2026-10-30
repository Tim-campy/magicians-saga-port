#pragma once
#include "animation_pass.hpp"
#include "assets/seb.hpp"
#include "../animation/seb_sprite.hpp"
#include "resource_catalog.hpp"
#include "select_prelude.hpp"
#include <optional>

namespace saga::core {
struct ResourceCatalogIndex {
    std::vector<std::int32_t> ids;
    std::vector<std::optional<std::size_t>> rows; // Indexed slots; duplicate ID keeps the last row.
};
// LoadStart indexing slice 0x1ba72d8..0x1ba7430, after host-side line parsing.
// Missing IDs use -1. max_slots is an input-memory policy, not an original game limit.
ResourceCatalogIndex index_resource_catalog(std::vector<std::int32_t> ids,std::size_t max_slots=1048576);
class ImageAnimationTextHost {
public:
    virtual ~ImageAnimationTextHost()=default;
    virtual CatalogText lower_case(const std::u16string&)=0;
    virtual bool starts_with(const std::u16string&,std::u16string_view)=0;
    virtual std::int32_t parse_id(std::u16string_view)=0;
};
// ResourceManagerExtension.LoadImageAnimationInfos 0x10eeb74. The native loop
// indexes raw post-ID catalog rows, independently of the indexed image IDs.
std::shared_ptr<ImageAnimationGroup> load_image_animation_infos(std::size_t image_count,CatalogRows,ImageAnimationTextHost&);

template<class Resource>struct ApplicationResourceFields {
    Resource *characters=nullptr,*common=nullptr,*enemies=nullptr,*equipment=nullptr;
    Resource *quest=nullptr,*residents=nullptr,*selection=nullptr;
    Resource *chips=nullptr,*bullets=nullptr,*effects=nullptr;
    Resource *hats=nullptr,*weapons=nullptr,*gauges=nullptr;
};
using ApplicationAnimationResources=ApplicationResourceFields<AnimationResource>;
// Same AppData fields projected for clocks or drawing; one selector contract.
template<class Fields>auto application_resource_slot(Fields& r,std::int32_t id)noexcept->decltype(&r.characters){
    switch(id){
    case 1:return &r.characters;case 2:return &r.common;case 3:return &r.enemies;case 4:return &r.equipment;
    case 7:return &r.quest;case 8:return &r.residents;case 11:return &r.selection;
    case 18:return &r.chips;case 19:return &r.bullets;case 20:return &r.effects;
    case 21:return &r.hats;case 22:return &r.weapons;case 23:return &r.gauges;
    default:return nullptr;
    }
}
template<class Resource>Resource* get_application_resource(const ApplicationResourceFields<Resource>& r,std::int32_t id)noexcept{
    auto slot=application_resource_slot(r,id);return slot?*slot:nullptr;
}
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
    std::vector<std::optional<SebSpriteState>> sprites_;
    AnimationResource resource_;
};
} // namespace saga::core
