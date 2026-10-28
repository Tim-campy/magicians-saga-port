#pragma once
#include "enemy_creation.hpp"
#include "enemy_impact.hpp"
#include "combat_event_receivers.hpp"
#include "../ecs/entity_events.hpp"

namespace saga::core {
// Borrowed GaugeSystem identity. Init stores it without requiring a nonnull result;
// no unobserved gauge/draw methods are supplied by this gameplay binding.
class EnemyGaugeService {public:virtual ~EnemyGaugeService()=default;};
class EnemyInitializationWorld {
public:
    virtual ~EnemyInitializationWorld()=default;
    virtual EntitySubset* enemy_subset(const EventIntegers& all)=0;
    virtual EnemyGaugeService* enemy_gauge_system()=0;
    virtual StageStartState* enemy_stage_system()=0;
    virtual MemberRoster* enemy_member_system()=0;
    virtual void subscribe_enemy_impact(EntityEvents::Single,std::optional<EventHashes>)=0;
    virtual void subscribe_enemy_contact(EntityEvents::Pair,std::optional<EventHashes>)=0;
    virtual void subscribe_enemy_human_dead(EntityEvents::Single,std::optional<EventHashes>)=0;
};
struct EnemyEventServices {
    // All receivers borrow this object; its lifetime must cover every subscription.
    const CombatRecordTables& records;
    const ParameterMaximumLevels& maxima;
    RandomSource& random;
    EnemyImpactHost& impact;
    ContactCollisionHost& contact;
    // Bind search_opponent to search_enemy_target on the same EnemyCreationSystem.
    CombatEventHost& retarget;
};
// EnemySystem.Init 0x114c338: supplied world (not a fresh global lookup),
// And(24), Gauge/Stage/Member, independent zero int32[10], then events 23/29/30.
// No counter/quest reset and no unsubscription on repeat or partial failure.
void initialize_enemies(EnemyCreationSystem&,EnemyInitializationWorld*,EnemyEventServices&,
                       std::optional<std::array<EventHashes,3>> hashes=std::nullopt);
}
