#include "archive_resource_manager.hpp"
#include <map>
#include <stdexcept>

namespace saga::desktop {
ArchiveResourceManager::ArchiveResourceManager(ResourceArchive& archive):archive_(archive){
    using namespace core;
    const auto& slots=archive.animations().slots;
    sprites_.resize(slots.size());clips_.resize(slots.size());
    animations_.clips=std::make_shared<std::vector<SharedAnimationClip*>>(slots.size(),nullptr);
    drawing_.animations=&animations_;
    drawing_.images=std::make_shared<std::vector<RenderImage*>>(archive.images().slots.size(),nullptr);
    std::map<std::string,std::size_t> first;
    for(std::size_t i=0;i<slots.size();++i){
        if(!slots[i])continue;
        const auto& selected=archive.animation(static_cast<std::int32_t>(i));
        const auto old=first.find(selected.filename);
        auto state=old==first.end()?prepare_seb_sprites(selected.animation):clone_seb_sprites(*sprites_.at(old->second));
        const auto maximum=old==first.end()?selected.animation.header_value:clips_.at(old->second)->max_frame;
        sprites_[i]=std::make_unique<SebSpriteState>(std::move(state));
        // LoadStart applies these after either fresh load or Seb(Seb), not only the fresh branch.
        reverse_seb(*sprites_[i],selected.reverse_u,selected.reverse_v);
        if(selected.cache_size>=1)set_seb_cache_size(*sprites_[i],selected.cache_size);
        clips_[i]=std::make_unique<SharedAnimationClip>(SharedAnimationClip{0,maximum,sprites_[i].get()});
        animations_.clips->at(i)=clips_[i].get();
        first.emplace(selected.filename,i);
    }
}
void ArchiveResourceManager::bind_image(std::int32_t id,core::PixelImage* image){
    const auto& slot=archive_.images().slots.at(static_cast<std::size_t>(id));
    if(!slot)throw std::invalid_argument("cannot bind an absent image catalog slot");
    if(image){
        const auto& selected=archive_.image(id);
        // The host supplies a fully prepared Image/OPT and its native dimensions.
        // Reject a mismatched binding rather than silently repairing its contract.
        if(image->standard_width!=selected.standard_width||image->standard_height!=selected.standard_height||image->render_queue!=selected.render_queue)
            throw std::invalid_argument("ready image does not match its catalog options");
    }
    drawing_.images->at(static_cast<std::size_t>(id))=image;
}
} // namespace saga::desktop
