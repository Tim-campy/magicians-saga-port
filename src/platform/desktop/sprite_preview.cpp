#include "sprite_preview.hpp"
#include <algorithm>

namespace saga::desktop {
using namespace core;
SpritePreview prepare_sprite_preview(const SpritePreviewOptions& o,const assets::Seb& file,const assets::OptGrid* opt,
                                     std::int32_t png_width,std::int32_t png_height,std::int32_t alpha_type){
    if(o.texture<0||o.texture>65535||o.width<=0||o.height<=0)throw std::invalid_argument("invalid selected preview binding/canvas");
    PreparedAnimationResource loaded(index_resource_catalog({0}),{&file});
    auto& sprites=*loaded.resource().clips->at(0)->sprites;
    reverse_seb(sprites,o.resource_reverse_u,o.resource_reverse_v);
    if(o.resource_cache_size>=1)set_seb_cache_size(sprites,o.resource_cache_size);
    std::unique_ptr<PixelImage> image=opt?std::make_unique<PreparedOptImage>(*opt):std::make_unique<PixelImage>();
    image->width=png_width;image->height=png_height;image->standard_width=o.standard_width;image->standard_height=o.standard_height;
    image->render_queue=o.image_render_queue;
    auto images=std::make_shared<std::vector<RenderImage*>>(static_cast<std::size_t>(o.texture)+1,nullptr);images->at(static_cast<std::size_t>(o.texture))=image.get();
    RenderDrawResource resource{&loaded.resource(),images};
    return prepare_sprite_preview(o,resource,0,png_width,png_height,alpha_type);
}
SpritePreview prepare_sprite_preview(const SpritePreviewOptions& o,RenderDrawResource& resource,std::int32_t clip,
                                     std::int32_t png_width,std::int32_t png_height,std::int32_t alpha_type){
    if(o.texture<0||o.texture>65535||o.width<=0||o.height<=0)throw std::invalid_argument("invalid selected preview binding/canvas");
    if(!resource.animations||!resource.animations->clips)throw std::invalid_argument("missing prepared preview animations");
    auto* selected=resource.animations->clips->at(static_cast<std::size_t>(clip));
    if(!selected)throw std::invalid_argument("missing prepared preview clip");
    BitmapApplication application;application.characters=&resource;BitmapTextures textures;
    if(!resource.images)throw std::invalid_argument("missing prepared preview images");
    auto* image=dynamic_cast<PixelImage*>(resource.images->at(static_cast<std::size_t>(o.texture)));
    if(!image)throw std::invalid_argument("selected preview image is not ready");
    textures.bind(*image,png_width,png_height,alpha_type);SpriteMatrix world;
    BitmapBackend backend(application,application,textures,o.height,world,o.raw_rgba);
    auto value=backend.resources().sprite(*selected,o.frame,o.layer);
    if(value&&value->at(1)!=-1&&value->at(1)!=o.texture)throw std::invalid_argument("SEB TexId does not match explicit PNG slot");
    RenderQueue queue=make_render_queue();RenderCommand command;
    command.values={-1,1,0,clip,o.frame,o.x,o.y,-1,-1,-1,-1,-1,-1,o.layer,o.alpha,o.color,o.angle,-1,o.skew?1:0};
    insert_render_command(queue,command);RenderDrawState draw{&queue,0,&backend.matrix()};draw_render_queue(draw,&backend.graphics(),0,backend.resources());
    SpritePreview result{backend.quads(),backend.last_sprite()};
    std::stable_sort(result.quads.begin(),result.quads.end(),[](const auto& a,const auto& b){return a.render_queue<b.render_queue;});
    return result;
}
} // namespace saga::desktop
