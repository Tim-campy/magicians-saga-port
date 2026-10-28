#include "startup_fixture.hpp"
#include <algorithm>
#include <limits>

int main()try{
    {StartupFixture f;auto old_rewards=std::make_shared<StageRewards>(StageRewards{10,20,30,{2}});f.state.replace_rewards(old_rewards);
     auto* old=create_drop(0,5,{0,0,0},25,f.treasures,f.ids,static_cast<DropFactoryHost&>(f));auto old_id=f.entity_id(*old);
     f.state.collection.collecting->push_back(old);f.state.state=5;f.state.remaining_time=77;f.state.loop_counter=19;
     f.state.boss_started=true;f.state.collection.boss_defeated=true;f.state.collection.reward_box_picked_up=true;
     f.state.move_front_pushed=true;f.state.retried=true;f.state.frame=99;f.state.total_damage=100;f.state.last_active_button=88;
     auto q=f.state.quest;auto area=std::make_shared<int>(7);
     start_stage(f.state,area,q,3,f.random,f);
     check(f.state.area==area&&f.state.quest==q&&f.state.stage==3&&f.state.remaining_time==600&&f.state.loop_counter==19);
     check(f.node(*old).lifetime.destroyed&&!f.registry.contains(static_cast<std::int32_t>(old_id))&&f.state.collection.collecting->empty());
     check(f.state.state==0&&!f.state.boss_started&&!f.state.collection.boss_defeated&&!f.state.collection.reward_box_picked_up);
     check(!f.state.move_front_pushed&&!f.state.retried&&f.state.frame==0&&f.state.total_damage==0&&f.state.min_z_rate==0&&f.state.max_z_rate==95);
     check(f.state.rewards()!=old_rewards&&f.state.collection.rewards==f.state.rewards().get()&&old_rewards->money==20);
     check(f.trace==std::vector<std::string>({"cleanup","fps","map-system","map","party","enemies","form","drop","paused","resume","bgm","start-sound"}));
     check(f.mapped==q&&f.enemy_quest==q&&f.form.scale==100&&f.state.event_states->size()==2&&!f.paused&&f.random.index==81);
     check(f.buttons[0].state==1&&f.buttons[0].frame==4&&f.buttons[1].state==0&&f.buttons[2].frame==4&&f.state.last_active_button==0);
     collect_drops(f.state.collection,f);check(f.state.collection.collecting->size()==1);
     for(int i=0;i<31;++i)update_collecting_drops(f.state.collection,f);
     check(f.state.rewards()->treasure_ids==EventIntegers{0}&&f.sounds==1&&f.state.collection.collecting->empty()&&!f.garbage.empty());}
    {StartupFixture f;f.state.boss_started=true;f.state.remaining_time=77;auto q=f.state.quest;
     f.on_cleanup=[](){throw std::runtime_error("cleanup");};fails([&]{start_stage(f.state,nullptr,q,9,f.random,f);});
     check(f.state.stage==9&&f.state.rewards()&&f.state.rewards()->money==0&&f.state.boss_started&&f.state.remaining_time==77&&f.trace.size()==1);}
    {StartupFixture f;f.state.remaining_time=77;f.state.boss_started=true;
     f.on_fps=[](){throw std::runtime_error("fps");};fails([&]{start_stage(f.state,nullptr,f.state.quest,0,f.random,f);});
     check(f.state.remaining_time==77&&f.state.boss_started&&f.trace.back()=="fps");}
    {StartupFixture f;auto original=f.state.quest;auto second=std::make_shared<StageQuest>(*original),third=std::make_shared<StageQuest>(*original);
     second->time_limit=20;third->distance=3;third->flags=2;third->events=scripts(std::make_shared<std::vector<EventInstruction>>(5));
     f.debug=true;f.auto_party=true;f.on_cleanup=[&]{f.state.quest=second;};f.on_map_load=[&]{f.state.quest=third;};
     start_stage(f.state,nullptr,original,0,f.random,f);
     check(f.mapped==second&&f.enemy_quest==original&&f.state.quest==third&&f.state.remaining_time==1200);
     check(f.random_queries==0&&f.quest_queries==0&&f.state.event_states->size()==5);}
    {StartupFixture f;MemberRoster replacement;for(int i=0;i<4;++i)replacement.members->append(&f.nodes.front().entity);
     f.on_enemy_start=[&]{f.state.members=&replacement;};start_stage(f.state,nullptr,f.state.quest,0,f.random,f);check(f.form.scale==75);}
    {StartupFixture f;f.state.last_active_button=77;f.state.attack_buttons->at(1)=nullptr;
     fails([&]{start_stage(f.state,nullptr,f.state.quest,0,f.random,f);});check(f.buttons[0].state==1&&f.buttons[0].frame==4&&f.state.last_active_button==77);
     check(f.trace.back()=="drop"&&f.paused&&!f.state.event_states);}
    {StartupFixture f;f.state.last_active_button=77;f.state.quest->events.reset();
     fails([&]{start_stage(f.state,nullptr,f.state.quest,0,f.random,f);});check(f.state.last_active_button==0&&f.buttons[2].frame==4&&f.paused);}
    {StartupFixture f;f.on_bgm=[](){throw std::runtime_error("bgm");};
     fails([&]{start_stage(f.state,nullptr,f.state.quest,0,f.random,f);});check(!f.paused&&f.state.event_states&&f.trace.back()=="bgm");}
    {StartupFixture f;f.state.quest->time_limit=std::numeric_limits<std::int32_t>::max();f.fps=2;
     reset_stage_timer(f.state,f);check(f.state.remaining_time==-2);
     f.state.quest->time_limit=2;auto original=f.state.quest;f.on_fps=[&]{f.state.quest=std::make_shared<StageQuest>();};
     reset_stage_timer(f.state,f);check(f.state.remaining_time==4&&f.state.quest!=original);}
    {StartupFixture f;f.state.humans=nullptr;fails([&]{start_stage(f.state,nullptr,f.state.quest,0,f.random,f);});
     check(f.trace.back()=="map"&&f.roster.members->entities().empty()&&f.state.remaining_time==600);}
    {AttackButtonState button{7,9};change_attack_button_state(button,-3);check(button.state==-3&&button.frame==0);}
}catch(const std::exception& e){std::cerr<<"uncaught stage startup error: "<<e.what()<<'\n';return 1;}
