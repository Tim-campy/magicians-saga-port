// Selected mission host. Full quest/map bootstrap, player input and combat schedule remain unbound.
#include "mission_profile.hpp"
#include "core/gameplay/game_random.hpp"
#include <charconv>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>

namespace {
std::int32_t integer(std::string_view value){
    std::int32_t result=0;const auto parsed=std::from_chars(value.data(),value.data()+value.size(),result);
    if(parsed.ec!=std::errc{}||parsed.ptr!=value.data()+value.size())throw std::invalid_argument("invalid int32 option");return result;
}
}
int main(int argc,char** argv)try{
    if(argc==2&&std::string_view(argv[1])=="--help"){
        std::cout<<"Usage: saga_mission --scene profile.txt --frames N --headless --fps N --debug 0|1 --rng system --seed int32\n"
                   "Runs selected Move/Animation/Render/Garbage passes without SDL or PNG decoding.\n"
                   "Human profiles use provisional FPS/RNG inputs; DEBUG and full combat remain unbound.\n"
                   "Quest/map bootstrap, interactive input and gameplay replay are not available.\n";return 0;
    }
    const std::set<std::string> allowed{"--scene","--frames","--fps","--debug","--rng","--seed"};
    std::map<std::string,std::string> options;bool headless=false;
    for(int i=1;i<argc;++i){const std::string key=argv[i];
        if(key=="--headless"){if(headless)throw std::invalid_argument("duplicate --headless");headless=true;continue;}
        if(!allowed.count(key)||i+1>=argc)throw std::invalid_argument("unsupported or incomplete option: "+key);
        if(!options.emplace(key,argv[++i]).second)throw std::invalid_argument("duplicate option: "+key);
    }
    if(!headless||options.size()!=allowed.size())throw std::invalid_argument("explicit scene/frames/headless/fps/debug/rng/seed required; see --help");
    const auto frames=integer(options.at("--frames")),fps=integer(options.at("--fps")),debug=integer(options.at("--debug")),seed=integer(options.at("--seed"));
    if(frames<1||frames>100000||fps<1||fps>1000||(debug!=0&&debug!=1))throw std::invalid_argument("frames1..100000, fps1..1000 and debug0/1 required");
    if(options.at("--rng")!="system")throw std::invalid_argument("only the explicit-seed system backend is available; no backend substitution");
    saga::desktop::MissionProfile profile(std::filesystem::u8path(options.at("--scene")),true,{},saga::desktop::MissionRuntimeConfig{fps,debug!=0,seed,options.at("--rng")});
    if(!profile.mission||!profile.simulate)throw std::invalid_argument("mission host requires selected entities and an explicit simulate row");
    for(std::int32_t frame=0;frame<frames;++frame)profile.step(); // Deterministic fixed passes, no wall-clock sleeping.
    std::cout<<"{\"frames\":"<<frames<<",\"initial_passes\":"<<*profile.simulate<<",\"fps\":"<<fps<<",\"debug\":"<<debug
             <<",\"rng_backend\":\"system\",\"seed\":"<<seed<<",\"rng_draws\":"<<(profile.humans?profile.humans->random_draws():0)<<",\"positions\":[";
    std::cout<<std::setprecision(std::numeric_limits<float>::max_digits10);
    for(std::size_t i=0;i<profile.created.size();++i){if(i)std::cout<<',';const auto& entity=*profile.created[i];
        std::cout<<"{\"id\":"<<profile.entity_world->id(entity)<<",\"destroyed\":"<<(profile.entity_world->destroyed(entity)?"true":"false");
        if(const auto* p=dynamic_cast<saga::core::ContactPosition*>(entity.get(0)))std::cout<<",\"x\":"<<p->x<<",\"y\":"<<p->y<<",\"z\":"<<p->z;
        if(const auto* h=dynamic_cast<saga::core::HumanRecordLink*>(entity.get(14)))std::cout<<",\"human_state\":"<<h->state<<",\"human_frame\":"<<h->frame<<",\"attack_frame\":"<<h->attack_frame;
        std::cout<<'}';
    }
    std::cout<<"],\"render_commands\":"<<profile.queue.counts->at(0)+profile.queue.counts->at(1)
             <<",\"scope\":\"selected scene; full schedule, quest bootstrap, input and replay unbound\"}\n";return 0;
}catch(const std::exception& error){std::cerr<<"saga_mission: "<<error.what()<<'\n';return 1;}
