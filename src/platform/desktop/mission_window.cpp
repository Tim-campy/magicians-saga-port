// Shared native mission window, input, replay and capture frontend.
#include "mission_window.hpp"
#include "bitmap_backend.hpp"
#include "bitmap_mesh_frame.hpp"
#include "gl_bitmap.hpp"
#include "mission_profile.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <string_view>
#include <tuple>

namespace {
using namespace saga::desktop;
using namespace saga::core;
using Surface=std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)>;
using Window=std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)>;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(std::string(message)+": "+SDL_GetError());}
struct Quit {~Quit(){SDL_Quit();}};
struct GpuQueries:MeshDepthHost {
    bool surface_is_null(MeshDepthSurface* p)override{return p==nullptr;}
    float low_depth_scaling()override{throw std::logic_error("capture has no low-depth scaling profile");}
};

}
namespace saga::desktop {
int run_native_mission(MissionProfile& profile,std::int32_t width,std::int32_t height,
    std::optional<std::int32_t> frames,std::optional<std::filesystem::path> capture){
    if(width<1||height<1||width>8192||height>8192||static_cast<std::uint64_t>(width)*height>16U*1024U*1024U)throw std::invalid_argument("invalid mission dimensions");
    if(frames&&(*frames<1||*frames>100000))throw std::invalid_argument("run frame count1..100000 required");
    if(capture&&std::filesystem::exists(*capture))throw std::invalid_argument("capture destination already exists");
    const bool loop=!capture||frames.has_value(),bounded=frames.has_value(),interactive=loop&&!bounded;
    const bool live_viewport=interactive&&profile.accepts_live_viewport();
    const auto frame_limit=frames.value_or(0);
    const auto output=capture.value_or(std::filesystem::path{});
    if(profile.resizable_viewport()&&(profile.view.width!=width||profile.view.height!=height)){
        if(!loop)throw std::invalid_argument("mission_presentation capture dimensions must match the prepared viewport");
        profile.resize_viewport(width,height); // Explicit frontend size; native Camera/Cell/Render follow on the first step.
    }
    auto& application=profile.application;
    auto& global=profile.global;
    auto& textures=profile.textures;
    auto& owners=profile.owners;
    auto& world=profile.world;
    auto& queue=profile.queue;
    auto& projection=profile.projection;
    auto& has_projection=profile.has_projection;
    auto& has_gpu_depth=profile.has_gpu_depth;
    auto& depth_bits=profile.depth_bits;
    auto& depth_step=profile.depth_step;
    auto& materials=profile.materials;
    auto& material_bindings=profile.material_bindings;
    auto& created=profile.created;
    auto& mission=profile.mission;
    auto& tick_ms=profile.tick_ms;
    auto& entity_world=profile.entity_world;
    BitmapBackend backend(application,global,textures,height,world);sort_render_queue(queue);
    if(!has_projection||!has_gpu_depth)throw std::invalid_argument("native capture needs projection and gpu_depth rows");
    if(loop&&(!mission||(!capture&&!tick_ms)))throw std::invalid_argument("scene loop requires selected entities and explicit tick_ms");
    require(SDL_Init(SDL_INIT_VIDEO),"SDL_Init");Quit quit;configure_gl_bitmap_context();
    const char* input_title=profile.native_stage_controls()?"Mission - W/S move, 1/2/3/F/click attack, P mission pause, Enter result, Space freeze, N step, Esc close":
        profile.humans&&profile.humans->stage_input_enabled()?"Mission - W/S move, 1/2/3/F/click attack, Enter result, Space freeze, N step, Esc close":"Mission - W/S move, 1/2/3/F/click attack, Space freeze, N step, Esc close";
    const char* title=loop?(profile.accepts_live_input()?input_title:"Mission preview - Space pause, N step, Esc close"):"Native AppData capture";
    Window window(SDL_CreateWindow(title,width,height,
        SDL_WINDOW_OPENGL|(interactive?0:SDL_WINDOW_HIDDEN)|(live_viewport?SDL_WINDOW_RESIZABLE:0)),SDL_DestroyWindow);require(window!=nullptr,"SDL_CreateWindow");
    GlBitmapRenderer renderer(*window,width,height,[&](std::size_t key){const auto* ready=textures.at(key).ready;
        if(!ready)throw std::invalid_argument("texture has no decoded archive image");return GlBitmapTexture{&ready->pixels,GlBitmapWrap::clamp,GlBitmapWrap::clamp};});
    MeshDepthSurface surface;MeshDepthState depth;depth.surface=&surface;GpuQueries queries;
    BitmapMaterials native_materials;
    BitmapMeshFrame frame(backend,depth,queries,{depth_bits,false,[&]{return depth_step!=0;}},[&](auto t,auto shader,auto queue_value)->SubmissionMaterial&{
        if(profile.native_materials)return native_materials.resolve(t,shader,queue_value);
        const auto found=material_bindings.find({t,shader,queue_value});
        if(found==material_bindings.end())throw std::invalid_argument("unbound material: texture="+std::to_string(t)+" shader="+std::to_string(shader)+" queue="+std::to_string(queue_value));
        return materials.at(found->second);
    });
    GlMeshBindings bindings;
    MeshSubmissionState s;s.cursors=std::make_shared<std::vector<std::int32_t>>(1,0);s.lengths=std::make_shared<std::vector<std::int32_t>>(1,0);
    s.lists=std::make_shared<std::vector<SubmissionRecords>>(1,frame.records().rows);OffscreenMatrix unit;unit.values[0]=unit.values[5]=unit.values[10]=unit.values[15]=1;
    s.matrices=std::make_shared<std::vector<OffscreenMatrix>>(1,unit);s.used_materials=std::make_shared<std::vector<SubmissionMaterial*>>();
    GlMeshSubmissionHost host(renderer,bindings,[](auto&,auto&){},[&]()->std::int32_t{
        if(profile.native_materials)return 0; // SAGA_STUB(mission-image-access-check): no eviction/aging service is bound.
        throw std::logic_error("capture has no image counter");
    });
    const auto resize=[&](std::int32_t w,std::int32_t h){
        if(!profile.resizable_viewport()||w<1||h<2||(w==width&&h==height))return;
        profile.resize_viewport(w,h);renderer.resize_target(w,h);backend.resize_surface(h);width=w;height=h;
    };
    if(live_viewport){
        int w=0,h=0;require(SDL_GetWindowSizeInPixels(window.get(),&w,&h),"SDL window pixel size");resize(w,h);
    }
    auto submit=[&]{
        frame.begin_frame();frame.draw(queue,0);profile.draw_enemy_gauges(backend);frame.draw(queue,1);frame.finish();bind_bitmap_mesh_frame(bindings,frame);
        s.cursors->at(0)=0;s.lengths->at(0)=frame.records().active_count;
        renderer.begin_frame(GlBitmapClear::color_and_depth,{0,0,0,0},projection);submit_mesh_streams(s,1,host);
    };
    std::uint64_t rendered=0,logic_steps=0;
    if(loop){bool running=true,paused=false;std::string_view current_title=title;
        while(running&&(!bounded||rendered<static_cast<std::uint64_t>(frame_limit))){const auto deadline=SDL_GetTicks()+static_cast<Uint64>(capture?0:*tick_ms);bool single_step=false;SDL_Event event;
            while(SDL_PollEvent(&event)){
                if(event.type==SDL_EVENT_QUIT)running=false;
                else if(event.type==SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED&&event.window.windowID==SDL_GetWindowID(window.get())&&live_viewport){
                    int w=0,h=0;require(SDL_GetWindowSizeInPixels(window.get(),&w,&h),"SDL live pixel size");resize(w,h);
                }
                else if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){
                    if(event.key.key==SDLK_ESCAPE)running=false;
                    else if(event.key.key==SDLK_SPACE)paused=!paused;
                    else if(interactive&&profile.accepts_live_input()&&profile.humans->exit_dialog_pending()&&(event.key.key==SDLK_Y||event.key.key==SDLK_N))
                        profile.humans->answer_stage_dialog(event.key.key==SDLK_Y?0:1);
                    else if(event.key.key==SDLK_N)single_step=true;
                    else if(interactive&&profile.accepts_live_input()){
                        if(event.key.key>=SDLK_1&&event.key.key<=SDLK_3)profile.humans->select_input_attack(static_cast<std::int32_t>(event.key.key-SDLK_1));
                        else if(event.key.key==SDLK_F)profile.humans->select_input_attack(profile.humans->selected_attack());
                        else if(event.key.key==SDLK_RETURN&&profile.humans->stage_input_enabled())profile.humans->pulse_stage_keys(0x100000U);
                        else if(profile.native_stage_controls()&&!profile.humans->exit_dialog_pending()){
                            const auto state=profile.humans->stage_state();
                            if(event.key.key==SDLK_P&&(state==1||state==2))profile.humans->pulse_stage_keys(state==1?0x2000000U:0x4000000U);
                            else if(event.key.key==SDLK_Q&&state==2)profile.humans->pulse_stage_keys(0x8000000U);
                        }
                    }
                }
                else if(interactive&&profile.accepts_live_input()&&event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_LEFT)
                    profile.humans->select_input_attack(profile.humans->selected_attack());
            }
            if(!running)break;
            if(interactive&&profile.accepts_live_input()){
                const auto* keys=SDL_GetKeyboardState(nullptr);const bool focused=SDL_GetKeyboardFocus()==window.get();
                profile.humans->set_input_movement(focused&&(keys[SDL_SCANCODE_W]||keys[SDL_SCANCODE_UP]),focused&&(keys[SDL_SCANCODE_S]||keys[SDL_SCANCODE_DOWN]));
                if(profile.accepts_fast_forward())profile.humans->set_input_fast_forward(focused&&keys[SDL_SCANCODE_TAB]);
            }
            if(!paused||single_step){if(profile.step())++logic_steps;else{paused=true;if(bounded)break;}}
            if(interactive&&profile.accepts_live_input()&&profile.native_stage_controls()){
                const char* next=profile.humans->exit_dialog_pending()?"Retreat from mission? Y confirms, N cancels":
                    profile.humans->stage_state()==2?"Mission paused - P resumes, Q requests retreat":input_title;
                if(next!=current_title){require(SDL_SetWindowTitle(window.get(),next),"SDL mission state title");current_title=next;}
            }
            if(profile.resizable_viewport()&&(profile.view.width!=width||profile.view.height!=height)){
                require(SDL_SetWindowSize(window.get(),profile.view.width,profile.view.height),"SDL replay window size");
                resize(profile.view.width,profile.view.height);
            }
            submit();renderer.present();++rendered;const auto now=SDL_GetTicks();if(now<deadline)SDL_Delay(static_cast<Uint32>(deadline-now));
        }
        if(!capture){std::cout<<"{\"entities\":"<<profile.selected_count<<",\"render_frames\":"<<rendered<<",\"logic_steps\":"<<logic_steps
                 <<",\"rng_draws\":"<<(profile.humans?profile.humans->random_draws():0)<<",\"positions\":[";
        std::cout<<std::setprecision(std::numeric_limits<float>::max_digits10);
        for(std::size_t i=0;i<created.size();++i){if(i)std::cout<<',';const auto* p=dynamic_cast<ContactPosition*>(created[i]->get(0));
            const bool dead=entity_world->destroyed(*created[i]);
            std::cout<<"{\"id\":"<<entity_world->id(*created[i])<<",\"destroyed\":"<<(dead?"true":"false");
            if(p)std::cout<<",\"x\":"<<p->x<<",\"y\":"<<p->y<<",\"z\":"<<p->z;
            else if(!dead)throw std::logic_error("selected live entity has no position");
            if(const auto* h=dynamic_cast<HumanRecordLink*>(created[i]->get(14)))std::cout<<",\"human_state\":"<<h->state<<",\"human_frame\":"<<h->frame<<",\"attack_frame\":"<<h->attack_frame;
            profile.write_actor_parameters(*created[i],std::cout);
            std::cout<<'}';}
        std::cout<<']';profile.write_combat_report(std::cout);
        std::cout<<",\"profile\":\"selected mission systems; fresh-world init, quest bootstrap and complete-state hashes unbound\"}\n";return 0;}
    }
    if(!loop||rendered==0)submit();auto pixels=renderer.read_pixels();
    Surface captured(SDL_CreateSurfaceFrom(width,height,SDL_PIXELFORMAT_RGBA32,pixels.rgba.data(),width*4),SDL_DestroySurface);require(captured!=nullptr,"SDL_CreateSurfaceFrom");
    require(SDL_SaveBMP(captured.get(),output.string().c_str()),"SDL_SaveBMP");std::set<std::size_t> used;for(const auto& q:backend.quads())used.insert(q.texture);
    std::cout<<"{\"resources\":"<<owners.size()<<",\"entities\":"<<profile.selected_count<<",\"commands\":"<<(queue.counts->at(0)+queue.counts->at(1))<<",\"quads\":"<<backend.quads().size()<<",\"textures\":"<<used.size()
             <<",\"capture_render_frames\":"<<rendered<<",\"capture_logic_steps\":"<<logic_steps
             <<",\"profile\":\""<<(profile.native_materials?"explicit projection, native material cache and depth":"explicit projection and material identities, native depth")<<"\"}\n";return 0;
}
} // namespace saga::desktop
