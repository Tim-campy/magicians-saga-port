#include "combat_stage_fixture.hpp"

struct FixtureWithEvents {UpdateFixture f;Services host{f};World world{f,host};};
struct DeathStage final:HumanDeathHost {UpdateFixture& f;explicit DeathStage(UpdateFixture& value):f(value){}
    void game_over(std::int32_t reason)override{stage_game_over(f.state,reason,f);}};
int main()try{
    {FixtureWithEvents t;auto& s=t.f.enemy_system;auto q=std::make_shared<StageQuest>();s.quest=q;
     s.created_enemies=9;s.defeated_enemies=8;s.loop_counter=7;s.remaining_enemies=6;
     auto previous=s.maximum_by_section;t.world.on_call=[&](const std::string& step){if(step=="subscribe23"){
         check(s.subset&&s.gauge==&t.world.gauge&&s.stage==&t.f.state&&s.members==&t.f.roster);
         check(s.maximum_by_section!=previous&&s.maximum_by_section!=s.created_by_section&&*s.maximum_by_section==EventIntegers(10,0)&&*s.created_by_section==EventIntegers(10,0));}};
     t.f.no_world=true; // Init must keep using its supplied world, even with no current global world.
     t.world.init();check(t.world.calls==std::vector<std::string>{"subset","gauge","stage","members","subscribe23","subscribe29","subscribe30"});
     check(s.quest==q&&s.created_enemies==9&&s.defeated_enemies==8&&s.loop_counter==7&&s.remaining_enemies==6);}
    {FixtureWithEvents t;auto old=t.f.enemy_system.subset;fails([&]{initialize_enemies(t.f.enemy_system,nullptr,t.host.bindings);});
     check(t.f.enemy_system.subset==old&&t.world.calls.empty());t.world.null_lookups=true;t.world.init();
     check(!t.f.enemy_system.subset&&!t.f.enemy_system.gauge&&!t.f.enemy_system.stage&&!t.f.enemy_system.members);
     t.host.events.send(23,nullptr);fails([&]{t.host.events.send(30,nullptr);});}
    for(const auto& stop:std::vector<std::string>{"subset","gauge","stage","members","subscribe23","subscribe29","subscribe30"}){
     FixtureWithEvents t;auto& s=t.f.enemy_system;auto old=s.maximum_by_section;auto* old_subset=s.subset;
     s.stage=nullptr;s.members=nullptr;t.world.fail=stop;fails([&]{t.world.init();});
     if(stop=="subset")check(s.subset==old_subset);
     if(stop=="stage"||stop=="members")check(s.gauge==&t.world.gauge);
     if(stop=="members")check(s.stage==&t.f.state&&!s.members);
     const bool subscribed=stop=="subscribe23"||stop=="subscribe29"||stop=="subscribe30";
     check((s.maximum_by_section!=old)==subscribed);
     auto& e=t.f.spawn(1).entity;t.host.events.send(23,&e);
     check(t.host.guards==(stop=="subscribe29"||stop=="subscribe30"?1:0));}
    // Repeat Init replaces arrays but retains subscriptions, including after a later failure.
    {FixtureWithEvents t;std::array<EventHashes,3> hashes{{{1,101},{2,102},{3,103}}};t.world.init(hashes);
     auto first=t.f.enemy_system.maximum_by_section;t.world.init();check(t.f.enemy_system.maximum_by_section!=first);
     auto& e=t.f.spawn(1).entity;t.host.events.send(23,&e);check(t.host.guards==2);
     check(t.host.events.unsubscribe_single(23,1));t.host.events.send(23,&e);check(t.host.guards==3);
     check(t.host.events.unsubscribe_pair(29,2)&&t.host.events.unsubscribe_single(30,3));}
    // Actual collision -> recoil -> live Enemy/Move passes -> walking, without consuming RNG while invincible.
    {FixtureWithEvents t;t.world.init();auto& f=t.f;f.begin();reset_enemy_population(f.enemy_system,f);f.enemy_system.remaining_enemies=0;
     auto& enemy=create_system_enemy(f.enemy_system,{2,2,1,0,{40,0,50}},f);auto* hero=member_hero(&f.world);
     field<HumanRecordLink>(*hero,14).invincible_time=1;f.random.index=0;t.host.events.send(29,&enemy,hero);
     check(field<EnemyRecordLink>(enemy,24).state==3&&field<ProjectileSpeed>(enemy,1).z==-10&&field<ContactPosition>(enemy,0).z==40);
     auto* move=t.world.subset({0,1});bool enabled=true;
     GameplayUpdate enemies(5,enabled,[&](std::int32_t){update_enemies(f.enemy_system,f);});
     GameplayUpdate movement(11,enabled,[&](std::int32_t){move_subset(*move);});
     std::vector<UpdateSystem*> list{&enemies,&movement};UpdateSchedule schedule{&list,nullptr};
     for(int i=0;i<20;++i)update_systems(schedule,1);
     check(field<EnemyRecordLink>(enemy,24).state==3&&field<ContactDirection>(enemy,12).locked);
     update_systems(schedule,1);check(field<EnemyRecordLink>(enemy,24).state==1&&field<EnemyRecordLink>(enemy,24).frame==1);
     check(!field<ContactDirection>(enemy,12).locked&&field<ContactPosition>(enemy,0).z<1&&f.random.index==0);}
    // The registered contact handler uses actual numeric damage, shared defeat counters and real EXP drops.
    {FixtureWithEvents t;t.world.init();auto& f=t.f;f.begin();reset_enemy_population(f.enemy_system,f);
     auto& attacker=create_system_enemy(f.enemy_system,{2,2,1,0,{0,0,50}},f);
     auto& target=create_system_enemy(f.enemy_system,{2,2,1,0,{0,0,50}},f);resolve_combat_parameter(target,0)->value=1;
     auto& source=create_entity(f.ids,0,false,static_cast<Fixture&>(f));ProjectileDamage area;area.parent=&attacker;area.value=1000;area.attribute=0;source.add(33,&area);
     f.random.index=0;f.random.values={0,5,99,0,0,0};t.host.events.send(29,&source,&target);
     check(resolve_combat_parameter(target,0)->value==0&&field<EnemyRecordLink>(target,24).state==4&&target.contains(27));
     check(t.host.damage_amount==433&&f.state.total_damage==433&&f.enemy_system.defeated_enemies==1&&f.enemy_system.remaining_enemies==9);
     check(t.host.defeat_counts[2]==1&&f.random.index==6&&f.nodes.back().drop.type==0&&f.nodes.back().drop.value==10);}
    // Human landing sends nested death before Enemy impact/Projectile cleanup; real target selection observes state 5.
    {FixtureWithEvents t;auto& f=t.f;DeathStage stage(f);
     t.host.events.subscribe(30,[&](ComponentSlots* e){t.host.calls.push_back("stage30");stage_human_dead(*e,t.host.records,stage);});
     t.host.events.subscribe(23,[&](ComponentSlots* e){t.host.calls.push_back("human23");human_projectile_impact(e,t.host);});
     t.world.init();ProjectileSystemState projectile;initialize_projectiles(projectile,&t.world,t.host);
     f.begin();reset_enemy_population(f.enemy_system,f);auto* hero=member_hero(&f.world);
     auto* sub=create_sub_character(0,0,1,f.party_host());add_member(f.roster,sub);
     field<ContactPosition>(*sub,0).x=1000;field<ContactPosition>(*sub,0).z=1000;
     auto& normal=create_system_enemy(f.enemy_system,{2,2,1,0,{0,0,50}},f);
     auto& boss=create_system_enemy(f.enemy_system,{2,2,1,1,{0,0,50}},f);
     field<HumanRecordLink>(*hero,14).state=4;auto& p=field<ContactPosition>(*hero,0);
     launch_projectile(*hero,{p.x,p.y,p.z},{p.x,p.y,p.z},4,nullptr,false,false,200,static_cast<EnemyImpactHost&>(t.host));
     t.host.calls.clear();f.random.index=0;projectile.pass->update(*projectile.subset,t.host);projectile.pass->update(*projectile.subset,t.host);
     check(t.host.calls==std::vector<std::string>{"human23","human-presentation","stage30","enemy30","enemy23","projectile23"});
     check(field<HumanRecordLink>(*hero,14).state==5&&!hero->contains(27)&&t.host.returned.back()==27);
     check(field<EnemyRecordLink>(normal,24).opponent==sub&&field<EnemyRecordLink>(boss,24).opponent==hero&&t.host.searches==2);
     check(f.state.dialog&&f.state.dialog->kind==98&&f.state.dialog->reason==1&&f.random.index==0);}
    // Death reads the current stored subset, and captured-component publication survives replacement during SearchTarget.
    {FixtureWithEvents t;t.world.init();auto& f=t.f;f.begin();reset_enemy_population(f.enemy_system,f);
     auto* dead=member_hero(&f.world);field<HumanRecordLink>(*dead,14).state=5;
     auto* sub=create_sub_character(0,0,1,f.party_host());add_member(f.roster,sub);
     auto& enemy=create_system_enemy(f.enemy_system,{2,2,1,0,{0,0,50}},f);auto& old=field<EnemyRecordLink>(enemy,24);old.opponent=dead;
     EnemyRecordLink replacement;replacement.enemy_id=2;
     t.host.on_search=[&](ComponentSlots& e){e.change(24,&replacement);};t.host.events.send(30,dead);
     check(old.opponent==sub&&!replacement.opponent);
     EntitySubset empty([](ComponentSlots*){return true;},[](const ComponentSlots&){return 0;});f.enemy_system.subset=&empty;
     t.host.events.send(30,dead);check(t.host.searches==1);}
}catch(const std::exception& e){std::cerr<<"uncaught enemy initialization error: "<<e.what()<<'\n';return 1;}
