// Explicit AppData commands, not a mission bootstrap. Native GPU mode requires a supplied profile.
#include "bitmap_backend.hpp"
#include "bitmap_mesh_frame.hpp"
#include "gl_bitmap.hpp"
#include "mission_scene.hpp"
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
struct GpuQueries:MeshDepthHost {
    bool surface_is_null(MeshDepthSurface* p)override{return p==nullptr;}
    float low_depth_scaling()override{throw std::logic_error("capture has no low-depth scaling profile");}
};
struct SelectedBullet {std::int32_t id,damage,attribute,parent;ProjectilePoint position;};
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
    std::ifstream input(scene);if(!input)throw std::runtime_error("cannot open scene description");
    BitmapApplication application,global;BitmapTextures textures;std::vector<std::unique_ptr<Resource>> owners;
    SpriteMatrix world;bool has_world=false;RenderQueue queue=make_render_queue();std::size_t commands=0;
    OffscreenMatrix projection;bool has_projection=false,has_gpu_depth=false;std::int32_t depth_bits=0,depth_step=0;
    std::map<std::int32_t,SubmissionMaterial> materials;
    std::map<std::tuple<std::size_t,std::uint32_t,std::int32_t>,std::int32_t> material_bindings;
    std::unique_ptr<ResourceArchive> tables;std::unique_ptr<GameplayCatalog> catalog;MissionProgress progress;
    std::optional<StageGeometry> geometry;
    MissionView view{};bool has_view=false,has_slots=false,has_animation=false;std::int32_t slots=0,frames=0,automatic=0,application_update=0;
    std::optional<std::int32_t> tick_ms;
    std::map<std::string,std::int32_t> resource_names;
    std::vector<std::variant<HumanCreation,EnemyCreation,SelectedBullet>> selected;
    std::unique_ptr<MissionEntities> entity_world;std::unique_ptr<MissionScene> mission;
    for(std::string line;std::getline(input,line);){
        std::istringstream row(line);row.imbue(std::locale::classic());std::string operation;
        if(!(row>>operation)||operation.front()=='#')continue;
        if(operation=="world"){
            if(has_world)throw std::invalid_argument("duplicate world matrix");
            for(auto& v:*world.values)if(!(row>>v))throw std::invalid_argument("world requires nine binary32 values");
            has_world=true;
        }else if(operation=="projection"){
            if(has_projection||!native)throw std::invalid_argument("projection requires native mode and one matrix");
            for(auto& v:projection.values)if(!(row>>v))throw std::invalid_argument("projection requires sixteen binary32 values");has_projection=true;
        }else if(operation=="gpu_depth"){
            if(has_gpu_depth||!native||!(row>>depth_bits>>depth_step)||depth_bits<17||depth_bits>32||(depth_step!=0&&depth_step!=1))
                throw std::invalid_argument("gpu_depth requires native mode, depth bits17..32 and step0/1");
            has_gpu_depth=true;
        }else if(operation=="material"){
            std::int32_t texture=0,material_queue=0,id=0,uniform=0;std::uint32_t shader=0;
            if(!native||!(row>>texture>>shader>>material_queue>>id>>uniform)||texture<0||(uniform!=0&&uniform!=1))throw std::invalid_argument("material requires texture shader queue identity uniform0/1 in native mode");
            auto inserted=materials.emplace(id,SubmissionMaterial{});auto& value=inserted.first->second;
            if(!inserted.second&&(value.queue.absolute!=material_queue||value.uniform_color!=(uniform!=0)))throw std::invalid_argument("conflicting material identity");
            value.queue.absolute=material_queue;value.uniform_color=uniform!=0;
            if(!material_bindings.emplace(std::make_tuple(static_cast<std::size_t>(texture),shader,material_queue),id).second)throw std::invalid_argument("duplicate material input binding");
        }else if(operation=="tables"){
            std::string path,language;std::int32_t dpi=0;
            if(!native||tables||!(row>>std::quoted(path)>>std::quoted(language)>>dpi))throw std::invalid_argument("tables requires native mode and one quoted archive/language/DPI profile");
            auto resolved=std::filesystem::u8path(path);if(resolved.is_relative())resolved=scene.parent_path()/resolved;
            tables=std::make_unique<ResourceArchive>(read(resolved),ResourceProfile{language,dpi},ArchiveContents::data_only);
        }else if(operation=="progress"){
            std::int32_t id=0;SavedCharacterProgression saved;
            if(!native||!(row>>id>>saved.level>>saved.experience)||!progress.characters.emplace(id,saved).second)throw std::invalid_argument("progress requires unique character ID, saved level and EXP");
        }else if(operation=="purchase"){
            std::int32_t id=0,count=0;if(!native||!(row>>id>>count)||!progress.shop_purchases.emplace(id,count).second)throw std::invalid_argument("purchase requires unique shop ID and saved count");
        }else if(operation=="resource_name"){
            std::string name;std::int32_t id=0;if(!native||!(row>>std::quoted(name)>>id)||!resource_names.emplace(name,id).second)throw std::invalid_argument("resource_name requires unique quoted name and explicit AppData ID");
        }else if(operation=="geometry"){
            StageGeometry g;if(!native||geometry||!(row>>g.step_x>>g.step_z>>g.height))throw std::invalid_argument("geometry requires step X/Z and height in native mode");geometry=g;
        }else if(operation=="view"){
            if(!native||has_view||!(row>>view.width>>view.height>>view.scale>>view.camera_x>>view.camera_y>>view.margin_x>>view.margin_y>>view.extra_depth_rate))throw std::invalid_argument("view requires eight int32 values in native mode");
            has_view=true;
        }else if(operation=="slots"){
            if(!native||has_slots||!(row>>slots)||slots<1||slots>1024)throw std::invalid_argument("slots requires one component count1..1024 in native mode");has_slots=true;
        }else if(operation=="tick_ms"){
            std::int32_t value=0;if(!native||tick_ms||!(row>>value)||value<1||value>1000)throw std::invalid_argument("tick_ms requires one diagnostic interval1..1000");tick_ms=value;
        }else if(operation=="animate"){
            if(!native||has_animation||!(row>>frames>>automatic>>application_update)||frames<0||frames>10000||(automatic!=0&&automatic!=1)||(application_update!=0&&application_update!=1))throw std::invalid_argument("animate requires count0..10000 and two flag0/1 values");
            has_animation=true;
        }else if(operation=="human"){
            HumanCreation h;if(!native||!(row>>h.direction>>h.character>>h.level>>h.formation_x>>h.formation_z>>h.experience>>h.x>>h.y>>h.z))throw std::invalid_argument("human requires six int32 and XYZ binary32 inputs");selected.emplace_back(h);
        }else if(operation=="enemy"){
            EnemyCreation e;if(!native||!(row>>e.direction>>e.id>>e.level>>e.flags>>e.position.x>>e.position.y>>e.position.z))throw std::invalid_argument("enemy requires direction ID level flags and XYZ");selected.emplace_back(e);
        }else if(operation=="bullet"){
            SelectedBullet b;if(!native||!(row>>b.id>>b.damage>>b.attribute>>b.parent>>b.position.x>>b.position.y>>b.position.z))throw std::invalid_argument("bullet requires ID damage attribute parent ordinal and XYZ");selected.emplace_back(b);
        }else if(operation=="global_alias"){
            std::int32_t target=0,source=0;if(!(row>>target>>source))throw std::invalid_argument("global_alias requires target/source AppData IDs");
            auto** slot=application_resource_slot(global,target);auto* value=get_application_resource(application,source);
            if(!slot||*slot||!value)throw std::invalid_argument("global alias needs an empty supported target and ready source");*slot=value;
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
    if(!selected.empty()){
        if(!tables||!has_view||!has_slots||!has_animation||commands)throw std::invalid_argument("entity capture needs tables/view/slots/animate and no manual draw rows");
        catalog=std::make_unique<GameplayCatalog>(load_gameplay_catalog(*tables,false,parse_event_decimal));
        entity_world=std::make_unique<MissionEntities>(*catalog,progress,slots,geometry?&*geometry:nullptr,[&](std::int32_t id){auto* r=get_application_resource(application,id);return r?r->animations:nullptr;},
            [&](std::string_view name){auto it=resource_names.find(std::string(name));if(it==resource_names.end())throw std::invalid_argument("unbound enemy resource name: "+std::string(name));return it->second;});
        std::vector<ComponentSlots*> created;
        for(const auto& request:selected){ComponentSlots* entity=nullptr;
            if(const auto* h=std::get_if<HumanCreation>(&request))entity=&entity_world->human(*h);
            else if(const auto* e=std::get_if<EnemyCreation>(&request))entity=entity_world->enemy(*e);
            else{const auto& b=std::get<SelectedBullet>(request);ComponentSlots* parent=nullptr;
                if(b.parent!=-1)parent=created.at(static_cast<std::size_t>(b.parent));
                entity=&entity_world->bullet({b.id,b.damage,b.attribute,parent,b.position});}
            if(!entity)throw std::invalid_argument("selected entity record is unavailable");created.push_back(entity);
        }
        mission=std::make_unique<MissionScene>(*entity_world,application,*entity_world->subset({0,2}),view,nullptr);
        for(std::int32_t i=0;i<frames;++i)mission->advance_animations({automatic!=0,application_update!=0});
        queue=mission->render();if(!mission->faults().empty())throw std::invalid_argument("entity render fault: "+mission->faults().front().message);
        commands=static_cast<std::size_t>(queue.counts->at(0))+static_cast<std::size_t>(queue.counts->at(1));
    }
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
                if(!paused||single_step){mission->advance_animations({automatic!=0,application_update!=0});queue=mission->render();
                    if(!mission->faults().empty())throw std::invalid_argument("entity render fault: "+mission->faults().front().message);++logic_steps;}
                submit();renderer.present();++rendered;const auto now=SDL_GetTicks();if(now<deadline)SDL_Delay(static_cast<Uint32>(deadline-now));
            }
            std::cout<<"{\"entities\":"<<selected.size()<<",\"render_frames\":"<<rendered<<",\"logic_steps\":"<<logic_steps
                     <<",\"profile\":\"selected entities and animation passes; gameplay input and full schedule unbound\"}\n";return 0;
        }
        submit();auto pixels=renderer.read_pixels();
        Surface captured(SDL_CreateSurfaceFrom(width,height,SDL_PIXELFORMAT_RGBA32,pixels.rgba.data(),width*4),SDL_DestroySurface);require(captured!=nullptr,"SDL_CreateSurfaceFrom");
        require(SDL_SaveBMP(captured.get(),output.string().c_str()),"SDL_SaveBMP");std::set<std::size_t> used;for(const auto& q:backend.quads())used.insert(q.texture);
        std::cout<<"{\"resources\":"<<owners.size()<<",\"entities\":"<<selected.size()<<",\"commands\":"<<commands<<",\"quads\":"<<backend.quads().size()<<",\"textures\":"<<used.size()
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
