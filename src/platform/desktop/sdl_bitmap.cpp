#include "sdl_bitmap.hpp"
#include <cmath>
namespace saga::desktop {
bool render_preview_quad(SDL_Renderer* renderer,SDL_Texture* texture,const PreviewQuad& quad,float canvas_height){
    const auto& r=quad.uv;
    const std::array<SDL_FPoint,4> uv{{{r.left,1.0F-r.bottom},{r.right,1.0F-r.bottom},
                                     {r.right,1.0F-r.top},{r.left,1.0F-r.top}}};
    std::array<SDL_Vertex,4> vertices;
    for(std::size_t i=0;i<4;++i){
        const auto& p=quad.points[i];
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(uv[i].x)||!std::isfinite(uv[i].y))
            return SDL_SetError("nonfinite selected sprite geometry cannot be submitted to SDL");
        vertices[i]={{p.x,canvas_height-p.y},{1.0F,1.0F,1.0F,1.0F},uv[i]};
    }
    constexpr std::array<int,6> indices{{0,1,2,0,2,3}};
    return SDL_RenderGeometry(renderer,texture,vertices.data(),4,indices.data(),6);
}
}
