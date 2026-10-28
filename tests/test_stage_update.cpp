#include "stage_update_fixture.hpp"

int main()try{
    // Real Start -> intro -> collection -> boss gate -> celebration -> grants -> town.
    {UpdateFixture f;f.treasures[0]->flags=1;f.treasures[0]->experience=15;f.treasures[0]->money=7;
     f.begin();auto& s=f.state;auto& hero=f.nodes.front();
     f.form.fade_frame=19;update_stage(s,f);check(s.state==0&&s.loop_counter==1);
     f.form.fade_frame=20;update_stage(s,f);check(s.state==1&&s.frame==0);
     hero.position.z=12;update_stage(s,f);check(s.boss_started&&!s.collection.reward_box_picked_up&&s.remaining_time==599);
     s.collection.boss_defeated=true;update_stage(s,f);
     check(s.state==3&&s.collection.reward_box_picked_up&&s.rewards()->treasure_ids==EventIntegers{0}&&hero.human.state==6);
     check(s.remaining_time==599&&f.jingles==1);
     update_stage(s,f);check(s.state==3&&s.frame==1&&f.money==0);
     f.pulse=0x100000;update_stage(s,f);
     check(s.state==4&&s.frame==0&&f.money==7&&hero.human.level==2&&hero.human.experience==5);
     check(f.upgrades==std::vector<std::pair<EquipmentSlot,std::int32_t>>({{EquipmentSlot::weapon,3},{EquipmentSlot::hat,0},{EquipmentSlot::cloth,0}}));
     check(f.levels==std::array<std::int32_t,4>{5,11,8,10}&&s.quest->completed==0);
     update_stage(s,f);check(s.quest->completed==1&&f.transition&&f.roster.members->entities().empty());
     check(f.records[0].saved.level==2&&f.records[0].saved.experience==5&&f.record.clears==1);
     f.transition();check(f.returned.result==0&&f.returned.area_id==12&&f.returned.stage==3);}
    // All due events run in order; flags are live and a null event result still consumes its row.
    {UpdateFixture f;f.begin();f.state.quest->events=scripts(std::make_shared<std::vector<EventInstruction>>());
     for(auto row:{EventIntegers{8,0},EventIntegers{9,0}})f.state.quest->events->at(0)->push_back(std::make_shared<EventIntegers>(row));
     f.state.event_states=std::make_shared<std::vector<std::uint8_t>>(2,std::uint8_t{0});f.event_result=true;f.state.state=1;
     const auto time=f.state.remaining_time;update_stage(f.state,f);
     check(f.events==EventIntegers{8,9}&&f.state.remaining_time==time&&f.keys==EventIntegers{0x20000,0x80000});
     check(*f.state.event_states==std::vector<std::uint8_t>({1,1}));
     update_stage(f.state,f);check(f.events.size()==2&&f.state.remaining_time==time-1);}
    {UpdateFixture f;f.begin();f.state.quest->events=scripts(std::make_shared<std::vector<EventInstruction>>(1,std::make_shared<EventIntegers>(EventIntegers{7,0})));
     f.state.event_states=std::make_shared<std::vector<std::uint8_t>>(1,std::uint8_t{0});auto old=f.state.event_states;
     f.on_event=[&](std::int32_t){f.state.event_states=std::make_shared<std::vector<std::uint8_t>>(1,std::uint8_t{0});};
     check(!execute_stage_events(f.state,f)&&old->at(0)==0&&f.state.event_states->at(0)==1);
     f.state.event_states->at(0)=0;f.on_event=[](std::int32_t){throw std::runtime_error("event");};
     fails([&]{execute_stage_events(f.state,f);});check(f.state.event_states->at(0)==0);
     f.state.quest->completed=1;check(!execute_stage_events(f.state,f));}
    // Pause/resume priority, exit dialog creation, cancellation and voluntary return.
    {UpdateFixture f;f.begin();f.state.state=1;f.pulse=0x2000000;update_stage(f.state,f);check(f.state.state==2&&f.paused);
     f.pulse=0x4000000;update_stage(f.state,f);check(f.state.state==1&&!f.paused);
     f.state.state=2;f.state.frame=11;f.pulse=0x8000000;update_stage(f.state,f);
     check(f.state.state==2&&f.state.frame==11&&f.state.dialog->kind==99);
     f.state.dialog->answer=1;f.pulse=0;update_stage(f.state,f);check(!f.state.dialog&&f.state.state==2);
     f.dialog(99,0);const auto counter=f.state.loop_counter;update_stage(f.state,f);
     check(!f.state.dialog&&f.transition&&f.state.loop_counter==counter);f.transition();check(f.returned.result==3);}
    // Timeout creates retry + defeat forms. Refusing retry chooses the reason-specific exit.
    {UpdateFixture f;f.begin();f.state.state=1;f.state.remaining_time=1;update_stage(f.state,f);
     check(f.state.remaining_time==0&&f.state.dialog->kind==98&&f.state.dialog->reason==2&&f.pushed.size()==2);
     f.state.dialog->answer=2;auto counter=f.state.loop_counter;update_stage(f.state,f);check(f.state.state==6&&f.state.loop_counter==counter);
     update_stage(f.state,f);f.transition();check(f.returned.result==2);}
    {UpdateFixture f;f.begin();f.state.retried=true;f.state.frame=77;stage_game_over(f.state,1,f);
     check(f.state.state==5&&f.state.frame==0&&!f.state.dialog&&f.pushed.size()==1);
     update_stage(f.state,f);f.transition();check(f.returned.result==1);}
    // Crystal and ad paths call the required retry service only after the preceding operation succeeds.
    {UpdateFixture f;f.dialog(98,0);f.affordable=true;f.crystals=150;update_stage(f.state,f);
     check(f.crystals==50&&f.retries==1&&!f.state.dialog&&f.state.loop_counter==0);
     check(f.log==std::vector<std::string>({"check","subtract","retry"}));}
    {UpdateFixture f;f.dialog(98,0,2);auto old=f.state.dialog;update_stage(f.state,f);
     check(f.state.dialog!=old&&f.state.dialog->kind==98&&f.state.dialog->reason==2&&f.pushed.size()==2&&f.retries==0);}
    {UpdateFixture f;f.dialog(98,1);f.on_make_ad=[&]{f.selected_stack=nullptr;};update_stage(f.state,f);
     check(f.retries==1&&f.pushed.size()==1&&!f.state.dialog);}
    {UpdateFixture f;f.dialog(98,1);f.fail="push";fails([&]{update_stage(f.state,f);});check(f.retries==0&&f.state.dialog);}
    {UpdateFixture f;f.dialog(98,0);f.affordable=true;f.crystals=150;f.fail="retry";
     fails([&]{update_stage(f.state,f);});check(f.crystals==50&&f.state.dialog&&f.state.loop_counter==0);}
    {UpdateFixture f;f.dialog(98,0);f.null_retry=true;fails([&]{update_stage(f.state,f);});check(f.state.dialog&&f.pushed.empty());}
    // Counter wrap/remainder and signed cooldown arithmetic, including unknown states.
    {UpdateFixture f;f.state.state=-1;f.state.frame=9;f.state.loop_counter=std::numeric_limits<std::int32_t>::max()-1;
     update_stage(f.state,f);check(f.state.loop_counter==0&&f.state.frame==9);
     f.state.loop_counter=std::numeric_limits<std::int32_t>::max();update_stage(f.state,f);check(f.state.loop_counter==-1);
     f.buttons[0].frame=std::numeric_limits<std::int32_t>::max();f.buttons[1].frame=9;f.buttons[2].frame=-2;
     update_stage_attack_buttons(f.state);check(f.buttons[0].frame==std::numeric_limits<std::int32_t>::min()&&f.buttons[1].frame==4&&f.buttons[2].frame==-1);}
    {UpdateFixture f;auto& host=static_cast<StageUpdateHost&>(f);
     check(stage_progress_rate(f.state,12,host)==100&&stage_progress_rate(f.state,0,host)==116);
     check(stage_progress_rate(f.state,1000,host)==0);f.state.quest->distance=1;check(stage_progress_rate(f.state,0,host)==0);
     f.state.quest->distance=4;check(stage_progress_rate(f.state,std::numeric_limits<float>::quiet_NaN(),host)==116);
     check(stage_progress_rate(f.state,std::numeric_limits<float>::infinity(),host)==0);}
    // Alive excludes exactly states 4 and 5; zero HP is not consulted.
    {HumanRecordLink h;for(std::int32_t i=-2;i<=8;++i){h.state=i;check(stage_human_alive(h)==(i!=4&&i!=5));}
     std::int32_t n=100;raise_stage_equipment_level(n);check(n==100);n=std::numeric_limits<std::int32_t>::max();
     raise_stage_equipment_level(n);check(n==std::numeric_limits<std::int32_t>::min());}
    {UpdateFixture f;f.begin();f.state.state=3;f.pulse=0x100000;f.on_result=[&]{f.nodes.front().entity.remove(8);};
     fails([&]{update_stage(f.state,f);});check(f.state.state==3&&f.state.frame==1&&f.upgrades.empty()&&f.record.clears==1);}
    {UpdateFixture f;f.begin();f.state.state=1;f.state.collection.boss_defeated=true;f.state.collection.reward_box_picked_up=true;
     f.state.humans=nullptr;fails([&]{update_stage(f.state,f);});check(f.state.state==1&&f.state.loop_counter==1);}
    {UpdateFixture f;f.begin();f.state.state=1;f.state.collection.boss_defeated=true;f.state.collection.reward_box_picked_up=true;
     f.nodes.front().human.state=2;f.on_interval=[&]{f.roster.members->append(&f.nodes.front().entity);};
     fails([&]{update_stage(f.state,f);});check(f.state.state==1&&f.nodes.front().human.state==6&&f.nodes.front().human.frame==0);}
    {UpdateFixture f;f.begin();f.state.state=3;f.pulse=0x100000;
     f.on_upgrade=[&](std::size_t i){if(i==1)f.nodes.front().equipment.ids->at(2)=6;if(i==2)throw std::runtime_error("hat");};
     fails([&]{update_stage(f.state,f);});check(f.state.state==3&&f.levels[0]==5&&f.levels[2]==7&&f.money==0&&f.record.clears==1);
     check(f.upgrades[1].second==6&&f.upgrades.size()==2);}
    {UpdateFixture f;f.begin();f.state.state=1;f.state.remaining_time=std::numeric_limits<std::int32_t>::min();
     update_stage(f.state,f);check(f.state.remaining_time==std::numeric_limits<std::int32_t>::max());}
}catch(const std::exception& e){std::cerr<<"uncaught stage update error: "<<e.what()<<'\n';return 1;}
