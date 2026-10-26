#include "core/gameplay/enemy_rewards.hpp"
#include "core/gameplay/combat_rules.hpp"
#include "core/gameplay/stage_rewards.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host final:EnemyRewardHost {
    EnemyRecord enemy;std::vector<std::string> calls;
    std::vector<std::pair<std::int32_t,std::int32_t>> pending;
    std::vector<bool> rolls{false,true};std::size_t ri=0;bool fail_spawn=false;
    Host(){enemy.experience=EventIntegers{10,1009};enemy.treasures=EnemyTreasureGroups{{{3,0},{4,1000},{5,2000}},{{999,999}}};}
    const EnemyRecord& enemy_data()override{calls.push_back("data");return enemy;}
    void spawn_experience_drop(std::int32_t value)override{
        calls.push_back("exp");if(fail_spawn)throw std::runtime_error("spawn");pending.push_back({0,value});
    }
    bool hits1000(std::int32_t value)override{calls.push_back("roll"+std::to_string(value));return rolls.at(ri++);}
    void spawn_treasure_drop(std::int32_t id)override{calls.push_back("treasure");pending.push_back({3,id});}
};
int main(int argc,char** argv){
    {
        EnemyRecord e;e.parameters=std::make_shared<std::vector<EventInstruction>>();
        e.parameters->push_back(std::make_shared<EventIntegers>(EventIntegers{100,1000}));
        std::array<std::int32_t,13> maxima{};maxima.fill(1000);
        check(enemy_parameter(e,0,1,maxima)==100&&enemy_parameter(e,0,1000,maxima)==1000);
        check(enemy_parameter(e,0,501,maxima)==407); // Linear growth would yield 550.
        check(enemy_parameter(e,0,0,maxima)==100&&enemy_parameter(e,0,(-2147483647-1),maxima)==1000);
        maxima[0]=200;check(enemy_parameter(e,0,200,maxima)==1000); // Live table change.
        e.parameters->at(0)=std::make_shared<EventIntegers>(EventIntegers{100});
        try{enemy_parameter(e,0,1200,maxima);check(false);}catch(const std::out_of_range&){}
        e.parameters->at(0)=nullptr;
        try{enemy_parameter(e,0,1200,maxima);check(false);}catch(const std::invalid_argument&){}
    }
    if(argc==2){
        std::ifstream f(argv[1],std::ios::binary);check(static_cast<bool>(f));
        std::string text{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
        const auto table=load_enemy_table(text,false,parse_event_decimal);
        std::size_t records=0,lookups=0;std::uint64_t hash=14695981039346656037ULL;
        for(const auto& row:table)if(row){++records;
            for(std::int32_t level=1;level<=1200;++level){
                hash^=static_cast<std::uint32_t>(enemy_experience(*row,level));hash*=1099511628211ULL;++lookups;
            }
        }
        std::cout<<"slots="<<table.size()<<" records="<<records<<" lookups="<<lookups<<" hash="<<hash<<'\n';return 0;
    }
    {Host h;check(enemy_experience(h.enemy,1)==10&&enemy_experience(h.enemy,1000)==1009);
        check(enemy_experience(h.enemy,0)==10&&enemy_experience(h.enemy,(-2147483647-1))==1009);
        h.enemy.experience=EventIntegers{100,1};check(enemy_experience(h.enemy,556)==44); // Integer interpolation would give 45.
        h.enemy.experience=EventIntegers{0,9999999};check(enemy_experience(h.enemy,108)==1071071);
        h.enemy.experience=EventIntegers{2147483647,2147483647};check(enemy_experience(h.enemy,1)==2147483647);
        h.enemy.experience=EventIntegers{(-2147483647-1),2147483647};check(enemy_experience(h.enemy,500)==(-2147483647-1));
        h.enemy.experience=EventIntegers{1};try{enemy_experience(h.enemy,1000);check(false);}catch(const std::out_of_range&){}
        h.enemy.experience.reset();try{enemy_experience(h.enemy,0);check(false);}catch(const std::invalid_argument&){} }
    check(easing_zero_amplitude(4,9,0,-1)==4&&easing_zero_amplitude(4,9,0,0)==9);
    {Host h;spawn_enemy_rewards(1,h);
        check(h.calls==std::vector<std::string>({"data","exp","data","data","roll0","roll1000","treasure"}));
        check(h.pending==std::vector<std::pair<std::int32_t,std::int32_t>>({{0,10},{3,4}}));
        StageRewards collected{};check(collected.experience==0&&collected.treasure_ids.empty());
        for(const auto& drop:h.pending)record_collected_drop(collected,drop.first,drop.second);
        check(collected.experience==10&&collected.treasure_ids==std::vector<std::int32_t>({4}));}
    {Host h;h.enemy.experience=EventIntegers{-1,-1};h.enemy.treasures=EnemyTreasureGroups{{}};spawn_enemy_rewards(1,h);
        check(h.calls==std::vector<std::string>({"data","data"})&&h.pending.empty());}
    {Host h;h.enemy.treasures=EnemyTreasureGroups{{{3,1000},{4}}};h.rolls={true};spawn_enemy_rewards(1,h);check(h.ri==1&&h.pending.back().second==3);}
    {Host h;h.enemy.treasures=EnemyTreasureGroups{{{3}}};try{spawn_enemy_rewards(1,h);check(false);}catch(const std::out_of_range&){}
        check(h.pending.size()==1&&h.ri==0);}
    {Host h;h.fail_spawn=true;try{spawn_enemy_rewards(1,h);check(false);}catch(const std::runtime_error&){}
        check(h.calls==std::vector<std::string>({"data","exp"}));}
    const std::string tail="\t6\t7\tres\t8\t9\t10\t11\t1\t2\t12\t13\t14";
    {auto table=load_enemy_table("2\tEnemy\t5\t2\t10\t20\t1\t2\t2\t3\t400\t2\t4\t500"+tail,false,parse_event_decimal);
        check(table.size()==3&&!table[0]&&table[2]->experience->at(1)==20);
        check(table[2]->treasures->at(0).at(1)==EventIntegers({4,500})&&table[2]->flags==14&&table[2]->rank==11);
        check(table[2]->parameters->at(0)->at(1)==13);}
    {auto table=load_enemy_table("0\tE\t0\t-1\t-1"+tail,false,parse_event_decimal);check(!table[0]->experience&&!table[0]->treasures);}
    for(const auto& bad:std::vector<std::string>{"1\t-1","1\t1\t-1"}){
        try{load_enemy_table("0\tE\t0\t2\t1\t2\t"+bad+tail,false,parse_event_decimal);check(false);}catch(const std::length_error&){}
    }
}
