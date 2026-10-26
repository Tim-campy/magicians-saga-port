#include "core/gameplay/quest_experience.hpp"
#include "core/gameplay/progression_tables.hpp"
#include <array>
#include <cstdlib>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool b){if(!b)std::abort();}
struct Host final:QuestExperienceHost {
    std::vector<QuestMemberId> order{2,1,2,3};std::size_t cursor=0;
    std::array<MemberExperience,4> members{{{1,0},{1,5},{1,0},{7,9}}};
    std::array<std::int32_t,4> character_ids{{1,1,2,1}},eligible{{1,1,1,0}};
    std::array<int,4> human_calls{};
    std::vector<std::string> calls;
    std::vector<std::int32_t> cost_indices;
    LevelTable levels=load_level_table("0\t0\t0\t0\t0\t0\t0\t0\t0\t0\n1\t99\t10\t0\t0\t0\t0\t0\t0\t0\n2\t2\t20\t0\t0\t0\t0\t0\t0\t0\n3\t3\t30\t0\t0\t0\t0\t0\t0\t0",false,parse_event_decimal);
    CharacterProgressionTable characters{std::nullopt,CharacterProgressionRecord{1,100},CharacterProgressionRecord{2,200}};
    bool mutate_parameter=false,fail_parameter=false,fail_second_human=false,replace_second_human=false;
    MemberExperience replacement{100,0};
    std::optional<QuestMemberId> next_member()override{
        calls.push_back("next");if(cursor==order.size())return std::nullopt;return order.at(cursor++);
    }
    MemberExperience& human(QuestMemberId id)override{
        calls.push_back("human"+std::to_string(id));
        if(++human_calls.at(static_cast<std::size_t>(id))==2){
            if(fail_second_human)throw std::runtime_error("result human lookup failed");
            if(replace_second_human)return replacement;
        }
        return members.at(static_cast<std::size_t>(id));
    }
    std::int32_t parameter_zero(QuestMemberId id)override{
        calls.push_back("parameter"+std::to_string(id));
        if(fail_parameter)throw std::runtime_error("parameter missing");
        if(mutate_parameter)members.at(static_cast<std::size_t>(id))={5,7};
        return eligible.at(static_cast<std::size_t>(id));
    }
    std::int32_t table_count()override{calls.push_back("count");return static_cast<std::int32_t>(levels.size());}
    std::int32_t next_experience(MemberExperience& member,std::int32_t index)override{
        calls.push_back("cost");cost_indices.push_back(index);
        for(std::size_t i=0;i<members.size();++i)if(&members[i]==&member){
            TableExperienceHost host(levels,characters,character_ids[i]);return host.next_experience(index);
        }
        throw std::runtime_error("unknown captured human");
    }
};
int main(){
    {Host h;auto r=grant_quest_member_experience(15,h);
        check(r.size()==4&&r[0].character==2&&r[1].character==1&&r[2].character==2&&r[3].character==3);
        check(h.members[1].level==2&&h.members[1].experience==10);
        check(h.members[2].level==2&&h.members[2].experience==10);
        check(r[0].levels_gained==0&&r[0].display_experience==0);
        check(r[2].levels_gained==1&&r[2].display_experience==15&&r[2].display_level==1);
        check(h.members[3].level==7&&h.members[3].experience==9);
        check(r[3].added_experience==15&&r[3].levels_gained==0&&r[3].display_experience==9&&r[3].level_up_frame==-1);
        check(h.calls[0]=="next"&&h.calls[1]=="human2"&&h.calls[2]=="parameter2");
        check(h.calls[h.calls.size()-4]=="human3"&&h.calls[h.calls.size()-3]=="parameter3"&&h.calls[h.calls.size()-2]=="human3");}
    {Host h;h.order.clear();check(grant_quest_member_experience(9,h).empty()&&h.calls==std::vector<std::string>{"next"});}
    {Host h;h.order={1};h.mutate_parameter=true;auto r=grant_quest_member_experience(5,h);
        check(h.cost_indices==std::vector<std::int32_t>({1,2}));
        check(h.members[1].level==6&&h.members[1].experience==2);
        check(r[0].levels_gained==5&&r[0].display_experience==5&&r[0].display_level==1);}
    {Host h;h.order={1};h.replace_second_human=true;auto r=grant_quest_member_experience(5,h);
        check(r[0].levels_gained==1&&r[0].display_level==99);}
    {Host h;h.order={1};h.fail_parameter=true;
        try{grant_quest_member_experience(100,h);check(false);}catch(const std::runtime_error&){}
        check(h.members[1].experience==5&&h.cost_indices.empty()&&h.human_calls[1]==1);}
    {Host h;h.order={1,2};h.fail_second_human=true;
        try{grant_quest_member_experience(15,h);check(false);}catch(const std::runtime_error&){}
        check(h.members[1].level==2&&h.members[1].experience==10&&h.members[2].experience==0&&h.cursor==1);}
    {Host h;h.order={1,2,3};h.character_ids[2]=0;
        try{grant_quest_member_experience(15,h);check(false);}catch(const std::invalid_argument&){}
        check(h.members[1].experience==10&&h.members[2].experience==15&&h.cursor==2&&h.human_calls[3]==0);}
    {auto r=make_quest_character_result(9,1,7,50,(-2147483647-1));
        check(r.display_level==2147483647&&r.display_experience==7&&r.added_experience==50&&r.level_up_frame==-1);}
}
