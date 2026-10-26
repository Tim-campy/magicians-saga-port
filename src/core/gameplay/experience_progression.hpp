#pragma once
#include <cstdint>
#include <optional>
#include <vector>
namespace saga::core {
std::int32_t character_witch_experience(const std::vector<std::int32_t>&);
std::int32_t scaled_level_experience(std::int32_t base,std::int32_t rate);
std::int32_t next_character_experience(std::int32_t level,std::int32_t need_exp_rate,
                                      const std::vector<std::optional<std::int32_t>>& table_exp);
struct MemberExperience {std::int32_t level,experience;};
class ExperienceProgressionHost {
public:
    virtual ~ExperienceProgressionHost() = default;
    virtual std::int32_t table_count() = 0;
    // Resolve the member's current CharaData and its needExpRate_ at each call.
    virtual std::int32_t next_experience(std::int32_t level_index) = 0;
};
// Per-member EXP/level mutation in ShowQuestResult; parameter slot 0 is supplied by the ECS host.
void grant_quest_experience(MemberExperience&,std::int32_t parameter_zero_value,
                            std::int32_t total_exp,ExperienceProgressionHost&);
// ShowQuestResult snapshots the loop's starting level before querying parameter 0.
void grant_quest_experience_from_level(MemberExperience&,std::int32_t parameter_zero_value,
                                      std::int32_t total_exp,ExperienceProgressionHost&,
                                      std::int32_t entry_level);
}
