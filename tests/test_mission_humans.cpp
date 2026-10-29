#include "mission_data_fixture.hpp"
#include "platform/desktop/mission_humans.hpp"
#include "core/gameplay/motion_pass.hpp"
#include <cstdlib>
#include <cmath>
#include <iostream>
using namespace saga::desktop;
using namespace saga::core;
namespace {
void check(bool v){if(!v)std::abort();}
template<class T>T& field(ComponentSlots& e,int slot){auto* p=dynamic_cast<T*>(e.get(slot));check(p!=nullptr);return *p;}
template<class F>void unbound(F f){try{f();}catch(const std::logic_error& e){check(std::string(e.what()).find("mission service unbound")!=std::string::npos);return;}std::abort();}
}
int main()try{
    ResourceArchive archive(saga::test::jar_fixture(saga::test::mission_tables()),{"English.lproj",1},ArchiveContents::data_only);
    auto data=load_gameplay_catalog(archive,false,parse_event_decimal);data.equipment[0]=data.equipment[1];data.equipment[0]->id=0;
    MissionProgress saved;saved.characters[2]={5,13};for(int i=0;i<8;++i)saved.shop_purchases[i]=0;
    saved.equipment_levels={{0,1},{1,1}};saved.hat_levels={{0,1}};saved.cloth_levels={{0,1}};
    StageGeometry geometry{24,24,6};SharedAnimationClip clock{0,12};AnimationResource animation{std::make_shared<std::vector<SharedAnimationClip*>>(64,&clock)};
    MissionEntities world(data,saved,35,&geometry,[&](auto){return &animation;},[](auto){return 3;});
    auto& hero=world.human({0,2,5,0,0,13,0,0,100});world.mark_player(hero);
    MissionHumans humans(world,data,saved,geometry,{25,false,42,"system"},{10,0,100,0,0});humans.set_state(hero,1,-1);
    auto& hp=*resolve_combat_parameter(hero,0);hp.value=1;
    humans.update(0,false);check(field<HumanRecordLink>(hero,14).frame==0&&humans.random_draws()==0);
    humans.update(1);check(field<HumanRecordLink>(hero,14).frame==1&&humans.random_draws()==1);
    check(hp.value>1);
    check(field<EntitySprite>(hero,2).animation.clip==40&&field<ProjectileSpeed>(hero,1).z<0);
    auto* movers=world.subset({0,1});move_subset(*movers);check(field<ContactPosition>(hero,0).z<100);
    humans.set_state(hero,0,0);check(field<EntitySprite>(hero,2).animation.clip==36);
    world.events().send(23,&hero); // Real Human Init registered the guarded receiver.
    field<HumanRecordLink>(hero,14).state=4;
    unbound([&]{world.events().send(23,&hero);});check(field<EntitySprite>(hero,2).animation.clip==12);
    check(field<HumanRecordLink>(hero,14).state==4); // Failure preserves the native prefix, no invented success.
    field<HumanRecordLink>(hero,14).state=0;
    auto* enemy=world.enemy({0,4,7,0,{10,0,20}});check(enemy!=nullptr);
    const auto z=field<ContactPosition>(hero,0).z;
    unbound([&]{world.events().send(29,&hero,enemy);});
    check(field<ContactPosition>(hero,0).z==z+3&&field<ContactDirection>(hero,12).locked);
    check(field<EntityPlayback>(hero,10).state.return_clip==36);
    std::cout<<"mission human frame, state, RNG and event services ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
