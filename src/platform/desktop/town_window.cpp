#include "town_window.hpp"
#include "bitmap_mesh_frame.hpp"
#include "gl_bitmap.hpp"
#include <SDL3/SDL.h>
#include <stdexcept>

namespace saga::desktop {
namespace {
void require(bool value,const char* operation){if(!value)throw std::runtime_error(std::string(operation)+": "+SDL_GetError());}
struct Quit {~Quit(){SDL_Quit();}};
struct Queries final:core::MeshDepthHost {
    bool surface_is_null(core::MeshDepthSurface* value)override{return value==nullptr;}
    float low_depth_scaling()override{throw std::logic_error("town capture has no low-depth profile");}
};
}
void capture_town_background(TownBackground& town,const std::filesystem::path& output){
    using namespace core;
    if(std::filesystem::exists(output))throw std::invalid_argument("town capture destination already exists");
    const auto& context=town.context();
    require(SDL_Init(SDL_INIT_VIDEO),"SDL_Init");Quit quit;configure_gl_bitmap_context();
    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(SDL_CreateWindow("Town background",context.width,context.height,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN),SDL_DestroyWindow);
    require(window!=nullptr,"SDL_CreateWindow");
    auto& textures=town.textures();
    GlBitmapRenderer renderer(*window,context.width,context.height,[&](std::size_t key){
        const auto* image=textures.at(key).ready;if(!image)throw std::invalid_argument("town texture is not decoded");
        return GlBitmapTexture{&image->pixels,GlBitmapWrap::clamp,GlBitmapWrap::clamp};
    });
    SpriteMatrix world;reset_matrix(world);
    auto& application=town.application();
    BitmapBackend backend(application,application,textures,context.height,world);
    MeshDepthSurface surface;MeshDepthState depth;depth.surface=&surface;Queries queries;BitmapMaterials materials;
    BitmapMeshFrame frame(backend,depth,queries,{context.depth_bits,false,[&]{return context.depth_step;}},[&](auto texture,auto shader,auto queue)->SubmissionMaterial&{
        return materials.resolve(texture,shader,queue);
    });
    frame.begin_frame();town.draw(backend);frame.finish();
    GlMeshBindings bindings;bind_bitmap_mesh_frame(bindings,frame);
    MeshSubmissionState submission;
    submission.cursors=std::make_shared<std::vector<std::int32_t>>(1,0);
    submission.lengths=std::make_shared<std::vector<std::int32_t>>(1,frame.records().active_count);
    submission.lists=std::make_shared<std::vector<SubmissionRecords>>(1,frame.records().rows);
    OffscreenMatrix unit;unit.values[0]=unit.values[5]=unit.values[10]=unit.values[15]=1;
    submission.matrices=std::make_shared<std::vector<OffscreenMatrix>>(1,unit);
    submission.used_materials=std::make_shared<std::vector<SubmissionMaterial*>>();
    GlMeshSubmissionHost host(renderer,bindings,[](auto&,auto&){},[]{return 0;}); // SAGA_STUB(town-image-access-check): no cache eviction/aging service.
    renderer.begin_frame(GlBitmapClear::color_and_depth,{0,0,0,0},bitmap_projection(context.width,context.height,context.projection));
    submit_mesh_streams(submission,1,host);
    auto pixels=renderer.read_pixels();
    std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> image(SDL_CreateSurfaceFrom(context.width,context.height,SDL_PIXELFORMAT_RGBA32,pixels.rgba.data(),context.width*4),SDL_DestroySurface);
    require(image!=nullptr,"SDL_CreateSurfaceFrom");require(SDL_SaveBMP(image.get(),output.string().c_str()),"SDL_SaveBMP");
}
} // namespace saga::desktop
