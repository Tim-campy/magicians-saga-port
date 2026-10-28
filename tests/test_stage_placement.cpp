#include "startup_fixture.hpp"
#include <limits>

int main()try{
    {StartupFixture f;check(stage_quest_height(*f.state.quest)==6&&stage_z(f.state,0,f)==84&&stage_z(f.state,100,f)==156);
     check(stage_formation_left(f)==28&&stage_formation_top(f.state,f)==84);
     f.state.quest->distance=-2;check(stage_z(f.state,50,f)==-96);
     f.state.quest->distance=4;f.geometry.step_z=std::numeric_limits<std::int32_t>::max();check(stage_z(f.state,0,f)==-1073741828);}
    {StartupFixture f;f.on_map_query=[&](int call){if(call==2)f.active_map=&f.other_geometry;};
     check(stage_z(f.state,0,f)==96&&f.map_queries==2);}
    {StartupFixture f;f.state.quest->money_rate=1;f.random.values={0,2,0};place_stage_money(f.state,f.random,f);
     auto& n=f.nodes.back();check(f.nodes.size()==2&&f.random.index==83&&f.map_queries==3);
     check(n.drop.type==1&&n.drop.value==1&&n.position.x==100&&n.position.z==91&&n.depth.extra==-10);}
    {StartupFixture f;f.state.quest->money_rate=1;f.random.values={0,2,99};place_stage_money(f.state,f.random,f);
     check(f.nodes.back().drop.value==15&&f.nodes.back().image.arrays.textures->at(0)==105);}
    {StartupFixture f;f.state.quest->money_rate=1;f.random.values={0,2,0};
     f.random.on_draw=[&](std::size_t i){if(i==1)f.active_map=&f.other_geometry;};place_stage_money(f.state,f.random,f);
     check(f.nodes.back().position.x==100&&f.nodes.back().position.z==182);}
    {StartupFixture f;f.active_map=nullptr;place_stage_money(f.state,f.random,f);
     check(f.nodes.size()==1&&f.random.index==81&&f.map_queries==1&&f.drop_queries==0);}
    {StartupFixture f;f.state.quest->completed=1;f.state.quest->cleared_reward_treasure=1;
     f.state.quest->fixed_treasure=scripts(std::make_shared<std::vector<EventInstruction>>(1,nullptr));
     place_stage_treasures(f.state,f.random,f);check(f.random.index==1&&f.nodes.size()==2&&f.nodes.back().drop.value==1);
     check(f.nodes.back().position.x==100&&f.nodes.back().position.z==156&&f.nodes.back().depth.extra==10);}
    {StartupFixture f;f.state.quest->fixed_treasure=scripts(std::make_shared<std::vector<EventInstruction>>(1,nullptr));
     fails([&]{place_stage_treasures(f.state,f.random,f);});check(f.random.index==1&&f.nodes.size()==1);}
    {StartupFixture f;auto entries=std::make_shared<std::vector<EventInstruction>>();
     entries->push_back(std::make_shared<EventIntegers>(EventIntegers{2,50}));entries->push_back(std::make_shared<EventIntegers>(EventIntegers{0,75}));
     f.state.quest->fixed_treasure=scripts(entries);f.state.quest->cleared_reward_treasure=1;f.random.values={99,2,99,2};
     f.on_drop=[&]{f.state.quest->completed=1;f.state.quest->fixed_treasure=scripts(std::make_shared<std::vector<EventInstruction>>());};
     place_stage_treasures(f.state,f.random,f);check(f.nodes.size()==4&&f.random.index==4);
     check(f.nodes[1].drop.value==2&&f.nodes[1].position.z==120&&f.nodes[2].drop.value==0&&f.nodes[3].drop.value==1);}
    {StartupFixture f;f.state.quest->prisoner_maximum=5;place_stage_prisoners(f.state,f.random,f);
     check(f.random.index==4&&f.nodes.size()==2&&f.map_queries==4);
     check(f.nodes.back().drop.type==2&&f.nodes.back().drop.value==0&&f.nodes.back().position.x==28&&f.nodes.back().position.z==91);}
    {StartupFixture f;f.state.quest->prisoner_maximum=3;f.random.values={0,99,0};place_stage_prisoners(f.state,f.random,f);
     check(f.random.index==3&&f.nodes.size()==3&&f.nodes[1].position.z==91&&f.nodes[2].position.z==120);}
    {StartupFixture f;f.active_map=nullptr;place_stage_prisoners(f.state,f.random,f);check(f.map_queries==1&&f.random.index==0&&f.nodes.size()==1);
     f.state.quest->prisoner_maximum=std::numeric_limits<std::int32_t>::min();f.state.quest->rescued_prisoners=1;
     check(remaining_stage_prisoners(*f.state.quest)==std::numeric_limits<std::int32_t>::max());}
    {StartupFixture f;f.state.quest->money_rate=1;f.state.quest->fixed_treasure.reset();f.random.values={0,2,0};
     fails([&]{place_stage_drops(f.state,f.random,f);});check(f.nodes.size()==2&&f.nodes.back().drop.type==1&&f.random.index==83);}
}catch(const std::exception& e){std::cerr<<"uncaught stage placement error: "<<e.what()<<'\n';return 1;}
