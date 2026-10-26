#pragma once
#include "combat_parameters.hpp"
#include "enemy_rewards.hpp"

namespace saga::core {
// Non-owning projections of resolved records/components; backing storage must survive a hit.
struct CombatEquipment { EquipmentParameters parameters;std::int32_t attribute=0; };
struct CombatHuman {
    const CharacterParameters* character=nullptr;
    std::array<const CombatEquipment*,4> equipment{};
};
struct CombatEnemy { const EnemyRecord* data=nullptr;std::int32_t level=1; };
struct CombatEntity {
    CombatHuman* human=nullptr;
    CombatEnemy* enemy=nullptr;
    const std::vector<DamageParameter*>* parameters=nullptr;
};
struct CombatConfiguration { bool debug=false,damage_diagnostics=false; };
class CombatPresentation {
public:
    virtual ~CombatPresentation()=default;
    virtual void miss(CombatEntity&)=0;
    virtual void effects(CombatEntity&,const DamageNumbers&)=0;
    virtual void send_damage(CombatEntity&,std::int32_t amount)=0; // World event 15.
    virtual void diagnostics(const DamageNumbers&)=0;
};
// Concrete numeric binding; ECS entity-to-record lookup and presentation remain outside.
class ResolvedDamageHost final:public DamageHost {
public:
    ResolvedDamageHost(CombatEntity& attacker,CombatEntity& receiver,CharacterParameterHost& characters,
        const ParameterMaximumLevels& enemy_maxima,const AttributeAffinityTables& affinities,
        const CombatConfiguration& config,CombatPresentation& presentation)
        :attacker_(attacker),receiver_(receiver),characters_(characters),enemy_maxima_(enemy_maxima),
         affinities_(affinities),config_(config),presentation_(presentation){}
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
    CombatEntity& entity(CombatSide);
    CombatEntity &attacker_,&receiver_;
    CharacterParameterHost& characters_;
    const ParameterMaximumLevels& enemy_maxima_;
    const AttributeAffinityTables& affinities_;
    const CombatConfiguration& config_;
    CombatPresentation& presentation_;
};
// StageSystem.OnDamage 0x11666f8; event receiver must be nonnull.
void record_stage_damage(std::int32_t& total,const CombatEntity*,std::int32_t amount);
}
