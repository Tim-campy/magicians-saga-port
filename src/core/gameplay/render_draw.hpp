#pragma once
#include "render_queue.hpp"
#include "animation_pass.hpp"
#include "../animation/seb_sprite.hpp"
#include <array>

namespace saga::core {
class RenderImage {public:virtual ~RenderImage()=default;};
using RenderImages=std::shared_ptr<std::vector<RenderImage*>>;
struct RenderDrawResource {AnimationResource* animations=nullptr;RenderImages images;};
class RenderDepth {
public:
    virtual ~RenderDepth()=default;
    std::int32_t group=0; // MeshManager +0x44, set before SetDepth.
    virtual void set_depth(std::int32_t index,std::int32_t relative)=0; // Native back bool's low bit, not CurrentDepthLayer.
};
class RenderMatrix {
public:
    virtual ~RenderMatrix()=default;
    virtual void set_values(std::array<float,9>)=0;
    virtual void pre_translate(float x,float y)=0;
    virtual void set_rotation(float angle,float x,float y)=0;
    virtual void reset()=0;
};
struct RenderImageDraw {
    RenderImage* image=nullptr;
    std::int32_t u=0,v=0,source_width=0,source_height=0;
    float x=0,y=0,width=0,height=0;
    bool scaled=false; // Preserves DrawImage versus DrawScaledImage dispatch.
};
class RenderGraphics {
public:
    virtual ~RenderGraphics()=default;
    std::int32_t reverse=0; // Graphics +0x40; absent flags do not overwrite it.
    virtual RenderDepth* mesh_manager()=0;
    virtual void push_render_mode(std::int32_t mode,std::int32_t alpha,std::int32_t inverse)=0;
    virtual void pop_render_mode()=0;
    virtual void push_blend_mode(std::int32_t mode,std::int32_t color)=0;
    virtual void pop_blend_mode()=0;
    virtual void push_matrix(RenderMatrix&)=0;
    virtual void pop_matrix()=0;
    virtual void draw_image(const RenderImageDraw&)=0;
};
struct RenderDrawState {
    RenderQueue* queue=nullptr;
    std::int32_t phase=0; // AppData +0x3e0, wrapping increment and signed remainder20.
    RenderMatrix* matrix=nullptr; // +0x3e8, fetched again for each operation.
};
class RenderDrawHost {
public:
    virtual ~RenderDrawHost()=default;
    virtual RenderDrawResource* resource(std::int32_t)=0; // This AppData.GetResourceManager.
    virtual RenderDrawResource* global_resource(std::int32_t)=0; // Fresh global AppData, only for image override.
    virtual SpriteArray sprite(SharedAnimationClip&,std::int32_t frame,std::int32_t layer)=0;
    virtual void invalid_image_override(std::int32_t resource,std::int32_t texture,std::int32_t clip,std::int32_t layer)=0;
    virtual void missing_image(std::int32_t resource,std::int32_t texture)=0;
    // Native FastMath.Tan's actual approximation/angle convention remains a backend input.
    virtual float transform_tangent(float angle)=0;
};
// AppData.DrawZSortObjects 0x1115e54. Consumes current row/count references without clearing.
// No implicit sort or exception cleanup; callback changes affect later reads/rows.
void draw_render_queue(RenderDrawState&,RenderGraphics*,std::int32_t type,RenderDrawHost&);
} // namespace saga::core
