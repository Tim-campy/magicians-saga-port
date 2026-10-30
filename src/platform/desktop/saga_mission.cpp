// Selected mission host. Loaded-world native Init is opt-in; original quest bootstrap remains unbound.
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

namespace {
std::int32_t integer(std::string_view value){
    std::int32_t result=0;const auto parsed=std::from_chars(value.data(),value.data()+value.size(),result);
    if(parsed.ec!=std::errc{}||parsed.ptr!=value.data()+value.size())throw std::invalid_argument("invalid int32 option");return result;
}
}
int main(int argc,char** argv)try{
    if(argc==2&&std::string_view(argv[1])=="--help"){
        std::cout<<"Usage: saga_mission --scene profile.txt --fps N --debug 0|1 --rng system --seed int32 [--data extracted-root] [--quest id] [--frames N] [--headless] [--replay inputs.txt] [--capture new-output.bmp]\n"
                   "Viewer builds open a mission window; --frames N uses a bounded hidden window.\n"
                   "--headless requires --frames N; builds without the viewer support only headless mode.\n"
                   "--data resolves declared archive filenames below textassets/ without fallback.\n"
                   "--quest replaces the scenario quest ID; mission_selection also binds that ID to decoded slot9/stage.\n"
                   "--capture requires viewer mode and --frames N; steps the native world before capturing its prepared viewport.\n"
                   "FPS/DEBUG/RNG and scenario state are explicit; deferred town transitions hold the old mission.\n"
                   "Input-enabled scenes replay native movement/attack controls; quest bootstrap and full-state hashes remain open.\n";return 0;
    }
    const std::set<std::string> required{"--scene","--fps","--debug","--rng","--seed"};
    auto allowed=required;for(const auto* key:{"--replay","--frames","--data","--quest","--capture"})allowed.insert(key);
    std::map<std::string,std::string> options;bool headless=false;
    for(int i=1;i<argc;++i){const std::string key=argv[i];
        if(key=="--headless"){if(headless)throw std::invalid_argument("duplicate --headless");headless=true;continue;}
        if(!allowed.count(key)||i+1>=argc)throw std::invalid_argument("unsupported or incomplete option: "+key);
        if(!options.emplace(key,argv[++i]).second)throw std::invalid_argument("duplicate option: "+key);
    }
    if(std::any_of(required.begin(),required.end(),[&](const auto& key){return !options.count(key);})||(headless&&!options.count("--frames")))throw std::invalid_argument("explicit scene/fps/debug/rng/seed and headless frames required; see --help");
    const auto frames=options.count("--frames")?integer(options.at("--frames")):0;
    if(options.count("--capture")&&(headless||!options.count("--frames")))throw std::invalid_argument("capture requires viewer mode and --frames N");
    const auto fps=integer(options.at("--fps")),debug=integer(options.at("--debug")),seed=integer(options.at("--seed"));
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
    saga::desktop::MissionProfile profile(std::filesystem::u8path(options.at("--scene")),true,decoder,saga::desktop::MissionRuntimeConfig{fps,debug!=0,seed,options.at("--rng")},replay,selection);
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
             <<",\"scope\":\"selected scene; fresh-world init, quest bootstrap and complete-state hashes unbound\"}\n";return 0;
}catch(const std::exception& error){std::cerr<<"saga_mission: "<<error.what()<<'\n';return 1;}
