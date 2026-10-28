#include "stage_update_fixture.hpp"
#include <limits>

template<class T>T& field(ComponentSlots& e,std::int32_t slot){auto* p=dynamic_cast<T*>(e.get(slot));check(p);return *p;}
int main()try{
    {UpdateFixture f;f.begin();auto& boss=create_boss_enemy(f.enemy_system,f);auto& b=field<EnemyRecordLink>(boss,24);
     auto* hero=member_hero(&f.world);check(b.enemy_id==2&&b.level==4&&b.flags==1&&b.opponent==hero&&f.yugami_queries==2);
     check(field<ContactPosition>(boss,0).x==100&&field<ContactPosition>(boss,0).z==154&&field<ProjectileSpeed>(boss,1).z==0);
     auto& extra=f.nodes.back().entity;auto& e=field<EnemyRecordLink>(extra,24);
     check(&extra!=&boss&&e.enemy_id==1&&e.level==1&&e.flags==0&&e.opponent==hero&&!extra.contains(12));
     check(field<ContactPosition>(extra,0).x==100&&field<ContactPosition>(extra,0).z==153);
     auto& modifier=field<ProjectileModification>(extra,15);check(modifier.type==9&&modifier.angle==30&&modifier.loop&&modifier.max_frame==0);
     check(!boss.contains(15));}
    // Yugami changes only the EnemyComponent level after factory stats/target selection.
    {UpdateFixture f;f.begin();f.yugami=7;auto& e=create_system_enemy(f.enemy_system,{2,2,4,1,{}},f);
     check(field<EnemyRecordLink>(e,24).level==200&&resolve_combat_parameter(e,0)->level==4&&resolve_combat_parameter(e,0)->max_level==1000);}
    {UpdateFixture f;f.begin();f.yugami=0;EnemyRecordLink replacement;replacement.level=3;
     ComponentSlots* created=nullptr;f.on_added=[&](ComponentSlots& e,std::int32_t slot){if(slot==24)created=&e;};
     f.on_yugami=[&]{check(created);created->change(24,&replacement);};
     auto& e=create_system_enemy(f.enemy_system,{0,2,4,1,{}},f);
     check(&field<EnemyRecordLink>(e,24)==&replacement&&replacement.level==150&&!replacement.opponent);
     check(f.node(e).enemy.opponent==member_hero(&f.world));}
    {UpdateFixture f;f.begin();f.on_yugami=[](){throw std::runtime_error("save unavailable");};
     const auto count=f.nodes.size();fails([&]{create_system_enemy(f.enemy_system,{0,2,4,1,{}},f);});
     check(f.nodes.size()==count+1&&f.nodes.back().enemy.opponent==member_hero(&f.world)&&f.nodes.back().enemy.level==4);}
    // Existing live targets are retained, irrespective of distance or membership.
    {UpdateFixture f;f.begin();auto& remote=f.spawn(1);remote.position.x=10000;remote.position.z=10000;
     auto& e=create_system_enemy(f.enemy_system,{0,0,1,0,{}},f);auto& link=field<EnemyRecordLink>(e,24);link.opponent=&remote.entity;
     check(search_enemy_target(f.enemy_system,&e,f)==&remote.entity);
     remote.human.state=4;check(search_enemy_target(f.enemy_system,&e,f)==member_hero(&f.world));}
    // Nearest living member uses X/Z only, strict first ties, no HP test and ordered NaN comparisons.
    {UpdateFixture f;f.begin();auto* hero=member_hero(&f.world);auto& h=field<ContactPosition>(*hero,0);h.x=100;h.z=100;
     auto& a=f.spawn(1);auto& b=f.spawn(2);a.position.x=3;a.position.z=4;a.position.y=10000;b.position.x=-3;b.position.z=-4;
     f.roster.members->append(&a.entity);f.roster.members->append(&b.entity);a.hp.value=0;
     auto& e=create_system_enemy(f.enemy_system,{0,0,1,0,{}},f);auto& link=field<EnemyRecordLink>(e,24);
     check(link.opponent==&a.entity);link.opponent=nullptr;a.human.state=5;check(search_enemy_target(f.enemy_system,&e,f)==&b.entity);
     b.position.x=std::numeric_limits<float>::quiet_NaN();h.x=std::numeric_limits<float>::infinity();
     check(!search_enemy_target(f.enemy_system,&e,f));}
    // A boss selects the global hero directly even when dead or outside the roster.
    {UpdateFixture f;f.begin();auto* hero=member_hero(&f.world);field<HumanRecordLink>(*hero,14).state=5;f.roster.members->clear();
     auto& e=create_system_enemy(f.enemy_system,{0,2,1,1,{}},f);check(field<EnemyRecordLink>(e,24).opponent==hero);}
    // The boss ID is captured before world queries; level is re-read after map coordinate callbacks.
    {UpdateFixture f;f.begin();auto q=f.enemy_system.quest;f.on_enemy_world=[&]{q->boss_id=0;};
     f.on_map_query=[&](int){q->boss_level=9;};auto& e=create_boss_enemy(f.enemy_system,f);
     check(field<EnemyRecordLink>(e,24).enemy_id==2&&field<EnemyRecordLink>(e,24).level==9);}
    // Full Update now reaches actual boss and additional-enemy creation, then the real fence.
    {UpdateFixture f;f.begin();f.state.state=1;field<ContactPosition>(*member_hero(&f.world),0).z=12;
     update_stage(f.state,f);check(f.state.boss_started&&f.made_boss&&f.made_boss->contains(24)&&f.state.boss_fence);
     check(field<EnemyRecordLink>(*f.made_boss,24).flags==1&&field<ProjectileModification>(*f.made_boss,15).max_frame==30);
     auto& group=f.groups.get(std::make_shared<ComponentGroupFilter>(EventIntegers{24}));check(group.size()==2);}
}catch(const std::exception& e){std::cerr<<"uncaught enemy creation error: "<<e.what()<<'\n';return 1;}
