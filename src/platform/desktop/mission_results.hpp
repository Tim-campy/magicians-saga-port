#pragma once
#include "core/gameplay/quest_rewards.hpp"
#include "core/gameplay/stage_cleanup.hpp"
#include "core/gameplay/member_roster.hpp"
#include <array>
#include <map>
#include <ostream>
#include <optional>

namespace saga::desktop {
class MissionHumans;
struct MissionResultScenario {
    std::int64_t money,crystals;std::int32_t rescued;
    core::QuestRecord record;
    std::shared_ptr<core::QuestRecord> quest_record;
    std::array<core::ItemStock,3> inventory;
    core::RecruitmentState recruits;
    std::optional<std::array<std::map<std::int32_t,core::ItemStock>,3>> inventory_records;
    std::optional<std::map<std::int32_t,core::RecruitmentState>> recruit_records;
};
// Mutable decoded state. Original save encoding and town presentation remain host boundaries.
class MissionResults final:private core::QuestRewardHost,private core::QuestExperienceHost,
    public core::QuestReturnHost,private core::MemberFinishHost,private core::StageCleanupHost {
public:
    MissionResults(MissionHumans&,MissionResultScenario);
    void grant();
    void report(std::ostream&);
    bool check_crystals(std::int64_t amount)const;
    void subtract_crystals(std::int32_t amount,std::int32_t yugami);
    bool transition_pending()const{return static_cast<bool>(transition_);}
    core::StageCleanupHost& cleanup_service(){return *this;}
private:
    struct Cursor {std::shared_ptr<core::PartyMembers> list;std::uint32_t version=0;std::size_t index=0;};
    std::optional<core::QuestMemberId> next(Cursor&);
    core::StageRewards& drops()override;
    std::optional<std::int32_t> next_treasure()override;
    const core::TreasureRecord& treasure(std::int32_t)override;
    core::ItemStock& item(core::RewardItem,std::int32_t)override;
    core::RecruitmentState& character(std::int32_t)override;
    void add_money(std::int32_t)override;void add_crystals(std::int32_t)override;
    std::int32_t& rescued_prisoners()override;
    core::QuestRecord& quest_record()override{return state_.quest_record?*state_.quest_record:state_.record;}
    core::QuestExperienceHost& members()override;
    std::int32_t defeated_enemies()override;
    std::int32_t collected_prisoners()override;
    std::int32_t total_damage()override;
    std::optional<core::QuestMemberId> next_member()override{return next(grant_cursor_);}
    core::MemberExperience& human(core::QuestMemberId)override;
    std::int32_t parameter_zero(core::QuestMemberId)override;
    std::int32_t table_count()override;
    std::int32_t next_experience(core::MemberExperience&,std::int32_t)override;
    core::QuestRecord& return_record()override{return quest_record();}
    void cleanup_stage()override{core::cleanup_quest_stage(*this);}
    core::MemberFinishHost& finish_members_host()override;
    void make_transition(std::int32_t,std::function<void()>,bool,std::uint32_t)override;
    std::int32_t return_area_id()override;std::int32_t return_stage()override;
    void change_to_town_form(bool)override;void send_town_return(std::int32_t,const core::TownReturnEvent&)override;
    std::optional<core::QuestMemberId> next_finish_member(core::FinishMemberList)override;
    core::MemberExperience& finish_human(core::QuestMemberId id)override{return human(id);}
    core::SavedCharacterProgression* saved_character(core::MemberExperience&)override;
    void clear_finish_members(core::FinishMemberList)override;
    core::QuestEntityLifetime& entity_lifetime(core::QuestMemberId)override;
    void delete_from_manager(core::QuestMemberId,bool)override;
    std::vector<std::optional<core::QuestMemberId>> snapshot_group(std::int32_t)override;
    core::QuestEntityHost& destruction_host()override;
    std::optional<core::QuestMemberId> extra_entity()override;
    void clear_extra_entity()override;void clear_collecting_drops()override;
    MissionHumans& owner_;MissionResultScenario state_;
    std::array<std::map<std::int32_t,core::ItemStock>,3> inventory_;
    std::map<std::int32_t,core::RecruitmentState> recruits_;
    std::shared_ptr<core::StageRewards> granting_;std::size_t treasure_index_=0;
    Cursor grant_cursor_;std::array<Cursor,2> finish_cursors_;
    std::optional<core::QuestRewardResult> result_;
    std::function<void()> transition_;
};
}
