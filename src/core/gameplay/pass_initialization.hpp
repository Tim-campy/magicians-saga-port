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
// Original Init methods: Projectile 0x115febc, Move 0x115fa0c,
// Collision 0x1145e68 and Garbage 0x11530b8. No enabled/pause guard.
void initialize_projectiles(ProjectileSystemState&,PassInitializationWorld*,CombatEventHost&,
                            std::optional<EventHashes> =std::nullopt);
void initialize_movement(EntitySubset*& destination,PassInitializationWorld*);
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
