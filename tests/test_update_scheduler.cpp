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
    std::vector<std::string>& log;std::function<void()> on_reset,on_start,on_stop;
    explicit Clock(std::vector<std::string>& l):log(l){}
    void reset()override{log.push_back("reset");if(on_reset)on_reset();}
    void start()override{log.push_back("start");if(on_start)on_start();}
    void stop()override{log.push_back("stop");if(on_stop)on_stop();}
};
struct System final:UpdateSystem {
    std::vector<std::string>& log;std::string name;bool needed=true;
    std::function<void()> on_gate,on_update;
    System(std::vector<std::string>& l,std::string n):log(l),name(std::move(n)){}
    bool needs_update(std::int32_t p)override{log.push_back(name+"?"+std::to_string(p));if(on_gate)on_gate();return needed;}
    void update(std::int32_t p)override{log.push_back(name+"!"+std::to_string(p));if(on_update)on_update();}
};
struct DynamicSystem final:DynamicDrawSystem {
    std::vector<std::string>& log;std::string name;bool needed=true;const void* identity;
    std::function<void()> on_gate,on_draw,on_identity;
    DynamicSystem(std::vector<std::string>& l,std::string n):log(l),name(std::move(n)),identity(this){}
    bool needs_dynamic_draw()override{log.push_back(name+"?");if(on_gate)on_gate();return needed;}
    void dynamic_draw()override{log.push_back(name+"!");if(on_draw)on_draw();}
    const void* base_identity()const override{log.push_back(name+"#");if(on_identity)on_identity();return identity;}
};
struct DynamicDepth final:DynamicDrawDepthHost {
    std::vector<std::string>& log;std::function<void()> callback;
    explicit DynamicDepth(std::vector<std::string>& value):log(value){}
    void set_dynamic_depth_layer(std::int32_t layer)override{check(layer==99);log.push_back("depth99");if(callback)callback();}
};
template<class F>void dynamic_fails(F body){bool failed=false;try{body();}catch(const std::exception&){failed=true;}check(failed);}
void dynamic_contracts(){
    {std::vector<std::string> log;DynamicDepth depth(log);Clock clock(log);DynamicSystem a(log,"a"),b(log,"b");b.needed=false;
     std::vector<DynamicDrawSystem*> list{&a,nullptr,&b};std::vector<const void*> all{nullptr,&a,&b};const auto original=all;
     DynamicDrawSchedule s{&list,&all,&clock};draw_dynamic_systems(s,&depth);
     check(log==std::vector<std::string>{"depth99","reset","start","a?","a!","stop","a#","reset","start","stop","reset","start","b?","stop","b#"});
     check(all==original&&list.size()==3&&list[1]==nullptr);}
    // Depth is required and set even for empty or missing lists; empty traversal needs no other services.
    {std::vector<std::string> log;DynamicDepth depth(log);std::vector<DynamicDrawSystem*> list;DynamicDrawSchedule s{&list,nullptr,nullptr};
     draw_dynamic_systems(s,&depth);check(log==std::vector<std::string>{"depth99"});
     log.clear();dynamic_fails([&]{draw_dynamic_systems(s,nullptr);});check(log.empty());
     s.systems=nullptr;dynamic_fails([&]{draw_dynamic_systems(s,&depth);});check(log==std::vector<std::string>{"depth99"});}
    {std::vector<std::string> log;DynamicDepth depth(log);DynamicDrawSchedule s;
     depth.callback=[] {throw std::runtime_error("depth");};dynamic_fails([&]{draw_dynamic_systems(s,&depth);});
     check(log==std::vector<std::string>{"depth99"});}
    // Capture the list after the depth callback, then retain that identity while observing its live Count.
    {std::vector<std::string> log;DynamicDepth depth(log);Clock clock(log);DynamicSystem a(log,"a"),b(log,"b"),c(log,"c");
     std::vector<DynamicDrawSystem*> old{&c},list{&a},other{&c};std::vector<const void*> all;
     DynamicDrawSchedule s{&old,&all,&clock};depth.callback=[&]{s.systems=&list;};
     a.on_draw=[&]{s.systems=&other;list.push_back(&b);};draw_dynamic_systems(s,&depth);
     check(std::find(log.begin(),log.end(),"b!")!=log.end()&&std::find(log.begin(),log.end(),"c?")==log.end());}
    {std::vector<std::string> log;DynamicDepth depth(log);Clock clock(log);DynamicSystem a(log,"a"),b(log,"b"),c(log,"c");
     std::vector<DynamicDrawSystem*> list{&a,&b,&c};std::vector<const void*> all;DynamicDrawSchedule s{&list,&all,&clock};
     a.on_gate=[&]{list.erase(list.begin());};draw_dynamic_systems(s,&depth);
     check(log==std::vector<std::string>{"depth99","reset","start","a?","a!","stop","a#","reset","start","c?","c!","stop","c#"});}
    // The element is captured before Reset; all three stopwatch reads are fresh.
    {std::vector<std::string> log,second,third;DynamicDepth depth(log);Clock a(log),b(second),c(third);DynamicSystem system(log,"x");
     std::vector<DynamicDrawSystem*> list{&system};std::vector<const void*> all;DynamicDrawSchedule s{&list,&all,&a};
     a.on_reset=[&]{s.stopwatch=&b;list[0]=nullptr;};b.on_start=[&]{s.stopwatch=&c;};draw_dynamic_systems(s,&depth);
     check(log==std::vector<std::string>{"depth99","reset","x?","x!","x#"}&&second==std::vector<std::string>{"start"}&&third==std::vector<std::string>{"stop"});}
    {std::vector<std::string> log;DynamicDepth depth(log);Clock clock(log);DynamicSystem a(log,"a");
     std::vector<DynamicDrawSystem*> list{&a};std::vector<const void*> all;DynamicDrawSchedule s{&list,nullptr,&clock};
     clock.on_stop=[&]{s.all_systems=&all;};a.on_identity=[&]{s.all_systems=nullptr;};draw_dynamic_systems(s,&depth);check(log.back()=="a#");}
    // An absent all-systems list fails after Stop even for a null or disabled entry.
    for(auto kind:{0,1,2}){std::vector<std::string> log;DynamicDepth depth(log);Clock clock(log);DynamicSystem a(log,"a");a.needed=kind!=2;
     std::vector<DynamicDrawSystem*> list{kind==0?nullptr:&a};DynamicDrawSchedule s{&list,nullptr,&clock};
     dynamic_fails([&]{draw_dynamic_systems(s,&depth);});check(log.back()=="stop"&&std::find(log.begin(),log.end(),"a#")==log.end());}
    // Reset/Start/gate/body/Stop failures do not invent later cleanup or identity lookup.
    for(const auto& failure:std::vector<std::string>{"reset","start","gate","draw","stop"}){
     std::vector<std::string> log;DynamicDepth depth(log);Clock clock(log);DynamicSystem a(log,"a");
     std::vector<DynamicDrawSystem*> list{&a};std::vector<const void*> all;DynamicDrawSchedule s{&list,&all,&clock};
     auto fail=[] {throw std::runtime_error("dynamic callback");};
     if(failure=="reset")clock.on_reset=fail;if(failure=="start")clock.on_start=fail;
     if(failure=="gate")a.on_gate=fail;if(failure=="draw")a.on_draw=fail;if(failure=="stop")clock.on_stop=fail;
     dynamic_fails([&]{draw_dynamic_systems(s,&depth);});
     const auto last=failure=="gate"?"a?":failure=="draw"?"a!":failure;
     check(log.back()==last&&std::find(log.begin(),log.end(),"a#")==log.end());}
    for(auto phase:{0,1,2}){std::vector<std::string> log;DynamicDepth depth(log);Clock clock(log);DynamicSystem a(log,"a");
     std::vector<DynamicDrawSystem*> list{&a};std::vector<const void*> all;DynamicDrawSchedule s{&list,&all,phase==0?nullptr:&clock};
     if(phase==1)clock.on_reset=[&]{s.stopwatch=nullptr;};if(phase==2)a.on_draw=[&]{s.stopwatch=nullptr;};
     dynamic_fails([&]{draw_dynamic_systems(s,&depth);});check(log.back()==(phase==0?"depth99":phase==1?"reset":"a!"));}
    {std::vector<std::string> log;DynamicDepth depth(log);Clock clock(log);DynamicSystem a(log,"a");
     std::vector<DynamicDrawSystem*> list{&a};std::vector<const void*> all;DynamicDrawSchedule s{&list,&all,&clock};
     a.identity=nullptr;draw_dynamic_systems(s,&depth);check(log.back()=="a#");} // Not-found and null identities are accepted.
    // Factory order and live enabled references are the only concrete callback gates.
    {std::array<bool,32> enabled;enabled.fill(true);std::array<DynamicDrawSystem*,32> factory{};
     std::vector<std::unique_ptr<GameplayDynamicDraw>> owners;std::vector<int> ran;std::vector<const void*> all;
     for(const auto id:gameplay_dynamic_draw_order){owners.push_back(std::make_unique<GameplayDynamicDraw>(enabled[static_cast<std::size_t>(id)],[&,id]{
         ran.push_back(id);if(id==3)enabled[4]=false;
     },&enabled[static_cast<std::size_t>(id)]));factory[static_cast<std::size_t>(id)]=owners.back().get();all.push_back(owners.back()->base_identity());}
     auto list=make_gameplay_dynamic_draw_list(factory);std::vector<std::string> log;DynamicDepth depth(log);Clock clock(log);DynamicDrawSchedule s{&list,&all,&clock};
     draw_dynamic_systems(s,&depth);check(ran==std::vector<int>{3,5,16,22,23,24,26,30});
     check(std::count(log.begin(),log.end(),"reset")==9&&std::count(log.begin(),log.end(),"stop")==9);
     ran.clear();enabled.fill(false);draw_dynamic_systems(s,&depth);check(ran.empty());factory[22]=nullptr;dynamic_fails([&]{make_gameplay_dynamic_draw_list(factory);});
     dynamic_fails([&]{GameplayDynamicDraw missing(enabled[0],{},nullptr);});}
}
int main(){
    dynamic_contracts();
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
