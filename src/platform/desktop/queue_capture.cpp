// Explicit AppData commands, not a mission bootstrap. Native GPU mode requires a supplied profile.
#include "bitmap_backend.hpp"
#include "bitmap_mesh_frame.hpp"
#include "mission_window.hpp"
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

}
int main(int argc,char** argv)try{
    std::optional<std::filesystem::path> replay;
    if(argc>=3&&std::string_view(argv[argc-2])=="--replay"){replay=std::filesystem::u8path(argv[argc-1]);argc-=2;}
    if(argc!=5&&argc!=6){std::cerr<<"Usage: queue_capture scene.txt width height new-output.bmp [--native-depth] [--replay inputs.txt]\n       queue_capture scene.txt width height --material-inputs|--interactive [--replay inputs.txt]\n       queue_capture scene.txt width height --run-frames count [--replay inputs.txt]\n";return 2;}
    const bool inspect=argc==5&&std::string_view(argv[4])=="--material-inputs";
    const bool interactive=argc==5&&std::string_view(argv[4])=="--interactive";
    const bool bounded=argc==6&&std::string_view(argv[4])=="--run-frames";const bool loop=interactive||bounded;
    const auto frame_limit=bounded?number(argv[5]):0;if(bounded&&(frame_limit<1||frame_limit>100000))throw std::invalid_argument("run frame count1..100000 required");
    const bool native=inspect||loop||argc==6;if(argc==6&&!bounded&&std::string_view(argv[5])!="--native-depth")throw std::invalid_argument("unknown capture option");
    const auto width=number(argv[2]),height=number(argv[3]);
    if(width<1||height<1||width>8192||height>8192||static_cast<std::uint64_t>(width)*height>16U*1024U*1024U)throw std::invalid_argument("invalid capture dimensions");
    const auto scene=std::filesystem::u8path(argv[1]),output=inspect||loop?std::filesystem::path{}:std::filesystem::u8path(argv[4]);
    if(!inspect&&!loop&&std::filesystem::exists(output))throw std::invalid_argument("capture destination already exists");
    MissionProfile profile(scene,native,decode_png,std::nullopt,replay);
    if(native&&!inspect)return run_native_mission(profile,width,height,bounded?std::optional<std::int32_t>(frame_limit):std::nullopt,
        loop?std::nullopt:std::optional<std::filesystem::path>(output));
    auto& application=profile.application;
    auto& global=profile.global;
    auto& textures=profile.textures;
    auto& owners=profile.owners;
    auto& world=profile.world;
    auto& queue=profile.queue;
    auto& commands=profile.commands;
    BitmapBackend backend(application,global,textures,height,world);sort_render_queue(queue);
    if(inspect){
        RenderDrawState drawing{&queue,0,&backend.matrix()};draw_render_queue(drawing,&backend.graphics(),0,backend.resources());profile.draw_enemy_gauges(backend);draw_render_queue(drawing,&backend.graphics(),1,backend.resources());
        std::set<std::tuple<std::size_t,std::uint32_t,std::int32_t>> inputs;for(const auto& q:backend.quads())inputs.emplace(q.texture,q.shader_key,q.render_queue);
        std::cout<<"{\"material_inputs\":[";bool first=true;
        for(const auto& key:inputs){if(!first)std::cout<<',';first=false;std::cout<<"{\"texture\":"<<std::get<0>(key)<<",\"shader\":"<<std::get<1>(key)<<",\"queue\":"<<std::get<2>(key)<<'}';}
        std::cout<<"]}\n";return 0;
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
