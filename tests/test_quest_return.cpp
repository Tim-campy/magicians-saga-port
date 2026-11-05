#include "core/gameplay/quest_return.hpp"
#include "core/gameplay/stage_cleanup.hpp"
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host final:MemberFinishHost,QuestReturnHost {
    std::vector<QuestMemberId> members{1,2,1},subs{2,2,3};std::size_t mi=0,si=0;
    std::map<QuestMemberId,MemberExperience> humans{{1,{1205,-8}},{2,{0,99}},{3,{4,7}}};
    std::map<QuestMemberId,SavedCharacterProgression> saved;
    std::map<QuestMemberId,QuestEntityLifetime> life;
    std::vector<std::string> calls;std::string fail;int human_count=0,fail_human=0;
    bool null_character=false,change_human=false;MemberExperience replacement{8,321};
    QuestRecord record;std::function<void()> callback,cleanup_action;std::int32_t area=5,stage=7;
    TownReturnEvent event{};
    void call(std::string name){calls.push_back(name);if(name==fail)throw std::runtime_error(name);}
    std::optional<QuestMemberId> next_finish_member(FinishMemberList list)override{
        call(list==FinishMemberList::members?"next_main":"next_sub");
        auto& ids=list==FinishMemberList::members?members:subs;
        auto& i=list==FinishMemberList::members?mi:si;
        if(i==ids.size())return std::nullopt;return ids.at(i++);
    }
    MemberExperience& finish_human(QuestMemberId id)override{
        call("human"+std::to_string(id));if(++human_count==fail_human)throw std::runtime_error("human");
        if(change_human&&human_count>1)return replacement;return humans.at(id);
    }
    SavedCharacterProgression* saved_character(MemberExperience& human)override{
        call("saved");if(null_character)return nullptr;
        for(auto& pair:humans)if(&pair.second==&human)return &saved[pair.first];
        throw std::runtime_error("unexpected captured human");
    }
    QuestEntityLifetime& entity_lifetime(QuestMemberId id)override{call("life"+std::to_string(id));return life[id];}
    void delete_from_manager(QuestMemberId id,bool notify)override{check(notify);check(!life[id].destroyed);call("delete"+std::to_string(id));}
    void clear_finish_members(FinishMemberList list)override{
        call(list==FinishMemberList::members?"clear_main":"clear_sub");
        (list==FinishMemberList::members?members:subs).clear();
    }
    QuestRecord& return_record()override{call("record");return record;}
    std::int32_t defeated_enemies()override{call("defeated");return 3;}
    std::int32_t collected_prisoners()override{throw std::runtime_error("unexpected clear record");}
    std::int32_t total_damage()override{throw std::runtime_error("unexpected damage query");}
    void cleanup_stage()override{call("cleanup");if(cleanup_action)cleanup_action();}
    MemberFinishHost& finish_members_host()override{call("finish");return *this;}
    void make_transition(std::int32_t mode,std::function<void()> fn,bool flag,std::uint32_t color)override{
        check(mode==1&&flag&&color==0xff000000U);
        const auto* payload=fn.target<QuestTownContinuation>();check(payload&&payload->host==static_cast<QuestReturnHost*>(this));
        call("transition");callback=std::move(fn);
    }
    std::int32_t return_area_id()override{call("area");return area;}
    std::int32_t return_stage()override{call("stage");return stage;}
    void change_to_town_form(bool flag)override{check(!flag);call("form");area=100;stage=200;}
    void send_town_return(std::int32_t id,const TownReturnEvent& payload)override{check(id==31);call("event");event=payload;}
};
struct Cleanup final:StageCleanupHost {
    Host& owner;std::optional<QuestMemberId> extra=3;
    explicit Cleanup(Host& h):owner(h){}
    std::vector<std::optional<QuestMemberId>> snapshot_group(std::int32_t slot)override{
        owner.call("group"+std::to_string(slot));
        if(slot==34)return {3};return {};
    }
    QuestEntityHost& destruction_host()override{return owner;}
    std::optional<QuestMemberId> extra_entity()override{return extra;}
    void clear_extra_entity()override{extra.reset();}
    void clear_collecting_drops()override{owner.call("collecting_clear");}
};
struct TownHost final:TownQuestReturnHost {
    std::int32_t damage=42;std::int64_t score=-1;
    bool form=true,marked=false,replace_damage_on_score=false;
    std::vector<std::string> calls;std::string fail;
    std::function<void()> event_action;
    void call(const std::string& name){calls.push_back(name);if(name==fail)throw std::runtime_error(name);}
    void execute_return_event(std::int32_t id)override{
        check(id==1133);call("event1133");if(event_action)event_action();
    }
    std::int32_t best_total_damage()override{call("damage");return damage;}
    std::int64_t high_score()override{call("score");if(replace_damage_on_score)damage=-99;return score;}
    void set_high_score(std::int64_t value)override{call("set_score");score=value;}
    bool town_form_exists()override{call("form_exists");return form;}
    void mark_quest_return()override{call("mark");marked=true;}
};
void test_town_return(){
    {TownHost h;on_town_quest_return({1,-7,123},h);
        check(h.calls==std::vector<std::string>({"event1133","damage","score","set_score","form_exists","mark"}));
        check(h.score==42&&h.marked);}
    for(const auto result:{0,2,3,-1,std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max()}){
        TownHost h;on_town_quest_return({result,0,0},h);
        check(h.calls==std::vector<std::string>({"damage","score","set_score","form_exists","mark"}));
        check(h.score==42&&h.marked);
    }
    {TownHost h;h.form=false;on_town_quest_return({1,0,0},h);
        check(h.score==42&&!h.marked&&h.calls.back()=="form_exists");}
    {TownHost h;h.event_action=[&]{h.damage=73;h.score=72;};on_town_quest_return({1,0,0},h);
        check(h.score==73&&h.marked);}
    {TownHost h;h.replace_damage_on_score=true;on_town_quest_return({0,0,0},h);
        check(h.damage==-99&&h.score==42&&h.marked);}
    struct Comparison {std::int32_t damage;std::int64_t score;bool write;};
    const Comparison comparisons[]{
        {0,0,false},{42,43,false},{42,42,false},
        {std::numeric_limits<std::int32_t>::max(),static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())+1,false},
        {std::numeric_limits<std::int32_t>::max(),std::numeric_limits<std::int64_t>::max(),false},
        {-1,0,false},{-3,-4,true},
        {std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::min(),false},
        {std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int64_t>::min(),true},
        {0,std::numeric_limits<std::int64_t>::min(),true}
    };
    for(const auto& comparison:comparisons){
        TownHost h;h.damage=comparison.damage;h.score=comparison.score;
        on_town_quest_return({0,-100,-200},h);
        check(h.score==(comparison.write?static_cast<std::int64_t>(comparison.damage):comparison.score)&&h.marked);
        const auto expected=comparison.write?
            std::vector<std::string>{"damage","score","set_score","form_exists","mark"}:
            std::vector<std::string>{"damage","score","form_exists","mark"};
        check(h.calls==expected);
    }
    const std::vector<std::string> order{"event1133","damage","score","set_score","form_exists","mark"};
    for(const auto& failure:order){
        TownHost h;h.fail=failure;
        try{on_town_quest_return({1,5,7},h);check(false);}catch(const std::runtime_error&){}
        const auto end=std::find(order.begin(),order.end(),failure)+1;
        check(h.calls==std::vector<std::string>(order.begin(),end)&&!h.marked);
        check(h.score==(end>order.begin()+4?42:-1));
    }
}
int main(){
    test_town_return();
    {Host h;Cleanup cleanup(h);h.cleanup_action=[&]{cleanup_quest_stage(cleanup);};return_to_town(0,h);
        check(h.life[3].destroyed&&!cleanup.extra&&h.saved[1].level==1200&&h.saved[2].experience==99&&h.callback);
        check(h.calls[0]=="cleanup"&&h.calls[1]=="group34"&&h.calls[4]=="group29"&&h.calls[7]=="group31");
        check(h.calls[9]=="collecting_clear"&&h.calls[10]=="finish");}

    {Host h;finish_quest_members(h);
        check(h.saved[1].level==1200&&h.saved[1].experience==-8&&h.saved[2].level==1&&h.saved[2].experience==99);
        check(h.members.empty()&&h.subs.empty()&&h.life[2].destroyed&&h.life[3].destroyed&&h.human_count==9);
        check(h.calls==std::vector<std::string>({"next_main","human1","saved","human1","human1","next_main","human2","saved","human2","human2",
            "next_main","human1","saved","human1","human1","next_main","next_sub","life2","delete2","next_sub","life2",
            "next_sub","life3","delete3","next_sub","clear_main","clear_sub"}));}
    {Host h;h.members={1};h.subs.clear();h.change_human=true;finish_quest_members(h);
        check(h.saved[1].level==8&&h.saved[1].experience==321);}
    {Host h;h.fail_human=3;h.saved[1]={9,123};try{finish_quest_members(h);check(false);}catch(const std::runtime_error&){}
        check(h.saved[1].level==1200&&h.saved[1].experience==123&&h.si==0&&!h.members.empty());}
    {Host h;h.null_character=true;try{finish_quest_members(h);check(false);}catch(const std::invalid_argument&){}
        check(h.human_count==2&&h.si==0);}
    {Host h;h.fail="delete3";try{finish_quest_members(h);check(false);}catch(const std::runtime_error&){}
        check(h.life[2].destroyed&&!h.life[3].destroyed&&!h.members.empty()&&!h.subs.empty());}
    {Host h;h.fail="clear_sub";try{finish_quest_members(h);check(false);}catch(const std::runtime_error&){}
        check(h.members.empty()&&!h.subs.empty());}
    {Host h;return_to_town(0,h);check(h.record.challenges==0&&h.callback&&h.event.area_id==0);
        const auto* payload=h.callback.target<QuestTownContinuation>();check(payload&&payload->result==0&&payload->host==static_cast<QuestReturnHost*>(&h));
        h.area=11;h.stage=12;h.callback();check(h.event.result==0&&h.event.area_id==11&&h.event.stage==12);
        check(h.calls[0]=="cleanup"&&h.calls[1]=="finish");}
    {Host h;return_to_town(2,h);check(h.record.challenges==1&&h.record.defeated==3&&h.record.clears==0);
        const auto* payload=h.callback.target<QuestTownContinuation>();check(payload&&payload->result==2&&payload->host==static_cast<QuestReturnHost*>(&h));
        check(h.calls[0]=="record"&&h.calls[1]=="defeated"&&h.calls[2]=="cleanup");
        const auto copied=h.callback;h.area=-17;h.stage=23;h.calls.clear();copied();
        check(h.event.result==2&&h.event.area_id==-17&&h.event.stage==23&&h.calls==std::vector<std::string>({"area","stage","form","event"}));}
    {Host h;h.fail="cleanup";try{return_to_town(1,h);check(false);}catch(const std::runtime_error&){}
        check(h.record.challenges==1&&h.human_count==0&&!h.callback);}
    {Host h;h.fail_human=3;try{return_to_town(0,h);check(false);}catch(const std::runtime_error&){}
        check(!h.callback&&h.saved[1].level==1200);}
    {Host h;return_to_town(3,h);h.fail="form";try{h.callback();check(false);}catch(const std::runtime_error&){}
        check(h.event.area_id==0&&h.calls.back()=="form");}
    for(const auto& failure:std::vector<std::string>{"area","stage","form","event"}){
        Host h;return_to_town(4,h);h.area=31;h.stage=47;h.calls.clear();h.fail=failure;
        try{h.callback();check(false);}catch(const std::runtime_error&){}
        const std::vector<std::string> order{"area","stage","form","event"};
        const auto end=std::find(order.begin(),order.end(),failure)+1;
        check(h.calls==std::vector<std::string>(order.begin(),end)&&h.event.area_id==0);
    }
}
