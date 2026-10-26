#pragma once
#include "combat_state.hpp"
#include "projectile_launch.hpp"

namespace saga::core {
inline constexpr std::int32_t defeat_color_slot=6,drop_depth_slot=4;
struct DefeatColor final:Component {std::array<std::int32_t,4> values{};}; // Raw Color +0x10..+0x1c, not normalized RGBA.
struct DropDepth final:Component {std::int32_t extra=0;}; // Depth +0x14.
struct EnemyDefeatCounters {std::int32_t defeated_enemies=0,remaining_enemies=0;}; // System +0x34/+0x68; caller owns initialization.
class EnemyDefeatHost:public ProjectileLaunchHost {
public:
    virtual std::int32_t& saved_defeat_count(const EnemyRecord&)=0; // EnemyData +0x20, separate from decoded catalog.
    virtual ComponentSlots* create_drop(std::int32_t type,std::int32_t value,ProjectilePoint,std::int32_t scale)=0;
};
// EnemyData.AddDefeatCount 0x116eaf4: wrap before clamping, including negative increments.
void add_enemy_defeat_count(std::int32_t&,std::int32_t increment);
// EnemySystem.Defeat 0x1150b98. No already-defeated guard; rewards are pending drops only.
void defeat_enemy(ComponentSlots&,const CombatRecordTables&,EnemyDefeatCounters&,RandomSource&,EnemyDefeatHost&);
}
