#include "bullet_fixture.hpp"
#include <limits>
#include <fstream>
#include <iterator>

std::string read_table(const char* path){std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("cannot read table");
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}

int main(int argc,char** argv)try{
    const auto parsed=load_bullet_table("2\t4\t1\tRes\t12\t13\tImpact\t14\t3\n0\t8\t2\tR\t20\t21\tI\t22\t0\n2\t9\t0\tNew\t30\t31\tN\t32\t1\textra",false,parse_event_decimal);
    check(parsed.size()==3&&!parsed[1]&&parsed[2]->image==30&&parsed[2]->category==9&&parsed[2]->attribute==0);
    check(parsed[0]->resource=="R"&&parsed[0]->impact_resource=="I"&&parsed[0]->trail_image==21&&parsed[0]->effect_image==22);
    fails([]{load_bullet_table("0\t1\t2\tR\t3\t4\tI\t5",false,parse_event_decimal);});
    {BulletRecord r;r.id=99;r.attribute=55;r.resource="prior";
     check(&load_bullet_record(r,"",parse_event_decimal)==&r&&r.id==99);
     fails([&]{load_bullet_record(r,"2\t4\tx",parse_event_decimal);});
     check(r.id==2&&r.category==4&&r.attribute==55&&r.resource=="prior");
     fails([&]{load_bullet_record(r,"3\t5\t1\tNew\t12",parse_event_decimal);});
     check(r.id==3&&r.category==5&&r.attribute==1&&r.resource=="New"&&r.image==12);}
    {HumanFixture t;BulletPools p(t.f);EquipmentCombatRecord w;w.bullet_id=-1;check(!equipment_bullet(&w,p)&&p.queries==0);
     w.bullet_id=2;check(equipment_bullet(&w,p)==&*p.records->at(2));p.records->at(2).reset();check(!equipment_bullet(&w,p));
     w.bullet_id=-2;fails([&]{equipment_bullet(&w,p);});fails([&]{equipment_bullet(nullptr,p);});}
    {HumanFixture t;BulletPools p(t.f);t.f.geometry={10,7,6};t.f.active_map=&t.f.geometry;
     auto& e=create_bullet({1,42,2,&t.hero(),{39.9F,5,-20.9F}},p);
     check(field<BulletComponent>(e,29).data_id==1&&field<ContactPosition>(e,0).y==5);
     const auto& cell=field<CellComponent>(e,5);check(cell.xi==3&&cell.yi==-2);
     check(field<EntitySprite>(e,2).animation.resource==19&&field<EntitySprite>(e,2).animation.clip==0&&!e.contains(10));
     check(field<EntityImage>(e,7).arrays.textures->at(0)==101);const auto& em=field<EmitterComponent>(e,31);
     check(em.category==0&&em.type==0&&em.interval==2&&em.frame==0&&em.maximum==-1&&em.argument==0);
     check(field<ProjectileCollision>(e,32).radius==20&&!field<ProjectileCollision>(e,32).visited);
     check(field<ProjectileDamage>(e,33).parent==&t.hero()&&field<ProjectileDamage>(e,33).value==42&&!e.contains(6));
     auto* group=&t.f.groups.get(std::make_shared<ComponentGroupFilter>(EventIntegers{29,32,33}));check(group->size()==1);
     p.records->at(1)->flags=1;auto& blend=create_bullet({1,1,0,nullptr,{}},p);check(field<DefeatColor>(blend,6).values[3]==3192);}
    for(const auto id:{-1,3}){HumanFixture t;BulletPools p(t.f);const auto before=t.f.nodes.size();fails([&]{create_bullet({id,1,0,nullptr,{}},p);});check(t.f.nodes.size()==before);}
    {HumanFixture t;BulletPools p(t.f);p.records.reset();fails([&]{create_bullet({},p);});check(p.map_queries==0);}
    {HumanFixture t;BulletPools p(t.f);p.missing_world=true;fails([&]{create_bullet({},p);});check(p.map_queries==0);}
    {HumanFixture t;BulletPools p(t.f);p.records->at(0).reset();const auto count=t.f.nodes.size();
     fails([&]{create_bullet({},p);});check(t.f.nodes.size()==count+1);auto& e=t.f.nodes.back().entity;
     check(e.contains(29)&&e.contains(0)&&e.contains(5)&&e.contains(1)&&e.contains(2)&&!e.contains(7)&&!e.contains(33));}
    {HumanFixture t;BulletPools p(t.f);t.f.geometry={10,7,6};t.f.other_geometry={99,5,6};t.f.active_map=&t.f.geometry;
     p.on_map=[&](std::size_t n){if(n==2)t.f.active_map=&t.f.other_geometry;};
     auto& e=create_bullet({0,0,0,nullptr,{29.9F,0,-19.9F}},p);check(field<CellComponent>(e,5).xi==2&&field<CellComponent>(e,5).yi==-3);}
    {HumanFixture t;BulletPools p(t.f);t.f.geometry={0,-1,6};t.f.active_map=&t.f.geometry;
     auto& e=create_bullet({0,0,0,nullptr,{100,0,std::numeric_limits<float>::infinity()}},p);
     check(field<CellComponent>(e,5).xi==0&&field<CellComponent>(e,5).yi==std::numeric_limits<std::int32_t>::min());}
    {HumanFixture t;BulletPools p(t.f);std::vector<std::int32_t> slots;t.f.on_added=[&](ComponentSlots&,std::int32_t slot){slots.push_back(slot);};
     p.records->at(0)->flags=1;create_bullet({},p);check(slots==EventIntegers{29,0,5,1,2,7,4,31,32,33,6});}
    {HumanFixture t;BulletPools p(t.f);const auto original=p.records;auto replacement=std::make_shared<BulletTable>(*original);
     replacement->at(0)->image=700;t.f.on_added=[&](ComponentSlots&,std::int32_t slot){if(slot==29)p.records=replacement;};
     auto& e=create_bullet({},p);check(field<EntityImage>(e,7).arrays.textures->at(0)==100&&p.records==replacement);}
    // Optional maintainer-data audit; default CTest remains entirely synthetic.
    if(argc==3){const auto bullets=load_bullet_table(read_table(argv[1]),false,parse_event_decimal);
        const auto weapons=load_equipment_combat_table(read_table(argv[2]),EquipmentTableKind::weapon_or_shield,false,parse_event_decimal);
        std::size_t rows=0,gear=0,references=0;std::uint64_t hash=14695981039346656037ULL;
        const auto value=[&](std::int32_t v){hash^=static_cast<std::uint32_t>(v);hash*=1099511628211ULL;};
        for(std::size_t i=0;i<bullets.size();++i)if(bullets[i]){const auto& r=*bullets[i];check(r.id==static_cast<std::int32_t>(i));++rows;
            for(const auto v:{r.id,r.category,r.attribute,r.image,r.trail_image,r.effect_image,r.flags})value(v);}
        for(const auto& row:weapons)if(row){const auto& r=*row;++gear;
            for(const auto v:{r.id,r.type,r.bullet_id,r.bullet_speed,r.max_height,r.distance,r.burst_count,r.use_mp})value(v);
            if(r.bullet_id!=-1){check(bullets.at(static_cast<std::size_t>(r.bullet_id)).has_value());++references;}}
        std::cout<<"bullets="<<rows<<" slots="<<bullets.size()<<" equipment="<<gear<<" references="<<references<<" hash="<<hash<<'\n';
    }
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
