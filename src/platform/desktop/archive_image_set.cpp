#include "archive_image_set.hpp"
#include "core/gameplay/render_colors.hpp"
#include <stdexcept>

namespace saga::desktop {
ArchiveImageSet::ArchiveImageSet(ArchiveResourceManager& manager,Decoder decode,std::uint64_t budget){
    const auto& slots=manager.image_catalog().slots;ready_.resize(slots.size());
    std::uint64_t used=0;
    for(std::size_t id=0;id<slots.size();++id){
        if(!slots[id])continue;
        const auto& source=manager.image_source(static_cast<std::int32_t>(id));
        auto value=std::make_unique<ReadyArchiveImage>();value->pixels=decode(source.png);
        const auto& pixels=value->pixels;
        if(pixels.width<=0||pixels.height<=0)throw std::invalid_argument("decoded image has invalid dimensions");
        const auto count=static_cast<std::uint64_t>(pixels.width)*static_cast<std::uint64_t>(pixels.height);
        if(count>0x7fffffffU||count*4!=pixels.rgba.size())throw std::invalid_argument("decoded RGBA image size mismatch");
        if(pixels.rgba.size()>budget-used)throw std::length_error("archive image RGBA budget exceeded");
        used+=pixels.rgba.size();
        value->image=source.opt?std::make_unique<core::PreparedOptImage>(*source.opt):std::make_unique<core::PixelImage>();
        auto& image=*value->image;image.width=pixels.width;image.height=pixels.height;
        image.standard_width=source.standard_width;image.standard_height=source.standard_height;image.render_queue=source.render_queue;
        core::ImageAlphaClassifier alpha;
        for(std::size_t i=3;i<pixels.rgba.size();i+=4)alpha.add(static_cast<float>(pixels.rgba[i])/255.0F);
        value->alpha_type=alpha.finish(static_cast<std::int32_t>(count));
        ready_[id]=std::move(value);++ready_count_;
    }
    bindings_=manager.drawing().images;
    if(!bindings_||bindings_->size()!=slots.size())throw std::invalid_argument("archive image binding array changed size");
    for(std::size_t id=0;id<ready_.size();++id)if(ready_[id])bindings_->at(id)=ready_[id]->image.get();
}
ArchiveImageSet::~ArchiveImageSet(){
    if(!bindings_)return;
    for(std::size_t id=0;id<ready_.size()&&id<bindings_->size();++id)
        if(ready_[id]&&bindings_->at(id)==ready_[id]->image.get())bindings_->at(id)=nullptr;
}
const ReadyArchiveImage& ArchiveImageSet::at(std::int32_t id)const{
    const auto& value=ready_.at(static_cast<std::size_t>(id));
    if(!value)throw std::invalid_argument("absent ready image slot");return *value;
}
} // namespace saga::desktop
