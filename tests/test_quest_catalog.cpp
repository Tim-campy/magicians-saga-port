#include "core/gameplay/quest_catalog.hpp"
#include "core/gameplay/quest_rewards.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
using namespace saga::core;
#define CHECK(x) do{if(!(x)){std::cerr<<"failed line "<<__LINE__<<'\n';std::abort();}}while(false)
AreaProgressState dirty_area_progress(){
    AreaProgressState state;state.discovery_state=7;state.is_new=true;state.arrival_stage=19;
    auto shared=std::make_shared<EventIntegers>(EventIntegers{8,9});
    state.stage_detail=std::make_shared<std::vector<EventInstruction>>(2,shared);
    state.stage_total_damage=state.stage_detail;state.area_detail=shared;
    return state;
}
void check_reset_area_arrays(const AreaProgressState& state){
    CHECK(state.stage_detail&&state.stage_detail->size()==3);
    CHECK(state.stage_total_damage&&state.stage_total_damage->size()==3);
    CHECK(state.stage_detail!=state.stage_total_damage);
    CHECK(state.area_detail&&*state.area_detail==EventIntegers(6,0));
    CHECK(state.arrival_stage==0);
    std::vector<EventInstruction> rows;
    for(std::size_t i=0;i<3;++i){
        const auto detail=state.stage_detail->at(i),damage=state.stage_total_damage->at(i);
        CHECK(detail&&*detail==EventIntegers(6,0)&&damage&&*damage==EventIntegers(2,0));
        CHECK(detail!=state.area_detail&&damage!=state.area_detail);
        for(const auto& old:rows)CHECK(old!=detail&&old!=damage);
        CHECK(detail!=damage);rows.push_back(detail);rows.push_back(damage);
    }
}
void test_area_new_game(){
    {AreaDefinition definition;definition.id=4;definition.flags=2;definition.image=17;definition.name="Synthetic";
     definition.stages=std::make_shared<EventIntegers>(EventIntegers{8,3});const auto stages=definition.stages;
     auto state=dirty_area_progress();const auto old_details=state.stage_detail,old_damage=state.stage_total_damage;
     const auto old_area=state.area_detail;
     reset_area_for_new_game(state,definition,[](bool){CHECK(false);});
     check_reset_area_arrays(state);CHECK(state.discovery_state==0&&state.is_new);
     CHECK(state.stage_detail!=old_details&&state.stage_total_damage!=old_damage&&state.area_detail!=old_area);
     CHECK(old_details==old_damage&&old_details->at(0)==old_area&&*old_area==EventIntegers({8,9}));
     old_area->at(0)=55;CHECK(state.area_detail->at(0)==0&&state.stage_detail->at(0)->at(0)==0);
     CHECK(definition.id==4&&definition.flags==2&&definition.image==17&&definition.name=="Synthetic"&&definition.stages==stages);
     const auto first_details=state.stage_detail,first_damage=state.stage_total_damage;const auto first_area=state.area_detail;
     state.stage_detail->at(1)->at(4)=71;state.stage_total_damage->at(2)->at(1)=72;state.area_detail->at(5)=73;
     state.is_new=false;reset_area_for_new_game(state,definition);check_reset_area_arrays(state);
     CHECK(state.discovery_state==0&&!state.is_new&&state.stage_detail!=first_details&&state.stage_total_damage!=first_damage&&state.area_detail!=first_area);
     CHECK(first_details->at(1)->at(4)==71&&first_damage->at(2)->at(1)==72&&first_area->at(5)==73);
     for(std::size_t i=0;i<3;++i)for(std::size_t j=0;j<3;++j){
         CHECK(state.stage_detail->at(i)!=first_details->at(j)&&state.stage_detail->at(i)!=first_damage->at(j));
         CHECK(state.stage_total_damage->at(i)!=first_details->at(j)&&state.stage_total_damage->at(i)!=first_damage->at(j));
     }}
    for(const auto flags:{0,1,2,3,-1}){
        AreaDefinition definition;definition.flags=flags;AreaProgressState state;state.discovery_state=9;
        reset_area_for_new_game(state,definition);check_reset_area_arrays(state);
        CHECK(state.discovery_state==(flags&1)&&state.is_new==((flags&1)!=0));
    }
    {AreaDefinition definition;definition.flags=1;auto state=dirty_area_progress();state.is_new=false;
     const auto old_details=state.stage_detail,old_damage=state.stage_total_damage;const auto old_area=state.area_detail;
     int calls=0;
     reset_area_for_new_game(state,definition,[&](bool argument){
         ++calls;CHECK(argument&&state.discovery_state==7&&!state.is_new&&state.arrival_stage==19);
         CHECK(state.stage_detail==old_details&&state.stage_total_damage==old_damage&&state.area_detail==old_area);
         state.discovery_state=22;state.is_new=true;definition.flags=0;
     });
     check_reset_area_arrays(state);CHECK(calls==1&&state.discovery_state==0&&state.is_new);}
    {AreaDefinition definition;definition.flags=1;auto state=dirty_area_progress();
     reset_area_for_new_game(state,definition,[&](bool argument){CHECK(argument);definition.flags=3;state.discovery_state=-9;state.is_new=false;});
     check_reset_area_arrays(state);CHECK(state.discovery_state==1&&!state.is_new);}
    {AreaDefinition definition;definition.flags=1;auto state=dirty_area_progress();
     const auto old_details=state.stage_detail,old_damage=state.stage_total_damage;const auto old_area=state.area_detail;
     try{reset_area_for_new_game(state,definition,[&](bool argument){
         CHECK(argument&&state.stage_detail==old_details&&state.area_detail==old_area);
         state.discovery_state=23;state.is_new=false;definition.flags=0;throw std::runtime_error("find");
     });CHECK(false);}catch(const std::runtime_error&){}
     CHECK(state.discovery_state==23&&!state.is_new&&state.arrival_stage==19);
     CHECK(state.stage_detail==old_details&&state.stage_total_damage==old_damage&&state.area_detail==old_area&&definition.flags==0);}
    {AreaDefinition definition;definition.flags=1;auto state=dirty_area_progress();state.is_new=false;
     reset_area_progress(state,definition);check_reset_area_arrays(state);
     CHECK(state.discovery_state==1&&!state.is_new); // oldNewGame never calls Find.
     definition.flags=0;state.is_new=true;reset_area_progress(state,definition);
     check_reset_area_arrays(state);CHECK(state.discovery_state==0&&state.is_new);}
}
int main(int argc,char** argv){
    if(argc==2){
        std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("cannot open quest table");
        const std::string text{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
        const auto table=load_quest_table(text,false,parse_event_decimal);
        std::size_t count=0;for(const auto& row:table)if(row)++count;
        std::cout<<"quest slots="<<table.size()<<" populated="<<count<<'\n';return 0;
    }
    test_area_new_game();
    {
        QuestDefinition r;r.stage.flags=3;r.stage.completed=7;r.stage.rescued_prisoners=9;
        r.record=std::make_shared<QuestRecord>(QuestRecord{1,2,3,4,5,6,7});const auto old=r.record;
        reset_quest_for_new_game(r);
        CHECK(r.discovery_state==1&&r.is_new&&r.stage.completed==0&&r.stage.flags==3&&r.stage.rescued_prisoners==9);
        CHECK(r.record&&r.record!=old&&old->best_experience==1);
        const auto& q=*r.record;
        CHECK(q.best_experience==0&&q.best_money==0&&q.challenges==0&&q.clears==0&&q.defeated==0&&q.rescued==0&&q.best_damage==0);
        r.discovery_state=2;r.is_new=false;find_quest_record(r,false);
        CHECK(r.discovery_state==2&&!r.is_new);
        r.discovery_state=0;find_quest_record(r,false);CHECK(r.discovery_state==1&&r.is_new); // Argument is ignored.
        r.stage.flags=2;r.stage.completed=1;r.discovery_state=0;r.is_new=false;
        reset_quest_for_new_game(r,[](bool){CHECK(false);});
        CHECK(r.discovery_state==0&&!r.is_new&&r.stage.completed==0); // Bit1 is not the discovery gate.
        const auto retained=r.record;r.stage.flags=1;r.stage.completed=9;
        try{reset_quest_for_new_game(r,[](bool argument){CHECK(argument);throw std::runtime_error("find");});CHECK(false);}catch(const std::runtime_error&){}
        CHECK(r.record==retained&&r.stage.completed==9);
        bool found=false;
        try{reset_quest_for_new_game(r,[&](bool argument){CHECK(argument&&r.stage.completed==9);found=true;r.discovery_state=4;},[&]()->std::shared_ptr<QuestRecord>{
            CHECK(found&&r.discovery_state==4&&r.stage.completed==0&&r.record==retained);throw std::runtime_error("allocate");
        });CHECK(false);}catch(const std::runtime_error&){}
        CHECK(r.stage.completed==0&&r.record==retained&&r.discovery_state==4);
        const auto supplied=std::make_shared<QuestRecord>();
        reset_quest_for_new_game(r,{},[&]{return supplied;});CHECK(r.record==supplied);
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
        auto table=load_area_table("\xef\xbb\xbf" "2\tFirst\t5\t0\t1\r\n5\tOther\t7\t1\t8\t4\n2\tLast\t6\t-1\t2",false,parse_event_decimal);
        CHECK(table.size()==6&&!table[0]&&!table[1]&&!table[3]&&!table[4]);
        CHECK(table[2]->name=="Last"&&!table[2]->stages&&table[2]->flags==2&&table[5]->stages->at(0)==8);
        CHECK(load_area_table("\r\n",true,parse_event_decimal).size()==1);
        try{load_area_table("-1\tBad\t0\t0\t0",false,parse_event_decimal);CHECK(false);}catch(const std::out_of_range&){}
        QuestTable quests(8);quests[7]=QuestDefinition{};quests[7]->id=7;
        auto selected=select_mission_records(table,quests,2,7);
        CHECK(selected.area==&*table[2]&&selected.quest==&*quests[7]); // Global quest index, independent of null Area.stages.
        quests[7]->image=91;CHECK(selected.quest->image==91);
        CHECK(!select_mission_records(table,quests,0,7).area); // Cache gaps are not filtered out.
        CHECK(!select_mission_records(table,quests,5,0).quest);
        try{select_mission_records(table,quests,-1,7);CHECK(false);}catch(const std::out_of_range&){}
        try{select_mission_records(table,quests,2,-1);CHECK(false);}catch(const std::out_of_range&){}
        try{select_mission_records(table,quests,99,99);CHECK(false);}catch(const std::out_of_range& e){
            CHECK(std::string(e.what()).find("area slot8")!=std::string::npos);
        }
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
