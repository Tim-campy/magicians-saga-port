#pragma once
#include "sprite_preview.hpp"
#include <SDL3/SDL.h>
#include "decoded_image.hpp"
#include <functional>
namespace saga::desktop {
// Converts the established bottom-left geometry/UV convention to SDL's top-left
// convention. Rasterization and nearest sampling are SDL's public implementation.
bool render_preview_quad(SDL_Renderer*,SDL_Texture*,const PreviewQuad&,float canvas_height);
// Texture/mask cache shared by the selected viewer and multi-image queue host.
// Pixel storage and renderer outlive this cache. Texture keys come from the backend.
class SdlBitmapTextures {
public:
    using Pixels=std::function<const DecodedImage&(std::size_t)>;
    SdlBitmapTextures(SDL_Renderer*,Pixels);
    ~SdlBitmapTextures();
    SDL_Texture* select(const BitmapQuad&);
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
