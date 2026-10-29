#include "sdl_bitmap.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace saga::desktop {
namespace {
void require(bool ok,const char* name){if(!ok)throw std::runtime_error(std::string(name)+": "+SDL_GetError());}
using Texture=std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)>;
using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)>;
}
struct SdlBitmapTextures::Impl {
    struct Entry {std::size_t key;core::SpriteAlphaTest test;float alpha;Texture texture;};
    SDL_Renderer* renderer;Pixels pixels;std::vector<Entry> entries;
    Impl(SDL_Renderer* r,Pixels p):renderer(r),pixels(std::move(p)){}
};
SdlBitmapTextures::SdlBitmapTextures(SDL_Renderer* r,Pixels p):impl_(std::make_unique<Impl>(r,std::move(p))){}
SdlBitmapTextures::~SdlBitmapTextures()=default;
SDL_Texture* SdlBitmapTextures::select(const BitmapQuad& quad){
    const auto test=quad.pipeline.alpha_test;const float factor=test==core::SpriteAlphaTest::none?1.0F:quad.color.a;
    if(test!=core::SpriteAlphaTest::none&&quad.pipeline.blend==core::SpriteBlend::replace)
        throw std::runtime_error("alpha-discard with replace blending requires a shader backend");
    auto& s=*impl_;const auto found=std::find_if(s.entries.begin(),s.entries.end(),[&](const auto& v){return v.key==quad.texture&&v.test==test&&v.alpha==factor;});
    if(found!=s.entries.end())return found->texture.get();
    const auto& source=s.pixels(quad.texture);
    if(source.width<=0||source.height<=0||static_cast<std::uint64_t>(source.width)*static_cast<std::uint64_t>(source.height)*4U!=source.rgba.size())
        throw std::invalid_argument("invalid SDL bitmap pixel storage");
    std::vector<std::uint8_t> masked;
    const auto* pixels=source.rgba.data();
    if(test!=core::SpriteAlphaTest::none){
        masked=source.rgba;
        for(std::size_t i=3;i<masked.size();i+=4)
            if(!core::accepts_sprite_alpha(test,(static_cast<float>(masked[i])/255.0F)*factor))masked[i]=0;
        pixels=masked.data();
    }
    Surface surface(SDL_CreateSurfaceFrom(source.width,source.height,SDL_PIXELFORMAT_RGBA32,const_cast<std::uint8_t*>(pixels),source.width*4),SDL_DestroySurface);
    require(surface!=nullptr,"SDL_CreateSurfaceFrom bitmap");
    Texture texture(SDL_CreateTextureFromSurface(s.renderer,surface.get()),SDL_DestroyTexture);
    require(texture!=nullptr,"SDL_CreateTextureFromSurface bitmap");
    require(SDL_SetTextureScaleMode(texture.get(),SDL_SCALEMODE_NEAREST),"SDL_SetTextureScaleMode bitmap");
    s.entries.push_back({quad.texture,test,factor,std::move(texture)});return s.entries.back().texture.get();
}
bool render_preview_quad(SDL_Renderer* renderer,SDL_Texture* texture,const PreviewQuad& quad,float canvas_height){
    const auto& r=quad.uv;
    using saga::core::SpriteBlend;
    SDL_BlendMode mode=SDL_BLENDMODE_NONE;
    switch(quad.pipeline.blend){
        case SpriteBlend::replace:break;
        case SpriteBlend::alpha:mode=SDL_BLENDMODE_BLEND;break;
        case SpriteBlend::add:mode=SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA,SDL_BLENDFACTOR_ONE,SDL_BLENDOPERATION_ADD,
                    SDL_BLENDFACTOR_SRC_ALPHA,SDL_BLENDFACTOR_ONE,SDL_BLENDOPERATION_ADD);break;
        case SpriteBlend::subtract_alpha:mode=SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA,SDL_BLENDFACTOR_ONE,SDL_BLENDOPERATION_REV_SUBTRACT,
                    SDL_BLENDFACTOR_ONE,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_REV_SUBTRACT);break;
        case SpriteBlend::subtract_add:mode=SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_ALPHA,SDL_BLENDFACTOR_ONE,SDL_BLENDOPERATION_REV_SUBTRACT,
                    SDL_BLENDFACTOR_SRC_ALPHA,SDL_BLENDFACTOR_ONE,SDL_BLENDOPERATION_REV_SUBTRACT);break;
        case SpriteBlend::attenuate:mode=SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_ZERO,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_ADD,
                    SDL_BLENDFACTOR_ONE,SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,SDL_BLENDOPERATION_ADD);break;
    }
    if(!SDL_SetTextureBlendMode(texture,mode))return false;
    const std::array<SDL_FPoint,4> uv{{{r.left,1.0F-r.bottom},{r.right,1.0F-r.bottom},
                                     {r.right,1.0F-r.top},{r.left,1.0F-r.top}}};
    std::array<SDL_Vertex,4> vertices;
    for(std::size_t i=0;i<4;++i){
        const auto& p=quad.points[i];
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(uv[i].x)||!std::isfinite(uv[i].y))
            return SDL_SetError("nonfinite selected sprite geometry cannot be submitted to SDL");
        vertices[i]={{p.x,canvas_height-p.y},{quad.color.r,quad.color.g,quad.color.b,quad.color.a},uv[i]};
    }
    constexpr std::array<int,6> indices{{0,1,2,0,2,3}};
    return SDL_RenderGeometry(renderer,texture,vertices.data(),4,indices.data(),6);
}
}
