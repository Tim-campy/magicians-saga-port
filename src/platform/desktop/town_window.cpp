#include "town_window.hpp"
#include "bitmap_mesh_frame.hpp"
#include "gl_bitmap.hpp"
#include "mission_window.hpp"
#include "mission_profile.hpp"
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
    const auto town_textures=[&](std::size_t key){
        const auto* image=textures.at(key).ready;if(!image)throw std::invalid_argument("town texture is not decoded");
        return GlBitmapTexture{&image->pixels,GlBitmapWrap::clamp,GlBitmapWrap::clamp};
    };
    GlBitmapRenderer renderer(*window,context.width,context.height,town_textures);
    MissionProfile* active=nullptr;std::unique_ptr<NativeMissionFrame> mission_frame;
    std::int32_t output_width=context.width,output_height=context.height;
    SpriteMatrix world;reset_matrix(world);
    auto& application=town.application();
    BitmapBackend backend(application,application,textures,context.height,world);
    MeshDepthSurface surface;MeshDepthState depth;depth.surface=&surface;Queries queries;BitmapMaterials materials;
    BitmapMeshFrame frame(backend,depth,queries,{context.depth_bits,false,[&]{return context.depth_step;}},[&](auto texture,auto shader,auto queue)->SubmissionMaterial&{
        return materials.resolve(texture,shader,queue);
    });
    const auto submit=[&]{
    auto* mission=town.active_mission();
    if(mission!=active){
        mission_frame.reset();active=mission;
        if(active){
            renderer.set_texture_source([active](std::size_t key){
                const auto* image=active->textures.at(key).ready;
                if(!image)throw std::invalid_argument("mission texture is not decoded");
                return GlBitmapTexture{&image->pixels,GlBitmapWrap::clamp,GlBitmapWrap::clamp};
            });
            mission_frame=std::make_unique<NativeMissionFrame>(*active,renderer);
        }else renderer.set_texture_source(town_textures);
    }
    const auto width=active?active->view.width:context.width,height=active?active->view.height:context.height;
    if(width!=output_width||height!=output_height){
        require(SDL_SetWindowSize(window.get(),width,height),"SDL scene viewport size");output_width=width;output_height=height;
    }
    if(mission_frame){mission_frame->draw(width,height);return;}
    renderer.resize_target(width,height);
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
        bool running=true;std::uint64_t dispatch=0;float pending_x=0,pending_y=0,pending_wheel=0;std::string title;
        const auto coordinates=[&](float x,float y){
            int width=0,height=0;require(SDL_GetWindowSize(window.get(),&width,&height),"SDL pointer window size");
            if(width<1||height<1)throw std::runtime_error("invalid town window dimensions");
            return std::array<std::int32_t,4>{factory_cell_coordinate(x*static_cast<float>(output_width)/width,1),
                factory_cell_coordinate(y*static_cast<float>(output_height)/height,1),width,height};
        };
        const auto delta=[](float& pending,float value){pending+=value;const auto result=factory_cell_coordinate(pending,1);pending-=static_cast<float>(result);return result;};
        while(running&&(!options.passes||dispatch<options.passes)){
            const auto deadline=SDL_GetTicks()+40;bool pulse=false;SDL_Event event;
            while(SDL_PollEvent(&event)){
                if(event.type==SDL_EVENT_QUIT)running=false;
                else if(auto* mission=town.active_mission()){
                    if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat&&event.key.key==SDLK_ESCAPE){running=false;continue;}
                    if(!mission->accepts_live_input())continue;
                    auto& humans=*mission->humans;
                    if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST){mission->cancel_map_pointer();mission->cancel_control_pointer();pending_x=pending_y=pending_wheel=0;}
                    else if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){
                        const auto key=event.key.key;
                        if(humans.stage_modal_pending()){
                            const auto kind=humans.stage_modal_kind();
                            if(kind==98){if(key==SDLK_C)humans.answer_stage_dialog(0);else if(key==SDLK_A)humans.answer_stage_dialog(1);else if(key==SDLK_N)humans.answer_stage_dialog(2);}
                            else if(kind==99){if(key==SDLK_Y||key==SDLK_N)humans.answer_stage_dialog(key==SDLK_Y?0:1);}
                            else if(key==SDLK_RETURN)humans.answer_stage_dialog(0);
                        }else if(mission->accepts_live_gameplay_input()&&humans.exit_dialog_pending()&&(key==SDLK_Y||key==SDLK_N))humans.answer_stage_dialog(key==SDLK_Y?0:1);
                        else if(key==SDLK_RETURN||key==SDLK_SPACE)pulse=true;
                        else if(mission->accepts_live_gameplay_input()){
                            if(key>=SDLK_1&&key<=SDLK_3)humans.select_input_attack(static_cast<std::int32_t>(key-SDLK_1));
                            else if(key==SDLK_F)humans.select_input_attack(humans.selected_attack());
                            else if(!humans.exit_dialog_pending()){
                                const auto state=humans.stage_state();
                                if(key==SDLK_P&&(state==1||state==2))humans.pulse_stage_keys(state==1?0x2000000U:0x4000000U);
                                else if(key==SDLK_Q&&state==2)humans.pulse_stage_keys(0x8000000U);
                            }
                        }
                    }else if(event.type==SDL_EVENT_MOUSE_BUTTON_UP&&event.button.button==SDL_BUTTON_LEFT)mission->control_pointer(1,0,0);
                    else if(mission->accepts_live_gameplay_input()){
                        if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_LEFT){const auto p=coordinates(event.button.x,event.button.y);mission->control_pointer(0,p[0],p[1]);}
                        else if(event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_RIGHT){pending_x=pending_y=pending_wheel=0;const auto p=coordinates(event.button.x,event.button.y);mission->map_pointer_touch(0,p[0],p[1],0,0,0);}
                        else if(event.type==SDL_EVENT_MOUSE_BUTTON_UP&&event.button.button==SDL_BUTTON_RIGHT){pending_x=pending_y=pending_wheel=0;const auto p=coordinates(event.button.x,event.button.y);mission->map_pointer_touch(1,p[0],p[1],0,0,0);}
                        else if(event.type==SDL_EVENT_MOUSE_MOTION&&(event.motion.state&SDL_BUTTON_RMASK)!=0){
                            const auto p=coordinates(event.motion.x,event.motion.y);
                            mission->map_pointer_touch(8,p[0],p[1],delta(pending_x,event.motion.xrel*static_cast<float>(output_width)/p[2]),delta(pending_y,event.motion.yrel*static_cast<float>(output_height)/p[3]),0);
                        }else if(event.type==SDL_EVENT_MOUSE_WHEEL){
                            const auto p=coordinates(event.wheel.mouse_x,event.wheel.mouse_y);
                            const auto pinch=delta(pending_wheel,event.wheel.y*20.0F);
                            if(pinch)mission->map_pointer_touch(12,p[0],p[1],0,0,pinch);
                        }
                    }
                }
                else if(event.type==SDL_EVENT_WINDOW_FOCUS_LOST&&!context.input_replay){town.cancel_camera_pointer();town.cancel_widget_pointer();pending_x=pending_y=pending_wheel=0;}
                else if(event.type==SDL_EVENT_KEY_DOWN&&!event.key.repeat){
                    if(!context.input_replay)town.clear_widget_marker(); // Selected GameView.OnKeyDown overload.
                    if(event.key.key==SDLK_ESCAPE)running=false;
                    else if(!context.input_replay&&event.key.key==SDLK_BACKSPACE)town.form_back();
                    else if(!context.input_replay&&(event.key.key==SDLK_RETURN||event.key.key==SDLK_SPACE))pulse=true;
                }else if(!context.input_replay&&event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_LEFT){
                    const auto p=coordinates(event.button.x,event.button.y);town.widget_pointer(0,p[0],p[1]);
                }else if(!context.input_replay&&event.type==SDL_EVENT_MOUSE_BUTTON_UP&&event.button.button==SDL_BUTTON_LEFT){
                    const auto p=coordinates(event.button.x,event.button.y);town.widget_pointer(1,p[0],p[1]);
                }else if(!context.input_replay&&event.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&event.button.button==SDL_BUTTON_RIGHT){
                    pending_x=pending_y=pending_wheel=0;const auto p=coordinates(event.button.x,event.button.y);town.camera_pointer(0,p[0],p[1],0,0);
                }else if(!context.input_replay&&event.type==SDL_EVENT_MOUSE_BUTTON_UP&&event.button.button==SDL_BUTTON_RIGHT){
                    pending_x=pending_y=pending_wheel=0;const auto p=coordinates(event.button.x,event.button.y);town.camera_pointer(1,p[0],p[1],0,0);
                }else if(!context.input_replay&&event.type==SDL_EVENT_MOUSE_MOTION&&(event.motion.state&SDL_BUTTON_RMASK)!=0){
                    const auto p=coordinates(event.motion.x,event.motion.y);
                    town.camera_pointer(8,p[0],p[1],delta(pending_x,event.motion.xrel*static_cast<float>(output_width)/p[2]),
                        delta(pending_y,event.motion.yrel*static_cast<float>(output_height)/p[3]));
                }
            }
            if(!running)break;
            if(auto* mission=town.active_mission();mission&&mission->accepts_live_input()){
                const auto* keys=SDL_GetKeyboardState(nullptr);const bool focused=SDL_GetKeyboardFocus()==window.get();
                mission->humans->set_input_movement(focused&&(keys[SDL_SCANCODE_W]||keys[SDL_SCANCODE_UP]),focused&&(keys[SDL_SCANCODE_S]||keys[SDL_SCANCODE_DOWN]));
                if(mission->accepts_fast_forward())mission->humans->set_input_fast_forward(focused&&keys[SDL_SCANCODE_TAB]);
            }
            ++dispatch;town.advance_update(dispatch,pulse||(options.pulse_every>0&&dispatch%static_cast<std::uint64_t>(options.pulse_every)==0));
            submit();renderer.present();const auto next_title=town.window_title();
            if(title!=next_title){require(SDL_SetWindowTitle(window.get(),next_title.c_str()),"SDL town title");title=next_title;}
            if(town.quest_handoff_pending())break; // Stop only at the explicitly held legacy entry or mission return boundary.
            const auto current=SDL_GetTicks();if(!options.hidden&&current<deadline)SDL_Delay(static_cast<Uint32>(deadline-current));
        }
        // A frame limit observes the same retained state as headless replay.
        // Only an actual close cancels live capture after the last dispatch.
        if(!running){
            if(auto* mission=town.active_mission()){
                if(mission->accepts_live_input()){mission->cancel_map_pointer();mission->cancel_control_pointer();}
            }else if(!context.input_replay){town.cancel_camera_pointer();town.cancel_widget_pointer();}
        }
        if(dispatch==0)submit();
    }else submit();
    if(options.capture){
        auto pixels=renderer.read_pixels(output_width,output_height);
        std::unique_ptr<SDL_Surface,decltype(&SDL_DestroySurface)> image(SDL_CreateSurfaceFrom(output_width,output_height,SDL_PIXELFORMAT_RGBA32,pixels.rgba.data(),output_width*4),SDL_DestroySurface);
        require(image!=nullptr,"SDL_CreateSurfaceFrom");require(SDL_SaveBMP(image.get(),options.capture->string().c_str()),"SDL_SaveBMP");
    }
}
void capture_town_background(TownBackground& town,const std::filesystem::path& output){
    TownWindowOptions options;options.capture=output;run_town_window(town,options);
}
} // namespace saga::desktop
