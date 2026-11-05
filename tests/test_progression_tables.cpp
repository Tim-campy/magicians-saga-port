#include "core/gameplay/progression_tables.hpp"
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <iostream>
#include <limits>
using namespace saga::core;
void check(bool b){if(!b)std::abort();}
std::string read(const char* path){
    std::ifstream f(path,std::ios::binary);
    if(!f)throw std::runtime_error("cannot open table");
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
int main(int argc,char** argv){
    // Optional local artifact smoke test; no game data is stored in this test.
    if(argc==3){
        auto levels=load_level_table(read(argv[1]),false,parse_event_decimal);
        auto characters=load_character_progression_table(read(argv[2]),false,parse_event_decimal);
        std::size_t count=0;
        for(std::size_t i=0;i<characters.size();++i)if(characters[i]){
            auto id=static_cast<std::int32_t>(i);TableExperienceHost host(levels,characters,id);
            for(std::size_t j=1;j<levels.size()&&j<=1200;++j)if(levels[j]){
                host.next_experience(static_cast<std::int32_t>(j));++count;
            }
        }
        std::cout<<"level slots="<<levels.size()<<" character slots="<<characters.size()<<" cost lookups="<<count<<'\n';
        return 0;
    }
    const std::string row="1\t999\t10\t3\t4\t5\t6\t7\t8\t9";
    auto levels=load_level_table("\xef\xbb\xbf"+row+"\r\n3\t3\t40\t0\t0\t0\t0\t0\t0\t0\n",false,parse_event_decimal);
    check(levels.size()==4&&!levels[0]&&!levels[2]);
    check(levels[1]->level==999&&levels[1]->shop_parameters[2]==5&&levels[1]->unlock_rescue==9);
    // Strings, two nested arrays, and all fields after the rate must be consumed correctly.
    const std::string character="2\tName\t7\tres\t8\tres2\t1\t2\t3\t4\t5\t150\t1\t2\t11\t12\t6\t7\t8\t-1\t9\t10\r\n";
    auto characters=load_character_progression_table(character,false,parse_event_decimal);
    check(characters.size()==3&&!characters[1]&&characters[2]->required_experience_rate==150&&characters[2]->image==8);
    check(characters[2]->base_name&&*characters[2]->base_name=="Name");
    check(characters[2]->unlock_level==7&&characters[2]->recruit_cost==3);
    check(characters[2]->resource=="res"&&characters[2]->secondary_resource=="res2");
    check(characters[2]->voice_id==6&&characters[2]->town_id==7&&characters[2]->area_id==8&&characters[2]->appear_rate==9&&!characters[2]->resident_terms&&characters[2]->flags==10);
    const auto fails=[](auto action){bool caught=false;try{action();}catch(const std::exception&){caught=true;}check(caught);};
    {auto signed_input=character;signed_input.replace(signed_input.find("\t7\tres"),6,"\t-2147483648\tres");
     signed_input.replace(signed_input.find("res2\t1\t2\t3\t4"),12,"res2\t1\t2\t2147483647\t4");
     const auto signed_records=load_character_progression_table(signed_input,false,parse_event_decimal);
     check(signed_records[2]->unlock_level==std::numeric_limits<std::int32_t>::min()&&signed_records[2]->recruit_cost==std::numeric_limits<std::int32_t>::max());}
    {CharacterProgressionTable catalog(6);
     const auto make=[](std::int32_t id,std::int32_t key,std::int32_t flags){CharacterProgressionRecord row;row.id=id;row.unlock_level=key;row.flags=flags;return row;};
     catalog[1]=make(5,-1,4);catalog[2]=make(6,-1,16);
     catalog[3]=make(7,std::numeric_limits<std::int32_t>::min(),20);
     catalog[4]=make(8,std::numeric_limits<std::int32_t>::max(),1);catalog[5]=make(9,0,4);
     std::vector<std::int32_t> output{99};std::vector<std::string> calls;
     const auto found=[&](const auto& row){calls.push_back("F"+std::to_string(row.id));return row.id!=7;};
     const auto recruited=[&](const auto& row){calls.push_back("R"+std::to_string(row.id));return row.id==9;};
     append_character_shop_entries(output,catalog,found,recruited);
     check(output==std::vector<std::int32_t>({99,5,6}));
     check(calls==std::vector<std::string>({"F7","F5","R5","F6","R6","F9","R9","F8"}));
     output={99};calls.clear();
     append_character_shop_entries(output,catalog,[&](const auto& row){
         const auto result=found(row);if(row.id==5){catalog[2]->unlock_level=std::numeric_limits<std::int32_t>::min();catalog[2]->flags=0;}
         return result;
     },[&](const auto& row){const auto result=recruited(row);if(row.id==5)catalog[1]->id=105;return result;});
     check(output==std::vector<std::int32_t>({99,105}));
     check(calls==std::vector<std::string>({"F7","F5","R5","F6","F9","R9","F8"}));
     catalog[1]->id=5;catalog[2]->unlock_level=-1;catalog[2]->flags=16;output={99};
     fails([&]{append_character_shop_entries(output,catalog,[&](const auto& row){if(row.id==6)throw std::runtime_error("late found");return row.id!=7;},recruited);});
     check(output==std::vector<std::int32_t>({99,5}));
     output={99};append_character_shop_entries(output,CharacterProgressionTable(3),found,recruited);check(output==std::vector<std::int32_t>{99});}
    {auto record=*characters[2];CharacterNewGameState state;state.discovery_state=7;state.is_new=true;
     state.level=99;state.experience=8;state.living_address=6;state.recruited=5;state.sorties=12;state.equipped_cane=4;
     reset_character_for_new_game(state,record);
     check(state.name==record.base_name&&state.status_definition==record.parameters&&*state.status==EventIntegers{11});
     check(*state.witch_experience==EventIntegers{0,300,0}&&state.level==4&&state.experience==0);
     check(state.living_address==-1&&state.recruited==0&&state.sorties==12&&state.equipped_cane==4);
     check(state.discovery_state==7&&state.is_new);const auto old=state.status,witch=state.witch_experience;
     reset_character_for_new_game(state,record);check(state.status!=old&&state.witch_experience!=witch);
     record.flags=1;state.level=99;state.living_address=6;state.recruited=5;int queries=0;
     reset_character_for_new_game(state,record,[&](bool fresh){check(fresh);++queries;
         check(state.level==99&&state.living_address==6&&state.recruited==5&&state.witch_experience->at(1)==300);
         record.initial_level=-17;state.discovery_state=1;state.is_new=true;});
     check(queries==1&&state.level==-17&&state.living_address==-1&&state.recruited==0&&state.is_new);
     state.level=99;state.living_address=6;state.recruited=5;
     fails([&]{reset_character_for_new_game(state,record,[&](bool){state.discovery_state=8;throw std::runtime_error("find");});});
     check(state.level==99&&state.living_address==6&&state.recruited==5&&state.discovery_state==8&&state.witch_experience->at(1)==300);
     const auto previous=state.status;record.parameters=nullptr;
     fails([&]{reset_character_for_new_game(state,record);});check(!state.status_definition&&state.status==previous&&state.name==record.base_name);
     record.parameters=std::make_shared<std::vector<EventInstruction>>();record.base_name=nullptr;
     reset_character_for_new_game(state,record);check(!state.name&&state.status->empty()&&state.level==-17&&state.recruited==0);}
    {CharacterProgressionRecord record;record.parameters=std::make_shared<std::vector<EventInstruction>>();
     record.parameters->push_back(std::make_shared<EventIntegers>(EventIntegers{7,8}));record.parameters->push_back(std::make_shared<EventIntegers>());
     CharacterNewGameState state;state.level=42;state.witch_experience=std::make_shared<EventIntegers>(EventIntegers{9});const auto old=state.witch_experience;
     fails([&]{reset_character_for_new_game(state,record);});check(*state.status==EventIntegers({7,0})&&state.witch_experience==old&&state.level==42);
     record.parameters->at(1)=nullptr;fails([&]{reset_character_for_new_game(state,record);});check(*state.status==EventIntegers({7,0})&&state.witch_experience==old);}
    std::int32_t id=2;TableExperienceHost host(levels,characters,id);
    check(host.next_experience(1)==15); // Uses id, not level_ (999).
    MemberExperience member{1,0};
    try{grant_quest_experience(member,1,20,host);check(false);}catch(const std::invalid_argument&){}
    check(member.level==2&&member.experience==5); // Mutation survives missing next row.
    characters[2]->required_experience_rate=50;check(host.next_experience(-1)==5);
    id=1;try{host.next_experience(1);check(false);}catch(const std::invalid_argument&){}
    auto duplicate=load_level_table(row+"\n1\t1\t30\t0\t0\t0\t0\t0\t0\t0\textra",true,parse_event_decimal);
    check(duplicate[1]->experience==30);
    check(load_level_table("\r\n",true,parse_event_decimal).size()==1);
    try{load_level_table("\r\n",false,parse_event_decimal);check(false);}catch(const std::invalid_argument&){}
    try{load_level_table("1\t2",false,parse_event_decimal);check(false);}catch(const std::out_of_range&){}
    try{load_level_table("-1\t0\t0\t0\t0\t0\t0\t0\t0\t0",false,parse_event_decimal);check(false);}catch(const std::out_of_range&){}
    try{load_character_progression_table(character.substr(0,character.find("\t150"))+"\t150",false,parse_event_decimal);check(false);}catch(const std::out_of_range&){}
}
