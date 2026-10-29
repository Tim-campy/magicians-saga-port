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
void initialize_debug_visualization(DebugVisualizationState& state,DebugInitializationWorld* world){
    if(!world)throw std::invalid_argument("missing debug initialization world");
    state.subset=world->debug_subset({14,0,1},{9});state.flags=0;
}
namespace {
std::int32_t debug_word(std::uint32_t bits){return static_cast<std::int32_t>(bits<0x80000000U?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);}
}
bool check_debug_flag(const DebugVisualizationState& state,std::int32_t mask){return (static_cast<std::uint32_t>(state.flags)&static_cast<std::uint32_t>(mask))!=0;}
void enable_debug_flag(DebugVisualizationState& state,std::int32_t mask){state.flags=debug_word(static_cast<std::uint32_t>(state.flags)|static_cast<std::uint32_t>(mask));}
void disable_debug_flag(DebugVisualizationState& state,std::int32_t mask){state.flags=debug_word(static_cast<std::uint32_t>(state.flags)&~static_cast<std::uint32_t>(mask));}
void switch_debug_flag(DebugVisualizationState& state,std::int32_t mask){if(check_debug_flag(state,mask))disable_debug_flag(state,mask);else enable_debug_flag(state,mask);}
void reset_debug_flags(DebugVisualizationState& state){state.flags=0;}
void update_debug_visualization(const DebugVisualizationState& state,bool application_debug){
    if(!application_debug||state.flags==0)return;
    // Native tests bits1..256 with BitUtil.Check and discards every result.
    // Preserve the observed inert body; do not introduce pathfinding or drawing.
    for(std::int32_t mask=1;mask<=256;mask*=2)static_cast<void>(check_debug_flag(state,mask));
}
void debug_visualization_enabled(DebugPathVisualization& path){path.visualize=true;path.wait_milliseconds=0;}
void debug_visualization_disabled(DebugPathVisualization& path){path.visualize=false;path.wait_milliseconds=0;}
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
