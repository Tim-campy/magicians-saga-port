// Explicit AppData commands, not a mission bootstrap or native depth buffer.
#include "bitmap_backend.hpp"
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
#include <set>
#include <sstream>

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
std::vector<std::uint8_t> read(const std::filesystem::path& path){
    std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("cannot open decoded archive");
    const auto size=in.tellg();if(size<8||size>static_cast<std::streamoff>(saga::assets::JarLimits{}.max_bytes))throw std::length_error("decoded archive size outside limits");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));in.seekg(0);in.read(reinterpret_cast<char*>(bytes.data()),size);
    if(!in)throw std::runtime_error("incomplete archive read");return bytes;
}
struct Resource {
    ResourceArchive archive;ArchiveResourceManager manager;ArchiveImageSet images;
    Resource(const std::filesystem::path& path,ResourceProfile profile):archive(read(path),std::move(profile)),manager(archive),images(manager,decode_png){}
};
void end(std::istringstream& row){row>>std::ws;if(!row.eof())throw std::invalid_argument("unexpected scene row fields");}
}
int main(int argc,char** argv)try{
    if(argc!=5){std::cerr<<"Usage: queue_capture scene.txt width height new-output.bmp\n";return 2;}
    const auto width=number(argv[2]),height=number(argv[3]);
    if(width<1||height<1||width>8192||height>8192||static_cast<std::uint64_t>(width)*height>16U*1024U*1024U)throw std::invalid_argument("invalid capture dimensions");
    const auto scene=std::filesystem::u8path(argv[1]),output=std::filesystem::u8path(argv[4]);
    if(std::filesystem::exists(output))throw std::invalid_argument("capture destination already exists");
    std::ifstream input(scene);if(!input)throw std::runtime_error("cannot open scene description");
    BitmapApplication application,global;BitmapTextures textures;std::vector<std::unique_ptr<Resource>> owners;
    SpriteMatrix world;bool has_world=false;RenderQueue queue=make_render_queue();std::size_t commands=0;
    for(std::string line;std::getline(input,line);){
        std::istringstream row(line);row.imbue(std::locale::classic());std::string operation;
        if(!(row>>operation)||operation.front()=='#')continue;
        if(operation=="world"){
            if(has_world)throw std::invalid_argument("duplicate world matrix");
            for(auto& v:*world.values)if(!(row>>v))throw std::invalid_argument("world requires nine binary32 values");
            has_world=true;
        }else if(operation=="resource"||operation=="global"){
            int id=0,dpi=0;std::string path,language;
            if(!(row>>id>>std::quoted(path)>>std::quoted(language)>>dpi))throw std::invalid_argument("resource requires id, quoted archive, quoted language and DPI");
            auto** slot=application_resource_slot(operation=="global"?global:application,id);
            if(!slot||*slot)throw std::invalid_argument("unsupported or duplicate AppData resource ID");
            auto resolved=std::filesystem::u8path(path);if(resolved.is_relative())resolved=scene.parent_path()/resolved;
            auto value=std::make_unique<Resource>(resolved,ResourceProfile{language,dpi});textures.bind(value->images);
            *slot=&value->manager.drawing();owners.push_back(std::move(value));
        }else if(operation=="draw"){
            if(commands>=100000)throw std::length_error("capture command budget exceeded");
            RenderCommand c;for(auto& v:c.values)if(!(row>>v))throw std::invalid_argument("draw requires nineteen int32 fields");
            insert_render_command(queue,c);++commands;
        }else throw std::invalid_argument("unknown scene row type");
        end(row);
    }
    if(!has_world)throw std::invalid_argument("an explicit world matrix is required");
    BitmapBackend backend(application,global,textures,height,world);sort_render_queue(queue);
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
