#include "core/gameplay/progression_tables.hpp"
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <iostream>
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
    check(characters.size()==3&&!characters[1]&&characters[2]->required_experience_rate==150);
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
