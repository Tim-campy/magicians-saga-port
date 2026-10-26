#include "quest_return.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
void set_saved_character_level(SavedCharacterProgression& character,std::int32_t level){
    // CharaData.set_level 0x116d5b4; exp_ is a distinct, unclamped field.
    character.level=std::clamp(level,1,1200);
}
void destroy_quest_entity(QuestMemberId id,bool notify,MemberFinishHost& host){
    // Entity.Destroy 0x1131aa0: manager deletion precedes the destroyed flag store.
    auto& lifetime=host.entity_lifetime(id);
    if(lifetime.destroyed)return;
    host.delete_from_manager(id,notify);
    lifetime.destroyed=true;
}
void finish_quest_members(MemberFinishHost& host){
    // MemberSystem.OnFinish 0x115d618: three distinct Entity.get_human calls per member.
    while(const auto id=host.next_finish_member(FinishMemberList::members)){
        auto& first_human=host.finish_human(*id);
        auto* character=host.saved_character(first_human);
        const auto& level_human=host.finish_human(*id);
        if(!character)throw std::invalid_argument("missing saved character");
        set_saved_character_level(*character,level_human.level);
        character->experience=host.finish_human(*id).experience;
    }
    while(const auto id=host.next_finish_member(FinishMemberList::sub_members))
        destroy_quest_entity(*id,true,host);
    host.clear_finish_members(FinishMemberList::members);
    host.clear_finish_members(FinishMemberList::sub_members);
}
void return_to_town(std::int32_t result,QuestReturnHost& host){
    // StageSystem.ReturnTown 0x1162dc4; result 0 was already recorded by ShowQuestResult.
    if(result!=0)update_quest_record(host.return_record(),result,0,0,host);
    host.cleanup_stage();
    finish_quest_members(host.finish_members_host());
    host.make_transition(1,[result,&host]{
        // <ReturnTown>b__0 0x116847c snapshots live area/stage before switching forms.
        const auto area=host.return_area_id();
        const auto stage=host.return_stage();
        const TownReturnEvent event{result,area,stage};
        host.change_to_town_form(false);
        host.send_town_return(31,event);
    },true,0xff000000U);
}
}
