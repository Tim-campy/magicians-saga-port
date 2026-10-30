#include "core/gameplay/quest_catalog.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
using namespace saga::core;
#define CHECK(x) do{if(!(x)){std::cerr<<"failed line "<<__LINE__<<'\n';std::abort();}}while(false)
int main(int argc,char** argv){
    if(argc==2){
        std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("cannot open quest table");
        const std::string text{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
        const auto table=load_quest_table(text,false,parse_event_decimal);
        std::size_t count=0;for(const auto& row:table)if(row)++count;
        std::cout<<"quest slots="<<table.size()<<" populated="<<count<<'\n';return 0;
    }
    {
        AreaDefinition area;
        CHECK(&load_area_definition(area,"5\tArea\t17\t2\t2\t-3\t7\textra",parse_event_decimal)==&area);
        CHECK(area.id==5&&area.name=="Area"&&area.image==17&&area.flags==7);
        CHECK(area.stages&&*area.stages==EventIntegers({2,-3}));
        const auto stages=area.stages;
        load_area_definition(area,std::nullopt,parse_event_decimal);load_area_definition(area,"",parse_event_decimal);
        CHECK(area.stages==stages&&area.id==5);
        try{load_area_definition(area,"8\tChanged\tbad",parse_event_decimal);CHECK(false);}catch(const std::invalid_argument&){}
        CHECK(area.id==8&&area.name=="Changed"&&area.image==17&&area.stages==stages);
        try{load_area_definition(area,"8\tChanged\t9\t2\t4\tbad",parse_event_decimal);CHECK(false);}catch(const std::invalid_argument&){}
        CHECK(area.image==9&&area.stages==stages&&area.flags==7);
        try{load_area_definition(area,"8\tChanged\t9\t1\t4\tbad",parse_event_decimal);CHECK(false);}catch(const std::invalid_argument&){}
        CHECK(area.stages!=stages&&*area.stages==EventIntegers({4})&&area.flags==7);
        load_area_definition(area,"8\tChanged\t9\t-2\t3",parse_event_decimal);CHECK(!area.stages&&area.flags==3);
        load_area_definition(area,"8\tChanged\t9\t0\t3",parse_event_decimal);CHECK(area.stages&&area.stages->empty());
        const auto table=load_area_table("\xef\xbb\xbf" "2\tFirst\t5\t0\t1\r\n5\tOther\t7\t1\t8\t4\n2\tLast\t6\t-1\t2",false,parse_event_decimal);
        CHECK(table.size()==6&&!table[0]&&!table[1]&&!table[3]&&!table[4]);
        CHECK(table[2]->name=="Last"&&!table[2]->stages&&table[2]->flags==2&&table[5]->stages->at(0)==8);
        CHECK(load_area_table("\r\n",true,parse_event_decimal).size()==1);
        try{load_area_table("-1\tBad\t0\t0\t0",false,parse_event_decimal);CHECK(false);}catch(const std::out_of_range&){}
    }
    const std::string prefix="2\tSynthetic\t17\tmap\t4\t0\t1\t2\t3\tignored\talso ignored\t5\t6\t10\t40\t25\t3\t20\t150\t";
    const std::string suffix="1\t1\t2\t4\t90\t0\t7\t1\t0\t-1\t8\t9\t10\t2\t11";
    QuestDefinition r;r.stage.rescued_prisoners=3;r.stage.completed=1;
    CHECK(&load_quest_definition(r,prefix+suffix+"\textra",parse_event_decimal)==&r);
    CHECK(r.id==2&&r.name=="Synthetic"&&r.image==17&&r.resource=="map"&&r.need_stamina==11);
    const auto& q=r.stage;
    CHECK(q.flags==2&&q.base_level==5&&q.boss_level==6&&q.distance==10&&q.time_limit==40);
    CHECK(q.enemy_interval==25&&q.enemy_maximum==3&&q.enemy_total_limit==20&&q.money_rate==150);
    CHECK(q.boss_id==7&&q.prisoner_maximum==8&&q.reward_treasure==9&&q.cleared_reward_treasure==10);
    CHECK(q.rescued_prisoners==3&&q.completed==1&&q.chip_images->size()==4);
    CHECK(q.normal_enemies->at(0)->at(0)->at(1)==90&&q.fixed_enemies->empty());
    CHECK(q.events->size()==1&&q.events->at(0)->empty()&&!q.fixed_treasure);
    const auto old_chips=q.chip_images;const auto old_pool=q.normal_enemies;
    load_quest_definition(r,std::nullopt,parse_event_decimal);load_quest_definition(r,"",parse_event_decimal);
    CHECK(r.id==2&&q.chip_images==old_chips);
    try{load_quest_definition(r,"6\tChanged\tbad",parse_event_decimal);CHECK(false);}catch(const std::invalid_argument&){}
    CHECK(r.id==6&&r.name=="Changed"&&r.image==17&&q.chip_images==old_chips);
    for(const auto* bad:{"1\t-1","1\t1\t-1","1\t1\t2\t4"}){
        try{load_quest_definition(r,prefix+bad,parse_event_decimal);CHECK(false);}catch(const std::exception&){}
        CHECK(q.normal_enemies==old_pool&&q.boss_id==7); // Store follows successful whole-array read.
    }
    auto nullable=prefix+"-3\t-1\t7\t-2\t0\t8\t9\t10\t2\t11";
    load_quest_definition(r,nullable,parse_event_decimal);
    CHECK(!q.normal_enemies&&!q.fixed_enemies&&!q.events&&q.fixed_treasure&&q.fixed_treasure->empty());
    auto table=load_quest_table("\xef\xbb\xbf"+prefix+suffix+"\r\n"+nullable+"\n",false,parse_event_decimal);
    CHECK(table.size()==3&&!table[0]&&!table[1]&&!table[2]->stage.events);
    CHECK(load_quest_table("\r\n",true,parse_event_decimal).size()==1);
    try{load_quest_table("\r\n",false,parse_event_decimal);CHECK(false);}catch(const std::invalid_argument&){}
    try{load_quest_table("-1"+prefix.substr(1)+suffix,false,parse_event_decimal);CHECK(false);}catch(const std::out_of_range&){}
}
