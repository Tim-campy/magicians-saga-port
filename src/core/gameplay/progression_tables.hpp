#pragma once
#include "event_catalog.hpp"
#include "experience_progression.hpp"
#include <array>

namespace saga::core {
struct LevelTableRecord {
    std::int32_t id=0, level=0, experience=0;
    std::array<std::int32_t,3> shop_parameters{};
    std::int32_t unlock_money=0, unlock_crystal=0, unlock_web_page=0, unlock_rescue=0;
};
// Projection of CharaData.Load, not its mutable save state.
struct CharacterProgressionRecord {
    std::int32_t id=0, required_experience_rate=0;
    std::int32_t weapon_id=0,initial_level=0,flags=0;
    EventScript parameters;
    std::int32_t image=0;
};
using LevelTable = std::vector<std::optional<LevelTableRecord>>;
using CharacterProgressionTable = std::vector<std::optional<CharacterProgressionRecord>>;
LevelTable load_level_table(std::string_view, bool normalize_crlf, const EventIntegerParser&);
CharacterProgressionTable load_character_progression_table(std::string_view, bool normalize_crlf,
                                                          const EventIntegerParser&);
// Adapter holds references so subsequent changes to decoded records remain visible.
class TableExperienceHost final : public ExperienceProgressionHost {
public:
    TableExperienceHost(const LevelTable& levels, const CharacterProgressionTable& characters,
                        const std::int32_t& character_id)
        : levels_(levels), characters_(characters), character_id_(character_id) {}
    std::int32_t table_count() override;
    std::int32_t next_experience(std::int32_t level_index) override;
private:
    const LevelTable& levels_;
    const CharacterProgressionTable& characters_;
    const std::int32_t& character_id_;
};
}
