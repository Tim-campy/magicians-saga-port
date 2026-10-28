#include "enemy_fixture.hpp"
#include <cmath>
#include <limits>

template<class T>T& field(ComponentSlots& e,std::int32_t slot){auto* p=dynamic_cast<T*>(e.get(slot));check(p);return *p;}
int main()try{
    {StartupFixture f;EnemyPoolFixture pools(f);std::vector<std::int32_t> order;
     f.on_added=[&](ComponentSlots&,std::int32_t slot){order.push_back(slot);};
     auto* e=create_enemy({2,2,4,0,{-25,8,49}},f.ids,pools);check(e);
     check(order==EventIntegers({0,5,4,7,2,1,24,20,23,6,32,12,10}));
     auto& n=f.node(*e);check(n.enemy.enemy_id==2&&n.enemy.level==4&&n.enemy.flags==0&&n.enemy.state==0&&n.enemy.frame==0&&!n.enemy.opponent);
     check(n.position.x==-25&&n.position.y==8&&n.position.z==49&&n.cell.xi==-1&&n.cell.yi==2&&n.depth.extra==10);
     check(n.sprite.animation.resource==3&&n.sprite.animation.clip==8&&n.sprite.animation.frame==0&&*n.sprite.layers==EventIntegers{-1});
     check(n.image.arrays.textures->at(0)==90&&n.image.arrays.resources->at(0)==3&&n.image.arrays.textures->size()==1);
     check(std::fabs(n.speed.z-2.3415F)<0.000001F&&n.speed.x==0&&n.speed.y==0);
     check(n.ai.state==0&&n.ai.scripts&&n.ai.scripts->array->empty()&&n.ai.scripts->count==0);
     check(n.color.values==std::array<std::int32_t,4>{255,255,255,255}&&n.direction.direction==2&&!n.direction.locked);
     check(n.parameters.values->size()==13&&n.parameters.extras->empty());
     for(std::int32_t id=0;id<13;++id){auto& p=*n.parameters.values->at(id);
        check(p.id==id&&p.value==100+id&&p.level==4&&p.max_level==1000&&p.maximum==(id<2?100+id:0));
        check(p.extra==0&&p.extra_maximum==0&&p.experience==0&&p.max_experience==0&&p.experience_buffer==0);}
     check(!e->contains(14)&&!e->contains(8)&&!e->contains(30));}
    {StartupFixture f;EnemyPoolFixture p(f);const auto count=f.nodes.size();
     check(!create_enemy({0,-1,1,0,{}},f.ids,p)&&p.record_queries==0);
     check(!create_enemy({0,3,1,0,{}},f.ids,p)&&p.record_queries==1&&f.nodes.size()==count);
     p.records->at(1).reset();fails([&]{create_enemy({0,1,1,0,{}},f.ids,p);});check(f.nodes.size()==count);}
    {StartupFixture f;EnemyPoolFixture p(f);auto replacement=std::make_shared<EnemyTable>(1);
     p.on_records=[&](std::size_t n){if(n==2)p.records=replacement;};const auto count=f.nodes.size();
     fails([&]{create_enemy({0,2,1,0,{}},f.ids,p);});check(f.nodes.size()==count);}
    {StartupFixture f;EnemyPoolFixture p(f);EnemyCreation args{2,1,7,1,{22,3,44}};
     p.on_resource=[&]{args.id=2;args.level=99;args.position.x=999;};
     auto* e=create_enemy(args,f.ids,p);check(e&&f.node(*e).enemy.enemy_id==1&&f.node(*e).enemy.level==7&&f.node(*e).position.x==22);}
    // Direction -1 is omitted; component flags 1/2 suppress speed, data flag 1 suppresses playback.
    {StartupFixture f;EnemyPoolFixture p(f);p.records->at(1)->flags=1;p.records->at(1)->seb=9;p.records->at(1)->frame=-2;
     auto* e=create_enemy({-1,1,1,2,{}},f.ids,p);check(e&&!e->contains(12)&&!e->contains(10));
     auto& n=f.node(*e);check(n.speed.z==0&&n.sprite.animation.clip==9&&n.sprite.animation.frame==-2);}
    // Captured clip/frame precede registration, while image and final playback flag are live.
    {StartupFixture f;EnemyPoolFixture p(f);auto& data=*p.records->at(0);
     f.on_added=[&](ComponentSlots&,std::int32_t slot){if(slot==4){data.image=77;data.seb=99;data.frame=88;}if(slot==32)data.flags=1;};
     auto* e=create_enemy({0,0,1,0,{}},f.ids,p);check(e);auto& n=f.node(*e);
     check(n.image.arrays.textures->at(0)==77&&n.sprite.animation.clip==8&&n.sprite.animation.frame==0&&!e->contains(10));}
    {StartupFixture f;EnemyPoolFixture p(f);auto& data=*p.records->at(0);data.flags=1;data.parameters.reset();
     check(enemy_move_speed(data,1,p.maxima)==0);data.flags=0;
     fails([&]{enemy_move_speed(data,1,p.maxima);});}
    {StartupFixture f;EnemyPoolFixture p(f);auto& data=*p.records->at(0);*data.parameters->at(5)=EventIntegers{0,0};
     check(enemy_move_speed(data,1,p.maxima)==0.5575F);
     auto ids=std::make_shared<EventIntegers>(EventIntegers{1,1,2});auto set=create_enemy_parameters(data,7,99,ids,p.maxima);
     check(set->values().size()==2&&set->values().at(1)->maximum==101&&set->values().at(2)->maximum==0&&set->values().at(1)->max_level==99);}
    {auto a=initial_enemy_parameter_ids(),b=initial_enemy_parameter_ids();check(a!=b&&a->size()==13);
     for(std::int32_t i=0;i<13;++i)check(a->at(static_cast<std::size_t>(i))==i);
     auto maxima=initial_enemy_parameter_maximum_levels();for(std::size_t i=0;i<13;++i)check(maxima[i]==(i<4?1000:200));
     check(enemy_parameter_has_maximum(0)&&enemy_parameter_has_maximum(1)&&!enemy_parameter_has_maximum(2)&&!enemy_parameter_has_maximum(-1));}
    // A late parameter failure retains the already registered visual/enemy/AI components.
    {StartupFixture f;EnemyPoolFixture p(f);p.records->at(0)->parameters->at(12).reset();const auto count=f.nodes.size();
     fails([&]{create_enemy({0,0,1,1,{}},f.ids,p);});check(f.nodes.size()==count+1);
     auto& e=f.nodes.back().entity;check(e.contains(24)&&e.contains(20)&&!e.contains(23)&&!e.contains(32));}
}catch(const std::exception& e){std::cerr<<"uncaught enemy factory error: "<<e.what()<<'\n';return 1;}
