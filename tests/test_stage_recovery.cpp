#include "stage_update_fixture.hpp"

template<class T>T& part(ComponentSlots& e,std::int32_t slot){auto* p=dynamic_cast<T*>(e.get(slot));check(p);return *p;}
ComponentSlots& effect(UpdateFixture& f,std::int32_t clip){
    return create_effect({{0,0,0},0,0,20,clip,true,false,11,0,nullptr,-1,true},f.ids,f);
}
int main()try{
    {UpdateFixture f;f.roster.formation->at(0)=1;f.begin();auto& s=f.state;
     auto* hero=member_hero(&f.world);auto* sub=f.roster.sub_members->entities().at(0);
     auto& hh=part<HumanRecordLink>(*hero,14);auto& sh=part<HumanRecordLink>(*sub,14);
     part<ContactPosition>(*hero,0).x=999;part<ContactPosition>(*hero,0).y=30;part<ContactPosition>(*hero,0).z=777;
     auto& sub_position=part<ContactPosition>(*sub,0);sub_position.x=0;sub_position.y=0;sub_position.z=0;
     hh.state=4;hh.frame=77;hh.attack_frame=42;hh.opponent=sub;sh.state=5;sh.frame=88;
     part<ProjectileSpeed>(*hero,1).z=5;
     auto& hp=*resolve_combat_parameter(*hero,0);hp.value=1;hp.maximum=123;
     auto& energy=*resolve_combat_parameter(*hero,1);energy.value=9;energy.maximum=0;
     auto& old=effect(f,100);auto old_id=f.entity_id(old);auto& keep=effect(f,101);auto keep_id=f.entity_id(keep);
     s.state=1;s.frame=23;s.loop_counter=19;s.boss_started=true;s.collection.boss_defeated=true;s.total_damage=91;
     s.rewards()->experience=123;s.remaining_time=0;
     retry_stage(s,f);
     check(s.retried&&s.remaining_time==600&&s.state==1&&s.frame==23&&s.loop_counter==19&&s.total_damage==91);
     check(s.boss_started&&s.collection.boss_defeated&&s.rewards()->experience==123);
     check(f.node(old).lifetime.destroyed&&!f.registry.contains(static_cast<std::int32_t>(old_id))&&f.registry.contains(static_cast<std::int32_t>(keep_id)));
     check(hh.state==0&&hh.frame==77&&hh.attack_frame==42&&hh.opponent==sub&&sh.state==0&&sh.frame==88);
     check(part<ProjectileSpeed>(*hero,1).z==5&&hp.value==123&&energy.value==9);
     check(part<ContactPosition>(*hero,0).x==100&&part<ContactPosition>(*hero,0).y==0&&part<ContactPosition>(*hero,0).z==777);
     check(part<ContactPosition>(*sub,0).x==28&&part<ContactPosition>(*sub,0).y==0&&part<ContactPosition>(*sub,0).z==705);
     check(hh.invincible_time==180&&sh.invincible_time==180&&f.duration_queries==4);
     for(auto* e:{hero,sub}){auto& m=part<ProjectileModification>(*e,15);
         check(m.type==3&&m.max_frame==180&&m.scale_x==1&&m.scale_y==1&&m.alpha==255&&!m.destroy&&!m.loop);}
     check(f.trace.back()=="bgm");}
    // Predicate runs over the complete source before any entity destruction.
    {UpdateFixture f;f.begin();auto& first=effect(f,100);auto& invalid=effect(f,100);invalid.remove(2);
     auto* hero=member_hero(&f.world);part<ContactPosition>(*hero,0).x=999;
     fails([&]{retry_stage(f.state,f);});check(f.state.retried&&f.state.remaining_time==600);
     check(!f.node(first).lifetime.destroyed&&part<ContactPosition>(*hero,0).x==999&&f.duration_queries==0);}
    {UpdateFixture f;f.begin();f.missing_destroy_world=true;fails([&]{retry_stage(f.state,f);});
     check(f.state.retried&&f.state.remaining_time==600&&f.duration_queries==0);}
    // Each of the two static duration reads is live. Removal precedes pool return.
    {UpdateFixture f;f.begin();auto& hero=*member_hero(&f.world);auto& h=part<HumanRecordLink>(hero,14);
     hero.add(15,&f.node(hero).modification);auto* previous=hero.get(15);
     f.on_duration=[&](std::size_t n){f.duration=static_cast<std::int32_t>(59+n);};
     f.on_release_modifier=[&]{check(!hero.contains(15));};retry_stage(f.state,f);
     check(h.invincible_time==60&&part<ProjectileModification>(hero,15).max_frame==61&&f.returned_modifiers==std::vector<Component*>{previous});}
    {UpdateFixture f;f.begin();auto& hero=*member_hero(&f.world);hero.add(15,&f.node(hero).modification);
     f.on_release_modifier=[](){throw std::runtime_error("return pool");};
     fails([&]{retry_stage(f.state,f);});check(!hero.contains(15)&&part<HumanRecordLink>(hero,14).invincible_time==180&&f.duration_queries==1);
     check(f.trace.back()!="bgm");}
    // A mutation during the ordered member pass preserves its earlier effects, then fails at MoveNext.
    {UpdateFixture f;f.begin();auto& hero=*member_hero(&f.world);
     f.on_record=[&](HumanRecordLink&){f.roster.members->append(&hero);};
     fails([&]{retry_stage(f.state,f);});check(hero.contains(15)&&f.duration_queries==2&&f.trace.back()!="bgm");}
    // GetFormationX/Z resolve global hero twice, with separate binary32 subtract/add operations.
    {UpdateFixture f;f.begin();auto& hero=*member_hero(&f.world);auto& other=f.spawn(1);auto& target=f.spawn(2);
     PlayerWorldState second{&other.entity,nullptr};part<ContactPosition>(hero,0).x=100;part<ContactPosition>(hero,0).z=200;
     other.human.formation_x=20;other.human.formation_z=40;target.human.formation_x=3;target.human.formation_z=7;
     int queries=0;f.on_recovery_world=[&]{f.current_binding=(++queries%2)?&f.world:&second;};
     check(member_formation_x(&target.entity,f)==83&&member_formation_z(&target.entity,f)==167&&queries==4);}
    // The hero's column uses signed remainder, not a positive modulo.
    {UpdateFixture f;f.begin();auto& hero=*member_hero(&f.world);part<HumanRecordLink>(hero,14).formation_index=-1;
     retry_stage(f.state,f);check(part<ContactPosition>(hero,0).x==-8);}
    {UpdateFixture f;f.begin();f.fps=30;retry_stage(f.state,f);
     check(f.state.remaining_time==300&&part<HumanRecordLink>(*member_hero(&f.world),14).invincible_time==180);
     check(initial_human_invincible_time(std::numeric_limits<std::int32_t>::max())==2147483645);}
    // Retry is reached through the real Update crystal dialog branch.
    {UpdateFixture f;f.begin();auto& hero=*member_hero(&f.world);part<HumanRecordLink>(hero,14).state=5;
     f.state.state=1;f.state.remaining_time=0;f.dialog(98,0);f.affordable=true;f.crystals=150;
     update_stage(f.state,f);check(!f.state.dialog&&f.state.retried&&f.state.remaining_time==600&&f.crystals==50);
     check(part<HumanRecordLink>(hero,14).state==0&&hero.contains(15));}
    // Boss entry uses the actual ZObject factory and publishes its fence only after modification succeeds.
    {UpdateFixture f;f.begin();auto& s=f.state;start_stage_boss_battle(s,f);
     check(s.boss_started&&s.min_z_rate==75&&s.max_z_rate==95&&f.made_boss&&s.boss_fence);
     auto& boss=part<ProjectileModification>(*f.made_boss,15);auto& fence=*s.boss_fence;
     check(boss.type==0&&boss.max_frame==30&&boss.scale_x==1&&boss.scale_y==0);
     check(part<ContactPosition>(fence,0).x==140&&part<ContactPosition>(fence,0).z==168);
     check(part<CellComponent>(fence,5).xi==5&&part<CellComponent>(fence,5).yi==7);
     check(part<EntityDepth>(fence,4).extra==200&&part<EntitySprite>(fence,2).animation.clip==44);
     check(!fence.contains(7)&&!fence.contains(10)&&!fence.contains(26));
     check(part<ProjectileModification>(fence,15).max_frame==20&&part<ProjectileModification>(fence,15).scale_y==0);
     check(f.log==std::vector<std::string>({"boss","boss-bgm","boss-sound"}));
     const auto count=f.nodes.size();start_stage_boss_battle(s,f);check(f.nodes.size()==count&&f.log.size()==3);
     const auto id=f.entity_id(fence);cleanup_quest_stage(f);check(!s.boss_fence&&!f.registry.contains(static_cast<std::int32_t>(id)));}
    {UpdateFixture f;f.begin();f.missing_boss=true;fails([&]{start_stage_boss_battle(f.state,f);});
     check(f.state.boss_started&&f.state.min_z_rate==0&&!f.state.boss_fence);start_stage_boss_battle(f.state,f);check(f.log.size()==1);}
    {UpdateFixture f;f.begin();f.missing_object_world=true;fails([&]{start_stage_boss_battle(f.state,f);});
     check(f.state.boss_started&&f.state.min_z_rate==75&&f.made_boss->contains(15)&&!f.state.boss_fence);}
    {UpdateFixture f;f.begin();f.on_boss_bgm=[](){throw std::runtime_error("bgm");};
     fails([&]{start_stage_boss_battle(f.state,f);});check(f.state.boss_fence&&f.log.back()=="boss-bgm");}
    // The shared generated modifier accepts every payload field, without normalization.
    {UpdateFixture f;auto& e=create_entity(f.ids,0,false,static_cast<DropFactoryHost&>(f));
     AnimationModificationRequest r;r.type=-4;r.angle=77;r.frame=-3;r.max_frame=18;r.anchor=34;
     r.x=2;r.y=3;r.z=4;r.scale_x=-1;r.scale_y=0;r.destroy=true;r.loop=true;r.alpha=-9;
     add_animation_modification(e,r,f);auto& p=part<ProjectileModification>(e,15);
     check(p.type==-4&&p.angle==77&&p.frame==-3&&p.max_frame==18&&p.anchor==34&&p.alpha==-9);
     check(p.x==2&&p.y==3&&p.z==4&&p.scale_x==-1&&p.scale_y==0&&p.destroy&&p.loop);}
}catch(const std::exception& e){std::cerr<<"uncaught stage recovery error: "<<e.what()<<'\n';return 1;}
