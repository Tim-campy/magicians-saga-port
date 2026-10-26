#include "core/gameplay/quest_rewards.hpp"
#include "core/gameplay/quest_return.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool value){if(!value)std::abort();}
struct Host final:QuestRewardHost,QuestExperienceHost,MemberFinishHost {
    SavedCharacterProgression saved;bool finish_seen=false,main_cleared=false,sub_cleared=false;
    std::optional<QuestMemberId> next_finish_member(FinishMemberList list)override{
        if(list==FinishMemberList::sub_members||finish_seen)return std::nullopt;
        finish_seen=true;return 42;
    }
    MemberExperience& finish_human(QuestMemberId id)override{check(id==42);return member;}
    SavedCharacterProgression* saved_character(MemberExperience& value)override{check(&value==&member);return &saved;}
    QuestEntityLifetime& entity_lifetime(QuestMemberId)override{throw std::runtime_error("no sub-members");}
    void delete_from_manager(QuestMemberId,bool)override{throw std::runtime_error("no sub-members");}
    void clear_finish_members(FinishMemberList list)override{
        if(list==FinishMemberList::members)main_cleared=true;else sub_cleared=true;
    }
    StageRewards drop{5,7,2,{1,1}};
    TreasureTable treasures=load_treasure_table("1\tChest\tres\t3\t10\t20\t30\t0\t0\t0\t0\t4",false,parse_event_decimal);
    std::array<ItemStock,3> items{{{0,false,0},{1,false,0},{2,false,-1}}};
    RecruitmentState recruit{1,false,0};QuestRecord record;
    MemberExperience member{1,0};std::size_t treasure_index=0,member_index=0;
    std::int32_t rescued=0,money=0,crystals=0;
    std::string fail;bool change_prisoners=false;
    std::vector<std::string> calls;
    void call(std::string name){calls.push_back(name);if(fail==name)throw std::runtime_error(name);}
    StageRewards& drops()override{call("drops");return drop;}
    std::optional<std::int32_t> next_treasure()override{
        call("treasure_next");if(treasure_index==drop.treasure_ids.size())return std::nullopt;
        return drop.treasure_ids.at(treasure_index++);
    }
    const TreasureRecord& treasure(std::int32_t id)override{
        call("treasure");const auto& row=treasures.at(static_cast<std::size_t>(id));
        if(!row)throw std::invalid_argument("treasure hole");return *row;
    }
    ItemStock& item(RewardItem kind,std::int32_t id)override{
        call(kind==RewardItem::weapon?"weapon":kind==RewardItem::cloth?"cloth":"hat");
        if(id!=0)throw std::out_of_range("item");return items.at(static_cast<std::size_t>(kind));
    }
    RecruitmentState& character(std::int32_t id)override{
        call("character");if(id!=0)throw std::out_of_range("character");return recruit;
    }
    void add_money(std::int32_t value)override{call("money");money=value;if(change_prisoners)drop.prisoners=9;}
    void add_crystals(std::int32_t value)override{call("crystals");crystals=value;}
    std::int32_t& rescued_prisoners()override{call("rescued");return rescued;}
    QuestRecord& quest_record()override{call("record");return record;}
    std::int32_t defeated_enemies()override{call("defeated");return 3;}
    std::int32_t collected_prisoners()override{call("prisoners");return drop.prisoners;}
    std::int32_t total_damage()override{call("damage");return 99;}
    QuestExperienceHost& members()override{call("members");return *this;}
    std::optional<QuestMemberId> next_member()override{call("member_next");if(member_index++==0)return 42;return std::nullopt;}
    MemberExperience& human(QuestMemberId id)override{check(id==42);call("human");return member;}
    std::int32_t parameter_zero(QuestMemberId)override{call("parameter");return 1;}
    std::int32_t table_count()override{return 3;}
    std::int32_t next_experience(MemberExperience&,std::int32_t index)override{return index*10;}
};
int main(int argc,char** argv){
    // Reward -> result -> in-memory character progression, without granting EXP twice.
    {Host h;auto result=grant_quest_rewards(h);finish_quest_members(h);
        check(result.members[0].added_experience==25&&h.saved.level==2&&h.saved.experience==15);
        check(h.record.challenges==1&&h.main_cleared&&h.sub_cleared);}

    if(argc==2){
        std::ifstream file(argv[1],std::ios::binary);check(static_cast<bool>(file));
        std::string text{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
        auto table=load_treasure_table(text,false,parse_event_decimal);std::size_t count=0;
        for(const auto& row:table)if(row)++count;
        std::cout<<"treasure slots="<<table.size()<<" records="<<count<<'\n';return 0;
    }
    {Host h;const auto r=grant_quest_rewards(h);
        check(r.experience==25&&r.money==47&&r.crystals==60&&r.prisoners==2);
        check(r.weapons.size()==2&&r.characters.size()==2&&r.clothes.size()==2&&r.hats.size()==2);
        check(r.weapons[0]==QuestRewardResult::Grant({0,1}));
        check(h.items[0].stock==1&&h.items[0].is_new&&h.items[1].stock==1&&!h.items[1].is_new);
        check(h.recruit.recruited==1&&h.recruit.is_new&&h.rescued==2);
        check(h.record.challenges==1&&h.record.clears==1&&h.record.defeated==3&&h.record.rescued==2&&h.record.best_damage==99);
        check(h.record.best_experience==25&&h.record.best_money==47&&h.member.level==2&&h.member.experience==15);
        check(r.members.size()==1&&r.members[0].added_experience==25);
        check(h.calls==std::vector<std::string>({"drops","treasure_next","treasure","weapon","character","cloth","hat",
            "treasure_next","treasure","weapon","character","cloth","hat","treasure_next","money","crystals",
            "drops","rescued","record","defeated","prisoners","damage","members","member_next","human","parameter","human","member_next"}));}
    {Host h;h.change_prisoners=true;auto r=grant_quest_rewards(h);check(r.prisoners==2&&h.rescued==9&&h.record.rescued==9);}
    {Host h;h.fail="cloth";try{grant_quest_rewards(h);check(false);}catch(const std::runtime_error&){}
        check(h.items[0].stock==1&&h.recruit.recruited==1&&h.items[2].stock==-1&&h.money==0&&h.member_index==0);}
    {Host h;h.fail="crystals";try{grant_quest_rewards(h);check(false);}catch(const std::runtime_error&){}
        check(h.money==47&&h.rescued==0&&h.record.challenges==0&&h.member_index==0);}
    {Host h;h.fail="defeated";try{grant_quest_rewards(h);check(false);}catch(const std::runtime_error&){}
        check(h.rescued==2&&h.record.challenges==1&&h.record.clears==0&&h.member_index==0);}
    {Host h;h.fail="prisoners";try{grant_quest_rewards(h);check(false);}catch(const std::runtime_error&){}
        check(h.record.challenges==1&&h.record.clears==1&&h.record.best_money==47&&h.record.rescued==0&&h.member_index==0);}
    {Host h;h.drop.treasure_ids.clear();h.record.best_money=100;h.record.challenges=2147483647;
        update_quest_record(h.record,2,999,999,h);
        check(h.record.challenges==(-2147483647-1)&&h.record.defeated==3&&h.record.best_money==100&&h.record.clears==0);
        check(h.calls==std::vector<std::string>{"defeated"});}
    {Host h;h.drop.treasure_ids={1};auto& row=*h.treasures[1];row.weapon_id=-1;row.character_id=-1;row.cloth_id=-1;row.hat_id=-1;
        row.experience=2147483647;auto r=grant_quest_rewards(h);
        check(r.experience==-2147483644&&h.member.experience==-2147483644&&r.weapons.empty());}
    {Host h;h.treasures[1]->weapon_id=-2;
        try{grant_quest_rewards(h);check(false);}catch(const std::out_of_range&){}check(h.money==0&&h.recruit.recruited==0);}
    {auto table=load_treasure_table("2\tA\tB\t3\t4\t5\t6\t7\t8\t9\t10\t11\n2\tC\tD\t13\t14\t15\t16\t-1\t-1\t-1\t-1\t21\textra",false,parse_event_decimal);
        check(table.size()==3&&!table[0]&&!table[1]&&table[2]->name=="C"&&table[2]->flags==21&&table[2]->experience==14);}
}
