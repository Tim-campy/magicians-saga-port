#include "stage_update_fixture.hpp"
#include "core/ecs/update_scheduler.hpp"
#include "core/gameplay/motion_pass.hpp"
#include <cmath>

template<class T>T& field(ComponentSlots& e,std::int32_t slot){auto* p=dynamic_cast<T*>(e.get(slot));check(p);return *p;}
struct FrameFixture:UpdateFixture {
    FrameFixture(){begin();reset_enemy_population(enemy_system,*this);enemy_system.remaining_enemies=0;random.index=0;
        // The short synthetic map uses an explicit hero position inside its gameplay bounds.
        field<ContactPosition>(*member_hero(&world),0).z=84;}
    ComponentSlots& make(std::int32_t flags=0,ProjectilePoint p={40,0,50}){
        return create_system_enemy(enemy_system,{2,2,1,flags,p},*this);
    }
};
int main()try{
    // GetZ runs toward decreasing Z. Progress is its inverse on this exact grid.
    {FrameFixture f;auto& coordinates=f.enemy_coordinates();
     check(stage_progress_rate(f.state,static_cast<float>(stage_z(f.state,0,coordinates)),coordinates)==0);
     check(stage_progress_rate(f.state,static_cast<float>(stage_z(f.state,50,coordinates)),coordinates)==50);
     check(stage_progress_rate(f.state,static_cast<float>(stage_z(f.state,100,coordinates)),coordinates)==100);}
    {EnemyRecordLink e;check(enemy_is_normal(e)&&!enemy_waits_for_players(e)&&enemy_is_alive(e)&&!enemy_is_dead(e));
     for(auto flags:{1,2,3,5,6}){e.flags=flags;check(!enemy_is_normal(e)&&enemy_waits_for_players(e));}
     e.flags=4;check(enemy_is_normal(e));for(auto state:{-1,0,1,2,3,5,6}){e.state=state;check(enemy_is_alive(e)&&!enemy_is_dead(e));}
     e.state=4;check(enemy_is_dead(e)&&!enemy_is_alive(e));}
    {check(within_enemy_activation_circle({0,99999,0},{150,-99999,200},250));
     check(within_enemy_activation_circle({0,0,0},{150,0,200},-250));
     check(!within_enemy_activation_circle({0,0,0},{251,0,0},250));
     check(!within_enemy_activation_circle({0,0,0},{std::numeric_limits<float>::quiet_NaN(),0,0},250));}
    // Real walking helper followed by the actual live movement pass, in factory order 5 -> 11.
    {FrameFixture f;auto& e=f.make();change_enemy_state(e,1);bool enabled=true;
     auto& group=f.groups.get(std::make_shared<ComponentGroupFilter>(EventIntegers{0,1}));
     GameplayUpdate enemies(5,enabled,[&](std::int32_t){update_enemies(f.enemy_system,f);});
     GameplayUpdate motion(11,enabled,[&](std::int32_t){move_subset(group);});
     std::vector<UpdateSystem*> list{&enemies,&motion};UpdateSchedule schedule{&list,nullptr};
     const auto amount=enemy_move_speed(*f.enemy_pools.records->at(2),1,f.enemy_pools.maxima);
     update_systems(schedule,1);auto& p=field<ContactPosition>(e,0);auto& v=field<ProjectileSpeed>(e,1);
     check(v.x==2&&v.z==amount&&p.x==42&&p.z==50.0F+amount&&field<EnemyRecordLink>(e,24).frame==1&&!e.contains(15));
     enabled=false;update_systems(schedule,1);check(p.x==42&&field<EnemyRecordLink>(e,24).frame==1);}
    {FrameFixture f;auto& e=f.make();auto* hero=member_hero(&f.world);auto& h=field<ContactPosition>(*hero,0);
     auto& p=field<ContactPosition>(e,0);auto& v=field<ProjectileSpeed>(e,1);h.z=200;p.z=50;
     update_enemy_walking(f.enemy_system,&e,f);check(v.x==0);h.z=199;update_enemy_walking(f.enemy_system,&e,f);check(v.x==2);
     field<HumanRecordLink>(*hero,14).state=5;update_enemy_walking(f.enemy_system,&e,f);check(v.x==2);
     f.node(*hero).lifetime.destroyed=true;update_enemy_walking(f.enemy_system,&e,f);check(v.x==0);
     f.node(*hero).lifetime.destroyed=false;h.z=std::numeric_limits<float>::quiet_NaN();update_enemy_walking(f.enemy_system,&e,f);check(v.x==0);}
    {FrameFixture f;auto& e=f.make(1,{40,0,95});auto& p=field<ContactPosition>(e,0);
     auto* hero=member_hero(&f.world);check(field<ContactPosition>(*hero,0).z==84);
     update_enemy_walking(f.enemy_system,&e,f);check(p.z==84&&field<ProjectileSpeed>(e,1).z>0);
     p.z=94;update_enemy_walking(f.enemy_system,&e,f);check(p.z==94);}
    // The HP blink uses the pre-increment frame; full healing does not reset stale colour.
    {FrameFixture f;auto& e=f.make();auto& hp=*resolve_combat_parameter(e,0);hp.value=50;hp.maximum=100;
     auto& link=field<EnemyRecordLink>(e,24);auto& color=field<DefeatColor>(e,6);
     update_enemies(f.enemy_system,f);check(color.values==std::array<std::int32_t,4>{255,0,0,255}&&link.frame==1);
     update_enemies(f.enemy_system,f);check(color.values==std::array<std::int32_t,4>{255,255,255,255}&&link.frame==2);
     link.frame=52;update_enemies(f.enemy_system,f);check(color.values[1]==0);
     hp.value=100;update_enemies(f.enemy_system,f);check(color.values[1]==0);
     hp.value=0;link.frame=1;update_enemies(f.enemy_system,f);check(color.values[1]==0&&link.frame==2);
     hp.value=std::numeric_limits<std::int32_t>::max();hp.extra=1;update_enemies(f.enemy_system,f);check(color.values[1]==0);}
    // Native inert states still advance the same signed wrapping counter.
    {FrameFixture f;auto& e=f.make();auto& link=field<EnemyRecordLink>(e,24);auto& v=field<ProjectileSpeed>(e,1);v.z=7;
     for(auto state:{0,2,4,7}){link.state=state;link.frame=std::numeric_limits<std::int32_t>::max()-1;
         update_enemies(f.enemy_system,f);check(link.frame==0&&v.z==7&&link.state==state);}
     link.frame=std::numeric_limits<std::int32_t>::max();update_enemies(f.enemy_system,f);check(link.frame==-1);}
    // Recovery keeps lateral/vertical speed, unlocks direction and skips RNG without RUN_AWAY.
    {FrameFixture f;auto& e=f.make();change_enemy_state(e,3);auto& v=field<ProjectileSpeed>(e,1);v.x=3;v.y=4;v.z=0.12F;
     field<ContactDirection>(e,12).locked=true;update_enemies(f.enemy_system,f);
     check(field<EnemyRecordLink>(e,24).state==1&&field<EnemyRecordLink>(e,24).frame==1);
     check(v.x==3&&v.y==4&&v.z==0&&!field<ContactDirection>(e,12).locked&&f.random.index==0);}
    {FrameFixture f;auto& e=f.make();change_enemy_state(e,3);auto& v=field<ProjectileSpeed>(e,1);
     for(auto z:{0.125F,std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){
         v.z=z;field<ContactDirection>(e,12).locked=true;update_enemy_bounding(f.enemy_system,&e,f);
         check(field<EnemyRecordLink>(e,24).state==3&&field<ContactDirection>(e,12).locked);}}
    {FrameFixture f;auto& e=f.make(1);change_enemy_state(e,3);field<ProjectileSpeed>(e,1).z=0;
     f.enemy_system.stage=nullptr;update_enemy_bounding(f.enemy_system,&e,f);
     check(field<EnemyRecordLink>(e,24).state==1&&f.random.index==0&&!e.contains(22));}
    // Exact 20% threshold and live FPS, followed by the actual run-away helper.
    for(auto draw:{19,20}){FrameFixture f;auto& e=f.make();change_enemy_state(e,3);field<ProjectileSpeed>(e,1).z=0;
     f.enemy_pools.records->at(2)->flags=2;f.random.values={draw};f.fps=30;f.enemy_system.remaining_enemies=4;
     update_enemy_bounding(f.enemy_system,&e,f);check(f.random.index==1);
     if(draw==19){check(field<EnemyRecordLink>(e,24).state==5&&field<GarbageLifetime>(e,22).frames==90&&f.enemy_system.remaining_enemies==3);
         update_enemy_run_away(&e);check(field<ProjectileSpeed>(e,1).z==-10);}
     else check(field<EnemyRecordLink>(e,24).state==1&&!e.contains(22)&&f.enemy_system.remaining_enemies==4);}
    {FrameFixture f;auto& e=f.make();change_enemy_state(e,3);field<ProjectileSpeed>(e,1).z=0;
     f.state.collection.boss_defeated=true;add_garbage_lifetime(e,7,static_cast<EnemyFrameHost&>(f));auto* old=e.get(22);
     f.on_garbage_release=[&]{check(!e.contains(22)&&f.returned_garbage.back()==old);};
     update_enemy_bounding(f.enemy_system,&e,f);check(f.returned_garbage.size()==1&&e.get(22)!=old);
     check(field<GarbageLifetime>(e,22).frames==180&&f.enemy_system.remaining_enemies==1&&f.random.index==0);}
    {FrameFixture f;auto& e=f.make();change_enemy_state(e,3);field<ProjectileSpeed>(e,1).z=0;
     f.state.collection.boss_defeated=true;f.fps=std::numeric_limits<std::int32_t>::max();
     update_enemies(f.enemy_system,f);check(field<GarbageLifetime>(e,22).frames==2147483645&&field<EnemyRecordLink>(e,24).frame==1);
     update_enemies(f.enemy_system,f);check(field<ProjectileSpeed>(e,1).z==-10&&field<EnemyRecordLink>(e,24).frame==2);}
    {FrameFixture f;auto& e=f.make();change_enemy_state(e,3);field<ProjectileSpeed>(e,1).z=0;
     f.state.collection.boss_defeated=true;add_garbage_lifetime(e,7,static_cast<EnemyFrameHost&>(f));
     f.on_garbage_release=[](){throw std::runtime_error("pool return");};
     fails([&]{update_enemy_bounding(f.enemy_system,&e,f);});
     check(!e.contains(22)&&field<EnemyRecordLink>(e,24).state==3&&field<ProjectileSpeed>(e,1).z==0&&f.enemy_system.remaining_enemies==0);}
    {FrameFixture f;auto& e=f.make();change_enemy_state(e,3);field<ProjectileSpeed>(e,1).z=0;
     f.state.collection.boss_defeated=true;add_garbage_lifetime(e,7,static_cast<EnemyFrameHost&>(f));
     f.on_enemy_fps=[](){throw std::runtime_error("FPS unavailable");};
     fails([&]{update_enemy_bounding(f.enemy_system,&e,f);});
     check(f.returned_garbage.size()==1&&!e.contains(22)&&field<EnemyRecordLink>(e,24).state==3&&f.enemy_system.remaining_enemies==0);}
    // The outer pass keeps its captured Enemy component even if a callback replaces slot 24.
    {FrameFixture f;auto& e=f.make();auto& old=field<EnemyRecordLink>(e,24);old.state=3;old.frame=6;
     field<ProjectileSpeed>(e,1).z=0;EnemyRecordLink replacement;replacement.enemy_id=2;
     f.state.collection.boss_defeated=true;f.on_enemy_fps=[&]{e.change(24,&replacement);f.enemy_system.remaining_enemies=9;};
     update_enemies(f.enemy_system,f);check(old.state==3&&old.frame==7&&replacement.state==5&&replacement.frame==0&&f.enemy_system.remaining_enemies==8);}
    // The HP capture also survives replacement of the entity's parameter set during recovery.
    {FrameFixture f;auto& e=f.make(0,{0,0,97});change_enemy_state(e,3);field<ProjectileSpeed>(e,1).z=0;
     f.state.collection.boss_defeated=true;CombatParameterLinks replacement;DamageParameter dead{0,100,0,0};
     DamageParameterMap values{{0,&dead}};replacement.values=&values;
     f.on_enemy_fps=[&]{e.change(23,&replacement);};update_enemies(f.enemy_system,f);
     check(resolve_combat_parameter(e,0)==&dead&&e.contains(15)&&field<ProjectileModification>(e,15).type==5);}
    // Off-map normals receive one fade/destroy modifier; they are not immediately destroyed.
    {FrameFixture f;auto& e=f.make(0,{0,0,97});update_enemies(f.enemy_system,f);
     auto* first=e.get(15);auto& m=field<ProjectileModification>(e,15);
     check(m.type==5&&m.destroy&&m.scale_x==1&&m.scale_y==1&&m.max_frame==0&&m.alpha==255&&!f.node(e).lifetime.destroyed);
     update_enemies(f.enemy_system,f);check(e.get(15)==first);}
    {FrameFixture f;auto& e=f.make(0,{0,0,96});update_enemies(f.enemy_system,f);check(!e.contains(15));
     field<ContactPosition>(e,0).z=97;resolve_combat_parameter(e,0)->value=0;update_enemies(f.enemy_system,f);check(!e.contains(15));
     resolve_combat_parameter(e,0)->value=100;field<ContactPosition>(e,0).z=std::numeric_limits<float>::quiet_NaN();
     update_enemies(f.enemy_system,f);check(!e.contains(15));}
    // Fixed activation uses an inclusive X/Z circle, no HP/alive gate, and resets frame after the outer increment.
    {FrameFixture f;auto& e=f.make(2,{350,100000,84});auto& link=field<EnemyRecordLink>(e,24);link.frame=17;link.state=4;
     resolve_combat_parameter(e,0)->value=0;update_enemies(f.enemy_system,f);
     check(link.flags==6&&link.state==1&&link.frame==0&&field<ProjectileSpeed>(e,1).z>0&&!e.contains(15));
     update_enemies(f.enemy_system,f);check(link.frame==1);}
    {FrameFixture f;auto& e=f.make(2,{351,0,84});update_enemies(f.enemy_system,f);auto& link=field<EnemyRecordLink>(e,24);
     check(link.flags==2&&link.state==0&&link.frame==1&&field<ProjectileSpeed>(e,1).z==0);}
    // Newly spawned normals enter the same frame's live pass, with the corrected forward coordinate.
    {UpdateFixture f;f.begin();f.enemy_system.loop_counter=1;f.random.index=0;f.random.values={0,1};
     field<ContactPosition>(*member_hero(&f.world),0).z=84;
     update_enemies(f.enemy_system,f);auto& e=f.nodes.back().entity;
     check(f.enemy_system.created_enemies==1&&field<EnemyRecordLink>(e,24).frame==1&&field<EnemyRecordLink>(e,24).state==1);
     check(field<ContactPosition>(e,0).z==36&&!e.contains(15)&&f.random.index==2);}
    // Structural changes invalidate the cursor after the current entity's ordered writes.
    {FrameFixture f;auto& e=f.make();change_enemy_state(e,1);bool added=false;ComponentSlots* next=nullptr;
     f.enemy_pools.on_records=[&](std::size_t){if(!added){added=true;next=&f.make();}};
     fails([&]{update_enemies(f.enemy_system,f);});check(next&&field<EnemyRecordLink>(e,24).frame==1&&field<EnemyRecordLink>(*next,24).frame==0);}
}catch(const std::exception& e){std::cerr<<"uncaught enemy frame error: "<<e.what()<<'\n';return 1;}
