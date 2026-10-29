#include "mission_data_fixture.hpp"
#include "platform/desktop/mission_humans.hpp"
#include "platform/desktop/mission_scene.hpp"
#include <cstdlib>
#include <algorithm>
#include <iostream>
using namespace saga::desktop;
using namespace saga::core;
namespace {
void check(bool v){if(!v)std::abort();}
template<class T>T& field(ComponentSlots& e,int slot){auto* p=dynamic_cast<T*>(e.get(slot));check(p!=nullptr);return *p;}
}
int main()try{
    ResourceArchive archive(saga::test::jar_fixture(saga::test::mission_tables()),{"English.lproj",1},ArchiveContents::data_only);
    auto data=load_gameplay_catalog(archive,false,parse_event_decimal);data.treasures=TreasureTable(1);
    TreasureRecord gem;gem.id=0;gem.image=77;gem.resource="unused-other-archive";data.treasures->at(0)=gem;
    auto& weapon=*data.equipment[1];weapon.bullet_speed=4;weapon.max_height=15;weapon.distance=12;weapon.range=0;weapon.use_mp=1;
    data.equipment[0]=weapon;data.equipment[0]->id=0;
    auto bullet=*data.bullets->at(3);bullet.id=0;bullet.attribute=0;bullet.category=0;data.bullets=std::make_shared<BulletTable>(1,bullet);
    data.enemies->at(4)->treasures=EnemyTreasureGroups{{EventIntegers{0,1000}}};
    MissionProgress saved;saved.characters[2]={5,13};for(int i=0;i<8;++i)saved.shop_purchases[i]=0;
    saved.equipment_levels={{0,1},{1,1}};saved.hat_levels={{0,1}};saved.cloth_levels={{0,1}};
    StageGeometry geometry{24,24,6};SharedAnimationClip clock{0,128};AnimationResource animation{std::make_shared<std::vector<SharedAnimationClip*>>(128,&clock)};
    MissionEntities world(data,saved,35,&geometry,[&](auto){return &animation;},[](auto){return 3;});
    auto& hero=world.human({0,2,5,0,0,13,0,0,100});world.mark_player(hero);
    auto* enemy=world.enemy({0,4,7,2,{0,0,40}});check(enemy!=nullptr);
    auto& hp=*resolve_combat_parameter(*enemy,0);hp.value=1;
    auto& mp=*resolve_combat_parameter(hero,1);mp.value=mp.maximum=10000;
    MissionHumans humans(world,data,saved,geometry,{25,false,42,"system"},{10,0,100,0,1});humans.enable_rewards({0,1,4});
    RenderDrawResource drawing{&animation,nullptr};MissionApplication app;app.characters=&drawing;app.enemies=&drawing;app.bullets=&drawing;app.effects=&drawing;app.quest=&drawing;
    MissionView view{100,100,100,0,0,0,0,100};MissionScene scene(world,app,*world.subset({0,2}),view,nullptr);
    std::size_t most_emitters=0;bool dying=false;
    for(int i=0;i<240;++i){humans.update();humans.advance_projectiles();scene.advance_movement(0);scene.advance_emitters(0);scene.advance_modifications(0);scene.advance_garbage(0);
        most_emitters=std::max(most_emitters,world.subset({31})->size());
        if(enemy->contains(24)&&field<EnemyRecordLink>(*enemy,24).state==4)dying=true;
    }
    check(hp.value==0&&dying&&world.destroyed(*enemy)&&!world.registry().contains(world.id(*enemy)));
    check(saved.enemy_defeat_counts.at(4)==5&&most_emitters>=3&&world.subset({31})->size()==0);
    check(humans.random_draws()>=19&&world.subset({34})->size()==2);
    bool experience=false,treasure=false;auto cursor=world.subset({34})->cursor();while(const auto entry=cursor.next()){
        auto& e=**entry;const auto& drop=field<DropComponent>(e,34);check(field<EntityDepth>(e,4).extra==-10&&field<EntitySprite>(e,2).animation.resource==7);
        if(drop.type==0){experience=true;check(drop.value>0&&e.contains(13)&&field<EntityImage>(e,7).arrays.textures->at(0)==98);}
        else{treasure=true;check(drop.type==3&&drop.value==0&&!e.contains(13)&&field<EntityImage>(e,7).arrays.textures->at(0)==77);}
    }
    check(experience&&treasure&&saved.characters.at(2).experience==13); // Pending drops are not permanent EXP grants.
    humans.enable_notifications({1,13,false,0,false,0,-1});humans.enable_collection();humans.advance_collection();
    auto assert_first_tick=[&]{auto pending=world.subset({34})->cursor();while(const auto e=pending.next())check(field<DropComponent>(**e,34).frame==1);};
    assert_first_tick();field<HumanRecordLink>(hero,14).state=5;resolve_combat_parameter(hero,0)->value=0;
    world.events().send(30,&hero); // Pending retry dialog keeps Stage state1 but gates subsequent collection.
    humans.advance_collection();assert_first_tick();check(saved.characters.at(2).experience==13);
    std::cout<<"lethal shot, saved defeat count, EXP/treasure drops, debris/trails and corpse cleanup ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
