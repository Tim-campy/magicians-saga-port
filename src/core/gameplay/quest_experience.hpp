#pragma once
#include "experience_progression.hpp"

namespace saga::core {
using QuestMemberId = std::uint64_t; // Opaque entity identity, not a CharaData row id.
struct QuestCharacterResult {
    QuestMemberId character;
    std::int32_t levels_gained, added_experience, display_level, display_experience;
    std::int32_t level_up_frame;
};
// resultCharacter_t..ctor 0x11e4a90. current_level comes from a second human lookup.
QuestCharacterResult make_quest_character_result(QuestMemberId, std::int32_t levels_gained,
    std::int32_t previous_experience, std::int32_t total_experience, std::int32_t current_level);

class QuestExperienceHost {
public:
    virtual ~QuestExperienceHost() = default;
    // Supply a fresh MemberSystem.members_ cursor in original order, retaining duplicates.
    // The host owns iterator lifetime/version checking; exhaustion is distinct from an invalid entity.
    virtual std::optional<QuestMemberId> next_member() = 0;
    virtual MemberExperience& human(QuestMemberId) = 0;
    virtual std::int32_t parameter_zero(QuestMemberId) = 0;
    virtual std::int32_t table_count() = 0;
    // Resolve CharaData from the captured human, not a newly fetched entity component.
    virtual std::int32_t next_experience(MemberExperience&,std::int32_t level_index) = 0;
};
// ShowQuestResult member cluster, after currency/prisoner/quest-record grants.
// Returns all display records only on success; exceptions preserve earlier member mutations.
std::vector<QuestCharacterResult> grant_quest_member_experience(std::int32_t total,
                                                              QuestExperienceHost&);
}
