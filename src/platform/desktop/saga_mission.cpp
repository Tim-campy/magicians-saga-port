// Native data-backed mission entry and explicit diagnostic scene entry share the same lifecycle.
#include "mission_profile.hpp"
#include "core/gameplay/game_random.hpp"
#ifdef SAGA_MISSION_WINDOW
#include "mission_window.hpp"
#include "decoded_image.hpp"
#endif
#include <charconv>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>

namespace {
std::int32_t integer(std::string_view value){
    std::int32_t result=0;const auto parsed=std::from_chars(value.data(),value.data()+value.size(),result);
    if(parsed.ec!=std::errc{}||parsed.ptr!=value.data()+value.size())throw std::invalid_argument("invalid int32 option");return result;
}
float real(std::string_view value){
    float result=0;const auto parsed=std::from_chars(value.data(),value.data()+value.size(),result);
    if(parsed.ec!=std::errc{}||parsed.ptr!=value.data()+value.size()||!std::isfinite(result))throw std::invalid_argument("invalid finite float option");return result;
}
bool boolean(std::string_view value){const auto n=integer(value);if(n!=0&&n!=1)throw std::invalid_argument("bool options require0/1");return n!=0;}
}
int main(int argc,char** argv)try{
    if(argc==2&&std::string_view(argv[1])=="--help"){
        std::cout<<"Usage: saga_mission --data extracted-root --quest id [options]\n"
                   "Diagnostic: --scene profile.txt --fps N|initial --debug 0|1|initial --rng system --seed int32\n"
                   "Viewer builds open a window; --frames N runs a bounded hidden window; --headless requires --frames N.\n"
                   "Common: --replay inputs.txt --capture new.bmp; capture requires viewer mode and bounded frames.\n"
                   "Data entry uses shipped FPS30/DEBUG0 and provisional desktop context: System RNG seed42,600x640,English.lproj,DPI1,tick40ms.\n"
                   "Context: --width --height --language --dpi --tick-ms --ui-debug --short-words --held-advance --print.\n"
                   "Font: --font path (otherwise manifest index, English/default skin only), --font-scale --app-ratio --layout-scale,\n"
                   "      --japanese --japanese-style --ft-load --kerning (defaults100/100/100,0/0/0/0).\n"
                   "Presentation: --camera-near --camera-far --camera-z --auto-images --application-update --depth-bits --depth-step.\n"
                   "Other: --shop-mask; --human-static-fps (default30, captured retry duration); --fps/--debug accept initial; --rng system/--seed override host defaults.\n"
                   "Bootstrap options apply only without --scene. Context inputs are provisional, not observed Android runtime settings.\n";return 0;
    }
    const std::set<std::string> legacy_required{"--scene","--fps","--debug","--rng","--seed"};
    const std::set<std::string> bootstrap_keys{"--width","--height","--language","--dpi","--tick-ms","--ui-debug","--short-words","--held-advance","--print",
        "--font","--font-scale","--app-ratio","--layout-scale","--japanese","--japanese-style","--ft-load","--kerning",
        "--camera-near","--camera-far","--camera-z","--auto-images","--application-update","--depth-bits","--depth-step","--shop-mask","--human-static-fps"};
    auto allowed=legacy_required;for(const auto* key:{"--replay","--frames","--data","--quest","--capture"})allowed.insert(key);
    allowed.insert(bootstrap_keys.begin(),bootstrap_keys.end());
    std::map<std::string,std::string> options;bool headless=false;
    for(int i=1;i<argc;++i){const std::string key=argv[i];
        if(key=="--headless"){if(headless)throw std::invalid_argument("duplicate --headless");headless=true;continue;}
        if(!allowed.count(key)||i+1>=argc)throw std::invalid_argument("unsupported or incomplete option: "+key);
        if(!options.emplace(key,argv[++i]).second)throw std::invalid_argument("duplicate option: "+key);
    }
    const bool diagnostic=options.count("--scene")!=0;
    if((diagnostic&&std::any_of(legacy_required.begin(),legacy_required.end(),[&](const auto& key){return !options.count(key);}))||
       (!diagnostic&&(!options.count("--data")||!options.count("--quest")))||(headless&&!options.count("--frames")))
        throw std::invalid_argument("explicit scene/fps/debug/rng/seed or --data/--quest and headless frames required; see --help");
    if(diagnostic&&std::any_of(bootstrap_keys.begin(),bootstrap_keys.end(),[&](const auto& key){return options.count(key)!=0;}))
        throw std::invalid_argument("bootstrap context options cannot be mixed with --scene");
    if(!diagnostic){
        options.try_emplace("--fps","initial");options.try_emplace("--debug","initial");
        options.try_emplace("--rng","system");options.try_emplace("--seed","42"); // Declared deterministic host policy.
    }
    const auto frames=options.count("--frames")?integer(options.at("--frames")):0;
    if(options.count("--capture")&&(headless||!options.count("--frames")))throw std::invalid_argument("capture requires viewer mode and --frames N");
    const auto defaults=saga::core::initial_game_frame_configuration;
    const auto fps=options.at("--fps")=="initial"?defaults.fps:integer(options.at("--fps"));
    const auto debug=options.at("--debug")=="initial"?static_cast<std::int32_t>(defaults.damage_debug):integer(options.at("--debug"));
    const auto seed=integer(options.at("--seed"));
    if((options.count("--frames")&&(frames<1||frames>100000))||fps<1||fps>1000||(debug!=0&&debug!=1))throw std::invalid_argument("frames1..100000, fps1..1000 and debug0/1 required");
    if(options.at("--rng")!="system")throw std::invalid_argument("only the explicit-seed system backend is available; no backend substitution");
    const auto replay=options.count("--replay")?std::optional<std::filesystem::path>(std::filesystem::u8path(options.at("--replay"))):std::nullopt;
    saga::desktop::MissionSelection selection;
    if(options.count("--data"))selection.data_root=std::filesystem::u8path(options.at("--data"));
    if(options.count("--quest"))selection.quest=integer(options.at("--quest"));
    saga::desktop::ArchiveImageSet::Decoder decoder;
#ifdef SAGA_MISSION_WINDOW
    if(!headless)decoder=saga::desktop::decode_png;
#else
    if(!headless)throw std::invalid_argument("this build has no window; build with SAGA_BUILD_VIEWER=ON or pass --headless --frames N");
#endif
    const saga::desktop::MissionRuntimeConfig runtime{fps,debug!=0,seed,options.at("--rng")};
    std::unique_ptr<saga::desktop::MissionProfile> owner;
    if(diagnostic)owner=std::make_unique<saga::desktop::MissionProfile>(std::filesystem::u8path(options.at("--scene")),true,decoder,runtime,replay,selection);
    else{
        saga::desktop::MissionBootstrapOptions b;b.data_root=*selection.data_root;b.quest=*selection.quest;
        const auto set_int=[&](const char* key,std::int32_t& target){if(options.count(key))target=integer(options.at(key));};
        const auto set_float=[&](const char* key,float& target){if(options.count(key))target=real(options.at(key));};
        const auto set_bool=[&](const char* key,bool& target){if(options.count(key))target=boolean(options.at(key));};
        set_int("--width",b.width);set_int("--height",b.height);set_int("--dpi",b.dpi);set_int("--tick-ms",b.tick_ms);
        set_int("--depth-bits",b.depth_bits);set_int("--depth-step",b.depth_step);set_int("--shop-mask",b.shop_find_mask);set_int("--human-static-fps",b.human_static_fps);
        if(options.count("--language"))b.language=options.at("--language");
        if(options.count("--font"))b.font.path=std::filesystem::u8path(options.at("--font"));
        set_int("--font-scale",b.font.requested_scale);set_int("--ft-load",b.font.load_flags);set_int("--kerning",b.font.kerning_mode);
        set_float("--app-ratio",b.font.application_ratio);set_float("--layout-scale",b.font.layout_scale);
        set_float("--camera-near",b.camera.near_clip);set_float("--camera-far",b.camera.far_clip);set_float("--camera-z",b.camera.z);
        set_bool("--japanese",b.font.japanese);set_bool("--japanese-style",b.font.japanese_style);
        set_bool("--auto-images",b.automatic_images);set_bool("--application-update",b.application_update);
        set_bool("--ui-debug",b.debug_controls);set_bool("--short-words",b.short_words);set_bool("--held-advance",b.held_advance);set_bool("--print",b.print);
        owner=std::make_unique<saga::desktop::MissionProfile>(b,decoder,runtime,replay);
    }
    auto& profile=*owner;
    if(!profile.mission||!profile.simulate)throw std::invalid_argument("mission host requires selected entities and an explicit simulate row");
#ifdef SAGA_MISSION_WINDOW
    if(!headless){
        if(!profile.has_view)throw std::invalid_argument("mission window requires explicit view dimensions");
        if(options.count("--capture")){
            const auto output=std::filesystem::u8path(options.at("--capture"));
            if(std::filesystem::exists(output))throw std::invalid_argument("capture destination already exists");
            return saga::desktop::run_native_mission(profile,profile.view.width,profile.view.height,frames,output);
        }
        return saga::desktop::run_native_mission(profile,profile.view.width,profile.view.height,frames?std::optional<std::int32_t>(frames):std::nullopt);
    }
#endif
    const auto initial=profile.logic_steps;std::int32_t executed=0;
    for(;executed<frames;++executed)if(!profile.step())break; // Deterministic fixed passes, no wall-clock sleeping.
    std::cout<<"{\"frames\":"<<executed<<",\"frames_requested\":"<<frames<<",\"initial_passes\":"<<initial<<",\"fps\":"<<fps<<",\"debug\":"<<debug
             <<",\"rng_backend\":\"system\",\"seed\":"<<seed<<",\"rng_draws\":"<<(profile.humans?profile.humans->random_draws():0)<<",\"positions\":[";
    std::cout<<std::setprecision(std::numeric_limits<float>::max_digits10);
    for(std::size_t i=0;i<profile.created.size();++i){if(i)std::cout<<',';const auto& entity=*profile.created[i];
        std::cout<<"{\"id\":"<<profile.entity_world->id(entity)<<",\"destroyed\":"<<(profile.entity_world->destroyed(entity)?"true":"false");
        if(const auto* p=dynamic_cast<saga::core::ContactPosition*>(entity.get(0)))std::cout<<",\"x\":"<<p->x<<",\"y\":"<<p->y<<",\"z\":"<<p->z;
        if(const auto* h=dynamic_cast<saga::core::HumanRecordLink*>(entity.get(14)))std::cout<<",\"human_state\":"<<h->state<<",\"human_frame\":"<<h->frame<<",\"attack_frame\":"<<h->attack_frame;
        profile.write_actor_parameters(*profile.created[i],std::cout);
        std::cout<<'}';
    }
    std::cout<<']';profile.write_combat_report(std::cout);
    std::cout<<",\"render_commands\":"<<profile.queue.counts->at(0)+profile.queue.counts->at(1)
             <<",\"scope\":\"selected mission; full application entry, save restoration and complete-state hashes unbound\"}\n";return 0;
}catch(const std::exception& error){std::cerr<<"saga_mission: "<<error.what()<<'\n';return 1;}
