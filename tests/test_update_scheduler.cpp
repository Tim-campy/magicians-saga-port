#include "core/ecs/update_scheduler.hpp"
#include <cstdlib>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
#include <algorithm>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Clock final:UpdateStopwatch {
    std::vector<std::string>& log;std::function<void()> on_reset;
    explicit Clock(std::vector<std::string>& l):log(l){}
    void reset()override{log.push_back("reset");if(on_reset)on_reset();}
    void start()override{log.push_back("start");}
    void stop()override{log.push_back("stop");}
};
struct System final:UpdateSystem {
    std::vector<std::string>& log;std::string name;bool needed=true;
    std::function<void()> on_gate,on_update;
    System(std::vector<std::string>& l,std::string n):log(l),name(std::move(n)){}
    bool needs_update(std::int32_t p)override{log.push_back(name+"?"+std::to_string(p));if(on_gate)on_gate();return needed;}
    void update(std::int32_t p)override{log.push_back(name+"!"+std::to_string(p));if(on_update)on_update();}
};
int main(){
    {struct Manager final:SystemPauseManager {std::vector<bool> calls;void pause_systems(bool value)override{calls.push_back(value);}} a,b;
        SystemConstructionWorld world{&a};GameplaySystemState system;system.enabled=false;int enabled=0;
        system.on_enabled=[&]{check(system.enabled&&system.manager==&a);++enabled;};
        construct_base_system(system,&world);check(enabled==1&&system.manager==&a);
        world.manager=&b;construct_base_system(system,&world);check(enabled==1&&system.manager==&b); // Rebind even if enabled.
        bool failed=false;try{construct_base_system(system,nullptr);}catch(const std::invalid_argument&){failed=true;}
        check(failed&&system.manager==&b&&system.enabled);
        GameplaySystemState pause;pause.enabled=false;
        pause.on_enabled=[&]{enable_pause_system(pause);};pause.on_disabled=[&]{disable_pause_system(pause);};
        world.manager=&a;construct_base_system(pause,&world);set_system_enabled(pause,true);set_system_enabled(pause,false);
        check(a.calls==std::vector<bool>{true,false});
        world.manager=nullptr;pause.enabled=false;failed=false;
        try{construct_base_system(pause,&world);}catch(const std::invalid_argument&){failed=true;}
        check(failed&&pause.enabled&&!pause.manager); // The enable store survives the captured-manager failure.
        GameplaySystemState plain;plain.enabled=false;construct_base_system(plain,&world);check(plain.enabled&&!plain.manager);
    }
    {std::array<GameplaySystemState,32> systems;bool paused=false;std::vector<int> calls;
        for(std::size_t i=0;i<systems.size();++i){
            systems[i].on_disabled=[&,i]{check(paused&&!systems[i].enabled);calls.push_back(static_cast<int>(i));};
            systems[i].on_enabled=[&,i]{check(!paused&&systems[i].enabled);calls.push_back(200+static_cast<int>(i));};
            systems[i].on_pause=[&,i]{check(paused);for(const auto id:gameplay_pause_order)check(!systems[static_cast<std::size_t>(id)].enabled);calls.push_back(100+static_cast<int>(i));};
        }
        systems[9].enabled=false;systems[22].enabled=false;
        pause_gameplay_systems(paused,systems,true);std::vector<int> expected;
        for(const auto id:gameplay_pause_order)if(id!=9)expected.push_back(id);
        for(int i=0;i<32;++i)expected.push_back(100+i);check(calls==expected);
        calls.clear();pause_gameplay_systems(paused,systems,true);expected.clear();
        for(int i=0;i<32;++i)expected.push_back(100+i);check(calls==expected); // Repeated true still sends every OnPause.
        calls.clear();pause_gameplay_systems(paused,systems,false);expected.clear();
        for(const auto id:gameplay_pause_order)expected.push_back(200+id);check(calls==expected);
        check(systems[9].enabled&&!systems[22].enabled); // Resume enables the cohort, not an earlier snapshot.
        calls.clear();pause_gameplay_systems(paused,systems,false);check(calls.empty());}
    {std::array<GameplaySystemState,32> systems;bool paused=false;int notified=0;
        systems[5].on_disabled=[] {throw std::runtime_error("disabled hook");};systems[0].on_pause=[&]{++notified;};
        bool failed=false;try{pause_gameplay_systems(paused,systems,true);}catch(const std::runtime_error&){failed=true;}
        check(failed&&paused&&!systems[2].enabled&&!systems[4].enabled&&!systems[5].enabled&&systems[9].enabled&&notified==0);
        GameplaySystemState single;single.on_disabled=[&]{check(!single.enabled);throw std::runtime_error("hook");};
        failed=false;try{set_system_enabled(single,false);}catch(const std::runtime_error&){failed=true;}check(failed&&!single.enabled);
        set_system_enabled(single,false); // No second hook even after the first one threw.
    }
    {std::array<GameplaySystemState,32> states;std::array<UpdateSystem*,32> factory{};
        std::vector<std::unique_ptr<GameplayUpdate>> owners;std::vector<int> ran;bool paused=false;int tick=0;
        for(const auto id:gameplay_update_order){
            owners.push_back(std::make_unique<GameplayUpdate>(id,states[static_cast<std::size_t>(id)].enabled,[&,id](std::int32_t){
                ran.push_back(id);if(id==3){++tick;if(tick==1)pause_gameplay_systems(paused,states,true);if(tick==3)pause_gameplay_systems(paused,states,false);}
            }));factory[static_cast<std::size_t>(id)]=owners.back().get();
        }
        auto list=make_gameplay_update_list(factory);UpdateSchedule schedule{&list,nullptr};
        update_systems(schedule,1);check(ran.front()==2&&ran[1]==3);check(std::find(ran.begin(),ran.end(),4)==ran.end());
        ran.clear();update_systems(schedule,1);check(ran.front()==3&&std::find(ran.begin(),ran.end(),2)==ran.end());
        ran.clear();update_systems(schedule,1);check(ran.front()==3&&ran[1]==4&&!paused);
    }
    {std::vector<std::string> log;Clock clock(log);System a(log,"a"),b(log,"b");b.needed=false;
        std::vector<UpdateSystem*> list{&a,&b};UpdateSchedule s{&list,&clock};update_systems(s,0);
        check(log==std::vector<std::string>{"reset","start","a?0","a!0","stop","reset","start","b?0","stop"});
        log.clear();s.stopwatch=nullptr;update_systems(s,-1);check(log==std::vector<std::string>{"a?-1","a!-1","b?-1"});}
    {std::vector<std::string> log;System a(log,"a"),b(log,"b");std::vector<UpdateSystem*> list{&a};UpdateSchedule s{&list,nullptr};
        a.on_update=[&]{list.push_back(&b);};update_systems(s,2);check(log.back()=="b!2");}
    {std::vector<std::string> log;System a(log,"a"),b(log,"b"),c(log,"c");std::vector<UpdateSystem*> list{&a,&b,&c};UpdateSchedule s{&list,nullptr};
        a.on_gate=[&]{list.erase(list.begin());};update_systems(s,1);check(log==std::vector<std::string>{"a?1","a!1","c?1","c!1"});} // Captured a updates; shifted b is skipped.
    {std::vector<std::string> log;System a(log,"a"),b(log,"b"),c(log,"c");std::vector<UpdateSystem*> list{&a,&b},other{&b,&c};UpdateSchedule s{&list,nullptr};
        a.on_update=[&]{s.systems=&other;};update_systems(s,1);check(log.back()=="c!1");}
    {std::vector<std::string> log,second;Clock a(log),b(second);System system(log,"x");std::vector<UpdateSystem*> list{&system};UpdateSchedule s{&list,&a};
        a.on_reset=[&]{s.stopwatch=&b;list[0]=nullptr;};update_systems(s,0);
        check(log==std::vector<std::string>{"reset","x?0","x!0"}&&second==std::vector<std::string>{"start","stop"});}
    {std::vector<std::string> log;Clock clock(log);std::vector<UpdateSystem*> list{nullptr};UpdateSchedule s{&list,&clock};bool failed=false;
        try{update_systems(s,0);}catch(const std::invalid_argument&){failed=true;}check(failed&&log==std::vector<std::string>{"reset","start"});}
    {std::vector<std::string> log;Clock clock(log);System system(log,"x");system.on_update=[] {throw std::runtime_error("body");};
        std::vector<UpdateSystem*> list{&system};UpdateSchedule s{&list,&clock};bool failed=false;
        try{update_systems(s,0);}catch(const std::runtime_error&){failed=true;}check(failed&&log.back()=="x!0");}
    {std::vector<UpdateSystem*> empty;UpdateSchedule s{&empty,nullptr};update_systems(s,0);s.systems=nullptr;bool failed=false;
        try{update_systems(s,1);}catch(const std::invalid_argument&){failed=true;}check(failed);}
    {std::array<UpdateSystem*,32> factory{};std::deque<std::unique_ptr<GameplayUpdate>> systems;bool enabled=true;std::vector<int> ran;
        for(const auto index:gameplay_update_order){systems.push_back(std::make_unique<GameplayUpdate>(index,enabled,[&,index](std::int32_t){ran.push_back(index);}));factory[static_cast<std::size_t>(index)]=systems.back().get();}
        auto list=make_gameplay_update_list(factory);UpdateSchedule s{&list,nullptr};update_systems(s,1);
        check(ran.size()==21&&ran.front()==2&&ran.back()==31);
        for(const auto index:gameplay_update_order){check(gameplay_needs_update(index,true,0));check(!gameplay_needs_update(index,false,0));
            check(gameplay_needs_update(index,true,-1)==(index!=0&&index!=8&&index!=20&&index!=21&&index!=26));}
        ran.clear();enabled=false;update_systems(s,1);check(ran.empty());factory[10]=nullptr;bool failed=false;
        try{make_gameplay_update_list(factory);}catch(const std::invalid_argument&){failed=true;}check(failed);}
}
