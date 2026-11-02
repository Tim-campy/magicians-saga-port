#pragma once
#include "render_queue.hpp"
#include "animation_pass.hpp"
#include "../animation/seb_sprite.hpp"
#include <array>
#include <string>

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
class NumberImageHost {
public:
    virtual ~NumberImageHost()=default;
    // Both operations resolve the captured AppData's current resCom (resource2),
    // SEB slot and required arrays afresh; they do not use global AppData/resQuest.
    virtual SpriteArray number_sprite(std::int32_t clip,std::int32_t frame,std::int32_t layer)=0;
    // Seb.DrawFrame, all layers (-1), with the current resCom image array.
    virtual void draw_number_frame(std::int32_t clip,std::int32_t frame,float x,float y)=0;
};
// AppData.DrawNumImage(string,padding,anchor) 0x1105ac8. The nullable UTF16 text is
// required after the frame0/layer0 metric read; strings are not sanitized/formatted here.
std::int32_t draw_number_images(NumberImageHost&,std::int32_t clip,const std::u16string* text,
    std::int32_t x,std::int32_t y,std::int32_t padding,std::int32_t anchor);
class NumberValueImageHost:public NumberImageHost {
public:
    virtual std::int32_t number_max_frame(std::int32_t clip)=0; // Fresh captured-AppData resCom/SEB lookup.
    // Graphics.PushLocalMatrix(Matrix.Scale(scale,scale,0,0)); not the global matrix stack.
    virtual void push_number_scale(float scale)=0;
    virtual void pop_number_scale()=0;
    virtual void number_color(std::int32_t red,std::int32_t green,std::int32_t blue)=0;
    virtual void number_minus_text(float x,float y)=0; // Graphics.DrawString("-") with the current font/color.
};
// AppData.DrawNumImage(header,int64,footer,...) 0x1105328. Explicit -1 dimensions
// select the frame0/layer0 metric. The local matrix is popped only on success.
// This numeric route does not use the string overload's special slash glyph.
std::int64_t draw_number_value_images(NumberValueImageHost&,std::int32_t clip,
    const std::u16string* header,std::int64_t value,const std::u16string* footer,
    std::int32_t x,std::int32_t y,std::int32_t padding,std::int32_t anchor,
    std::int32_t scale,std::int32_t width,std::int32_t height);
} // namespace saga::core
