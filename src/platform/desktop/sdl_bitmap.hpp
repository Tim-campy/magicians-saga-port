#pragma once
#include "sprite_preview.hpp"
#include <SDL3/SDL.h>
namespace saga::desktop {
// Converts the established bottom-left geometry/UV convention to SDL's top-left
// convention. Rasterization and nearest sampling are SDL's public implementation.
bool render_preview_quad(SDL_Renderer*,SDL_Texture*,const PreviewQuad&,float canvas_height);
}
