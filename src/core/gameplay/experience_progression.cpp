#include "experience_progression.hpp"
#include <algorithm>
#include <stdexcept>
namespace saga::core {
namespace {
std::int32_t signed32(std::uint32_t x){return x<=0x7fffffffU?static_cast<std::int32_t>(x):
    static_cast<std::int32_t>(static_cast<std::int64_t>(x)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return signed32(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
}
// data.CharaData.GetExp 0x116db54: witchExp_[0], not the saved exp_ field.
std::int32_t character_witch_experience(const std::vector<std::int32_t>& values){return values.at(0);}
// data.TableData.GetExp 0x116db30: int32 multiply wraps before signed /100.
std::int32_t scaled_level_experience(std::int32_t base,std::int32_t rate){
    return signed32(static_cast<std::uint32_t>(base)*static_cast<std::uint32_t>(rate))/100;
}
// data.CharaData.GetNextExp 0x116d9e4. Preserve table gaps and clamp only the requested level.
std::int32_t next_character_experience(std::int32_t level,std::int32_t rate,
                                      const std::vector<std::optional<std::int32_t>>& table){
    const auto& value=table.at(static_cast<std::size_t>(std::clamp(level,1,1200)));
    if(!value)throw std::invalid_argument("missing experience table row");
    return scaled_level_experience(*value,rate);
}
// ecs.StageSystem.ShowQuestResult 0x1164548, mutation block 0x1164d30..0x1164dd0.
void grant_quest_experience(MemberExperience& member,std::int32_t parameter_zero,
                            std::int32_t total,ExperienceProgressionHost& host){
    if(parameter_zero<1)return;
    auto index=member.level;
    member.experience=add(member.experience,total);
    while(index<host.table_count()){
        const auto cost=host.next_experience(index);
        // ARM64 SUBS/B.LT compares signed operands, even if subtraction overflows.
        if(member.experience<cost)break;
        member.experience=signed32(static_cast<std::uint32_t>(member.experience)-static_cast<std::uint32_t>(cost));
        member.level=std::clamp(add(member.level,1),1,1200);
        index=add(index,1); // Separate cursor continues even when the stored level reaches its cap.
    }
}
}
