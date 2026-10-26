#include "core/ecs/entity_events.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
int main(){
    EntityEvents events;ComponentSlots a(35),b(35);std::vector<int> calls;
    events.send(99,nullptr);events.send(99,nullptr,nullptr);
    CombatReceivers r;
    r.stage_dead=[&](ComponentSlots* p){check(p==&a);calls.push_back(3);};
    r.human_impact=[&](ComponentSlots*){calls.push_back(4);events.send(30,&a);};
    r.enemy_impact=[&](ComponentSlots*){calls.push_back(5);};
    r.enemy_dead=[&](ComponentSlots*){calls.push_back(6);};
    r.projectile_impact=[&](ComponentSlots*){calls.push_back(10);};
    r.human_contact=[&](ComponentSlots* x,ComponentSlots* y){check(x==&a&&y==&b);calls.push_back(14);};
    r.enemy_contact=[&](ComponentSlots*,ComponentSlots*){calls.push_back(15);};
    bind_combat_receivers(events,r);events.send(23,nullptr);
    check(calls==std::vector<int>{4,3,6,5,10});calls.clear();events.send(29,&a,&b);
    check(calls==std::vector<int>{14,15});
    calls.clear();events.send(29,&a);check(calls.empty()); // Separate arities, same id.
    {EntityEvents bus;bool nested=false;
        bus.subscribe(1,EntityEvents::Single([&](ComponentSlots* p){calls.push_back(1);if(!nested){nested=true;
            bus.subscribe(1,EntityEvents::Single([&](ComponentSlots*){calls.push_back(3);}));bus.send(1,p);}}));
        bus.subscribe(1,EntityEvents::Single([&](ComponentSlots*){calls.push_back(2);}));
        bus.send(1,nullptr);check(calls==std::vector<int>{1,1,2,3,2});}
    {EntityEvents bus;calls.clear();
        bus.subscribe(1,EntityEvents::Single([n=0,&calls](ComponentSlots*)mutable{calls.push_back(++n);}));
        bus.send(1,nullptr);bus.subscribe(1,EntityEvents::Single([](ComponentSlots*){}));bus.send(1,nullptr);
        check(calls==std::vector<int>{1,2});} // Preserve callable state when combining.
    {EntityEvents bus;calls.clear();EntityEvents::Pair callback=[&](ComponentSlots*,ComponentSlots*){calls.push_back(7);};
        bus.subscribe(1,callback);bus.subscribe(1,callback);bus.send(1,nullptr,nullptr);check(calls.size()==2);}
    {EntityEvents bus;calls.clear();
        bus.subscribe(1,EntityEvents::Single([](ComponentSlots*){throw std::runtime_error("receiver");}));
        bus.subscribe(1,EntityEvents::Single([&](ComponentSlots*){calls.push_back(2);}));
        bool failed=false;try{bus.send(1,nullptr);}catch(const std::runtime_error&){failed=true;}check(failed&&calls.empty());}
}
