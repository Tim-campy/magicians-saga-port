#include "core/gameplay/experience_progression.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool b){if(!b)std::abort();}
struct Host:ExperienceProgressionHost {
    std::vector<std::optional<std::int32_t>> rows{0,10,20,30};
    std::vector<std::int32_t> indices;int counts=0;std::int32_t rate=100;
    std::int32_t table_count()override{++counts;return static_cast<std::int32_t>(rows.size());}
    std::int32_t next_experience(std::int32_t index)override{indices.push_back(index);return next_character_experience(index,rate,rows);}
};
int main(){
    check(character_witch_experience({7,99})==7);
    try{character_witch_experience({});check(false);}catch(const std::out_of_range&){}
    check(scaled_level_experience(199,50)==99&&scaled_level_experience(-199,50)==-99);
    check(scaled_level_experience(2147483647,100)==-1);
    {Host h;check(next_character_experience(-3,100,h.rows)==10);
        h.rows.resize(1201);h.rows[1200]=123;check(next_character_experience(2147483647,100,h.rows)==123);
        try{next_character_experience(5,100,h.rows);check(false);}catch(const std::invalid_argument&){} }
    {Host h;MemberExperience m{1,5};grant_quest_experience(m,0,100,h);check(m.level==1&&m.experience==5&&h.counts==0);}
    {Host h;MemberExperience m{1,5};grant_quest_experience(m,1,30,h);
        check(m.level==3&&m.experience==5&&h.indices==std::vector<std::int32_t>({1,2,3}));}
    {Host h;h.rows={0,0,0};MemberExperience m{1,0};grant_quest_experience(m,1,0,h);check(m.level==3&&m.experience==0&&h.counts==3);}
    {Host h;h.rows.resize(1203);h.rows[1200]=1;MemberExperience m{1200,5};grant_quest_experience(m,1,0,h);
        check(m.level==1200&&m.experience==2&&h.indices==std::vector<std::int32_t>({1200,1201,1202}));}
    {Host h;h.rows={0,1};MemberExperience m{1,2147483647};grant_quest_experience(m,1,1,h);check(m.experience==(-2147483647-1)&&m.level==1);}
    {Host h;h.rows={0,-1};MemberExperience m{1,2147483647};grant_quest_experience(m,1,0,h);check(m.experience==(-2147483647-1)&&m.level==2);}
    {Host h;h.rows={0,1};MemberExperience m{1,(-2147483647-1)};grant_quest_experience(m,1,0,h);check(m.experience==(-2147483647-1)&&m.level==1);}
    {Host h;h.rows[2].reset();MemberExperience m{1,0};
        try{grant_quest_experience(m,1,50,h);check(false);}catch(const std::invalid_argument&){}
        check(m.level==2&&m.experience==40);}
}
