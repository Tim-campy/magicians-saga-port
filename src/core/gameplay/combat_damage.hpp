#pragma once
#include "combat_rules.hpp"
#include "game_random.hpp"
#include <cstdint>
#include <unordered_map>

namespace saga::core {
enum class CombatSide { attacker, receiver };
enum class CombatKind { other, enemy, human };
struct DamageParameter { std::int32_t value=0,maximum=0,extra=0,extra_maximum=0; };
using DamageParameterMap=std::unordered_map<std::int32_t,DamageParameter*>;
struct ReceiverDefence { std::int32_t parameter2=0,attribute=4,bonus=0; };
struct DamageNumbers {
    std::int32_t amount=0,affinity=0;
    bool critical=false;
};
// Entity bindings remain explicit. kind() must prefer human over enemy.
// parameter() resolves MemberSystem totals / EnemyData / zero in that order.
class DamageHost {
public:
    virtual ~DamageHost()=default;
    virtual CombatKind kind(CombatSide)=0;
    virtual std::int32_t parameter(CombatSide,std::int32_t index)=0;
    // Captured once on hit; pointee must survive presentation/event callbacks.
    virtual DamageParameter* receiver_parameter_zero()=0;
    // Resolve P2, attribute, then attribute bonus; other => {0,4,0}.
    virtual ReceiverDefence receiver_defence()=0;
    virtual const AttributeAffinityTables& affinity_tables()=0;
    virtual std::int32_t attacker_parameter_one_rate()=0;
    virtual bool debug_enabled()=0;
    virtual void show_miss()=0;
    // Adapter boundary: effects/animation, event 15, then optional diagnostics.
    // Full effect-table selection and the concrete ECS host are not ported yet.
    virtual void after_damage(const DamageNumbers&)=0;
};
// EnemySystem.Damage (0x114fc44): numeric/state composition with explicit host services.
// Return is the death predicate after callbacks, not whether the hit succeeded.
bool apply_combat_damage(std::int32_t attack,std::int32_t attack_attribute,
                         RandomSource&,DamageHost&);
}
