#include "core/ecs/entity_events.hpp"
#include "core/ecs/world_teardown.hpp"
#include "core/ecs/dynamic_subsets.hpp"
#include "core/gameplay/stage_cleanup.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Node {int id;ComponentEvents events;ComponentSlots slots{35,&events};EntityRegistration record{id,&slots,&events};QuestEntityLifetime lifetime;explicit Node(int n):id(n){}};
struct World final:RegisteredEntityDestruction,QuestEntityHost,EntityDeletionHost,WorldTeardownHost {
    std::vector<std::unique_ptr<Node>> nodes;ComponentEvents root;EntityRegistry registry{root};RegistryDestruction state{&registry};
    DynamicSubsets groups{registry,[&](const ComponentSlots& e){return node(e).id;}};
    EntityEvents events;std::vector<int> log;std::function<void()> on_remove;
    World(){root.subscribe_added(groups);root.subscribe_removed(groups);root.subscribe_changed(groups);}
    Node& node(const ComponentSlots& e){for(auto& n:nodes)if(&n->slots==&e)return *n;throw std::invalid_argument("entity");}
    Node& node(QuestMemberId id){for(auto& n:nodes)if(n->id==id)return *n;throw std::invalid_argument("id");}
    Node& add(int id,bool temporary=false){nodes.push_back(std::make_unique<Node>(id));auto& n=*nodes.back();registry.add(&n.record,temporary);return n;}
    void destroy(ComponentSlots& e,bool remove)override{check(!remove);destroy_quest_entity(node(e).id,remove,*this);}
    QuestEntityLifetime& entity_lifetime(QuestMemberId id)override{return node(id).lifetime;}
    void delete_from_manager(QuestMemberId id,bool remove)override{delete_entity_from_manager(id,remove,*this);}
    void remove_all_components(QuestMemberId id)override{node(id).slots.remove_all();const auto callback=on_remove;if(callback)callback();}
    void dispose_entity_events(QuestMemberId id)override{node(id).events.dispose();}
    void erase_registration(QuestMemberId id)override{registry.erase(node(id).id);}
    void clear_visible_entities()override{log.push_back(1);}
    void destroy_registered_entities()override{log.push_back(2);destroy_all_entities(state,*this);}
    void remove_systems()override{log.push_back(3);}
    void remove_event_handlers()override{log.push_back(4);events.remove_all();}
};
int main(){
    {World w;Component tag;auto& a=w.add(7);a.slots.add(22,&tag);
        auto key=std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{22});auto& group=w.groups.get(key);
        auto& temporary=w.add(-1,true);temporary.slots.add(22,&tag);check(group.size()==2);
        int events=0;w.events.subscribe(23,EntityEvents::Single([&](ComponentSlots*){++events;}),EventHashes{1,101});
        destroy_world(w);check(w.log==std::vector<int>{1,2,3,4}&&a.lifetime.destroyed&&!temporary.lifetime.destroyed);
        check(!w.registry.contains(7)&&group.size()==1&&&w.groups.get(key)==&group);
        w.events.send(23,nullptr);check(events==0);a.slots.add(22,&tag);check(group.size()==1); // Events disposed.
    }
    {World w;auto& a=w.add(1);auto& b=w.add(2);Component tag;a.slots.add(0,&tag);b.slots.add(0,&tag);
        w.on_remove=[&]{w.on_remove=nullptr;w.add(3);};
        fails([&]{destroy_world(w);});check(w.log==std::vector<int>{1,2});
        check(a.lifetime.destroyed&&!b.lifetime.destroyed&&w.registry.contains(1)&&w.registry.contains(3));}
    {World w;auto& a=w.add(1);Component tag;a.slots.add(0,&tag);
        w.on_remove=[]{throw std::runtime_error("removal");};fails([&]{destroy_world(w);});
        check(!a.lifetime.destroyed&&w.registry.contains(1)&&w.log==std::vector<int>{1,2});}
    {World w;auto& a=w.add(1);Component tag;a.slots.add(0,&tag);a.lifetime.destroyed=true;
        destroy_world(w);check(a.slots.contains(0)&&!w.registry.contains(1));} // Existing destroyed guard skips removal.
    {World w;ComponentObserver observer;EntityRegistry replacement(observer);w.add(1);
        w.on_remove=[&]{w.state.registry=&replacement;};destroy_all_entities(w.state,w);
        check(w.registry.contains(1));} // Enumeration captured old registry; final Clear uses current reference.
    {World w;w.state.registry=nullptr;fails([&]{destroy_all_entities(w.state,w);});}
}
