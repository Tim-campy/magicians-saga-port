#pragma once
#include "archive_resource_manager.hpp"
#include "decoded_image.hpp"
#include <functional>

namespace saga::desktop {
struct ReadyArchiveImage {
    DecodedImage pixels;
    std::unique_ptr<core::PixelImage> image;
    std::int32_t alpha_type=0;
};
// Desktop-owned synchronous preparation, not native async loading. Decoder and
// total RGBA budget are host inputs. Publish only after the whole set succeeds;
// failures leave old bindings intact. Image objects are distinct per catalog ID.
class ArchiveImageSet {
public:
    using Decoder=std::function<DecodedImage(const std::vector<std::uint8_t>&)>;
    ArchiveImageSet(ArchiveResourceManager&,Decoder,std::uint64_t max_rgba_bytes=128U*1024U*1024U);
    ~ArchiveImageSet();
    ArchiveImageSet(const ArchiveImageSet&)=delete;
    ArchiveImageSet& operator=(const ArchiveImageSet&)=delete;
    const ReadyArchiveImage& at(std::int32_t)const;
    const ReadyArchiveImage* get(std::int32_t id)const{return ready_.at(static_cast<std::size_t>(id)).get();}
    std::size_t slot_count()const{return ready_.size();}
    std::size_t ready_count()const{return ready_count_;}
private:
    std::vector<std::unique_ptr<ReadyArchiveImage>> ready_;
    core::RenderImages bindings_;
    std::size_t ready_count_=0;
};
} // namespace saga::desktop
