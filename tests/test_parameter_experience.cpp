#include "core/gameplay/parameter_experience.hpp"
#include "core/gameplay/pass_initialization.hpp"
#include <cstdlib>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool value){if(!value)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Parameters {
    ComponentSlots entity{35};DamageParameter p;DamageParameterMap values{{0,&p}};CombatParameterLinks links;
    Parameters(){links.values=&values;entity.add(combat_parameter_slot,&links);p.level=1;p.max_level=10;p.max_experience=10;}
};
struct World final:ParameterInitializationWorld,ParameterExperienceInitializationWorld {
    EntityEvents events;std::vector<Receiver> receivers;std::vector<std::optional<EventHashes>> hashes;
    bool fail=false;
    void subscribe_dropoff(EntityEvents::Single f,std::optional<EventHashes> h)override{
        if(fail)throw std::runtime_error("subscription");events.subscribe(16,std::move(f),h);
    }
    void subscribe_parameter_experience(Receiver f,std::optional<EventHashes> h)override{
        if(fail)throw std::runtime_error("subscription");receivers.push_back(std::move(f));hashes.push_back(h);
    }
    void send(ComponentSlots* e,std::int32_t key){for(auto& f:receivers)f(e,key);}
};
int main(){
    const auto low=std::numeric_limits<std::int32_t>::min(),high=std::numeric_limits<std::int32_t>::max();
    {ParameterExperienceSystem s;check(s.queues&&s.queues->size()==13);
     for(std::size_t i=0;i<13;++i){check(s.queues->at(i)&&s.queues->at(i)->empty());
         for(std::size_t j=0;j<i;++j)check(s.queues->at(i)!=s.queues->at(j));}
     update_parameter_experience(s);}
    {Parameters a;World w;ParameterExperienceSystem s;const auto queues=s.queues;
     initialize_parameter_experience(s,&w,EventHashes{21,121});w.send(&a.entity,0);w.send(nullptr,12);
     check(s.queues==queues&&s.queues->at(0)->size()==1&&s.queues->at(12)->at(0)==nullptr);
     check(a.p.experience==0&&a.p.experience_buffer==0&&a.p.level==1);
     initialize_parameter_experience(s,&w,EventHashes{22,122});w.send(&a.entity,0);
     check(s.queues==queues&&s.queues->at(0)->size()==3&&w.hashes.at(0)->original==21&&w.hashes.at(1)->wrapper==122);
     fails([&]{initialize_parameter_experience(s,nullptr);});w.fail=true;
     fails([&]{initialize_parameter_experience(s,&w);});check(s.queues==queues&&w.receivers.size()==2);}
    {Parameters a;World w;initialize_parameters(&w,EventHashes{16,116});initialize_parameters(&w,EventHashes{16,117});
     w.events.send(16,nullptr);w.events.send(16,&a.entity);update_parameters();check(a.p.level==1&&a.p.value==0);
     check(w.events.unsubscribe_single(16,16)&&w.events.unsubscribe_single(16,16)&&!w.events.unsubscribe_single(16,16));
     fails([&]{initialize_parameters(nullptr);});w.fail=true;fails([&]{initialize_parameters(&w);});}
    {Parameters a;check(resolve_combat_parameter(a.entity,0)==&a.p);fails([&]{resolve_combat_parameter(a.entity,3);});
     a.values[3]=nullptr;check(resolve_combat_parameter(a.entity,3)==nullptr);
     a.links.values=nullptr;fails([&]{resolve_combat_parameter(a.entity,0);});
     a.entity.remove(combat_parameter_slot);fails([&]{resolve_combat_parameter(a.entity,0);});}
    {Parameters a;a.p.value=71;a.p.maximum=81;a.p.extra=5;a.p.extra_maximum=6;a.p.experience=7;a.p.experience_buffer=8;
     check(add_parameter_level(a.p,20)==9&&a.p.level==10);check(add_parameter_level(a.p,-50)==0&&a.p.level==10);
     a.p.level=3;check(add_parameter_level(a.p,-7)==-2&&a.p.level==1);
     a.p.level=-1;a.p.max_level=0;check(add_parameter_level(a.p,0)==2&&a.p.level==1); // Upper clamp precedes lower.
     a.p.level=high-1;a.p.max_level=high;check(add_parameter_level(a.p,2)==-2147483645&&a.p.level==1);
     a.p.level=low;check(add_parameter_level(a.p,0)==-2147483647&&a.p.level==1);
     check(a.p.value==71&&a.p.maximum==81&&a.p.extra==5&&a.p.extra_maximum==6&&a.p.experience==7&&a.p.experience_buffer==8);}
    {Parameters a;ParameterExperienceSystem s;a.p.experience=8;a.p.experience_buffer=12;a.p.value=123;a.p.maximum=456;
     queue_parameter_experience(s,&a.entity,0);update_parameter_experience(s);
     check(a.p.experience_buffer==7&&a.p.experience==3&&a.p.level==2&&s.queues->at(0)->size()==1);
     update_parameter_experience(s);check(a.p.experience_buffer==2&&a.p.experience==8&&a.p.level==2);
     update_parameter_experience(s);check(a.p.experience_buffer==0&&a.p.experience==0&&a.p.level==3&&s.queues->at(0)->empty());
     check(a.p.value==123&&a.p.maximum==456&&a.p.max_experience==10);}
    {Parameters a;ParameterExperienceSystem s;a.p.experience_buffer=11;
     queue_parameter_experience(s,&a.entity,0);queue_parameter_experience(s,&a.entity,0);update_parameter_experience(s);
     check(a.p.experience_buffer==1&&a.p.experience==0&&a.p.level==2&&s.queues->at(0)->size()==2);
     update_parameter_experience(s);check(a.p.experience_buffer==0&&a.p.experience==1&&s.queues->at(0)->empty());}
    {Parameters a,b;ParameterExperienceSystem s;a.p.experience_buffer=1;b.p.experience_buffer=2;
     queue_parameter_experience(s,&a.entity,0);queue_parameter_experience(s,&b.entity,0);update_parameter_experience(s);
     check(a.p.experience==1&&b.p.experience==2&&s.queues->at(0)->empty());} // Removal must not skip the shifted entry.
    {Parameters a;ParameterExperienceSystem s;a.p.level=10;a.p.experience=9;a.p.experience_buffer=2;
     queue_parameter_experience(s,&a.entity,0);update_parameter_experience(s);
     check(a.p.level==10&&a.p.experience==1&&a.p.experience_buffer==0);}
    {Parameters a;ParameterExperienceSystem s;a.p.max_experience=0;a.p.experience_buffer=5;
     queue_parameter_experience(s,&a.entity,0);update_parameter_experience(s);check(a.p.experience==5&&a.p.level==6);}
    {Parameters a;ParameterExperienceSystem s;a.p.max_experience=-2;a.p.experience_buffer=2;
     queue_parameter_experience(s,&a.entity,0);update_parameter_experience(s);check(a.p.experience==6&&a.p.level==3);}
    for(const auto buffer:{0,-1,low}){Parameters a;ParameterExperienceSystem s;a.p.experience_buffer=buffer;a.p.experience=7;
     queue_parameter_experience(s,&a.entity,0);update_parameter_experience(s);
     check(a.p.experience_buffer==0&&a.p.experience==7&&a.p.level==1&&s.queues->at(0)->empty());}
    {Parameters a;ParameterExperienceSystem s;a.p.experience=high;a.p.max_experience=high;a.p.experience_buffer=1;
     queue_parameter_experience(s,&a.entity,0);update_parameter_experience(s);check(a.p.experience==low&&a.p.level==1);}
    {Parameters a;a.p.experience=low;a.p.max_experience=1;level_up_parameter(&a.entity,0);check(a.p.experience==high&&a.p.level==2);
     a.p.experience=1;a.p.max_experience=5;level_up_parameter(&a.entity,0);check(a.p.experience==0&&a.p.level==3);
     fails([&]{level_up_parameter(nullptr,0);});fails([&]{level_up_parameter(&a.entity,9);});}
    {Parameters a;ParameterExperienceSystem s;a.p.experience_buffer=1;queue_parameter_experience(s,&a.entity,0);queue_parameter_experience(s,nullptr,0);
     fails([&]{update_parameter_experience(s);});check(a.p.experience==1&&a.p.experience_buffer==0&&s.queues->at(0)->size()==1);
     check(s.queues->at(0)->at(0)==nullptr);}
    {ParameterExperienceSystem s;fails([&]{queue_parameter_experience(s,nullptr,-1);});fails([&]{queue_parameter_experience(s,nullptr,13);});
     s.queues->at(0).reset();fails([&]{queue_parameter_experience(s,nullptr,0);});fails([&]{update_parameter_experience(s);});
     s.queues.reset();fails([&]{queue_parameter_experience(s,nullptr,0);});fails([&]{update_parameter_experience(s);});}
    // Exercise all thirteen keys; no HP/alive/human predicate is consulted.
    for(std::int32_t key=0;key<13;++key){Parameters a;ParameterExperienceSystem s;a.values={{key,&a.p}};a.p.experience_buffer=1;
     queue_parameter_experience(s,&a.entity,key);update_parameter_experience(s);check(a.p.experience==1&&s.queues->at(static_cast<std::size_t>(key))->empty());}
}
