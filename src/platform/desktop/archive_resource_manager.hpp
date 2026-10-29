#pragma once
#include "resource_archive.hpp"
#include "core/gameplay/image_draw.hpp"

namespace saga::desktop {
// Synchronously prepares the catalog's complete indexed SEB array. File-backed
// records are shared only for equal normalized filename strings, as LoadStart
// does; clocks, options and evaluation caches belong to distinct instances.
// Images are borrowed, explicitly bound ready objects: cache/atlas identities
// and PNG/texture decoding belong to the host, not an invented loading policy.
class ArchiveResourceManager {
public:
    explicit ArchiveResourceManager(ResourceArchive&);
    ArchiveResourceManager(const ArchiveResourceManager&)=delete;
    ArchiveResourceManager& operator=(const ArchiveResourceManager&)=delete;
    core::AnimationResource& animations(){return animations_;}
    core::RenderDrawResource& drawing(){return drawing_;}
    const ArchiveImage& image_source(std::int32_t id){return archive_.image(id);}
    const ArchiveCatalog& image_catalog()const{return archive_.images();}
    void bind_image(std::int32_t id,core::PixelImage* ready_image);
private:
    ResourceArchive& archive_; // Must outlive this manager.
    std::vector<std::unique_ptr<core::SebSpriteState>> sprites_;
    std::vector<std::unique_ptr<core::SharedAnimationClip>> clips_;
    core::AnimationResource animations_;
    core::RenderDrawResource drawing_;
};
} // namespace saga::desktop
