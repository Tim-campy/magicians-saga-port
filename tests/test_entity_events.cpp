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
    {EntityEvents bus;calls.clear();bool nested=false;
        bus.subscribe(23,EntityEvents::Single([&](ComponentSlots* p){calls.push_back(1);if(!nested){nested=true;
            check(bus.unsubscribe_single(23,22));bus.send(23,p);}}),EventHashes{11,101});
        bus.subscribe(23,EntityEvents::Single([&](ComponentSlots*){calls.push_back(2);}),EventHashes{22,102});
        bus.send(23,&a);check(calls==std::vector<int>{1,1,2}); // Outer snapshot retains removed receiver.
        calls.clear();bus.send(23,&a);check(calls==std::vector<int>{1});}
    {EntityEvents bus;calls.clear();std::vector<int> log;bus.removal_log=[&](int id){log.push_back(id);};
        bus.subscribe(29,EntityEvents::Pair([&](ComponentSlots*,ComponentSlots*){calls.push_back(1);}),EventHashes{7,100});
        bus.subscribe(29,EntityEvents::Pair([&](ComponentSlots*,ComponentSlots*){calls.push_back(2);}),EventHashes{7,200});
        check(bus.unsubscribe_pair(29,7));bus.send(29,nullptr,nullptr);
        check(calls==std::vector<int>{2}&&log==std::vector<int>{29}); // First hash match, not last duplicate.
        check(!bus.unsubscribe_single(29,7));check(bus.unsubscribe_pair(29,7));check(!bus.unsubscribe_pair(29,7));}
    {EntityEvents bus;calls.clear();
        bus.subscribe(23,EntityEvents::Single([&](ComponentSlots*){calls.push_back(1);}),EventHashes{1,101});
        bus.subscribe(29,EntityEvents::Pair([&](ComponentSlots*,ComponentSlots*){calls.push_back(2);}),EventHashes{2,102});
        bus.remove_all();bus.send(23,nullptr);bus.send(29,nullptr,nullptr);check(calls.empty());bus.remove_all();}
    {EntityEvents bus;calls.clear();
        bus.subscribe(23,EntityEvents::Single([&](ComponentSlots*){calls.push_back(1);}),EventHashes{1,2});
        bus.subscribe(23,EntityEvents::Single([&](ComponentSlots*){calls.push_back(2);}),EventHashes{2,102});
        bool failed=false;try{bus.remove_all();}catch(const std::runtime_error&){failed=true;}
        check(failed);bus.send(23,nullptr);check(calls==std::vector<int>{1}); // Wrapper hash collides with another original hash.
        bus.remove_all();calls.clear();bus.send(23,nullptr);check(calls.empty());}
    {EntityEvents bus;calls.clear();
        bus.subscribe(23,EntityEvents::Single([&](ComponentSlots*){calls.push_back(1);}),EventHashes{1,101});
        bus.subscribe(29,EntityEvents::Pair([](ComponentSlots*,ComponentSlots*){}),EventHashes{2,2});
        bool failed=false;try{bus.remove_all();}catch(const std::runtime_error&){failed=true;}
        check(failed&&!bus.unsubscribe_single(23,1));bus.send(23,nullptr);check(calls==std::vector<int>{1});
        // Single tracking was cleared before pair cleanup failed; dictionaries were not cleared yet.
        bus.remove_all();calls.clear();bus.send(23,nullptr);check(calls.empty());}
    {EntityEvents bus;calls.clear();
        bus.subscribe(23,EntityEvents::Single([&](ComponentSlots*){calls.push_back(1);}),EventHashes{1,101});
        bus.removal_log=[](int){throw std::runtime_error("log");};bool failed=false;
        try{bus.unsubscribe_single(23,1);}catch(const std::runtime_error&){failed=true;}
        check(failed);bus.send(23,nullptr);check(calls.empty()&&!bus.unsubscribe_single(23,1));}
    {EntityEvents bus;bus.subscribe(23,EntityEvents::Single([](ComponentSlots*){}));
        check(!bus.unsubscribe_single(24,1));bool failed=false;
        try{bus.unsubscribe_single(23,1);}catch(const std::logic_error&){failed=true;}check(failed);}
    {EntityEvents bus;calls.clear();std::array<EventHashes,7> hashes{};
        for(std::size_t i=0;i<hashes.size();++i)hashes[i]={static_cast<std::int32_t>(i+1),static_cast<std::int32_t>(i+101)};
        r.hashes=hashes;bind_combat_receivers(bus,r);bus.remove_all();
        bus.send(23,&a);bus.send(29,&a,&b);bus.send(30,&a);check(calls.empty());
        bind_combat_receivers(bus,r);bus.send(29,&a,&b);check(calls==std::vector<int>{14,15});}
}
