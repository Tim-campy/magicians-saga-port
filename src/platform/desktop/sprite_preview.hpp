#pragma once
#include "core/gameplay/bitmap_geometry.hpp"
#include <string>

namespace saga::desktop {
struct SpritePreviewOptions {
    std::string seb;
    std::int32_t frame=0,layer=0,texture=-1;
    std::int32_t width=0,height=0,x=0,y=0,standard_width=0,standard_height=0;
    std::int32_t angle=0;bool skew=false; // Explicit AppData command fields, not inferred entity modifiers.
};
struct PreviewQuad {std::array<core::BitmapPoint,4> points;core::ImageBounds uv;};
struct SpritePreview {
    std::vector<PreviewQuad> quads;
    std::optional<std::array<std::int32_t,12>> sprite;
};
// Technical, explicitly selected SEB layer + texture binding; not catalog loading.
// Unsupported matrix/material/UI services fail, never silently approximate.
SpritePreview prepare_sprite_preview(const SpritePreviewOptions&,const assets::Seb&,const assets::OptGrid*,
                                     std::int32_t png_width,std::int32_t png_height);
} // namespace saga::desktop
