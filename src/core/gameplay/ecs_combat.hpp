#pragma once
#include "combat_records.hpp"

namespace saga::core {
struct CombatParameterLinks final:Component {const DamageParameterMap* values=nullptr;};
struct CombatSavedLevels {
    const EventIntegers &characters,&equipment,&hats,&cloth;
};
class EcsCombatPresentation {
public:
    virtual ~EcsCombatPresentation()=default;
    virtual void miss(ComponentSlots&)=0;
    virtual void effects(ComponentSlots&,const DamageNumbers&)=0;
    virtual void send_damage(ComponentSlots&,std::int32_t)=0;
    virtual void diagnostics(const DamageNumbers&)=0;
};
// Queries slots/IDs/catalogs afresh. No component or equipment projection cache.
class EcsDamageHost final:public DamageHost {
public:
    EcsDamageHost(ComponentSlots& attacker,ComponentSlots& receiver,const CombatRecordTables& records,
        const CombatSavedLevels& levels,CharacterParameterHost& characters,
        const ParameterMaximumLevels& enemy_maxima,const AttributeAffinityTables& affinities,
        const CombatConfiguration& config,EcsCombatPresentation& presentation)
        :attacker_(attacker),receiver_(receiver),records_(records),levels_(levels),characters_(characters),
        enemy_maxima_(enemy_maxima),affinities_(affinities),config_(config),presentation_(presentation){}
    CombatKind kind(CombatSide) override;
    std::int32_t parameter(CombatSide,std::int32_t) override;
    DamageParameter* receiver_parameter_zero() override;
    ReceiverDefence receiver_defence() override;
    const AttributeAffinityTables& affinity_tables() override{return affinities_;}
    std::int32_t attacker_parameter_one_rate() override;
    bool debug_enabled() override{return config_.debug;}
    void show_miss() override{presentation_.miss(receiver_);}
    void after_damage(const DamageNumbers&) override;
private:
    ComponentSlots& entity(CombatSide);
    ComponentSlots &attacker_,&receiver_;
    const CombatRecordTables& records_;
    const CombatSavedLevels& levels_;
    CharacterParameterHost& characters_;
    const ParameterMaximumLevels& enemy_maxima_;
    const AttributeAffinityTables& affinities_;
    const CombatConfiguration& config_;
    EcsCombatPresentation& presentation_;
};
DamageParameter* resolve_combat_parameter(const ComponentSlots&,std::int32_t key);
void record_ecs_stage_damage(std::int32_t& total,const ComponentSlots&,std::int32_t amount);
}
