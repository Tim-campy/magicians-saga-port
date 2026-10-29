#pragma once
#include "motion_pass.hpp"
#include "collision_pass.hpp"
#include "combat_event_receivers.hpp"
#include "../ecs/entity_events.hpp"
#include <optional>

namespace saga::core {
class PassInitializationWorld {
public:
    virtual ~PassInitializationWorld()=default;
    // World.GetSubSet may return null; dedup/population policy belongs to the world.
    virtual EntitySubset* subset(const std::vector<std::int32_t>& all)=0;
    virtual void subscribe_impact(EntityEvents::Single,std::optional<EventHashes>)=0; // Event 23.
};
struct ProjectileSystemState {EntitySubset* subset=nullptr;std::optional<ProjectilePass> pass;};
struct CollisionSystemState {EntitySubset* subset=nullptr;std::optional<CollisionPass> pass;};
struct BattleSystemState {EntitySubset* humans=nullptr;EntitySubset* monsters=nullptr;};
// BattleSystem.Init 0x114227c: And20/14 then And20/16; nullable results.
void initialize_battle(BattleSystemState&,PassInitializationWorld*);
// Update 0x114236c drains humans only; no attacks, Current reads or monster search.
void update_battle(BattleSystemState&);
struct DebugVisualizationState {EntitySubset* subset=nullptr;std::int32_t flags=0;};
class DebugInitializationWorld {
public:
    virtual ~DebugInitializationWorld()=default;
    virtual EntitySubset* debug_subset(const EventIntegers& all,const EventIntegers& none)=0;
};
void initialize_debug_visualization(DebugVisualizationState&,DebugInitializationWorld*); // 0x1147c7c.
void enable_debug_flag(DebugVisualizationState&,std::int32_t);void disable_debug_flag(DebugVisualizationState&,std::int32_t);
void switch_debug_flag(DebugVisualizationState&,std::int32_t);bool check_debug_flag(const DebugVisualizationState&,std::int32_t);
void reset_debug_flags(DebugVisualizationState&);
void update_debug_visualization(const DebugVisualizationState&,bool application_debug); // 0x1147f68.
struct DebugPathVisualization {bool visualize=false;std::int64_t wait_milliseconds=0;}; // Astar static +0x20/+0x28.
void debug_visualization_enabled(DebugPathVisualization&);void debug_visualization_disabled(DebugPathVisualization&);
// Original Init methods: Projectile 0x115febc, Move 0x115fa0c,
// Collision 0x1145e68 and Garbage 0x11530b8. No enabled/pause guard.
void initialize_projectiles(ProjectileSystemState&,PassInitializationWorld*,CombatEventHost&,
                            std::optional<EventHashes> =std::nullopt);
void initialize_movement(EntitySubset*& destination,PassInitializationWorld*);
// RotateSystem.Init 0x1161e00: exact metadata-backed And({12,1,2,7}), nullable result.
void initialize_rotation(EntitySubset*& destination,PassInitializationWorld*);
void initialize_collisions(CollisionSystemState&,PassInitializationWorld*);
void initialize_garbage(EntitySubset*& destination,PassInitializationWorld*);
class ParameterInitializationWorld {
public:
    virtual ~ParameterInitializationWorld()=default;
    virtual void subscribe_dropoff(EntityEvents::Single,std::optional<EventHashes>)=0; // Event 16.
};
// ParameterSystem.Init 0x115fd8c subscribes only; neither body reads its payload.
void initialize_parameters(ParameterInitializationWorld*,std::optional<EventHashes> =std::nullopt);
void parameter_dropoff(ComponentSlots*) noexcept; // OnDropoff 0x115fe50: RET.
void update_parameters() noexcept; // Update 0x115fe3c: RET; scheduler owns the gate.
}
