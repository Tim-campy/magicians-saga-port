#pragma once
#include "core/gameplay/bitmap_geometry.hpp"
#include "core/gameplay/render_colors.hpp"
#include <string>

namespace saga::desktop {
struct SpritePreviewOptions {
    std::string seb;
    std::int32_t frame=0,layer=0,texture=-1;
    std::int32_t width=0,height=0,x=0,y=0,standard_width=0,standard_height=0;
    std::int32_t angle=0;bool skew=false; // Explicit AppData command fields, not inferred entity modifiers.
    std::int32_t alpha=255,color=-1;bool raw_rgba=false;
    std::array<std::uint8_t,4> background{0,0,0,0}; // Explicit technical canvas clear, not mission state.
    std::int32_t resource_reverse_u=0,resource_reverse_v=0,resource_cache_size=0,image_render_queue=0;
};
struct PreviewQuad {
    std::array<core::BitmapPoint,4> points;core::ImageBounds uv;
    core::RenderColor color{1,1,1,1};core::SpritePixelPipeline pipeline;std::uint32_t shader_key=0;
    std::int32_t queue_offset=0,render_queue=2000;
};
struct SpritePreview {
    std::vector<PreviewQuad> quads;
    std::optional<std::array<std::int32_t,12>> sprite;
};
// Technical, explicitly selected SEB layer + texture binding; not catalog loading.
// Unsupported special material/UI services fail, never silently approximate.
SpritePreview prepare_sprite_preview(const SpritePreviewOptions&,const assets::Seb&,const assets::OptGrid*,
                                     std::int32_t png_width,std::int32_t png_height,std::int32_t alpha_type);
// Ready catalog manager: preserve its clip ID, clocks and cache instances.
SpritePreview prepare_sprite_preview(const SpritePreviewOptions&,core::RenderDrawResource&,std::int32_t clip,
                                     std::int32_t png_width,std::int32_t png_height,std::int32_t alpha_type);
} // namespace saga::desktop
