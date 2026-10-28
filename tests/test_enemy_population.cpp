#include "stage_update_fixture.hpp"

template<class T>T& field(ComponentSlots& e,std::int32_t slot){auto* p=dynamic_cast<T*>(e.get(slot));check(p);return *p;}
EventScript rows(std::initializer_list<EventIntegers> input){auto result=std::make_shared<std::vector<EventInstruction>>();
    for(const auto& row:input)result->push_back(std::make_shared<EventIntegers>(row));return result;}
struct SnapshotFixture:UpdateFixture {
    std::function<void()> on_remove;
    void remove_all_components(QuestMemberId id)override{auto callback=std::move(on_remove);if(callback)callback();Fixture::remove_all_components(id);}
};
int main()try{
    // Lot uses its original 256-cell expansion, truncation and exact draw count.
    {WeightedLotteryState state;Draws random;EventIntegers weights{1,2};random.values={2};
     check(weighted_lottery(&weights,state,random)==1&&random.index==1);
     check(state.table->at(0)==0&&state.table->at(1)==1&&state.table->at(2)==1&&state.table->at(255)==-1);
     EventIntegers huge{300,1};random.index=0;random.values={254};check(weighted_lottery(&huge,state,random)==0&&random.index==1);
     check(state.table->at(254)==0&&state.table->at(255)==-1);}
    {WeightedLotteryState state;Draws random;EventIntegers empty;state.table->at(0)=7;
     check(weighted_lottery(nullptr,state,random)==-1&&weighted_lottery(&empty,state,random)==-1&&random.index==0&&state.table->at(0)==7);
     EventIntegers zero{0,-3};random.values={0,1,256};fails([&]{weighted_lottery(&zero,state,random);});check(random.index==3);}
    {WeightedLotteryState state;Draws random;EventIntegers weights{1,1};random.values={0,1};
     random.on_draw=[&](std::size_t n){if(n==0)state.table->at(0)=-1;};
     check(weighted_lottery(&weights,state,random)==1&&random.index==2);}
    {WeightedLotteryState state;Draws random;EventIntegers weights{1,1};auto old=state.table;
     random.values={0};random.on_draw=[&](std::size_t){state.table=std::make_shared<EventIntegers>(256,-1);};
     check(weighted_lottery(&weights,state,random)==0&&random.index==1&&state.table!=old&&old->at(0)==0);}
    {WeightedLotteryState state;Draws random;EventIntegers weights{1,1};auto old=state.table;
     random.values={0,1};random.on_draw=[&](std::size_t n){if(n==0){old->at(0)=-1;state.table.reset();}};
     fails([&]{weighted_lottery(&weights,state,random);});check(random.index==2);}
    {WeightedLotteryState state;Draws random;EventIntegers weights{1};state.table=std::make_shared<EventIntegers>(1,0);
     random.values={0};check(weighted_lottery(&weights,state,random)==0);
     state.table=std::make_shared<EventIntegers>();random.index=0;fails([&]{weighted_lottery(&weights,state,random);});check(random.index==0);}
    // Actual StageSystem.Start reaches the concrete EnemyStageEntry adapter.
    {UpdateFixture f;f.state.quest->enemy_total_limit=120;f.state.quest->base_level=7;
     f.state.quest->fixed_enemies=scripts(rows({{1,20},{2,80}}));f.begin();auto& s=f.enemy_system;
     check(s.quest==f.state.quest&&s.remaining_enemies==120&&s.created_enemies==0&&s.defeated_enemies==0&&s.loop_counter==0);
     check(s.subset->size()==3&&f.yugami_queries==4);
     for(std::size_t i=0;i<10;++i)check(s.maximum_by_section->at(i)==(i<7?12:99999)&&s.created_by_section->at(i)==0);
     auto cursor=s.subset->cursor();auto* first=*cursor.next();auto* second=*cursor.next();auto* initial=*cursor.next();
     check(field<EnemyRecordLink>(*first,24).enemy_id==1&&field<EnemyRecordLink>(*first,24).flags==2&&field<EnemyRecordLink>(*first,24).level==7);
     check(field<ContactPosition>(*first,0).z==98&&field<ContactPosition>(*second,0).z==141);
     check(field<EnemyRecordLink>(*initial,24).enemy_id==0&&field<EnemyRecordLink>(*initial,24).level==1&&!initial->contains(12));
     check(field<ContactPosition>(*initial,0).x==100&&field<ContactPosition>(*initial,0).z==34);
     auto& m=field<ProjectileModification>(*initial,15);check(m.type==9&&m.angle==30&&m.loop&&m.max_frame==0);}
    {UpdateFixture f;f.yugami=7;f.begin();auto& s=f.enemy_system;check(s.remaining_enemies==10);
     check(s.maximum_by_section->at(0)==100&&s.maximum_by_section->at(9)==9999900);
     auto first=*s.subset->cursor().next();check(field<EnemyRecordLink>(*first,24).level==50);}
    // Reset commits counters before array failure, and snapshots before destruction callbacks.
    {UpdateFixture f;f.begin();auto& s=f.enemy_system;s.created_enemies=9;s.defeated_enemies=8;s.loop_counter=7;
     auto* old=*s.subset->cursor().next();s.created_by_section.reset();
     fails([&]{reset_enemy_population(s,f);});check(s.created_enemies==0&&s.defeated_enemies==0&&s.loop_counter==0&&s.remaining_enemies==10&&!f.node(*old).lifetime.destroyed);}
    {SnapshotFixture f;f.begin();auto* old=*f.enemy_system.subset->cursor().next();ComponentSlots* added=nullptr;
     f.on_remove=[&]{f.on_remove=nullptr;added=&create_system_enemy(f.enemy_system,{0,2,1,0,{}},f);};
     reset_enemy_population(f.enemy_system,f);check(added&&f.node(*old).lifetime.destroyed&&!f.node(*added).lifetime.destroyed&&f.enemy_system.subset->size()==1);}
    // Fixed rows and map receiver are captured; later row levels read the current quest.
    {UpdateFixture f;f.begin();reset_enemy_population(f.enemy_system,f);auto q=f.enemy_system.quest;
     q->base_level=7;q->fixed_enemies=scripts(rows({{1,20},{2,80}}));auto replacement=std::make_shared<StageQuest>(*q);
     replacement->base_level=9;replacement->fixed_enemies=scripts(rows({}));int calls=0;
     f.on_enemy_factory=[&]{if(++calls==1){f.enemy_system.quest=replacement;f.geometry.step_x=200;}};
     create_fixed_enemies(f.enemy_system,f);auto cursor=f.enemy_system.subset->cursor();auto* a=*cursor.next();auto* b=*cursor.next();
     check(field<EnemyRecordLink>(*a,24).level==7&&field<EnemyRecordLink>(*b,24).level==9);
     check(field<ContactPosition>(*a,0).x==100&&field<ContactPosition>(*b,0).x==200&&calls==2);}
    {UpdateFixture f;f.enemy_system.quest=f.state.quest;f.active_map=nullptr;create_fixed_enemies(f.enemy_system,f);
     f.state.quest->fixed_enemies=scripts(rows({{1,20}}));fails([&]{create_fixed_enemies(f.enemy_system,f);});}
    // Failure after fixed placement leaves those enemies and the previous section maxima.
    {UpdateFixture f;auto& s=f.enemy_system;s.maximum_by_section=std::make_shared<EventIntegers>(10,77);
     f.state.quest->fixed_enemies=scripts(rows({{1,20}}));f.enemy_pools.records->at(0).reset();
     fails([&]{start_enemy_population(s,f.state.quest,f);});
     check(s.subset->size()==1&&s.maximum_by_section->at(0)==77&&s.created_by_section->at(0)==0&&s.remaining_enemies==10);}
    // One normal spawn: weighted ID, then column draw, real components/target and section accounting.
    {UpdateFixture f;f.begin();auto& s=f.enemy_system;f.random.index=0;f.random.values={0,2};
     auto* e=create_normal_enemy(s,f);check(e&&s.created_enemies==1&&s.created_by_section->at(0)==1&&f.random.index==2);
     check(field<EnemyRecordLink>(*e,24).enemy_id==2&&field<EnemyRecordLink>(*e,24).level==3&&field<EnemyRecordLink>(*e,24).state==1);
     check(field<ContactPosition>(*e,0).x==136&&field<ContactPosition>(*e,0).z==156&&field<EnemyRecordLink>(*e,24).opponent==member_hero(&f.world));}
    {UpdateFixture f;f.begin();f.enemy_system.quest->normal_enemies=scripts(rows({}));f.random.index=0;
     check(!create_normal_enemy(f.enemy_system,f)&&f.enemy_system.created_enemies==1&&f.random.index==0&&f.enemy_system.created_by_section->at(0)==0);
     f.enemy_system.quest->normal_enemies=scripts(rows({{1,300},{2}}));
     fails([&]{create_normal_enemy(f.enemy_system,f);});check(f.enemy_system.created_enemies==2&&f.random.index==0);}
    // Successful creation survives a later section-index failure, without the walking transition.
    {UpdateFixture f;f.begin();auto* hero=member_hero(&f.world);field<ContactPosition>(*hero,0).z=12;f.random.index=0;f.random.values={0,0};
     auto count=f.enemy_system.subset->size();fails([&]{create_normal_enemy(f.enemy_system,f);});
     check(f.enemy_system.subset->size()==count+1&&f.nodes.back().enemy.state==0&&f.enemy_system.created_enemies==1);}
    // Population gate counts record NO_ANIMATION, including dead enemies; it is not an alive count.
    {UpdateFixture f;f.state.quest->enemy_total_limit=100;f.state.quest->enemy_interval=1;f.state.quest->enemy_maximum=1;
     f.enemy_pools.records->at(0)->flags=1;f.begin();f.random.index=0;f.random.values={0,0,0,0};auto& s=f.enemy_system;
     check(&update_enemy_population(s,f)==s.subset&&s.created_enemies==1&&s.subset->size()==2&&f.random.index==2);
     f.nodes.back().enemy.state=4;update_enemy_population(s,f);check(s.created_enemies==1&&f.random.index==2);
     f.enemy_pools.records->at(2)->flags=1;update_enemy_population(s,f);check(s.created_enemies==2&&f.random.index==4);}
    {UpdateFixture f;f.begin();f.random.index=0;f.random.values={0,0};update_enemy_population(f.enemy_system,f);
     check(f.enemy_system.loop_counter==1&&f.enemy_system.created_enemies==0&&f.random.index==0);
     update_enemy_population(f.enemy_system,f);check(f.enemy_system.loop_counter==2&&f.enemy_system.created_enemies==1&&f.random.index==2);}
    {UpdateFixture f;f.yugami=7;f.begin();f.state.quest->enemy_interval=999;f.random.index=0;f.random.values={0,0};
     update_enemy_population(f.enemy_system,f);check(f.enemy_system.created_enemies==1&&f.nodes.back().enemy.level==150);}
    {UpdateFixture f;f.begin();field<ContactPosition>(*member_hero(&f.world),0).z=12;
     fails([&]{update_enemy_population(f.enemy_system,f);});check(f.enemy_system.loop_counter==1&&f.enemy_system.created_enemies==0);
     f.state.collection.boss_defeated=true;update_enemy_population(f.enemy_system,f);check(f.enemy_system.loop_counter==2);}
}catch(const std::exception& e){std::cerr<<"uncaught enemy population error: "<<e.what()<<'\n';return 1;}
