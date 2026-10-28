#pragma once
#include "human_frame.hpp"
#include "human_contact.hpp"
#include "human_impact.hpp"
#include "../ecs/entity_events.hpp"

namespace saga::core {
class HumanInitializationWorld {
public:
    virtual ~HumanInitializationWorld()=default;
    virtual EntitySubset* human_subset(const EventIntegers&)=0;
    virtual StageGeometry* human_map()=0;
    virtual StageStartState* human_stage_system()=0;
    virtual EnemyCreationSystem* human_enemy_system()=0;
    virtual MemberRoster* human_member_system()=0;
    virtual void subscribe_human_impact(EntityEvents::Single,std::optional<EventHashes>)=0;
    virtual void subscribe_human_contact(EntityEvents::Pair,std::optional<EventHashes>)=0;
};
struct HumanEventServices {const CombatRecordTables& records;HumanImpactHost& impact;HumanContactHost& contact;};
// HumanSystem.Init 0x1154f10: supplied world, And(14), map, Stage/Enemy/Member,
// then event 23 impact and event 29 contact; no counter reset or unsubscription.
void initialize_humans(HumanFrameSystem&,HumanInitializationWorld*,HumanEventServices&,
                       std::optional<std::array<EventHashes,2>> hashes=std::nullopt);
}
