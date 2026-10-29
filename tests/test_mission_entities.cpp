#include "mission_data_fixture.hpp"
#include "platform/desktop/mission_entities.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

using namespace saga::core;
using namespace saga::desktop;
namespace {
void check(bool v){if(!v)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
template<class T>T& field(ComponentSlots& e,std::int32_t slot){auto* value=dynamic_cast<T*>(e.get(slot));check(value!=nullptr);return *value;}
}
int main()try{
    ResourceArchive archive(saga::test::jar_fixture(saga::test::mission_tables()),{"English.lproj",1},ArchiveContents::data_only);
    const auto data=load_gameplay_catalog(archive,false,parse_event_decimal);MissionProgress saved;saved.characters[2]={5,13};
    for(int i=0;i<8;++i)saved.shop_purchases[i]=0;
    StageGeometry map{24,24,6};SharedAnimationClip clock{0,12};AnimationResource animation{std::make_shared<std::vector<SharedAnimationClip*>>(64,&clock)};
    MissionEntities world(data,saved,35,&map,[&](std::int32_t id){return id==1||id==3||id==19||id==20?&animation:nullptr;},
        [](std::string_view name){if(name!="enemy-book")throw std::invalid_argument("unbound resource");return 3;});
    auto* humans=world.subset({14});auto* bodies=world.subset({0,2});check(humans->size()==0);
    auto& hero=world.human({0,2,5,0,0,13,0,0,0});auto& visual=field<EntityImage>(hero,7).arrays;
    check(*visual.textures==EventIntegers({200,8,8,9,12})&&*visual.resources==EventIntegers({1,1,1,21,22}));
    check(field<HumanRecordLink>(hero,14).level==5&&field<HumanRecordLink>(hero,14).experience==13&&hero.contains(28));
    check(humans->size()==1&&bodies->size()==1&&world.registry().find(world.id(hero))==&hero);
    auto* enemy=world.enemy({0,4,7,0,{10,0,30}});check(enemy!=nullptr&&field<EntitySprite>(*enemy,2).animation.resource==3);
    check(field<EntityImage>(*enemy,7).arrays.textures->at(0)==7&&field<EnemyRecordLink>(*enemy,24).level==7);
    auto& bullet=world.bullet({3,42,1,&hero,{3,4,5}});check(field<BulletComponent>(bullet,29).data_id==3&&field<ProjectileDamage>(bullet,33).parent==&hero);
    check(field<EntityImage>(bullet,7).arrays.textures->at(0)==5&&field<EntitySprite>(bullet,2).animation.resource==19);
    auto& effect=world.effect({0,0,0},0,7,false);check(effect.contains(10)&&field<EntityImage>(effect,7).arrays.resources->at(0)==20);
    check(bodies->size()==4&&world.id(hero)!=world.id(*enemy)&&world.id(bullet)!=world.id(effect));
    AnimationSystemState state;initialize_animation(state,&world);check(state.subset->size()==3&&state.resources->size()==24);
    update_animations(state,{false,true});check(field<EntitySprite>(hero,2).animation.frame==1);
    saved.characters.erase(2);fails([&]{world.human({0,2,5,0,0,13,0,0,0});});check(humans->size()==1);
    check(world.character_table()->at(2)==nullptr);
    check(world.enemy({0,-1,1,0,{}})==nullptr);fails([&]{world.bullet({0,1,0,&hero,{}});});
    fails([&]{world.release_collision(&field<ProjectileCollision>(hero,32));});
    std::cout<<"mission entity bindings ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
