#pragma once
#include "archive_image_set.hpp"
#include "core/gameplay/animation_resources.hpp"
#include "core/gameplay/bitmap_geometry.hpp"
#include "core/gameplay/render_colors.hpp"
#include "core/gameplay/sprite_matrix.hpp"
#include "core/gameplay/mesh_depth.hpp"
#include "core/gameplay/offscreen_transform.hpp"
#include "core/gameplay/gauge_drawing.hpp"
#include "core/gameplay/quest_camera.hpp"
#include <map>

namespace saga::desktop {
struct BitmapCamera {float near_clip,far_clip,z;};
// Native offscreen resize placement, then the public orthographic camera boundary.
core::OffscreenMatrix bitmap_projection(std::int32_t width,std::int32_t height,BitmapCamera);
using BitmapApplication=core::ApplicationResourceFields<core::RenderDrawResource>;
struct BitmapDepthKey {std::int32_t group=0,index=0,relative=0;}; // group is native CurrentDepthLayer.
struct BitmapQuad {
    std::array<core::BitmapPoint,4> points;core::ImageBounds uv;
    core::RenderColor color{1,1,1,1};core::SpritePixelPipeline pipeline;std::uint32_t shader_key=0;
    std::int32_t queue_offset=0,render_queue=2000;
    std::size_t texture=0;BitmapDepthKey depth; // Native depth input, not a guessed Z mapping.
    bool native_depth=false;
    std::optional<std::array<std::array<float,2>,4>> native_uv; // Exact AddQuad stores, including binary32 rounding.
};
class BitmapDepthHost:public core::MeshDepthHost {
public:
    virtual core::MeshDepthState& mesh_depth_state()=0;
    // Actual builder/material identity and lifetime are supplied by the mesh host.
    virtual core::MeshBuilderDepth& depth_builder(std::size_t texture,std::uint32_t shader,std::int32_t queue)=0;
    virtual bool depth_step_enabled()=0;
    virtual void emit_quad(BitmapQuad& quad){
        auto& builder=depth_builder(quad.texture,quad.shader_key,quad.render_queue);
        core::begin_mesh_quad_depth(builder,depth_step_enabled());
        for(auto& point:quad.points)point.z=core::append_mesh_vertex_depth(builder,point.z);
        quad.native_depth=true;
    }
};
struct BitmapTextureBinding {
    std::int32_t width,height,alpha_type;
    const ReadyArchiveImage* ready=nullptr; // Optional borrowed decoded-pixel ownership.
};
class BitmapTextures {
public:
    std::size_t bind(core::PixelImage&,std::int32_t width,std::int32_t height,std::int32_t alpha);
    void bind(const ArchiveImageSet&);
    std::size_t key(core::PixelImage&)const;
    const BitmapTextureBinding& at(std::size_t i)const{return entries_.at(i);}
private:
    std::map<core::PixelImage*,std::size_t> indices_;
    std::vector<BitmapTextureBinding> entries_;
};
// Reusable bitmap emission from real AppData queues. No SDL dependency: quads
// retain per-image texture keys and native depth inputs in emission order.
// Caller owns resource/image lifetimes and supplies world matrix + surface height.
// Atlas, culling/local-coordinate services and GPU depth/order remain unbound.
class BitmapBackend {
public:
    using GlobalApplication=std::function<BitmapApplication*()>;
    BitmapBackend(BitmapApplication&,GlobalApplication,BitmapTextures&,std::int32_t surface_height,
                  const core::SpriteMatrix& world,bool raw_rgba=false);
    BitmapBackend(BitmapApplication&,BitmapApplication& global,BitmapTextures&,std::int32_t surface_height,
                  const core::SpriteMatrix& world,bool raw_rgba=false);
    ~BitmapBackend();
    BitmapBackend(const BitmapBackend&)=delete;
    BitmapBackend& operator=(const BitmapBackend&)=delete;
    core::RenderGraphics& graphics();
    core::RenderDrawHost& resources();
    core::RenderMatrix& matrix();
    const std::vector<BitmapQuad>& quads()const;
    std::optional<std::array<std::int32_t,12>> last_sprite()const;
    void clear_quads(); // Retains buffers and graphics/SEB caches between frames.
    void draw_parameter_gauge(const core::ParameterGaugeRequest&,core::QuestCameraHost&,core::GaugeDrawState* shared=nullptr);
    void set_matrix(const core::SpriteMatrix*);
    void set_depth_host(BitmapDepthHost*); // Borrowed; null selects the existing 2D diagnostic profile.
    void resize_surface(std::int32_t height);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
} // namespace saga::desktop
