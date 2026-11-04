#include "town_window.hpp"
#include "bitmap_mesh_frame.hpp"
#include "gl_bitmap.hpp"
#include "core/gameplay/entity_visual.hpp"
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
void run_town_window(TownBackground& town,const TownWindowOptions& options){
    using namespace core;
    if(options.capture&&std::filesystem::exists(*options.capture))throw std::invalid_argument("town capture destination already exists");
    const auto& context=town.context();
    require(SDL_Init(SDL_INIT_VIDEO),"SDL_Init");Quit quit;configure_gl_bitmap_context();
    std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(SDL_CreateWindow("Town background",context.width,context.height,SDL_WINDOW_OPENGL|(options.hidden?SDL_WINDOW_HIDDEN:0)),SDL_DestroyWindow);
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
    const auto submit=[&]{
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
    };
    if(options.interactive){
        bool running=true;std::uint64_t dispatch=0;float pending_x=0,pending_y=0;std::string title;
        const auto coordinates=[&](float x,float y){
            int width=0,height=0;require(SDL_GetWindowSize(window.get(),&width,&height),"SDL pointer window size");
            if(width<1||height<1)throw std::runtime_error("invalid town window dimensions");
            return std::array<std::int32_t,4>{factory_cell_coordinate(x*static_cast<float>(context.width)/width,1),
                factory_cell_coordinate(y*static_cast<float>(context.height)/height,1),width,height};
        };
        const auto delta=[](float& pending,float value){pending+=value;const auto result=factory_cell_coordinate(pending,1);pending-=static_cast<float>(result);return result;};
        while(running&&(!options.passes||dispatch<options.passes)){
            const auto deadline=SDL_GetTicks()+40;bool pulse=false;SDL_Event event;
            while(SDL_PollEvent(&event)){
                if(event.type==SDL_EVENT_QUIT)running=false;
                else if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST&&!context.input_replay){town.cancel_camera_pointer();pending_x=pending_y=0;}
                else if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){
                    if(event.key.key==SDLK_ESCAPE)running=false;
                    else if(!context.input_replay&&(event.key.key==SDLK_RETURN||event.key.key==SDLK_SPACE))pulse=true;
                }else if(!context.input_replay&&event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_RIGHT){
                    pending_x=pending_y=0;const auto p=coordinates(event.button.x,event.button.y);town.camera_pointer(0,p[0],p[1],0,0);
                }else if(!context.input_replay&&event.type==SDL_EVENT_MOUSE_BUTTON_UP&&event.button.button==SDL_BUTTON_RIGHT){
                    pending_x=pending_y=0;const auto p=coordinates(event.button.x,event.button.y);town.camera_pointer(1,p[0],p[1],0,0);
                }else if(!context.input_replay&&event.type==SDL_EVENT_MOUSE_MOTION&&(event.motion.state&SDL_BUTTON_RMASK)!=0){
                    const auto p=coordinates(event.motion.x,event.motion.y);
                    town.camera_pointer(8,p[0],p[1],delta(pending_x,event.motion.xrel*static_cast<float>(context.width)/p[2]),
                        delta(pending_y,event.motion.yrel*static_cast<float>(context.height)/p[3]));
                }
            }
            if(!running)break;
            ++dispatch;town.advance_update(dispatch,pulse||(options.pulse_every>0&&dispatch%static_cast<std::uint64_t>(options.pulse_every)==0));
            submit();renderer.present();const auto next_title=town.window_title();
            if(title!=next_title){require(SDL_SetWindowTitle(window.get(),next_title.c_str()),"SDL town title");title=next_title;}
            const auto current=SDL_GetTicks();if(!options.hidden&&current<deadline)SDL_Delay(static_cast<Uint32>(deadline-current));
        }
        // A frame limit observes the same retained state as headless replay.
        // Only an actual close cancels live capture after the last dispatch.
        if(!running&&!context.input_replay)town.cancel_camera_pointer();
        if(dispatch==0)submit();
    }else submit();
    if(options.capture){
        auto pixels=renderer.read_pixels();
        std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> image(SDL_CreateSurfaceFrom(context.width,context.height,SDL_PIXELFORMAT_RGBA32,pixels.rgba.data(),context.width*4),SDL_DestroySurface);
        require(image!=nullptr,"SDL_CreateSurfaceFrom");require(SDL_SaveBMP(image.get(),options.capture->string().c_str()),"SDL_SaveBMP");
    }
}
void capture_town_background(TownBackground& town,const std::filesystem::path& output){
    TownWindowOptions options;options.capture=output;run_town_window(town,options);
}
} // namespace saga::desktop
