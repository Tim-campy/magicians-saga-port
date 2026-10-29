#include "bitmap_backend.hpp"
#include "core/gameplay/render_transform.hpp"
#include "core/gameplay/material_queue.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cmath>

namespace saga::desktop {
using namespace core;
OffscreenMatrix bitmap_projection(std::int32_t width,std::int32_t height,BitmapCamera camera){
    if(width<1||height<2||width>8192||height>8192||static_cast<std::uint64_t>(width)*height>16U*1024U*1024U||
       !std::isfinite(camera.near_clip)||!std::isfinite(camera.far_clip)||!std::isfinite(camera.z)||camera.far_clip<=camera.near_clip)
        throw std::invalid_argument("invalid orthographic target/camera");
    const auto p=offscreen_camera_placement(width,height,camera.z);
    const float aspect=static_cast<float>(width)/static_cast<float>(height),half_width=p.half_height*aspect;
    const float span=camera.far_clip-camera.near_clip;
    if(!std::isfinite(span))throw std::invalid_argument("orthographic clip span overflow");
    OffscreenMatrix result;auto& m=result.values;
    m[0]=1.0F/half_width;m[5]=1.0F/p.half_height;m[10]=2.0F/span;m[15]=1;
    m[12]=-p.x*m[0];m[13]=-p.y*m[5];
    const float translation=-camera.z*m[10];m[14]=translation-(camera.far_clip+camera.near_clip)/span;
    for(const auto value:m)if(!std::isfinite(value))throw std::invalid_argument("nonfinite orthographic matrix");
    return result;
}
namespace {
[[noreturn]] void unbound(){throw std::runtime_error("bitmap backend requires an unbound graphics service");}
}
struct BitmapMatrix final:RenderMatrix {
    SpriteMatrix value;
    MatrixValues supplied=std::make_shared<std::vector<float>>(9);
    void set_values(std::array<float,9> v)override{std::copy(v.begin(),v.end(),supplied->begin());set_matrix_values(value,supplied);}
    void pre_translate(float x,float y)override{pre_translate_matrix(value,x,y);}
    void set_rotation(float angle,float x,float y)override{set_rotate_matrix(value,angle,x,y);}
    void reset()override{reset_matrix(value);}
};
struct BitmapBackend::Impl final:RenderDrawHost,RenderGraphics,RenderDepth,ImageDrawHost,
                         BitmapTexture,BitmapAlphaHost,BitmapGeometryHost,RenderTransformHost,RenderColorHost,GaugeDrawingHost {
    GaugeDrawState gauges;QuestCameraHost* gauge_camera=nullptr;
    PixelImage* gauge_image(std::int32_t id)override{
        auto* r=global_resource(23);if(!r||!r->images)throw std::invalid_argument("missing gauge resource23 images");
        auto* image=dynamic_cast<PixelImage*>(r->images->at(static_cast<std::size_t>(id)));
        if(!image)throw std::invalid_argument("missing decoded gauge image");return image;
    }
    std::int32_t gauge_view_width()override{return quest_game_width(gauge_camera->current_camera_form(),*gauge_camera);}
    std::int32_t gauge_view_height()override{return quest_game_height(gauge_camera->current_camera_form(),*gauge_camera);}
    std::shared_ptr<std::vector<float>> gauge_alpha(PixelImage& image,std::int32_t x,std::int32_t y,
        std::int32_t w,std::int32_t h,std::shared_ptr<std::vector<float>> alpha)override{
        const auto* ready=textures.at(textures.key(image)).ready;
        if(!ready)throw std::invalid_argument("gauge alpha requires decoded pixels");
        return decoded_image_alpha(ready->pixels,x,y,w,h,std::move(alpha));
    }
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
    BitmapMatrix application_matrix;
    SpriteMatrix temporary_matrix;
    RenderColorState colors;RenderColorWork color_work;
    GraphicsQueueState queues;
    bool raw_rgba;
    std::vector<BitmapQuad> output;
    std::optional<std::array<std::int32_t,12>> last;
    BitmapDepthKey depth;
    BitmapTextureBinding current{0,0,0};
    BitmapApplication& application;GlobalApplication global;BitmapTextures& textures;
    BitmapDepthHost* depth_host=nullptr;
    Impl(BitmapApplication& app,GlobalApplication global_app,BitmapTextures& images,std::int32_t height,const SpriteMatrix& world,bool raw)
        :surface{height},raw_rgba(raw),application(app),global(std::move(global_app)),textures(images){
        geometry.surface=&surface;geometry.mesh_modes=std::make_shared<std::vector<BitmapMeshCoordinates*>>(1,&mode);
        const auto nan=std::numeric_limits<float>::quiet_NaN();
        image_state.culling_disabled=true;image_state.clip=std::make_shared<std::vector<float>>(4,nan);
        image_state.bounds=std::make_shared<ImageBounds>();image_state.statistics=&statistics;
        colors.cache_dirty=&cache.dirty;set_render_mode(colors,0,255,0);set_blend_mode(colors,0,-1);
        set_graphics_matrix(transform,&world);
    }
    RenderDrawResource* resource(std::int32_t id)override{return get_application_resource(application,id);}
    RenderDrawResource* global_resource(std::int32_t id)override{
        auto* app=global();if(!app)throw std::invalid_argument("missing global bitmap application");return get_application_resource(*app,id);
    }
    SpriteArray sprite(SharedAnimationClip& clip,std::int32_t frame,std::int32_t layer)override{
        if(!clip.sprites)throw std::invalid_argument("bitmap clip has no prepared sprite data");
        auto value=seb_sprite(*clip.sprites,seb_work,frame,layer);
        if(value){std::array<std::int32_t,12> copy;std::copy_n(value->begin(),12,copy.begin());last=copy;}
        return value;
    }
    void invalid_image_override(std::int32_t,std::int32_t,std::int32_t,std::int32_t)override{unbound();}
    void missing_image(std::int32_t,std::int32_t)override{unbound();}
    float transform_tangent(float angle)override{return degree_tangent(angle);}
    RenderDepth* mesh_manager()override{return this;}
    void set_depth(std::int32_t index,std::int32_t relative)override{
        depth={group,index,relative};
        if(depth_host){auto& state=depth_host->mesh_depth_state();state.layer=group;set_mesh_depth(state,index,relative,*depth_host);}
    }
    void emit(BitmapQuad quad){
        if(depth_host)depth_host->emit_quad(quad);
        output.push_back(quad);
    }
    void push_render_mode(std::int32_t m,std::int32_t a,std::int32_t b)override{if(raw_rgba)unbound();core::push_render_mode(colors,m,a,b);}
    void pop_render_mode()override{if(raw_rgba)unbound();core::pop_render_mode(colors);}
    void push_blend_mode(std::int32_t m,std::int32_t c)override{if(raw_rgba)unbound();core::push_blend_mode(colors,m,c);}
    void pop_blend_mode()override{if(raw_rgba)unbound();core::pop_blend_mode(colors);}
    void sync_matrix(){image_state.local_matrix_index=transform.local_index;image_state.local_coordinate_count=transform.local_coordinate_count;}
    void push_matrix(RenderMatrix& matrix)override{
        auto* selected=dynamic_cast<BitmapMatrix*>(&matrix);if(!selected)unbound();
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
        const auto texture=textures.key(*source.page);current=textures.at(texture);
        if(image.render_queue!=0)push_graphics_queue(queues,add_queue_offset(queues.value,image.render_queue));
        begin_render_matrix(transform,source.draw.x,source.draw.y,*this);
        try{
            update_bitmap_sampling(cache,*source.page,source.draw,source_work,*this,*this);
            if(cache.valid){
                const auto destination=bitmap_destination(source.draw,0.0F); // Explicit zero-expansion profile.
                points=transform_bitmap_rectangle(geometry,&destination,points,nullptr,*this);
                BitmapQuad quad;quad.texture=texture;quad.depth=depth;std::copy_n(points->begin(),4,quad.points.begin());quad.uv=*cache.uv;
                quad.queue_offset=queues.value;quad.render_queue=add_queue_offset(2000,queues.value);
                if(raw_rgba)emit(quad);
                else{
                    auto passes=get_render_passes(colors,true,color_work.white,color_work,*this);
                    for(const auto& pass:*passes){
                        auto value=begin_render_pass(*pass,*this);
                        try{
                            if(pass->material||colors.blend_mode==3)unbound(); // Grayscale generation is a separate bitmap path.
                            const auto shader=sprite_shader_state(colors,true,current.alpha_type,value->at(0).a,value,pass->alpha_test,false);
                            quad.shader_key=sprite_shader_key(shader);quad.pipeline=sprite_pixel_pipeline(quad.shader_key);
                            struct QueueHost final:MaterialQueueHost {
                                std::uint32_t key;explicit QueueHost(std::uint32_t k):key(k){}
                                std::int32_t shader_queue()override{return sprite_shader_queue(key);}
                            } queue_host(quad.shader_key);
                            MaterialQueueState material_queue;set_material_queue_offset(material_queue,queues.value,queue_host);
                            quad.render_queue=material_queue.absolute;
                            quad.color=quad.pipeline.modulate?value->at(0):RenderColor{1,1,1,1};emit(quad);
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
    std::int32_t width()override{return current.width;}
    std::int32_t height()override{return current.height;}
    std::int32_t alpha_type(PixelImage& image)override{return raw_rgba?0:textures.at(textures.key(image)).alpha_type;}
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
    std::int32_t image_alpha_type()override{return current.alpha_type;}
    bool render_gui_mode(RenderColorState&)override{return false;} // Explicit mesh/desktop profile.
    void push_gui_matrix()override{unbound();}
    void load_gui_pixel_matrix()override{unbound();}
    std::int32_t screen_width()override{unbound();}
    void gui_viewport(float,float,float,float)override{unbound();}
    void pop_gui_matrix()override{unbound();}
};

std::size_t BitmapTextures::bind(PixelImage& image,std::int32_t width,std::int32_t height,std::int32_t alpha){
    if(width<=0||height<=0)throw std::invalid_argument("invalid bitmap texture dimensions");
    if(const auto it=indices_.find(&image);it!=indices_.end()){
        const auto& previous=at(it->second);
        if(previous.width!=width||previous.height!=height||previous.alpha_type!=alpha)throw std::invalid_argument("bitmap image binding changed");
        return it->second;
    }
    const auto key=entries_.size();entries_.push_back({width,height,alpha,nullptr});
    try{indices_.emplace(&image,key);}catch(...){entries_.pop_back();throw;}
    return key;
}
void BitmapTextures::bind(const ArchiveImageSet& images){
    for(std::size_t i=0;i<images.slot_count();++i)if(const auto* ready=images.get(static_cast<std::int32_t>(i))){
        const auto key=bind(*ready->image,ready->pixels.width,ready->pixels.height,ready->alpha_type);entries_.at(key).ready=ready;
    }
}
std::size_t BitmapTextures::key(PixelImage& image)const{
    const auto it=indices_.find(&image);if(it==indices_.end())throw std::invalid_argument("bitmap image has no ready texture");return it->second;
}
BitmapBackend::BitmapBackend(BitmapApplication& app,GlobalApplication global,BitmapTextures& images,std::int32_t height,const SpriteMatrix& world,bool raw)
    :impl_(std::make_unique<Impl>(app,std::move(global),images,height,world,raw)){}
BitmapBackend::BitmapBackend(BitmapApplication& app,BitmapApplication& global,BitmapTextures& images,std::int32_t height,const SpriteMatrix& world,bool raw)
    :BitmapBackend(app,[&global]{return &global;},images,height,world,raw){}
BitmapBackend::~BitmapBackend()=default;
RenderGraphics& BitmapBackend::graphics(){return *impl_;}
void BitmapBackend::draw_parameter_gauge(const ParameterGaugeRequest& r,QuestCameraHost& camera,GaugeDrawState* shared){
    impl_->gauge_camera=&camera;core::draw_parameter_gauge(shared?*shared:impl_->gauges,impl_.get(),r,*impl_);
}
RenderDrawHost& BitmapBackend::resources(){return *impl_;}
RenderMatrix& BitmapBackend::matrix(){return impl_->application_matrix;}
const std::vector<BitmapQuad>& BitmapBackend::quads()const{return impl_->output;}
std::optional<std::array<std::int32_t,12>> BitmapBackend::last_sprite()const{return impl_->last;}
void BitmapBackend::clear_quads(){impl_->output.clear();impl_->last.reset();}
void BitmapBackend::set_matrix(const SpriteMatrix* matrix){set_graphics_matrix(impl_->transform,matrix);}
void BitmapBackend::set_depth_host(BitmapDepthHost* host){impl_->depth_host=host;}
void BitmapBackend::resize_surface(std::int32_t height){if(height<1||height>8192)throw std::invalid_argument("invalid bitmap surface height");impl_->surface.height=height;}
} // namespace saga::desktop
