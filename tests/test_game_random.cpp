#include "core/gameplay/game_random.hpp"
#include "core/gameplay/enemy_rewards.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Fixed final:RandomSource {std::int32_t value;int calls=0;explicit Fixed(std::int32_t v):value(v){}std::int32_t next()override{++calls;return value;}};
struct Replay final:RandomReplayHost {
    bool exists=false;std::string preference="off";int files=0,prefs=0;
    bool replay_file_exists()override{++files;return exists;}
    std::string replay_record_preference()override{++prefs;return preference;}
};
struct Enemy final:EnemyRewardHost {
    LegacySystemRandom random{42};EnemyRecord record;std::vector<std::int32_t> treasures;
    Enemy(){record.experience=EventIntegers{0,0};record.treasures=EnemyTreasureGroups{{{3,500},{4,500},{5,1000}}};}
    const EnemyRecord& enemy_data()override{return record;}
    void spawn_experience_drop(std::int32_t)override{check(false);}
    bool hits1000(std::int32_t threshold)override{return random_hits1000(random,threshold);}
    void spawn_treasure_drop(std::int32_t id)override{treasures.push_back(id);}
};
int main(){
    // Public seeded System.Random reference vectors, also obtained through Windows PowerShell's System.Random.
    const std::array<std::int32_t,7> seeds{{0,1,-1,42,123456789,(-2147483647-1),2147483647}};
    const std::array<std::array<std::int32_t,6>,7> expected{{
        {{1559595546,1755192844,1649316166,1198642031,442452829,1200195957}},
        {{534011718,237820880,1002897798,1657007234,1412011072,929393559}},
        {{534011718,237820880,1002897798,1657007234,1412011072,929393559}},
        {{1434747710,302596119,269548474,1122627734,361709742,563913476}},
        {{1091672793,381850644,1335622286,865414785,1968738143,1473299219}},
        {{1559595546,1755192844,1649316172,1198642031,442452829,1200195955}},
        {{1559595546,1755192844,1649316172,1198642031,442452829,1200195955}}}};
    for(std::size_t i=0;i<seeds.size();++i){LegacySystemRandom random(seeds[i]);for(auto value:expected[i])check(random.next()==value);}
    {LegacySystemRandom a(42),b(42);check(a.next()==1434747710);check(a.next()==302596119);check(b.next()==1434747710);}
    check(random_seed_from_millis(0x100000001LL,false)==1&&random_seed_from_millis(0x80000000LL,false)==(-2147483647-1));
    check(random_seed_from_millis(-1,false)==-1&&random_seed_from_millis(123,true)==0);
    {RandomConfiguration config;Replay replay;apply_replay_random_configuration(config,false,replay);check(!config.fixed_seed&&replay.files==0&&replay.prefs==0);
        replay.exists=true;apply_replay_random_configuration(config,true,replay);check(config.fixed_seed&&replay.files==1&&replay.prefs==0);
        replay.exists=false;apply_replay_random_configuration(config,true,replay);check(config.fixed_seed&&replay.prefs==1);}
    {RandomConfiguration config;Replay replay;replay.preference="ON";apply_replay_random_configuration(config,true,replay);check(!config.fixed_seed);
        replay.preference="on";apply_replay_random_configuration(config,true,replay);check(config.fixed_seed);}
    {RandomConfiguration config;Fixed a(1),b(2),c(3);SelectedRandomSource selected(config,&a,&b,&c);
        check(selected.next()==1);config.logic=1;check(selected.next()==2);config.logic=2;check(selected.next()==3);
        config.logic=-1;check(selected.next()==1);check(a.calls==2&&b.calls==1&&c.calls==1);
        SelectedRandomSource missing(config,&a,nullptr,nullptr);config.logic=2;try{missing.next();check(false);}catch(const std::invalid_argument&){} }
    {Fixed source(-5);check(random_bounded(source,3)==2&&jrandom_bounded(source,3)==-2);
        check(random_inclusive(source,10,20)==5&&random_bounded(source,0)==5&&jrandom_bounded(source,0)==-5);
        check(random_inclusive(source,7,7)==7&&source.calls==6);}
    {Fixed source((-2147483647-1));check(random_bounded(source,1000)==-648&&random_hits1000(source,0));
        check(jrandom_bounded(source,-1)==0);}
    {Fixed source(9);check(!random_hits1000(source,0)&&random_hits1000(source,1000)&&source.calls==2);}
    {Enemy enemy;spawn_enemy_rewards(1,enemy);check(enemy.treasures==std::vector<std::int32_t>{4});check(enemy.random.next()==269548474);}
}
