#include "mission_data_fixture.hpp"
#include "platform/desktop/mission_scene.hpp"
#include "platform/desktop/mission_spatial.hpp"
#include "core/animation/seb_sprite.hpp"
#include <cstdlib>
#include <iostream>

using namespace saga::core;
using namespace saga::desktop;
namespace {
void check(bool v){if(!v)std::abort();}
}
int main()try{
    ResourceArchive archive(saga::test::jar_fixture(saga::test::mission_tables()),{"English.lproj",1},ArchiveContents::data_only);
    const auto data=load_gameplay_catalog(archive,false,parse_event_decimal);MissionProgress saved;saved.characters[2]={5,13};for(int i=0;i<8;++i)saved.shop_purchases[i]=0;
    saga::assets::Seb seb{0,100,{{0,2,std::int16_t{0}}},std::vector<saga::assets::SebRecord>(2)};
    for(std::size_t i=0;i<2;++i){seb.records[i].value_count=10;seb.records[i].values={static_cast<std::int16_t>(i*100),0,0,0,2,2,0,0,0,0};}
    PreparedAnimationResource animations(std::vector<const saga::assets::Seb*>(64,&seb));RenderDrawResource resource{&animations.resource(),nullptr};
    MissionApplication app;app.characters=&resource;app.enemies=&resource;
    MissionEntities world(data,saved,35,nullptr,[&](std::int32_t id){auto* r=get_application_resource(app,id);return r?r->animations:nullptr;},[](std::string_view){return 3;});
    auto* visible=world.subset({0,2});MissionView view{128,96,100,-20,-30,0,0,100};MissionScene scene(world,app,*visible,view,nullptr);
    auto& hero=world.human({0,2,5,0,0,13,0,0,0});auto* enemy=world.enemy({0,4,7,0,{10,0,0}});check(enemy!=nullptr);
    auto& queue=scene.render();check(scene.faults().empty()&&queue.counts->at(0)==2);
    const auto& first=*queue.buffers->at(0)->at(0);check(first[render_x]==20&&first[render_y]==30&&first[render_texture]==200);
    auto& enemy_position=*dynamic_cast<ContactPosition*>(enemy->get(0));
    const auto speed=dynamic_cast<ProjectileSpeed*>(enemy->get(1))->z;
    check(speed>0);scene.advance_movement(0,false);check(enemy_position.z==0);
    scene.advance_movement(1);check(enemy_position.z==speed); // Move ignores the pass index.
    scene.advance_movement(0);check(enemy_position.z==speed+speed);
    auto& human_position=*dynamic_cast<ContactPosition*>(hero.get(0));check(human_position.x==0&&human_position.z==0);
    enemy->remove(1);scene.advance_movement(0);check(enemy_position.z==speed+speed); // Live subset removal.
    auto& restored=world.acquire_speed();restored.z=3;enemy->add(1,&restored);
    scene.advance_movement(0);check(enemy_position.z==(speed+speed)+3); // Reinsertion is observed.
    scene.render();bool saw_enemy=false;
    for(int i=0;i<queue.counts->at(0);++i){const auto& command=*queue.buffers->at(0)->at(static_cast<std::size_t>(i));
        if(command[render_resource]==3){saw_enemy=true;check(command[render_x]==static_cast<std::int32_t>(10-enemy_position.z)+20);}}
    check(saw_enemy);enemy_position.z=0;scene.render();
    scene.advance_animations({false,true});scene.render();check(queue.buffers->at(0)->at(0)->at(render_frame)==1);
    hero.add(21,&world.acquire_input());scene.render();check(queue.counts->at(0)==1&&queue.buffers->at(0)->at(0)->at(render_resource)==3);
    hero.remove(21);dynamic_cast<EntitySprite*>(hero.get(2))->animation.clip=999;scene.render();
    check(scene.faults().size()==1&&scene.faults().front().entity==world.id(hero)&&queue.counts->at(0)==0);
    dynamic_cast<EntitySprite*>(hero.get(2))->animation.clip=36;app.effects=&resource;
    auto& effect=world.effect({20,0,0},0,-1,false);auto* lifetime=dynamic_cast<GarbageLifetime*>(effect.get(22));
    check(lifetime!=nullptr);lifetime->frames=2;scene.advance_garbage(0,false);check(lifetime->frames==2);
    scene.render();check(queue.counts->at(0)==3);scene.advance_garbage(1);check(!world.destroyed(effect)&&lifetime->frames==1);
    scene.render();scene.advance_garbage(0);check(world.destroyed(effect)&&!world.registry().contains(world.id(effect)));
    check(queue.counts->at(0)==3); // Native Render precedes Garbage; the produced final-frame commands survive.
    scene.render();check(queue.counts->at(0)==2&&scene.faults().empty());
    std::int32_t pairs=0;world.events().subscribe(29,[&](ComponentSlots* a,ComponentSlots* b){check(a==&hero&&b==enemy);++pairs;});
    scene.advance_collisions(1,false);check(pairs==0&&scene.collision_events()==0);
    scene.advance_collisions(1);check(pairs==1&&scene.collision_events()==1&&scene.area_collision_events()==0);
    enemy->remove(32);scene.advance_collisions(0);check(pairs==1); // The initialized live group observes removal.
    auto& effects=world.effect_world();auto& number=create_effect({{0,20,0},3,9,2,58,false,false,30,0,nullptr,-1,false},effects.ids,effects.factory);
    check(number.contains(26)&&!number.contains(4)&&!number.contains(5));
    scene.render();check(scene.faults().empty()&&queue.counts->at(0)==2); // Explicit numeric-label presentation stub; no invented Depth.
    for(int i=0;i<30;++i)scene.advance_garbage(0);
    check(world.destroyed(number)&&!world.registry().contains(world.id(number)));
    auto& notice=create_effect({{0,50,0},7,3,0,0,false,false,30,0,nullptr,-1,true},effects.ids,effects.factory);
    check(notice.contains(26)&&!notice.contains(4)&&!notice.contains(5));
    scene.render();check(scene.faults().empty());
    for(int i=0;i<30;++i)scene.advance_garbage(0);check(world.destroyed(notice));
    {
        MissionEntities mapped(data,saved,35,nullptr,[&](std::int32_t){return &animations.resource();},[](std::string_view){return 3;});
        auto& map=mapped.create_initial_map();check(map.width==3&&map.height==8&&map.step_z==312);
        auto& actor=mapped.human({0,2,5,0,0,13,0,0,1000});mapped.mark_player(actor);
        auto* group=mapped.subset({5});
        struct Counter final:ComponentObserver {
            int adds=0,removes=0;
            void added(ComponentSlots&,std::int32_t,Component*)override{++adds;}
            void removed(ComponentSlots&,std::int32_t,Component*)override{++removes;}
        } prior;
        group->notifications=&prior;
        auto quest=std::make_shared<StageQuest>();quest->distance=4;StageQuestForm state;
        MissionView dimensions{128,96,100,0,0,0,0,100};
        const auto images=std::make_shared<EventIntegers>(EventIntegers{0,1,2,3});
        {
            MissionSpatial spatial(mapped,map,dimensions,state,quest,images);
            check(map.height==6&&mapped.subset({9})->size()==18&&prior.adds>0);
            auto contains=[&]{auto at=spatial.visible().cursor();while(const auto e=at.next())if(*e==&actor)return true;return false;};
            const auto x=spatial.camera_host().current_camera_form()->x;
            spatial.advance_camera(1);check(spatial.camera_host().current_camera_form()->x==x); // Native pass-zero gate.
            spatial.advance_camera();spatial.advance_cells(1);spatial.advance_visibility();
            check(dynamic_cast<CellComponent*>(actor.get(5))->yi==3&&contains());
            actor.add(21,&mapped.acquire_input());spatial.advance_visibility();check(!contains());
            actor.remove(21);spatial.advance_visibility();check(contains());
            mapped.destroy(actor);spatial.advance_visibility();check(!contains()&&prior.removes>0);
        }
        check(group->notifications==&prior);
        bool caught=false;
        try{MissionSpatial invalid(mapped,map,dimensions,state,quest,nullptr);}catch(const std::invalid_argument&){caught=true;}
        check(caught&&group->notifications==&prior); // Constructor failure must not leave a dangling subscriber.
    }
    std::cout<<"mission scene render bindings ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
