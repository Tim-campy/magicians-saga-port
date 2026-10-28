#include "core/ecs/entity_registration.hpp"
#include "core/ecs/entity_creation.hpp"
#include "core/ecs/entity_subset.hpp"
#include <cstdlib>
#include <functional>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Observer final:ComponentObserver {
    int adds=0,removes=0,changes=0;std::function<void()> on_add;
    void added(ComponentSlots&,std::int32_t,Component*)override{++adds;if(on_add)on_add();}
    void removed(ComponentSlots&,std::int32_t,Component*)override{++removes;}
    void changed(ComponentSlots&,std::int32_t,Component*,Component*)override{++changes;}
};
struct FailingSubscriptions final:ComponentSubscriptions {
    ComponentEvents events;std::vector<int> calls;
    void subscribe_added(ComponentObserver& o)override{calls.push_back(1);events.subscribe_added(o);}
    void subscribe_removed(ComponentObserver&)override{calls.push_back(2);throw std::runtime_error("removed subscription");}
    void subscribe_changed(ComponentObserver&)override{calls.push_back(3);}
};
struct Node {
    std::int32_t id;ComponentEvents events;ComponentSlots slots{35,&events};
    EntityRegistration registration{id,&slots,&events};
    explicit Node(std::int32_t value):id(value){}
};
struct World final:EntityCreationHost {
    std::vector<std::unique_ptr<Node>> nodes;
    const std::vector<std::int32_t> all{22};SubsetFilter filter{&all};
    EntitySubset group{filter,[this](const ComponentSlots& e){return node(e).id;}};
    SubsetMembership membership{{&group}};EntityRegistry registry{membership};
    Node& node(const ComponentSlots& e){for(auto& n:nodes)if(&n->slots==&e)return *n;throw std::invalid_argument("node");}
    bool registered_id(std::int32_t id)override{return registry.contains(id);}
    ComponentSlots* create(std::int32_t type,std::int32_t id)override{
        if(type!=0)return nullptr;nodes.push_back(std::make_unique<Node>(id));return &nodes.back()->slots;
    }
    void register_entity(ComponentSlots& e,bool temporary)override{registry.add(&node(e).registration,temporary);}
};
int main(){
    {Observer observer;EntityRegistry registry(observer);ComponentEvents events;ComponentSlots e(35,&events);Component a,b;
        e.add(0,&a);EntityRegistration record{5,&e,&events};registry.add(&record,false);
        check(registry.find(5)==&e&&observer.adds==0); // No initial component synchronization.
        e.change(0,&b);e.remove(0);e.add(0,&a);check(observer.changes==1&&observer.removes==1&&observer.adds==1);
        fails([&]{registry.add(&record,false);});e.add(1,&a);check(observer.adds==2);
        registry.add(&record,true);e.add(2,&a);check(observer.adds==4); // Repeated subscriptions retained.
        registry.erase(5);check(!registry.contains(5));events.dispose();e.add(3,&a);check(observer.adds==4);
        fails([&]{registry.add(nullptr,true);});}
    {Observer observer;EntityRegistry registry(observer);FailingSubscriptions subscriptions;ComponentSlots e(35,&subscriptions.events);Component c;
        EntityRegistration record{8,&e,&subscriptions};fails([&]{registry.add(&record,false);});
        check(registry.find(8)==&e&&subscriptions.calls==std::vector<int>{1,2});
        e.add(0,&c);e.remove(0);check(observer.adds==1&&observer.removes==0);}
    {Observer a,b;ComponentEvents events;ComponentSlots e(35,&events);Component c;
        events.subscribe_added(a);events.subscribe_added(b);a.on_add=[&]{events.dispose();};
        e.add(0,&c);check(a.adds==1&&b.adds==1);e.add(1,&c);check(a.adds==1&&b.adds==1);}
    {Observer a,b;ComponentEvents events;ComponentSlots e(35,&events);Component c;
        events.subscribe_added(a);events.subscribe_added(b);a.on_add=[]{throw std::runtime_error("callback");};
        fails([&]{e.add(0,&c);});check(a.adds==1&&b.adds==0&&e.contains(0));}
    {World w;EntityIdState ids;Component garbage;
        auto& ordinary=create_entity(ids,0,false,w);auto& temporary=create_entity(ids,0,true,w);
        check(w.registry.find(0)==&ordinary&&!w.registry.contains(-1)&&ids.counter==2);
        ordinary.add(22,&garbage);temporary.add(22,&garbage);check(w.group.size()==2&&w.node(temporary).id==-1);
        ordinary.remove_all();check(w.group.size()==1);w.node(ordinary).events.dispose();w.registry.erase(0);
        auto& next=create_entity(ids,0,false,w);check(w.registry.find(2)==&next);next.add(22,&garbage);check(w.group.size()==2);
        fails([&]{create_entity(ids,9,false,w);});check(ids.counter==4);}
}
