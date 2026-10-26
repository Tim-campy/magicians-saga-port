#include "core/ecs/update_scheduler.hpp"
#include <cstdlib>
#include <deque>
#include <memory>
#include <stdexcept>
#include <string>
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
