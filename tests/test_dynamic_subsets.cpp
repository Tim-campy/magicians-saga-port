#include "core/ecs/dynamic_subsets.hpp"
#include "core/gameplay/pass_initialization.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Node {int id;ComponentEvents events;ComponentSlots slots{35,&events};EntityRegistration record{id,&slots,&events};explicit Node(int n):id(n){}};
struct World:PassInitializationWorld {
    std::vector<std::unique_ptr<Node>> nodes;ComponentEvents root;EntityRegistry registry{root};
    DynamicSubsets groups{registry,[&](const ComponentSlots& e){for(auto& n:nodes)if(&n->slots==&e)return n->id;throw std::invalid_argument("id");}};
    World(){root.subscribe_added(groups);root.subscribe_removed(groups);root.subscribe_changed(groups);}
    Node& add(int id,bool temporary=false){nodes.push_back(std::make_unique<Node>(id));auto& n=*nodes.back();registry.add(&n.record,temporary);return n;}
    EntityEvents events;
    EntitySubset* subset(const std::vector<std::int32_t>& all)override{return &groups.get(std::make_shared<ComponentGroupFilter>(all));}
    void subscribe_impact(EntityEvents::Single receiver,std::optional<EventHashes> hashes)override{events.subscribe(23,std::move(receiver),hashes);}
};
struct ImpactHost final:CombatEventHost {
    bool destroyed(ComponentSlots&)override{return false;}
    void destroy(ComponentSlots&,bool)override{check(false);}
    ComponentSlots* search_opponent(ComponentSlots&)override{check(false);return nullptr;}
};
struct Observed final:ComponentObserver {int count=0;std::function<void()> callback;void added(ComponentSlots&,std::int32_t,Component*)override{++count;if(callback)callback();}};
struct Custom final:GroupFilter {
    std::function<bool(ComponentSlots*)> match=[](ComponentSlots*){return true;};
    std::function<int()> code=[]{return 7;};int hash_calls=0;
    int hash()override{++hash_calls;return code();}
    bool equals(GroupFilter& other)override{return this==&other;}
    bool matches(ComponentSlots* e)override{return match(e);}
};
std::shared_ptr<ComponentGroupFilter> filter(const std::vector<std::int32_t>& all){auto f=std::make_shared<ComponentGroupFilter>();f->identity.lists.all=&all;return f;}
int main(){
    {World w;ImpactHost host;auto& entity=w.add(1);Component position,speed,projectile_tag,collision_tag,garbage_tag;
        entity.slots.add(0,&position).add(1,&speed);
        ProjectileSystemState projectiles;CollisionSystemState collisions;EntitySubset* movement=nullptr;EntitySubset* garbage=nullptr;
        initialize_projectiles(projectiles,&w,host);initialize_movement(movement,&w);initialize_collisions(collisions,&w);initialize_garbage(garbage,&w);
        check(movement->size()==1&&projectiles.subset->size()==0&&collisions.subset->size()==0&&garbage->size()==0);
        check(w.subset({0,1})==movement);
        entity.slots.add(27,&projectile_tag).add(32,&collision_tag).add(22,&garbage_tag);
        check(projectiles.subset->size()==1&&collisions.subset->size()==1&&garbage->size()==1);
        auto& temporary=w.add(-1,true);temporary.slots.add(0,&position).add(1,&speed).add(27,&projectile_tag);
        check(projectiles.subset->size()==2&&movement->size()==2&&!w.registry.contains(-1));
    }
    {World w;Component tag;auto& a=w.add(30);auto& b=w.add(10);auto& c=w.add(20);
        a.slots.add(22,&tag);b.slots.add(22,&tag);c.slots.add(22,&tag);w.registry.erase(30);w.registry.erase(10);
        auto& d=w.add(40);auto& e=w.add(50);d.slots.add(22,&tag);e.slots.add(22,&tag);
        auto& temporary=w.add(-1,true);temporary.slots.add(22,&tag);
        std::vector<std::int32_t> all{22};auto key=filter(all);auto& group=w.groups.get(key);
        auto cursor=group.cursor();check(cursor.next()==&e.slots&&cursor.next()==&d.slots&&cursor.next()==&c.slots&&!cursor.next());
        check(&w.groups.get(filter(all))==&group);Observed observed;group.notifications=&observed;
        temporary.slots.add(0,&tag);check(group.size()==4&&observed.count==1);
        temporary.slots.change(22,nullptr);check(group.size()==4);temporary.slots.remove(0);check(group.size()==3);
        fails([&]{w.groups.get(nullptr);});}
    {World w;auto& a=w.add(1);auto key=std::make_shared<Custom>();int matches=0;
        key->match=[&](ComponentSlots*){++matches;return true;};auto& group=w.groups.get(key);
        check(matches==1&&key->hash_calls==1); // Cold lookup skips hashing until publication.
        check(&w.groups.get(key)==&group&&matches==1&&key->hash_calls==2);
        (void)a;}
    {World w;Component tag;auto& a=w.add(1);std::vector<std::int32_t> all{0};auto key=filter(all);auto& first=w.groups.get(key);
        all[0]=1;auto fresh=filter(all);check(&w.groups.get(fresh)!=&first); // Cached old key hash is retained.
        a.slots.add(1,&tag);check(first.size()==1);}
    {World w;auto& a=w.add(1);auto key=std::make_shared<Custom>();bool once=false;
        key->match=[&](ComponentSlots*){if(!once){once=true;w.registry.erase(1);}return true;};
        fails([&]{w.groups.get(key);});check(key->hash_calls==0); // Population failure precedes publication/hash.
        auto& empty=w.groups.get(key);check(empty.size()==0&&key->hash_calls==1);(void)a;}
    {World w;w.add(1);auto key=std::make_shared<Custom>();bool nested=false;EntitySubset* published=nullptr;
        key->match=[&](ComponentSlots*){if(!nested){nested=true;published=&w.groups.get(key);}return true;};
        fails([&]{w.groups.get(key);});check(published&&&w.groups.get(key)==published);}
    {World w;Component tag;auto& a=w.add(1);std::vector<std::int32_t> empty,all{1};auto& first=w.groups.get(filter(empty));
        Observed observer;first.notifications=&observer;observer.callback=[&]{w.groups.get(filter(all));};
        fails([&]{a.slots.add(0,&tag);});check(a.slots.contains(0)&&observer.count==1);
        // The group created during the callback is published; the old directory cursor invalidates.
        check(w.groups.get(filter(all)).size()==0);}
}
