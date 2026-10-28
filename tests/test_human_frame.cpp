#include "human_frame_fixture.hpp"
#include <cmath>

int main()try{
    // Concrete Init uses the supplied world, stores nullable results and does not reset counters.
    {UpdateFixture f;FrameServices host(f);HumanWorld world(f,host);host.system.loop_counter=9;f.no_world=true;world.init();
     check(world.calls==std::vector<std::string>{"subset","map","stage","enemies","members","subscribe23","subscribe29"});
     check(host.system.map==f.active_map&&host.system.stage==&f.state&&host.system.enemies==&f.enemy_system&&host.system.members==&f.roster&&host.system.loop_counter==9);
     fails([&]{initialize_humans(host.system,nullptr,world.services);});world.null_results=true;world.init();
     check(!host.system.subset&&!host.system.map&&!host.system.stage&&!host.system.enemies&&!host.system.members);}
    for(const auto& stop:std::vector<std::string>{"subset","map","stage","enemies","members","subscribe23","subscribe29"}){
     UpdateFixture f;FrameServices host(f);HumanWorld world(f,host);world.fail=stop;fails([&]{world.init();});
     if(stop=="subset")check(!host.system.subset);if(stop=="map")check(host.system.subset&&!host.system.map);
     if(stop=="stage")check(host.system.map&&!host.system.stage);if(stop=="enemies")check(host.system.stage&&!host.system.enemies);
     if(stop=="members")check(host.system.enemies&&!host.system.members);
     auto& e=f.spawn(1).entity;host.events.send(23,&e);check(host.guards==(stop=="subscribe29"?1:0));}
    {UpdateFixture f;FrameServices host(f);HumanWorld world(f,host);std::array<EventHashes,2> hashes{{{4,104},{5,105}}};world.init(hashes);world.init();
     auto& e=f.spawn(1).entity;host.events.send(23,&e);check(host.guards==2&&host.events.unsubscribe_single(23,4)&&host.events.unsubscribe_pair(29,5));
     host.events.send(23,&e);check(host.guards==3);}
    {HumanFixture t;check(stage_attack_active(t.f.state));t.no_attack();check(!stage_attack_active(t.f.state));
     t.f.buttons[0].state=1;t.f.state.attack_buttons->at(1)=nullptr;check(stage_attack_active(t.f.state));
     t.f.buttons[0].state=0;fails([&]{stage_attack_active(t.f.state);});
     t.f.state.attack_buttons.reset();fails([&]{stage_attack_active(t.f.state);});}
    // Targeting uses XYZ, inclusive wrapped range, live slot order, state/HP guards and first ties.
    {HumanFixture t;ContactPosition origin;origin.x=100;origin.z=100;
     auto& a=t.enemy_at({100,100,100});auto& b=t.enemy_at({100,0,128});auto& c=t.enemy_at({100,0,72});
     check(nearest_attack_enemy(t.f.enemy_system,&origin,1)==&b);
     field<EnemyRecordLink>(b,24).state=4;check(nearest_attack_enemy(t.f.enemy_system,&origin,-1)==&c);
     resolve_combat_parameter(c,0)->value=0;check(!nearest_attack_enemy(t.f.enemy_system,&origin,1));
     field<ContactPosition>(a,0).y=std::numeric_limits<float>::quiet_NaN();check(!nearest_attack_enemy(t.f.enemy_system,&origin,100));
     field<ContactPosition>(a,0).y=28;check(nearest_attack_enemy(t.f.enemy_system,&origin,std::numeric_limits<std::int32_t>::max())==&a);}
    {HumanFixture t;check(!nearest_attack_enemy(t.f.enemy_system,nullptr,1));auto& e=t.enemy_at();field<EnemyRecordLink>(e,24).state=4;
     check(!nearest_attack_enemy(t.f.enemy_system,nullptr,1));field<EnemyRecordLink>(e,24).state=0;fails([&]{nearest_attack_enemy(t.f.enemy_system,nullptr,1);});}
    {HumanFixture t;t.flat_parameters();auto& h=t.hero();auto& record=t.host.gear.at(3).value();record.distance=20;record.burst_count=20;record.use_mp=8;
     check(human_attack_distance(t.host.system,&h,t.host)==12&&human_burst_count(t.host.system,&h,t.host)==10);
     auto& mp=*resolve_combat_parameter(h,1);mp.maximum=5;mp.value=5;check(human_has_mp(t.host.system,&h,0,t.host));
     mp.value=4;check(!human_has_mp(t.host.system,&h,0,t.host));mp.maximum=0;mp.value=0;check(human_has_mp(t.host.system,&h,0,t.host));
     mp.maximum=5;mp.extra_maximum=5;mp.value=4;mp.extra=4;check(human_has_mp(t.host.system,&h,0,t.host));
     record.distance=-3;record.burst_count=-2;check(human_attack_distance(t.host.system,&h,t.host)==-3&&human_burst_count(t.host.system,&h,t.host)==-2);}
    {HumanFixture t;auto& h=t.hero();t.f.state.attack_attribute=2;check(human_attack_attribute(t.host.system,&*t.host.characters[0],t.host)==2);
     t.host.characters[1]->weapon_id=7;t.host.gear[7]->attribute=1;auto& sub=t.sub();field<EquipmentRecordLinks>(sub,8).ids->at(0)=3;
     t.host.gear[3]->attribute=0;check(human_attack_attribute(t.host.system,&*t.host.characters[1],t.host)==1);
     t.host.parameters=[](ComponentSlots&,std::int32_t key){return key==12?120:0;};t.host.gear[3]->distance=2;t.host.gear[3]->burst_count=1;t.host.gear[3]->use_mp=3;
     check(human_mp_cost(t.host.system,&h,2,t.host)==33&&human_attack_distance(t.host.system,&h,t.host)==6&&human_burst_count(t.host.system,&h,t.host)==7);}
    {HumanFixture t;auto& h=t.hero();auto& mp=*resolve_combat_parameter(h,1);mp.value=30;t.host.gear[3]->use_mp=3;
     t.host.parameters=[](ComponentSlots&,std::int32_t key){return key==10?120:0;};
     check(human_mp_cost(t.host.system,&h,0,t.host)==33);use_human_mp(&h,t.host);check(mp.value==27);}
    {HumanFixture t;t.flat_parameters(100);auto& h=t.hero();check(human_move_speed(t.host.system,&h,t.host)==2.23F);
     check(human_attack_speed(t.host.system,&h,t.host)==100&&human_attack_interval(t.host.system,&h,t.host)==60&&human_burst_wait(t.host.system,&h,t.host)==6);
     t.flat_parameters(-100);check(human_attack_interval(t.host.system,&h,t.host)==2&&human_burst_wait(t.host.system,&h,t.host)==2);
     t.flat_parameters(std::numeric_limits<std::int32_t>::max());check(human_move_speed(t.host.system,&h,t.host)<0);}
    // Rejection does not clear the actor's earlier target or speed, and does not try the next candidate.
    for(auto kind:{0,1,2}){HumanFixture t;auto& h=t.hero();auto& a=t.enemy_at();t.enemy_at({100,0,130});
     auto& human=field<HumanRecordLink>(h,14);human.opponent=&a;field<ProjectileSpeed>(h,1).x=9;
     if(kind==0)field<EnemyRecordLink>(a,24).state=5;
     if(kind==1)t.f.enemy_pools.records->at(2)->flags=1;
     if(kind==2)t.f.node(a).lifetime.destroyed=true;
     check(!search_human_attack(t.host.system,&h,t.host)&&human.opponent==&a&&field<ProjectileSpeed>(h,1).x==9);}
    {HumanFixture t;t.no_attack();check(!search_human_attack(t.host.system,nullptr,t.host));
     t.f.buttons[0].state=1;fails([&]{search_human_attack(t.host.system,nullptr,t.host);});}
    // The complete waiting -> acquisition -> state-2 path uses real catalog/parameter and state kernels.
    {HumanFixture t;auto& target=t.enemy_at();auto& h=t.hero();auto& v=field<ProjectileSpeed>(h,1);v.x=3;v.y=4;v.z=5;
     t.update();auto& link=field<HumanRecordLink>(h,14);check(link.state==2&&link.opponent==&target&&link.frame==1&&link.attack_frame==1);
     check(v.x==0&&v.y==0&&v.z==0&&t.host.system.loop_counter==1&&t.host.shots==0);}
    // Human -> Enemy -> Move follows factory order: holding movement delays attack; release acquires the live enemy.
    {HumanFixture t;t.flat_parameters(100);t.f.geometry.step_x=312;t.f.geometry.step_z=312;auto& h=t.hero();
     auto& p=field<ContactPosition>(h,0);p.x=312;p.z=1164;field<HumanRecordLink>(h,14).state=1;field<ProjectileSpeed>(h,1).z=-1;
     auto& target=t.enemy_at({312,0,1140});change_enemy_state(target,1);auto* move=t.enemy_world.subset({0,1});bool enabled=true;
     GameplayUpdate human_system(4,enabled,[&](std::int32_t){t.update();});
     GameplayUpdate enemy_system(5,enabled,[&](std::int32_t){update_enemies(t.f.enemy_system,t.f);});
     GameplayUpdate movement(11,enabled,[&](std::int32_t){move_subset(*move);});std::vector<UpdateSystem*> list{&human_system,&enemy_system,&movement};
     UpdateSchedule schedule{&list,nullptr};update_systems(schedule,1);check(p.z<1164&&field<HumanRecordLink>(h,14).state==1);
     field<ProjectileSpeed>(h,1).z=0;update_systems(schedule,1);
     check(field<HumanRecordLink>(h,14).state==2&&field<HumanRecordLink>(h,14).opponent==&target&&t.host.shots==0);}
    // Both actual Init methods deliver Human contact before Enemy contact, then the two frame passes recover.
    {HumanFixture t;t.flat_parameters(100);auto& h=t.hero();auto& target=t.enemy_at({40,0,50});field<HumanRecordLink>(h,14).invincible_time=100;
     t.host.calls.clear();t.host.events.send(29,&h,&target);check(t.host.calls.front()=="human29");
     check(field<HumanRecordLink>(h,14).state==3&&field<EnemyRecordLink>(target,24).state==3);
     auto* move=t.enemy_world.subset({0,1});for(int i=0;i<21;++i){t.update();update_enemies(t.f.enemy_system,t.f);move_subset(*move);}
     check(field<HumanRecordLink>(h,14).state==0&&field<EnemyRecordLink>(target,24).state==1&&t.f.random.index==0);}
    // MoveBase preserves existing velocities on arrival/teleport. Move's nonpositive speed selects the stat default.
    {HumanFixture t;t.flat_parameters(100);auto& h=t.hero();auto& p=field<ContactPosition>(h,0);auto& v=field<ProjectileSpeed>(h,1);
     p.x=0;p.z=0;p.y=7;v.x=9;v.y=8;v.z=7;
     check(!move_human_base(t.host.system,&h,3,4,1,t.host)&&v.x==0.6F&&v.z==0.8F&&v.y==8&&p.x==0&&p.y==7);
     check(move_human_base(t.host.system,&h,3,4,5,t.host)&&p.x==3&&p.z==4&&v.x==0.6F&&v.z==0.8F);
     check(move_human_base(t.host.system,&h,100,200,-1,t.host)&&p.x==100&&p.z==200);
     check(!move_human(t.host.system,&h,200,200,-1,t.host)&&p.x==100&&v.x==2.23F);
     check(!move_human_base(t.host.system,&h,200,200,std::numeric_limits<float>::quiet_NaN(),t.host)&&std::isnan(v.x)&&std::isnan(v.z));}
    // Positive travel is reduced by 0.7 after dash; negative travel is not. X/Y speed survive.
    for(auto direction:{-1.0F,1.0F}){HumanFixture t;t.flat_parameters(100);t.no_attack();auto& h=t.hero();auto& v=field<ProjectileSpeed>(h,1);
     auto& link=field<HumanRecordLink>(h,14);link.state=1;link.dash_speed=2;v.x=3;v.y=4;v.z=direction;t.host.system.loop_counter=1;
     update_human_walking(t.host.system,&h,t.host);const float amount=2.23F+2.0F;
     check(v.z==(direction>0?amount*0.7F:-amount)&&v.x==3&&v.y==4&&(field<EntitySprite>(h,2).animation.clip&~3)==8);}
    {HumanFixture t;t.no_attack();t.flat_parameters(100);auto& h=t.hero();field<HumanRecordLink>(h,14).state=1;
     field<ProjectileSpeed>(h,1).z=std::numeric_limits<float>::quiet_NaN();t.host.system.loop_counter=1;
     fails([&]{update_human_walking(t.host.system,&h,t.host);});check(t.f.random.index==0);}
    // Sub characters start walking toward formation; far formation adjustment updates only Z velocity.
    {HumanFixture t;t.no_attack();t.flat_parameters(100);auto& sub=t.sub();auto& p=field<ContactPosition>(sub,0);p.x=54;p.z=90;
     update_human_waiting(t.host.system,&sub,t.host);check(field<HumanRecordLink>(sub,14).state==1);
     auto& v=field<ProjectileSpeed>(sub,1);v.x=7;t.host.system.loop_counter=1;update_human_walking(t.host.system,&sub,t.host);
     check(p.x==54&&p.z==90&&v.x==7&&v.z<0);
     p.x=64;p.z=84;update_human_walking(t.host.system,&sub,t.host);check(field<HumanRecordLink>(sub,14).state==0&&v.z==0&&v.x==7);}
    {HumanFixture t;t.no_attack();t.flat_parameters(100);auto& sub=t.sub();field<HumanRecordLink>(sub,14).state=1;
     field<ProjectileSpeed>(t.hero(),1).z=1;field<ContactPosition>(sub,0).z=157;t.host.system.loop_counter=1;
     update_human_walking(t.host.system,&sub,t.host);check(field<ProjectileSpeed>(sub,1).z==0);
     field<ProjectileSpeed>(t.hero(),1).z=-1;update_human_walking(t.host.system,&sub,t.host);check(field<ProjectileSpeed>(sub,1).z==-2.23F);}
    // Walking recovery consumes exactly one inclusive 3..5 draw and uses total maxima but base-maximum clamping.
    {HumanFixture t;t.no_attack();t.flat_parameters();auto& h=t.hero();field<HumanRecordLink>(h,14).state=1;
     auto& hp=*resolve_combat_parameter(h,0);auto& mp=*resolve_combat_parameter(h,1);hp.value=50;hp.maximum=100;hp.extra_maximum=200;
     mp.value=10;mp.maximum=30;mp.extra_maximum=100;t.f.random.values={0};update_human_walking(t.host.system,&h,t.host);
     check(hp.value==59&&mp.value==13&&t.f.random.index==1);
     t.host.gear[3]->attribute=0;t.host.system.loop_counter=30;update_human_walking(t.host.system,&h,t.host);check(t.f.random.index==1);
     t.host.gear[3]->attribute=3;update_human_walking(t.host.system,&h,t.host);check(t.f.random.index==2);}
    {DamageParameter p{90,100,0,1000};check(add_human_parameter(p,20)==10&&p.value==100);
     p.maximum=0;check(add_human_parameter(p,-200)==0&&p.value==-100);
     p.value=std::numeric_limits<std::int32_t>::max();check(add_human_parameter(p,1)==0&&p.value==std::numeric_limits<std::int32_t>::min());}
    // Burst orchestration requires an explicit Shoot producer; tests supply only its observed result/MP side effect.
    {HumanFixture t;t.flat_parameters(100);t.f.fps=10;auto& h=t.hero();auto& target=t.enemy_at();auto& link=field<HumanRecordLink>(h,14);
     t.host.parameters=[](ComponentSlots&,std::int32_t key){return key>=10?0:100;};t.host.gear[3]->burst_count=2;
     link.state=2;link.opponent=&target;link.attack_frame=10;t.host.shooter=[](ComponentSlots&,ComponentSlots&){return true;};
     field<EntitySprite>(h,2).animation.frame=7;t.update();check(t.host.shots==1&&link.state==2&&link.attack_frame==11&&field<EntitySprite>(h,2).animation.frame==0);
     t.update();check(t.host.shots==1&&link.attack_frame==12);t.update();check(t.host.shots==2&&link.state==0&&!link.opponent&&link.attack_frame==1&&link.frame==1);}
    {HumanFixture t;t.flat_parameters(100);auto& h=t.hero();auto& target=t.enemy_at();auto& link=field<HumanRecordLink>(h,14);
     link.state=2;link.opponent=&target;link.attack_frame=human_attack_interval(t.host.system,&h,t.host);
     const auto count=link.attack_frame;fails([&]{t.update();});check(t.host.shots==1&&link.attack_frame==count&&link.frame==0&&t.host.system.loop_counter==0);}
    {HumanFixture t;t.flat_parameters(100);t.f.fps=10;auto& h=t.hero();auto& target=t.enemy_at();auto& link=field<HumanRecordLink>(h,14);
     link.state=2;link.opponent=&target;link.attack_frame=10;
     t.host.shooter=[&](ComponentSlots& actor,ComponentSlots&){resolve_combat_parameter(actor,1)->value=0;return false;};
     t.update();check(t.host.shots==1&&link.state==0&&!link.opponent&&link.attack_frame==1);}
    // Captured Human writes survive replacement by the required producer.
    {HumanFixture t;t.flat_parameters(100);t.f.fps=10;auto& h=t.hero();auto& target=t.enemy_at();auto& old=field<HumanRecordLink>(h,14);
     old.state=2;old.opponent=&target;old.attack_frame=10;old.frame=6;t.host.gear[3]->burst_count=2;
     t.host.parameters=[](ComponentSlots&,std::int32_t key){return key>=10?0:100;};HumanRecordLink replacement;replacement.character_id=0;replacement.state=4;
     t.host.shooter=[&](ComponentSlots& e,ComponentSlots&){e.change(14,&replacement);return true;};t.update();
     check(old.state==2&&old.attack_frame==11&&old.frame==7&&replacement.frame==0&&replacement.attack_frame==0);}
    {HumanFixture t;auto& h=t.hero();field<HumanRecordLink>(h,14).state=4;auto& p=field<ContactPosition>(h,0);p.z=1000;
     t.update();check(p.z==192);p.z=-1000;t.update();check(p.z==16);}
    {HumanFixture t;auto& h=t.hero();auto& link=field<HumanRecordLink>(h,14);link.state=4;link.invincible_time=1;
     AnimationModificationRequest r;r.type=3;add_animation_modification(h,r,t.f);auto* old=h.get(15);t.update();
     check(link.invincible_time==0&&!h.contains(15)&&t.f.returned_modifiers.back()==old);
     add_animation_modification(h,r,t.f);link.invincible_time=std::numeric_limits<std::int32_t>::min();t.update();check(link.invincible_time==std::numeric_limits<std::int32_t>::min()&&h.contains(15));}
    {HumanFixture t;auto& h=t.hero();auto& link=field<HumanRecordLink>(h,14);link.state=4;link.invincible_time=1;
     AnimationModificationRequest r;r.type=3;add_animation_modification(h,r,t.f);
     t.f.on_release_modifier=[](){throw std::runtime_error("pool return");};fails([&]{t.update();});
     check(link.invincible_time==0&&!h.contains(15)&&link.frame==0&&t.host.system.loop_counter==0);}
    {HumanFixture t;auto& h=t.hero();auto& link=field<HumanRecordLink>(h,14);link.state=5;link.frame=std::numeric_limits<std::int32_t>::max();
     link.attack_frame=std::numeric_limits<std::int32_t>::max();t.host.system.loop_counter=std::numeric_limits<std::int32_t>::max();
     t.update();check(link.frame==-1&&link.attack_frame==std::numeric_limits<std::int32_t>::min()&&t.host.system.loop_counter==-1);}
    {HumanFixture t;t.flat_parameters(100);auto& h=t.hero();auto& link=field<HumanRecordLink>(h,14);link.state=3;link.dash_speed=2;
     field<ProjectileSpeed>(h,1).z=0.12F;field<ContactDirection>(h,12).locked=true;t.update();
     check(link.state==0&&link.frame==1&&link.dash_speed==0&&!field<ContactDirection>(h,12).locked);}
    {HumanFixture t;auto& h=t.hero();auto& v=field<ProjectileSpeed>(h,1);v.x=3;v.y=4;v.z=5;t.f.random.values={3};
     update_human_celebration(&h,t.host);check(v.x==3&&v.y==4&&v.z==0&&field<ContactDirection>(h,12).direction==3&&field<EntitySprite>(h,2).animation.clip==3&&t.f.random.index==1);
     field<HumanRecordLink>(h,14).frame=1;update_human_celebration(&h,t.host);check(t.f.random.index==1);
     h.remove(12);field<HumanRecordLink>(h,14).frame=0;fails([&]{update_human_celebration(&h,t.host);});check(t.f.random.index==2);}
    // Structural mutation completes this actor's ordered writes before invalidating the live group cursor.
    {HumanFixture t;t.flat_parameters(100);t.no_attack();auto& h=t.hero();field<HumanRecordLink>(h,14).state=4;bool added=false;
     t.host.on_fps=[&]{if(!added){added=true;t.sub();}};fails([&]{t.update();});
     check(field<HumanRecordLink>(h,14).frame==1&&t.host.system.loop_counter==0);}
}catch(const std::exception& e){std::cerr<<"uncaught human frame error: "<<e.what()<<'\n';return 1;}
