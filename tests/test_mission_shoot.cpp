#include "mission_data_fixture.hpp"
#include "platform/desktop/mission_humans.hpp"
#include "platform/desktop/mission_scene.hpp"
#include "core/gameplay/motion_pass.hpp"
#include <cstdlib>
#include <iostream>
using namespace saga::desktop;
using namespace saga::core;
namespace {
void check(bool v){if(!v)std::abort();}
template<class T>T& field(ComponentSlots& e,int slot){auto* p=dynamic_cast<T*>(e.get(slot));check(p!=nullptr);return *p;}
}
int main()try{
    ResourceArchive archive(saga::test::jar_fixture(saga::test::mission_tables()),{"English.lproj",1},ArchiveContents::data_only);
    auto data=load_gameplay_catalog(archive,false,parse_event_decimal);
    auto& weapon=*data.equipment[1];weapon.bullet_speed=4;weapon.max_height=15;weapon.distance=12;weapon.range=0;weapon.use_mp=1;
    data.equipment[0]=weapon;data.equipment[0]->id=0;
    auto bullet=*data.bullets->at(3);bullet.id=0;bullet.attribute=0;bullet.category=0;
    data.bullets=std::make_shared<BulletTable>(1,bullet);
    MissionProgress saved;saved.characters[2]={5,13};for(int i=0;i<8;++i)saved.shop_purchases[i]=0;
    saved.equipment_levels={{0,1},{1,1}};saved.hat_levels={{0,1}};saved.cloth_levels={{0,1}};
    StageGeometry geometry{24,24,6};SharedAnimationClip clock{0,128};AnimationResource animation{std::make_shared<std::vector<SharedAnimationClip*>>(128,&clock)};
    MissionEntities world(data,saved,35,&geometry,[&](auto){return &animation;},[](auto){return 3;});
    auto& hero=world.human({0,2,5,0,0,13,0,0,100});world.mark_player(hero);
    auto* enemy=world.enemy({0,4,7,2,{0,0,40}});check(enemy!=nullptr);
    auto& hp=*resolve_combat_parameter(*enemy,0);hp.value=hp.maximum=100000;
    auto& mp=*resolve_combat_parameter(hero,1);mp.value=mp.maximum=10000;
    MissionHumans humans(world,data,saved,geometry,{25,false,42,"system"},{10,0,100,0,1});
    RenderDrawResource drawing{&animation,nullptr};MissionApplication app;app.characters=&drawing;app.enemies=&drawing;app.bullets=&drawing;app.effects=&drawing;
    MissionView view{100,100,100,0,0,0,0,100};MissionScene scene(world,app,*world.subset({0,2}),view,nullptr);
    auto* movers=world.subset({0,1});auto* bullets=world.subset({29});ComponentSlots* first=nullptr;bool saw_destroyed=false;
    world.events().subscribe(23,EntityEvents::Single([&](ComponentSlots* e){if(e==first)saw_destroyed=world.destroyed(*e);}));
    for(int i=0;i<80;++i){humans.update();humans.advance_projectiles();move_subset(*movers);scene.advance_modifications(0);
        if(!first){auto cursor=bullets->cursor();if(const auto e=cursor.next())first=*e;}
        if(first&&world.destroyed(*first))break;
    }
    check(first&&saw_destroyed&&world.destroyed(*first)&&!world.registry().contains(world.id(*first)));
    check(!first->contains(27)&&!first->contains(32)&&humans.shots()>0&&mp.value<10000);
    check(hp.value<100000&&hp.value>0&&humans.total_damage()>0&&humans.random_draws()>0);
    const auto& shake=field<ProjectileModification>(*enemy,15);check(shake.type==4&&shake.max_frame==8&&shake.frame==1&&shake.x==-4);
    for(int i=0;i<8;++i)scene.advance_modifications(0);check(!enemy->contains(15));
    // Lethal damage must stop at the still-unbound native defeat service, after subtracting HP.
    hp.value=1;bool stopped=false;
    for(int i=0;i<80&&!stopped;++i)try{humans.update();humans.advance_projectiles();move_subset(*movers);}
        catch(const std::logic_error& error){check(std::string(error.what()).find("enemy defeat")!=std::string::npos);stopped=true;}
    check(stopped&&hp.value==0&&!world.destroyed(*enemy));
    std::cout<<"mission shot/MP/flight/impact/damage/native destruction and lethal boundary ok\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
