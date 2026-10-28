#include "core/gameplay/pass_initialization.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host final:CombatEventHost {
    int destroyed_count=0;
    bool destroyed(ComponentSlots&)override{return destroyed_count!=0;}
    void destroy(ComponentSlots&,bool remove)override{check(remove);++destroyed_count;}
    ComponentSlots* search_opponent(ComponentSlots&)override{check(false);return nullptr;}
};
struct World final:PassInitializationWorld {
    std::vector<std::vector<std::int32_t>> requests;
    std::vector<std::int32_t> slots{0,1};SubsetFilter filter{&slots};
    EntitySubset group{filter,[](const ComponentSlots&){return 1;}};
    EntityEvents events;ProjectileSystemState* observing=nullptr;
    bool fail_subset=false,fail_subscription=false,null_subset=false;
    EntitySubset* subset(const std::vector<std::int32_t>& all)override{
        requests.push_back(all);if(fail_subset)throw std::runtime_error("subset");
        return null_subset?nullptr:&group; // Synthetic world service, not dynamic GetSubSet.
    }
    void subscribe_impact(EntityEvents::Single callback,std::optional<EventHashes> hashes)override{
        check(observing&&observing->pass.has_value());
        check(observing->subset==(null_subset?nullptr:&group));
        if(fail_subscription)throw std::runtime_error("subscribe");events.subscribe(23,std::move(callback),hashes);
    }
};
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
int main(){
    World world;Host host;ProjectileSystemState projectile;CollisionSystemState collision;
    EntitySubset* move=nullptr;EntitySubset* garbage=nullptr;world.observing=&projectile;
    initialize_projectiles(projectile,&world,host,EventHashes{1,101});
    initialize_movement(move,&world);initialize_collisions(collision,&world);initialize_garbage(garbage,&world);
    check(world.requests==std::vector<std::vector<std::int32_t>>{{27,0,1},{0,1},{32,0},{22}});
    check(move==&world.group&&garbage==&world.group&&collision.subset==&world.group&&collision.pass);
    ComponentSlots emitter(35);Component tag;emitter.add(31,&tag);
    world.events.send(23,&emitter);check(host.destroyed_count==1);
    // Reinitialization subscribes another receiver; Init does not unsubscribe the first.
    initialize_projectiles(projectile,&world,host,EventHashes{1,102});
    world.events.send(23,&emitter);check(host.destroyed_count==3);
    check(world.events.unsubscribe_single(23,1));world.events.send(23,&emitter);check(host.destroyed_count==4);
    world.fail_subset=true;auto* old=projectile.subset;
    fails([&]{initialize_projectiles(projectile,&world,host);});check(projectile.subset==old&&projectile.pass);
    fails([&]{initialize_movement(move,nullptr);});check(move==&world.group);
    world.fail_subset=false;world.null_subset=true;world.fail_subscription=true;
    fails([&]{initialize_projectiles(projectile,&world,host);});check(!projectile.subset&&projectile.pass);
    initialize_collisions(collision,&world);check(!collision.subset&&collision.pass);
    initialize_garbage(garbage,&world);check(!garbage);
}
