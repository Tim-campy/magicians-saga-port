#include "sprite_preview.hpp"
#include "core/gameplay/animation_resources.hpp"
#include <algorithm>
#include <limits>

namespace saga::desktop {
namespace {
using namespace core;
[[noreturn]] void unbound(){throw std::runtime_error("selected sprite requires an unbound graphics service (matrix, blend or external image)");}
struct PreviewHost final:RenderDrawHost,RenderGraphics,RenderDepth,ImageDrawHost,
                         BitmapTexture,BitmapAlphaHost,BitmapGeometryHost {
    RenderDrawResource resource_value;
    SebStaticWork seb_work;
    ImageDrawState image_state;
    ImageDrawStatistics statistics;
    SpriteArray opt_work=std::make_shared<SpriteValues>(7,0);
    BitmapSamplingCache cache;
    BitmapMeshCoordinates mode{true};BitmapSurface surface;
    BitmapGeometryState geometry;
    BitmapPoints points=std::make_shared<std::vector<BitmapPoint>>(4);
    ImageBounds source_work;
    std::int32_t png_width,png_height;
    SpritePreview result;
    PreviewHost(const SpritePreviewOptions& options,std::int32_t w,std::int32_t h):surface{options.height},png_width(w),png_height(h){
        geometry.surface=&surface;geometry.mesh_modes=std::make_shared<std::vector<BitmapMeshCoordinates*>>(1,&mode);
        const auto nan=std::numeric_limits<float>::quiet_NaN();
        image_state.culling_disabled=true;image_state.clip=std::make_shared<std::vector<float>>(4,nan);
        image_state.bounds=std::make_shared<ImageBounds>();image_state.statistics=&statistics;
    }
    RenderDrawResource* resource(std::int32_t id)override{if(id!=1)unbound();return &resource_value;}
    RenderDrawResource* global_resource(std::int32_t)override{unbound();}
    SpriteArray sprite(SharedAnimationClip& clip,std::int32_t frame,std::int32_t layer)override{
        auto value=seb_sprite(*clip.sprites,seb_work,frame,layer);
        if(value){std::array<std::int32_t,12> copy;std::copy_n(value->begin(),12,copy.begin());result.sprite=copy;}
        return value;
    }
    void invalid_image_override(std::int32_t,std::int32_t,std::int32_t,std::int32_t)override{unbound();}
    void missing_image(std::int32_t,std::int32_t)override{unbound();}
    float transform_tangent(float)override{unbound();}
    RenderDepth* mesh_manager()override{return this;}
    void set_depth(std::int32_t,std::int32_t)override{} // One caller-selected layer; no inter-object depth policy.
    void push_render_mode(std::int32_t,std::int32_t,std::int32_t)override{unbound();}
    void pop_render_mode()override{unbound();}
    void push_blend_mode(std::int32_t,std::int32_t)override{unbound();}
    void pop_blend_mode()override{unbound();}
    void push_matrix(RenderMatrix&)override{unbound();}
    void pop_matrix()override{unbound();} // Also ImageDrawHost cleanup, only reached after a supported push.
    void draw_image(const RenderImageDraw& d)override{
        auto* image=dynamic_cast<PixelImage*>(d.image);if(d.image&&!image)unbound();image_state.flip=reverse;
        if(d.scaled)draw_scaled_image_rect(image_state,image,d.x,d.y,d.width,d.height,d.u,d.v,d.source_width,d.source_height,*this);
        else draw_image_rect(image_state,image,d.x,d.y,d.u,d.v,d.source_width,d.source_height,*this);
    }
    bool culling_overlap(const ImageCullRect&,const ImageRect&)override{unbound();}
    void null_image_debug()override{unbound();}
    bool begin_sample()override{return false;}
    void end_sample(bool)override{}
    void check_use(PixelImage&)override{} // Inputs were read completely before this synchronous preview.
    void clipping_unavailable()override{unbound();}
    void push_rotation(float,float,float)override{unbound();}
    void push_scale(float,float,float,float)override{unbound();}
    void push_translation(float,float)override{unbound();}
    void bitmap(PixelImage& image,const BitmapDraw& request)override{
        auto source=prepare_bitmap_source(image,request,nullptr,*this); // Explicit non-atlas preview profile.
        update_bitmap_sampling(cache,*source.page,source.draw,source_work,*this,*this);
        if(!cache.valid)return;
        const auto destination=bitmap_destination(source.draw,0.0F); // Explicit zero-expansion profile.
        points=transform_bitmap_rectangle(geometry,&destination,points,nullptr,*this);
        PreviewQuad quad;std::copy_n(points->begin(),4,quad.points.begin());quad.uv=*cache.uv;
        result.quads.push_back(quad);
        cache.image=&image;cache.dirty=false;
    }
    SpriteArray piece(PixelImage& image,std::int32_t column,std::int32_t row,std::int32_t pass)override{
        auto* prepared=dynamic_cast<PreparedOptImage*>(&image);if(!prepared)unbound();return prepared->piece(column,row,pass,opt_work);
    }
    SpriteArray frame_piece(PixelImage&,std::int32_t,std::int32_t)override{unbound();}
    void use_frame(PixelImage&,std::int32_t)override{unbound();}
    std::int32_t width()override{return png_width;}
    std::int32_t height()override{return png_height;}
    std::int32_t alpha_type(PixelImage&)override{return 0;} // Explicit replace-only technical profile, not inferred game alpha state.
    void set_alpha_type(PixelImage&,std::int32_t)override{unbound();}
    bool render_target_alive()override{unbound();}
    std::int32_t render_target_height()override{unbound();}
    std::int32_t screen_height()override{unbound();}
    void screen_transform(float&,float&)override{} // Explicit identity screen-matrix input.
    std::optional<std::array<float,2>> fill_point(float,float)override{unbound();}
    void add_fill_point(float,float,float,float)override{unbound();}
};
}
SpritePreview prepare_sprite_preview(const SpritePreviewOptions& o,const assets::Seb& file,const assets::OptGrid* opt,
                                     std::int32_t png_width,std::int32_t png_height){
    if(o.texture<0||o.texture>65535||o.width<=0||o.height<=0)throw std::invalid_argument("invalid selected preview binding/canvas");
    PreparedAnimationResource loaded(index_resource_catalog({0}),{&file});
    PreviewHost host(o,png_width,png_height);
    std::unique_ptr<PixelImage> image=opt?std::make_unique<PreparedOptImage>(*opt):std::make_unique<PixelImage>();
    image->width=png_width;image->height=png_height;image->standard_width=o.standard_width;image->standard_height=o.standard_height;
    auto images=std::make_shared<std::vector<RenderImage*>>(static_cast<std::size_t>(o.texture)+1,nullptr);images->at(static_cast<std::size_t>(o.texture))=image.get();
    host.resource_value={&loaded.resource(),images};
    // Verify the explicit selected texture before entering the consumer; do not
    // reinterpret other TexIds or guess catalog/resource/locale precedence.
    auto value=host.sprite(*loaded.resource().clips->at(0),o.frame,o.layer);
    if(value&&value->at(1)!=-1&&value->at(1)!=o.texture)throw std::invalid_argument("SEB TexId does not match explicit PNG slot");
    RenderQueue queue=make_render_queue();RenderCommand command;
    command.values={-1,1,0,0,o.frame,o.x,o.y,-1,-1,-1,-1,-1,-1,o.layer,255,-1,0,-1,0};
    insert_render_command(queue,command);RenderDrawState draw{&queue,0,nullptr};draw_render_queue(draw,&host,0,host);
    return std::move(host.result);
}
} // namespace saga::desktop
