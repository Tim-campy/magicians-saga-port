#include "emitter_fixture.hpp"
#include <limits>

int main()try{
    {HumanFixture t;Shooter s(t);EmitterPools pools(s);std::vector<std::int32_t> slots;
     t.f.on_added=[&](ComponentSlots&,std::int32_t slot){slots.push_back(slot);};
     auto& e=create_emitter({3,2,-4,9,17,{1,2,3}},pools);check(slots==EventIntegers{31,0,1});
     const auto& em=field<EmitterComponent>(e,31);check(em.category==3&&em.type==2&&em.interval==-4&&em.frame==0&&em.maximum==9&&em.argument==17);
     check(field<ContactPosition>(e,0).y==2&&field<ProjectileSpeed>(e,1).x==0&&!e.contains(5)&&!e.contains(2));}
    {HumanFixture t;Shooter s(t);EmitterPools pools(s);std::vector<std::int32_t> slots;t.f.active_map=&t.f.geometry;t.f.geometry={10,7,6};
     t.f.on_added=[&](ComponentSlots&,std::int32_t slot){slots.push_back(slot);};
     auto& e=create_particle({19,1,123,{39.9F,5,-20.9F},{1,-2,3}},pools);
     check(slots==EventIntegers{0,1,5,2,7,4,22});check(pools.clip_queries==1&&pools.world_queries==1&&pools.map_queries==2);
     check(field<CellComponent>(e,5).xi==3&&field<CellComponent>(e,5).yi==-2);
     check(field<ProjectileSpeed>(e,1).y==-2&&field<EntitySprite>(e,2).animation.resource==19&&field<EntitySprite>(e,2).animation.clip==1);
     check(field<EntityImage>(e,7).arrays.textures->at(0)==123&&field<GarbageLifetime>(e,22).frames==7&&!e.contains(10)&&!e.contains(15));}
    {HumanFixture t;Shooter s(t);EmitterPools pools(s);pools.resources.bullets=nullptr;const auto count=t.f.nodes.size();
     fails([&]{create_particle({19,1,0,{},{}},pools);});check(t.f.nodes.size()==count&&pools.world_queries==0);}
    {HumanFixture t;Shooter s(t);EmitterPools pools(s);pools.missing_world=true;
     fails([&]{create_particle({19,1,0,{},{}},pools);});check(pools.clip_queries==1&&pools.map_queries==0);
     fails([&]{create_emitter({},pools);});}
    {HumanFixture t;Shooter s(t);EmitterPools pools(s);t.f.geometry={10,7,6};t.f.other_geometry={99,5,6};t.f.active_map=&t.f.geometry;
     pools.on_map=[&](std::size_t n){if(n==2)t.f.active_map=&t.f.other_geometry;};
     auto& e=create_particle({19,1,0,{29.9F,0,-19.9F},{}},pools);check(field<CellComponent>(e,5).xi==2&&field<CellComponent>(e,5).yi==-3);}
    for(const auto maximum:{0,1,std::numeric_limits<std::int32_t>::min()}){HumanFixture t;Shooter s(t);EmitterPools pools(s);pools.clock.max_frame=maximum;
     auto& e=create_particle({19,1,-1,{},{}},pools);const auto expected=maximum==std::numeric_limits<std::int32_t>::min()?std::numeric_limits<std::int32_t>::max():maximum-1;
     check(field<GarbageLifetime>(e,22).frames==expected&&field<EntityImage>(e,7).arrays.textures->at(0)==-1);}
    {HumanFixture t;Shooter s(t);EmitterPools pools(s);pools.clock.max_frame=9;
     t.f.on_added=[&](ComponentSlots&,std::int32_t slot){if(slot==0)pools.clock.max_frame=100;};
     auto& e=create_particle({19,1,0,{},{}},pools);check(field<GarbageLifetime>(e,22).frames==8);}
    {HumanFixture t;Shooter s(t);EmitterPools pools(s);t.f.active_map=nullptr;const auto count=t.f.nodes.size();
     fails([&]{create_particle({19,1,0,{},{}},pools);});auto& e=t.f.nodes.back().entity;
     check(t.f.nodes.size()==count+1&&e.contains(0)&&e.contains(1)&&!e.contains(5)&&!e.contains(22));}
    {HumanFixture t;Shooter s(t);BulletComponent b;b.data_id=0;s.pools.on_records=[&]{b.data_id=1;};
     check(resolve_bullet_record(b,s.pools)==&*s.pools.records->at(1));s.pools.on_records={};s.pools.records->at(1).reset();
     check(!resolve_bullet_record(b,s.pools));b.data_id=-1;fails([&]{resolve_bullet_record(b,s.pools);});}
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
