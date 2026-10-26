#pragma once
#include "event_catalog.hpp"
#include "quest_experience.hpp"
#include "stage_rewards.hpp"
#include <array>
#include <string>

namespace saga::core {
struct TreasureRecord {
    std::int32_t id=0;
    std::string name,resource;
    std::int32_t image=0,experience=0,money=0,crystals=0;
    std::int32_t weapon_id=-1,character_id=-1,cloth_id=-1,hat_id=-1,flags=0;
};
using TreasureTable=std::vector<std::optional<TreasureRecord>>;
TreasureTable load_treasure_table(std::string_view,bool normalize_crlf,const EventIntegerParser&);
enum class RewardItem { weapon,cloth,hat };
struct RecruitmentState {std::int32_t discovery_state=0;bool is_new=false;std::int32_t recruited=0;};
void recruit_character(RecruitmentState&);
struct QuestRecord {
    std::int32_t best_experience=0,best_money=0,challenges=0,clears=0;
    std::int32_t defeated=0,rescued=0,best_damage=0;
};
class QuestRecordHost {
public:
    virtual ~QuestRecordHost()=default;
    virtual std::int32_t defeated_enemies()=0;
    virtual std::int32_t collected_prisoners()=0;
    virtual std::int32_t total_damage()=0;
};
void update_quest_record(QuestRecord&,std::int32_t result,std::int32_t experience,
                         std::int32_t money,QuestRecordHost&);
struct QuestRewardResult {
    std::int32_t experience=0,money=0,crystals=0,prisoners=0;
    using Grant=std::array<std::int32_t,2>;
    std::vector<Grant> weapons,characters,clothes,hats;
    std::vector<QuestCharacterResult> members;
};
class QuestRewardHost:public QuestRecordHost {
public:
    virtual StageRewards& drops()=0;
    // Fresh ordered treasure cursor; retain duplicates and enforce host collection validity.
    virtual std::optional<std::int32_t> next_treasure()=0;
    virtual const TreasureRecord& treasure(std::int32_t)=0;
    virtual ItemStock& item(RewardItem,std::int32_t)=0;
    virtual RecruitmentState& character(std::int32_t)=0;
    // Original AppData secure-value storage remains an explicit adapter.
    virtual void add_money(std::int32_t)=0;
    virtual void add_crystals(std::int32_t)=0;
    virtual std::int32_t& rescued_prisoners()=0;
    virtual QuestRecord& quest_record()=0;
    virtual QuestExperienceHost& members()=0;
};
// ShowQuestResult grant pipeline. Caller binds world/data/storage; display consumes the result.
// No rollback: faults preserve earlier effects and prevent later grants/presentation.
QuestRewardResult grant_quest_rewards(QuestRewardHost&);
}
