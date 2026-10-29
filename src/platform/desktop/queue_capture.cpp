// Explicit AppData commands, not a mission bootstrap. Native GPU mode requires a supplied profile.
#include "bitmap_backend.hpp"
#include "bitmap_mesh_frame.hpp"
#include "gl_bitmap.hpp"
#include "mission_profile.hpp"
#include "assets/jar_archive.hpp"
#include "sdl_bitmap.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <limits>
#include <set>
#include <sstream>
#include <tuple>
#include <variant>

namespace {
using namespace saga::desktop;
using namespace saga::core;
using Texture=std::unique_ptr<SDL_Texture,decltype(&SDL_DestroyTexture)>;
using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)>;
using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
using Renderer=std::unique_ptr<SDL_Renderer,decltype(&SDL_DestroyRenderer)>;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(std::string(message)+": "+SDL_GetError());}
struct Quit {~Quit(){SDL_Quit();}};
int number(std::string_view s){int result=0;const auto p=std::from_chars(s.data(),s.data()+s.size(),result);
    if(p.ec!=std::errc{}||p.ptr!=s.data()+s.size())throw std::invalid_argument("invalid int32 argument");return result;}
struct GpuQueries:MeshDepthHost {
    bool surface_is_null(MeshDepthSurface* p)override{return p==nullptr;}
    float low_depth_scaling()override{throw std::logic_error("capture has no low-depth scaling profile");}
};

}
int main(int argc,char** argv)try{
    if(argc!=5&&argc!=6){std::cerr<<"Usage: queue_capture scene.txt width height new-output.bmp [--native-depth]\n       queue_capture scene.txt width height --material-inputs|--interactive\n       queue_capture scene.txt width height --run-frames count\n";return 2;}
    const bool inspect=argc==5&&std::string_view(argv[4])=="--material-inputs";
    const bool interactive=argc==5&&std::string_view(argv[4])=="--interactive";
    const bool bounded=argc==6&&std::string_view(argv[4])=="--run-frames";const bool loop=interactive||bounded;
    const auto frame_limit=bounded?number(argv[5]):0;if(bounded&&(frame_limit<1||frame_limit>100000))throw std::invalid_argument("run frame count1..100000 required");
    const bool native=inspect||loop||argc==6;if(argc==6&&!bounded&&std::string_view(argv[5])!="--native-depth")throw std::invalid_argument("unknown capture option");
    const auto width=number(argv[2]),height=number(argv[3]);
    if(width<1||height<1||width>8192||height>8192||static_cast<std::uint64_t>(width)*height>16U*1024U*1024U)throw std::invalid_argument("invalid capture dimensions");
    const auto scene=std::filesystem::u8path(argv[1]),output=inspect||loop?std::filesystem::path{}:std::filesystem::u8path(argv[4]);
    if(!inspect&&!loop&&std::filesystem::exists(output))throw std::invalid_argument("capture destination already exists");
    MissionProfile profile(scene,native,decode_png);
    auto& application=profile.application;
    auto& global=profile.global;
    auto& textures=profile.textures;
    auto& owners=profile.owners;
    auto& world=profile.world;
    auto& queue=profile.queue;
    auto& commands=profile.commands;
    auto& projection=profile.projection;
    auto& has_projection=profile.has_projection;
    auto& has_gpu_depth=profile.has_gpu_depth;
    auto& depth_bits=profile.depth_bits;
    auto& depth_step=profile.depth_step;
    auto& materials=profile.materials;
    auto& material_bindings=profile.material_bindings;
    auto& simulate=profile.simulate;
    auto& created=profile.created;
    auto& mission=profile.mission;
    auto& tick_ms=profile.tick_ms;
    auto& entity_world=profile.entity_world;
    BitmapBackend backend(application,global,textures,height,world);sort_render_queue(queue);
    if(inspect){
        RenderDrawState drawing{&queue,0,&backend.matrix()};draw_render_queue(drawing,&backend.graphics(),0,backend.resources());draw_render_queue(drawing,&backend.graphics(),1,backend.resources());
        std::set<std::tuple<std::size_t,std::uint32_t,std::int32_t>> inputs;for(const auto& q:backend.quads())inputs.emplace(q.texture,q.shader_key,q.render_queue);
        std::cout<<"{\"material_inputs\":[";bool first=true;
        for(const auto& key:inputs){if(!first)std::cout<<',';first=false;std::cout<<"{\"texture\":"<<std::get<0>(key)<<",\"shader\":"<<std::get<1>(key)<<",\"queue\":"<<std::get<2>(key)<<'}';}
        std::cout<<"]}\n";return 0;
    }
    if(native){
        if(!has_projection||!has_gpu_depth)throw std::invalid_argument("native capture needs projection and gpu_depth rows");
        if(loop&&(!mission||!tick_ms))throw std::invalid_argument("scene loop requires selected entities and explicit tick_ms");
        require(SDL_Init(SDL_INIT_VIDEO),"SDL_Init");Quit quit;configure_gl_bitmap_context();
        Window window(SDL_CreateWindow(loop?"Mission scene preview - Space pause, N step, Esc close":"Native AppData capture",width,height,
            SDL_WINDOW_OPENGL|(interactive?0:SDL_WINDOW_HIDDEN)),SDL_DestroyWindow);require(window!=nullptr,"SDL_CreateWindow");
        GlBitmapRenderer renderer(*window,width,height,[&](std::size_t key){const auto* ready=textures.at(key).ready;
            if(!ready)throw std::invalid_argument("texture has no decoded archive image");return GlBitmapTexture{&ready->pixels,GlBitmapWrap::clamp,GlBitmapWrap::clamp};});
        MeshDepthSurface surface;MeshDepthState depth;depth.surface=&surface;GpuQueries queries;
        BitmapMeshFrame frame(backend,depth,queries,{depth_bits,false,[&]{return depth_step!=0;}},[&](auto t,auto shader,auto queue_value)->SubmissionMaterial&{
            const auto found=material_bindings.find({t,shader,queue_value});
            if(found==material_bindings.end())throw std::invalid_argument("unbound material: texture="+std::to_string(t)+" shader="+std::to_string(shader)+" queue="+std::to_string(queue_value));
            return materials.at(found->second);
        });
        GlMeshBindings bindings;
        MeshSubmissionState s;s.cursors=std::make_shared<std::vector<std::int32_t>>(1,0);s.lengths=std::make_shared<std::vector<std::int32_t>>(1,0);
        s.lists=std::make_shared<std::vector<SubmissionRecords>>(1,frame.records().rows);OffscreenMatrix unit;unit.values[0]=unit.values[5]=unit.values[10]=unit.values[15]=1;
        s.matrices=std::make_shared<std::vector<OffscreenMatrix>>(1,unit);s.used_materials=std::make_shared<std::vector<SubmissionMaterial*>>();
        GlMeshSubmissionHost host(renderer,bindings,[](auto&,auto&){},[]()->std::int32_t{throw std::logic_error("capture has no image counter");});
        auto submit=[&]{
            frame.begin_frame();frame.draw(queue,0);frame.draw(queue,1);frame.finish();bind_bitmap_mesh_frame(bindings,frame);
            s.cursors->at(0)=0;s.lengths->at(0)=frame.records().active_count;
            renderer.begin_frame(GlBitmapClear::color_and_depth,{0,0,0,0},projection);submit_mesh_streams(s,1,host);
        };
        if(loop){bool running=true,paused=false;std::uint64_t rendered=0,logic_steps=0;
            while(running&&(!bounded||rendered<static_cast<std::uint64_t>(frame_limit))){const auto deadline=SDL_GetTicks()+static_cast<Uint64>(*tick_ms);bool single_step=false;SDL_Event event;
                while(SDL_PollEvent(&event)){
                    if(event.type==SDL_EVENT_QUIT)running=false;
                    else if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){
                        if(event.key.key==SDLK_ESCAPE)running=false;
                        else if(event.key.key==SDLK_SPACE)paused=!paused;
                        else if(event.key.key==SDLK_N)single_step=true;
                    }
                }
                if(!running)break;
                if(!paused||single_step){profile.step();++logic_steps;}
                submit();renderer.present();++rendered;const auto now=SDL_GetTicks();if(now<deadline)SDL_Delay(static_cast<Uint32>(deadline-now));
            }
            std::cout<<"{\"entities\":"<<profile.selected_count<<",\"render_frames\":"<<rendered<<",\"logic_steps\":"<<logic_steps
                     <<",\"movement_steps\":"<<(simulate?logic_steps+static_cast<std::uint64_t>(*simulate):0)<<",\"rng_draws\":"<<(profile.humans?profile.humans->random_draws():0)<<",\"positions\":[";
            std::cout<<std::setprecision(std::numeric_limits<float>::max_digits10);
            for(std::size_t i=0;i<created.size();++i){if(i)std::cout<<',';const auto* p=dynamic_cast<ContactPosition*>(created[i]->get(0));
                const bool dead=entity_world->destroyed(*created[i]);
                std::cout<<"{\"id\":"<<entity_world->id(*created[i])<<",\"destroyed\":"<<(dead?"true":"false");
                if(p)std::cout<<",\"x\":"<<p->x<<",\"y\":"<<p->y<<",\"z\":"<<p->z;
                else if(!dead)throw std::logic_error("selected live entity has no position");
                if(const auto* h=dynamic_cast<HumanRecordLink*>(created[i]->get(14)))std::cout<<",\"human_state\":"<<h->state<<",\"human_frame\":"<<h->frame<<",\"attack_frame\":"<<h->attack_frame;
                std::cout<<'}';}
            std::cout<<"],\"profile\":\"selected movement/animation/render/garbage passes; gameplay input and full schedule unbound\"}\n";return 0;
        }
        submit();auto pixels=renderer.read_pixels();
        Surface captured(SDL_CreateSurfaceFrom(width,height,SDL_PIXELFORMAT_RGBA32,pixels.rgba.data(),width*4),SDL_DestroySurface);require(captured!=nullptr,"SDL_CreateSurfaceFrom");
        require(SDL_SaveBMP(captured.get(),output.string().c_str()),"SDL_SaveBMP");std::set<std::size_t> used;for(const auto& q:backend.quads())used.insert(q.texture);
        std::cout<<"{\"resources\":"<<owners.size()<<",\"entities\":"<<profile.selected_count<<",\"commands\":"<<commands<<",\"quads\":"<<backend.quads().size()<<",\"textures\":"<<used.size()
                 <<",\"profile\":\"explicit projection and material identities, native depth\"}\n";return 0;
    }
    RenderDrawState drawing{&queue,0,&backend.matrix()};draw_render_queue(drawing,&backend.graphics(),0,backend.resources());
    auto quads=backend.quads();std::stable_sort(quads.begin(),quads.end(),[](const auto& a,const auto& b){return a.render_queue<b.render_queue;});
    require(SDL_Init(SDL_INIT_VIDEO),"SDL_Init");Quit quit;
    Window window(SDL_CreateWindow("AppData queue capture",width,height,SDL_WINDOW_HIDDEN),SDL_DestroyWindow);require(window!=nullptr,"SDL_CreateWindow");
    Renderer renderer(SDL_CreateRenderer(window.get(),nullptr),SDL_DestroyRenderer);require(renderer!=nullptr,"SDL_CreateRenderer");
    if(std::string(SDL_GetRendererName(renderer.get()))=="software")throw std::runtime_error("queue capture requires a floating-UV graphics renderer");
    Texture canvas(SDL_CreateTexture(renderer.get(),SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_TARGET,width,height),SDL_DestroyTexture);require(canvas!=nullptr,"SDL_CreateTexture");
    require(SDL_SetRenderTarget(renderer.get(),canvas.get()),"SDL_SetRenderTarget");
    require(SDL_SetRenderDrawColor(renderer.get(),0,0,0,0),"SDL_SetRenderDrawColor");require(SDL_RenderClear(renderer.get()),"SDL_RenderClear");
    SdlBitmapTextures cache(renderer.get(),[&](std::size_t key)->const DecodedImage&{
        const auto* ready=textures.at(key).ready;if(!ready)throw std::invalid_argument("texture has no decoded archive image");return ready->pixels;
    });
    std::set<std::size_t> used;
    for(const auto& q:quads){used.insert(q.texture);require(render_preview_quad(renderer.get(),cache.select(q),q,static_cast<float>(height)),"SDL_RenderGeometry");}
    Surface captured(SDL_RenderReadPixels(renderer.get(),nullptr),SDL_DestroySurface);require(captured!=nullptr,"SDL_RenderReadPixels");
    require(SDL_SaveBMP(captured.get(),output.string().c_str()),"SDL_SaveBMP");
    std::cout<<"{\"resources\":"<<owners.size()<<",\"commands\":"<<commands<<",\"quads\":"<<quads.size()<<",\"textures\":"<<used.size()
             <<",\"profile\":\"explicit world and commands, material queue order, no native depth buffer\"}\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"queue_capture: "<<e.what()<<'\n';return 1;}
