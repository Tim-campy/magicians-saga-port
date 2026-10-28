#include "core/ecs/system_initialization.hpp"
#include "core/ecs/entity_events.hpp"
#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <utility>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct System final:InitSystem {
    std::function<void(InitializationWorld*)> body;
    explicit System(std::function<void(InitializationWorld*)> b):body(std::move(b)){}
    void init(InitializationWorld* world)override{body(world);}
};
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
int main(){
    {InitializationWorld world;std::vector<int> calls;InitList list;InitializationSchedule s{&list,true};
        System a([&](InitializationWorld* p){check(p==&world&&s.paused);calls.push_back(1);});
        System b([&](InitializationWorld* p){check(p==&world);calls.push_back(2);});
        list.append(&a);list.append(&b);initialize_systems(s,&world);
        check(!s.paused&&calls==std::vector<int>{1,2});}
    {InitList list;InitializationSchedule s{&list,true};System a([&](InitializationWorld* p){check(!p);list.append(nullptr);});
        list.append(&a);fails([&]{initialize_systems(s,nullptr);});check(s.paused);}
    {InitList list,other;InitializationSchedule s{&list,true};int count=0;
        System a([&](InitializationWorld*){s.systems=&other;});System b([&](InitializationWorld*){++count;});
        list.append(&a);list.append(&b);initialize_systems(s,nullptr);check(count==1&&!s.paused);}
    {InitList list;InitializationSchedule s{&list,true};System a([&](InitializationWorld*){list.replace(0,nullptr);});
        list.append(&a);fails([&]{initialize_systems(s,nullptr);});check(s.paused);}
    {InitList list;InitializationSchedule s{&list,true};list.append(nullptr);fails([&]{initialize_systems(s,nullptr);});check(s.paused);
        list.clear();initialize_systems(s,nullptr);check(!s.paused);s.systems=nullptr;fails([&]{initialize_systems(s,nullptr);});}
    {InitList list;InitializationSchedule s{&list,true};EntityEvents events;std::vector<int> calls;
        System stage([&](InitializationWorld*){events.subscribe(30,EntityEvents::Single([&](ComponentSlots*){calls.push_back(3);}));});
        System human([&](InitializationWorld*){events.subscribe(23,EntityEvents::Single([&](ComponentSlots* e){calls.push_back(4);events.send(30,e);}));});
        System enemy([&](InitializationWorld*){events.subscribe(23,EntityEvents::Single([&](ComponentSlots*){calls.push_back(5);}));
            events.subscribe(30,EntityEvents::Single([&](ComponentSlots*){calls.push_back(6);}));});
        System projectile([&](InitializationWorld*){events.subscribe(23,EntityEvents::Single([&](ComponentSlots*){calls.push_back(10);}));});
        list.append(&stage);list.append(&human);list.append(&enemy);list.append(&projectile);
        initialize_systems(s,nullptr);events.send(23,nullptr);check(calls==std::vector<int>{4,3,6,5,10});}
    {InitList list;InitializationSchedule s{&list,true};EntityEvents events;int calls=0;
        System a([&](InitializationWorld*){events.subscribe(23,EntityEvents::Single([&](ComponentSlots*){++calls;}));throw std::runtime_error("init");});
        list.append(&a);fails([&]{initialize_systems(s,nullptr);});events.send(23,nullptr);check(calls==1&&s.paused);}
}
