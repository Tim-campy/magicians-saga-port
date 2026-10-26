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
// StageSystem.ReturnTown: non-clear record, cleanup, writeback, then deferred transition.
void return_to_town(std::int32_t result,QuestReturnHost&);
}
