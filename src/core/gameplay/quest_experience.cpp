#include "quest_experience.hpp"

namespace saga::core {
namespace {
std::int32_t subtract(std::int32_t a,std::int32_t b) {
    const auto value=static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b);
    return value<=0x7fffffffU?static_cast<std::int32_t>(value):
        static_cast<std::int32_t>(static_cast<std::int64_t>(value)-0x100000000LL);
}
class MemberHost final : public ExperienceProgressionHost {
public:
    MemberHost(QuestExperienceHost& host,MemberExperience& human):host_(host),human_(human){}
    std::int32_t table_count() override {return host_.table_count();}
    std::int32_t next_experience(std::int32_t index) override {return host_.next_experience(human_,index);}
private:
    QuestExperienceHost& host_;
    MemberExperience& human_;
};
}
QuestCharacterResult make_quest_character_result(QuestMemberId id,std::int32_t gained,
    std::int32_t previous_exp,std::int32_t total,std::int32_t current_level) {
    return {id,gained,total,subtract(current_level,gained),previous_exp,-1};
}
std::vector<QuestCharacterResult> grant_quest_member_experience(std::int32_t total,
                                                              QuestExperienceHost& host) {
    std::vector<QuestCharacterResult> results;
    while(const auto id=host.next_member()) {
        auto& human=host.human(*id);
        const auto entry_level=human.level;
        const auto entry_exp=human.experience;
        const auto eligibility=host.parameter_zero(*id);
        MemberHost progression(host,human);
        grant_quest_experience_from_level(human,eligibility,total,progression,entry_level);
        const auto gained=subtract(human.level,entry_level);
        // The original result constructor queries Entity.get_human again, even for ineligible members.
        const auto current_level=host.human(*id).level;
        results.push_back(make_quest_character_result(*id,gained,entry_exp,total,current_level));
    }
    return results;
}
}
