#include "sprite_preview.hpp"
#include "core/gameplay/animation_resources.hpp"
#include "core/gameplay/render_transform.hpp"
#include "core/gameplay/material_queue.hpp"
#include <algorithm>
#include <limits>

namespace saga::desktop {
namespace {
using namespace core;
[[noreturn]] void unbound(){throw std::runtime_error("selected sprite requires an unbound graphics service (blend, local coordinates or external image)");}
struct PreviewMatrix final:RenderMatrix {
    SpriteMatrix value;
    MatrixValues supplied=std::make_shared<std::vector<float>>(9);
    void set_values(std::array<float,9> v)override{std::copy(v.begin(),v.end(),supplied->begin());set_matrix_values(value,supplied);}
    void pre_translate(float x,float y)override{pre_translate_matrix(value,x,y);}
    void set_rotation(float angle,float x,float y)override{set_rotate_matrix(value,angle,x,y);}
    void reset()override{reset_matrix(value);}
};
struct PreviewHost final:RenderDrawHost,RenderGraphics,RenderDepth,ImageDrawHost,
                         BitmapTexture,BitmapAlphaHost,BitmapGeometryHost,RenderTransformHost,RenderColorHost {
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
    RenderTransformState transform;
    PreviewMatrix application_matrix;
    SpriteMatrix temporary_matrix;
    RenderColorState colors;RenderColorWork color_work;
    GraphicsQueueState queues;
    std::int32_t alpha_class;bool raw_rgba;
    std::int32_t png_width,png_height;
    SpritePreview result;
    PreviewHost(const SpritePreviewOptions& options,std::int32_t w,std::int32_t h,std::int32_t a):surface{options.height},alpha_class(a),raw_rgba(options.raw_rgba),png_width(w),png_height(h){
        geometry.surface=&surface;geometry.mesh_modes=std::make_shared<std::vector<BitmapMeshCoordinates*>>(1,&mode);
        const auto nan=std::numeric_limits<float>::quiet_NaN();
        image_state.culling_disabled=true;image_state.clip=std::make_shared<std::vector<float>>(4,nan);
        image_state.bounds=std::make_shared<ImageBounds>();image_state.statistics=&statistics;
        colors.cache_dirty=&cache.dirty;set_render_mode(colors,0,255,0);set_blend_mode(colors,0,-1);
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
    float transform_tangent(float angle)override{return degree_tangent(angle);}
    RenderDepth* mesh_manager()override{return this;}
    void set_depth(std::int32_t,std::int32_t)override{} // One caller-selected layer; no inter-object depth policy.
    void push_render_mode(std::int32_t m,std::int32_t a,std::int32_t b)override{if(raw_rgba)unbound();core::push_render_mode(colors,m,a,b);}
    void pop_render_mode()override{if(raw_rgba)unbound();core::pop_render_mode(colors);}
    void push_blend_mode(std::int32_t m,std::int32_t c)override{if(raw_rgba)unbound();core::push_blend_mode(colors,m,c);}
    void pop_blend_mode()override{if(raw_rgba)unbound();core::pop_blend_mode(colors);}
    void sync_matrix(){image_state.local_matrix_index=transform.local_index;image_state.local_coordinate_count=transform.local_coordinate_count;}
    void push_matrix(RenderMatrix& matrix)override{
        auto* selected=dynamic_cast<PreviewMatrix*>(&matrix);if(!selected)unbound();
        push_graphics_matrix(transform,&selected->value);sync_matrix();
    }
    void pop_matrix()override{pop_graphics_matrix(transform,*this);sync_matrix();}
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
    void push_rotation(float angle,float x,float y)override{set_rotate_matrix(temporary_matrix,angle,x,y);push_graphics_matrix(transform,&temporary_matrix);sync_matrix();}
    void push_scale(float x,float y,float cx,float cy)override{set_scale_matrix(temporary_matrix,x,y,cx,cy);push_graphics_matrix(transform,&temporary_matrix);sync_matrix();}
    void push_translation(float x,float y)override{set_translate_matrix(temporary_matrix,x,y);push_graphics_matrix(transform,&temporary_matrix);sync_matrix();}
    void bitmap(PixelImage& image,const BitmapDraw& request)override{
        auto source=prepare_bitmap_source(image,request,nullptr,*this); // Explicit non-atlas preview profile.
        if(image.render_queue!=0)push_graphics_queue(queues,add_queue_offset(queues.value,image.render_queue));
        begin_render_matrix(transform,source.draw.x,source.draw.y,*this);
        try{
            update_bitmap_sampling(cache,*source.page,source.draw,source_work,*this,*this);
            if(cache.valid){
                const auto destination=bitmap_destination(source.draw,0.0F); // Explicit zero-expansion profile.
                points=transform_bitmap_rectangle(geometry,&destination,points,nullptr,*this);
                PreviewQuad quad;std::copy_n(points->begin(),4,quad.points.begin());quad.uv=*cache.uv;
                quad.queue_offset=queues.value;quad.render_queue=add_queue_offset(2000,queues.value);
                if(raw_rgba)result.quads.push_back(quad);
                else{
                    auto passes=get_render_passes(colors,true,color_work.white,color_work,*this);
                    for(const auto& pass:*passes){
                        auto value=begin_render_pass(*pass,*this);
                        try{
                            if(pass->material||colors.blend_mode==3)unbound(); // Grayscale generation is a separate bitmap path.
                            const auto shader=sprite_shader_state(colors,true,alpha_class,value->at(0).a,value,pass->alpha_test,false);
                            quad.shader_key=sprite_shader_key(shader);quad.pipeline=sprite_pixel_pipeline(quad.shader_key);
                            struct QueueHost final:MaterialQueueHost {
                                std::uint32_t key;explicit QueueHost(std::uint32_t k):key(k){}
                                std::int32_t shader_queue()override{return sprite_shader_queue(key);}
                            } queue_host(quad.shader_key);
                            MaterialQueueState material_queue;set_material_queue_offset(material_queue,queues.value,queue_host);
                            quad.render_queue=material_queue.absolute;
                            quad.color=quad.pipeline.modulate?value->at(0):RenderColor{1,1,1,1};result.quads.push_back(quad);
                        }catch(...){end_render_pass(*pass,*this);throw;}
                        end_render_pass(*pass,*this);
                    }
                }
                cache.image=&image;cache.dirty=false;
            }
        }catch(...){end_render_matrix(transform,*this);if(image.render_queue!=0)pop_graphics_queue(queues);throw;}
        end_render_matrix(transform,*this);
        if(image.render_queue!=0)pop_graphics_queue(queues);
    }
    SpriteArray piece(PixelImage& image,std::int32_t column,std::int32_t row,std::int32_t pass)override{
        auto* prepared=dynamic_cast<PreparedOptImage*>(&image);if(!prepared)unbound();return prepared->piece(column,row,pass,opt_work);
    }
    SpriteArray frame_piece(PixelImage&,std::int32_t,std::int32_t)override{unbound();}
    void use_frame(PixelImage&,std::int32_t)override{unbound();}
    std::int32_t width()override{return png_width;}
    std::int32_t height()override{return png_height;}
    std::int32_t alpha_type(PixelImage&)override{return raw_rgba?0:alpha_class;}
    void set_alpha_type(PixelImage&,std::int32_t)override{unbound();}
    bool render_target_alive()override{unbound();}
    std::int32_t render_target_height()override{unbound();}
    std::int32_t screen_height()override{unbound();}
    void screen_transform(float& x,float& y)override{map_matrix_point(*transform.screen,x,y);}
    std::optional<std::array<float,2>> fill_point(float,float)override{unbound();}
    void add_fill_point(float,float,float,float)override{unbound();}
    void matrix_underflow()override{unbound();}
    void begin_local_coordinates(float,float)override{unbound();}
    void end_local_coordinates()override{unbound();}
    std::int32_t image_alpha_type()override{return alpha_class;}
    bool render_gui_mode(RenderColorState&)override{return false;} // Explicit mesh/desktop profile.
    void push_gui_matrix()override{unbound();}
    void load_gui_pixel_matrix()override{unbound();}
    std::int32_t screen_width()override{unbound();}
    void gui_viewport(float,float,float,float)override{unbound();}
    void pop_gui_matrix()override{unbound();}
};
}
SpritePreview prepare_sprite_preview(const SpritePreviewOptions& o,const assets::Seb& file,const assets::OptGrid* opt,
                                     std::int32_t png_width,std::int32_t png_height,std::int32_t alpha_type){
    if(o.texture<0||o.texture>65535||o.width<=0||o.height<=0)throw std::invalid_argument("invalid selected preview binding/canvas");
    PreparedAnimationResource loaded(index_resource_catalog({0}),{&file});
    auto& sprites=*loaded.resource().clips->at(0)->sprites;
    reverse_seb(sprites,o.resource_reverse_u,o.resource_reverse_v);
    if(o.resource_cache_size>=1)set_seb_cache_size(sprites,o.resource_cache_size);
    PreviewHost host(o,png_width,png_height,alpha_type);
    std::unique_ptr<PixelImage> image=opt?std::make_unique<PreparedOptImage>(*opt):std::make_unique<PixelImage>();
    image->width=png_width;image->height=png_height;image->standard_width=o.standard_width;image->standard_height=o.standard_height;
    image->render_queue=o.image_render_queue;
    auto images=std::make_shared<std::vector<RenderImage*>>(static_cast<std::size_t>(o.texture)+1,nullptr);images->at(static_cast<std::size_t>(o.texture))=image.get();
    host.resource_value={&loaded.resource(),images};
    // Verify the explicit selected texture before entering the consumer; do not
    // reinterpret other TexIds or guess catalog/resource/locale precedence.
    auto value=host.sprite(*loaded.resource().clips->at(0),o.frame,o.layer);
    if(value&&value->at(1)!=-1&&value->at(1)!=o.texture)throw std::invalid_argument("SEB TexId does not match explicit PNG slot");
    RenderQueue queue=make_render_queue();RenderCommand command;
    command.values={-1,1,0,0,o.frame,o.x,o.y,-1,-1,-1,-1,-1,-1,o.layer,o.alpha,o.color,o.angle,-1,o.skew?1:0};
    insert_render_command(queue,command);RenderDrawState draw{&queue,0,&host.application_matrix};draw_render_queue(draw,&host,0,host);
    std::stable_sort(host.result.quads.begin(),host.result.quads.end(),[](const auto& a,const auto& b){return a.render_queue<b.render_queue;});
    return std::move(host.result);
}
} // namespace saga::desktop
