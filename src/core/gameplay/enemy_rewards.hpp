#pragma once
#include "event_catalog.hpp"
#include <optional>
#include <array>

namespace saga::core {
using EnemyTreasureGroups=std::vector<std::vector<EventIntegers>>;
struct EnemyRecord {
    std::int32_t id=0;std::string name;std::int32_t enemy_seb=0;
    std::optional<EventIntegers> experience;
    std::optional<EnemyTreasureGroups> treasures;
    std::int32_t voice=0,attribute=0;std::string resource;
    std::int32_t image=0,seb=0,frame=0,rank=0;
    EventScript parameters;std::int32_t flags=0;
};
using EnemyTable=std::vector<std::optional<EnemyRecord>>;
EnemyTable load_enemy_table(std::string_view,bool normalize_crlf,const EventIntegerParser&);
std::int32_t enemy_experience(const EnemyRecord&,std::int32_t level);
// EnemyData.GetParamValue: maxima is live state, separate from the character array.
std::int32_t enemy_parameter(const EnemyRecord&,std::int32_t index,std::int32_t level,
                             const std::array<std::int32_t,13>& maximum_levels);
class EnemyRewardHost {
public:
    virtual ~EnemyRewardHost()=default;
    virtual const EnemyRecord& enemy_data()=0;
    // Creates pending drops with original positioning/scale/depth; does not credit stage totals.
    virtual void spawn_experience_drop(std::int32_t)=0;
    virtual bool hits1000(std::int32_t threshold)=0;
    virtual void spawn_treasure_drop(std::int32_t id)=0;
};
// Reward suffix of EnemySystem.Defeat, after motion/state/counter updates.
// RNG backend and drop rendering/collection remain explicit host contracts.
void spawn_enemy_rewards(std::int32_t level,EnemyRewardHost&);
}
