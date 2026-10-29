#include "pass_initialization.hpp"
#include <stdexcept>

namespace saga::core {
void initialize_rotation(EntitySubset*& destination,PassInitializationWorld* world){
    if(!world)throw std::invalid_argument("missing rotation initialization world");
    destination=world->subset({12,1,2,7});
}
namespace {
PassInitializationWorld& require(PassInitializationWorld* world){
    if(!world)throw std::invalid_argument("missing gameplay initialization world");
    return *world;
}
}
void initialize_battle(BattleSystemState& state,PassInitializationWorld* world){
    auto& target=require(world);state.humans=target.subset({20,14});state.monsters=target.subset({20,16});
}
void update_battle(BattleSystemState& state){
    if(!state.humans)throw std::invalid_argument("missing battle human subset");
    auto cursor=state.humans->cursor();while(cursor.next()){}
}
void initialize_projectiles(ProjectileSystemState& state,PassInitializationWorld* world,CombatEventHost& host,
                            std::optional<EventHashes> hashes){
    auto& target=require(world);
    state.subset=target.subset({27,0,1});
    state.pass.emplace(); // Fresh persistent completion list, before subscription.
    target.subscribe_impact([&host](ComponentSlots* e){projectile_impact_cleanup(e,host);},hashes);
}
void initialize_movement(EntitySubset*& destination,PassInitializationWorld* world){
    destination=require(world).subset({0,1});
}
void initialize_collisions(CollisionSystemState& state,PassInitializationWorld* world){
    state.subset=require(world).subset({32,0});
    state.pass.emplace(); // Fresh persistent pair list after storing the subset.
}
void initialize_garbage(EntitySubset*& destination,PassInitializationWorld* world){
    destination=require(world).subset({22});
}
void initialize_parameters(ParameterInitializationWorld* world,std::optional<EventHashes> hashes){
    if(!world)throw std::invalid_argument("missing parameter initialization world");
    world->subscribe_dropoff(parameter_dropoff,hashes);
}
void parameter_dropoff(ComponentSlots*) noexcept {}
void update_parameters() noexcept {}
}
