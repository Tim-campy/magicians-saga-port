#pragma once
#include "quest_rewards.hpp"
#include <functional>

namespace saga::core {
struct SavedCharacterProgression {std::int32_t level=1,experience=0;};
void set_saved_character_level(SavedCharacterProgression&,std::int32_t);
struct QuestEntityLifetime {bool destroyed=false;};
enum class FinishMemberList { members,sub_members };
class QuestEntityHost {
public:
    virtual ~QuestEntityHost()=default;
    virtual QuestEntityLifetime& entity_lifetime(QuestMemberId)=0;
    // Boolean controls removal from the entity registry, not component notifications.
    virtual void delete_from_manager(QuestMemberId,bool remove_registration)=0;
};
class MemberFinishHost:public QuestEntityHost {
public:
    // Fresh ordered cursors, opened lazily for each list; disposal/version checks belong to host.
    virtual std::optional<QuestMemberId> next_finish_member(FinishMemberList)=0;
    virtual MemberExperience& finish_human(QuestMemberId)=0;
    // May return null; original delays that null check until after the second human lookup.
    virtual SavedCharacterProgression* saved_character(MemberExperience&)=0;
    virtual void clear_finish_members(FinishMemberList)=0;
};
void destroy_quest_entity(QuestMemberId,bool remove_registration,QuestEntityHost&);
// MemberSystem.OnFinish: write memory, destroy sub-members, clear lists. Does not write a save file.
void finish_quest_members(MemberFinishHost&);
struct TownReturnEvent {std::int32_t result,area_id,stage;};
class TownQuestReturnHost {
public:
    virtual ~TownQuestReturnHost()=default;
    virtual void execute_return_event(std::int32_t)=0;
    // Scan the canonical Quest catalog; native null rows/records must fail.
    virtual std::int32_t best_total_damage()=0;
    virtual std::int64_t high_score()=0;
    virtual void set_high_score(std::int64_t)=0;
    virtual bool town_form_exists()=0;
    virtual void mark_quest_return()=0;
};
// TownSystem.OnReturnFromQuest 0x1169b04; area_id/stage are not read here.
void on_town_quest_return(const TownReturnEvent&,TownQuestReturnHost&);
class QuestReturnHost:public QuestRecordHost {
public:
    virtual QuestRecord& return_record()=0;
    virtual void cleanup_stage()=0;
    virtual MemberFinishHost& finish_members_host()=0;
    // Retain the callback and invoke at the original transition point; this host must outlive it.
    virtual void make_transition(std::int32_t mode,std::function<void()>,bool,std::uint32_t color)=0;
    virtual std::int32_t return_area_id()=0;
    virtual std::int32_t return_stage()=0;
    virtual void change_to_town_form(bool)=0;
    virtual void send_town_return(std::int32_t event_id,const TownReturnEvent&)=0;
};
// Inspectable payload of the deferred ReturnTown callback. The borrowed host outlives it.
struct QuestTownContinuation {
    std::int32_t result=0;QuestReturnHost* host=nullptr;
    void operator()()const;
};
// StageSystem.ReturnTown: non-clear record, cleanup, writeback, then deferred transition.
void return_to_town(std::int32_t result,QuestReturnHost&);
}
