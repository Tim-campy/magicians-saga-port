#include "party_fixture.hpp"

int main()try{
    {Fixture f;check(f.nodes.size()==1&&f.ids.counter==1&&(*f.roster.formation)[10]==0);
     auto& hero=f.nodes.front();check(hero.human.level==1&&hero.human.experience==0&&f.records[0].saved.level==4&&f.records[0].saved.experience==42);
     check(hero.entity.contains(28)&&hero.entity.contains(30)&&!hero.entity.contains(20));
     check(f.world.cached_player==&hero.entity&&f.roster.members->entities().empty()&&f.roster.sub_members->entities().empty());
     (*f.roster.formation)[0]=1;start_party(f.roster,static_cast<PartyStartHost&>(f));
     check(f.roster.members->entities().size()==2&&f.creations.size()==1);
     check(hero.position.x==172&&hero.position.z==272&&f.sorties[0]==1&&f.sorties[1]==1);}
    {Fixture f(false);f.table->at(0)=nullptr;f.records[1].flags=2;f.records[2].flags=2;
     auto& hero=create_hero(&f.factory,f);check(f.node(hero).human.character_id==1&&hero.contains(30));
     check(!f.world.cached_player&&(*f.roster.formation)[10]==-1);
     check(world_player_entity(f.world)==&hero);}
    {Fixture f(false);f.records[0].flags=0;f.records[1].flags=2;f.records[1].id=5;
     auto& hero=create_hero(&f.factory,f);check(f.node(hero).human.character_id==5&&hero.contains(20)&&hero.contains(30)&&!hero.contains(28));}
    {Fixture f(false);for(auto& record:f.records)record.flags=0;
     fails([&]{create_hero(&f.factory,f);});check(f.nodes.empty()&&f.ids.counter==0);
     f.table.reset();fails([&]{create_hero(&f.factory,f);});}
    {Fixture f(false);f.null_player_marker=true;
     fails([&]{initialize_member_system(f.roster,&f.world,f);});check(f.nodes.size()==1&&f.registry.contains(0));
     check(f.nodes.front().entity.contains(28)&&!f.nodes.front().entity.contains(30)&&!f.world.cached_player&&(*f.roster.formation)[10]==-1);}
    {Fixture f(false);f.on_added=[](ComponentSlots&,std::int32_t slot){if(slot==30)throw std::runtime_error("player listener");};
     fails([&]{initialize_member_system(f.roster,&f.world,f);});check(f.nodes.size()==1&&f.registry.contains(0));
     check(f.nodes.front().entity.contains(30)&&!f.world.cached_player&&(*f.roster.formation)[10]==-1);}
    {Fixture f(false);f.no_world=true;f.world.loaded=255;
     initialize_member_system(f.roster,&f.world,f);check(f.calls.empty()&&f.nodes.empty()&&!world_is_new_game(f.world));
     f.world.loaded=2;check(!world_is_new_game(f.world));f.world.loaded=1;check(!world_is_new_game(f.world));
     f.world.loaded=0;check(world_is_new_game(f.world));fails([&]{initialize_member_system(f.roster,&f.world,f);});}
    {Fixture f(false);fails([&]{initialize_member_system(f.roster,nullptr,f);});check(f.calls.empty());}
    {Fixture f(false);PlayerWorldState argument;f.world.loaded=1;
     initialize_member_system(f.roster,&argument,f);check(f.nodes.size()==1&&(*f.roster.formation)[10]==0);
     check(!argument.cached_player&&f.world.cached_player==&f.nodes.front().entity);}
    {Fixture f(false);(*f.roster.formation)[10]=7;initialize_member_system(f.roster,&f.world,f);
     check((*f.roster.formation)[10]==7&&f.nodes.size()==1); // Existing slot blocks assignment; creation is retained.
    }
    {Fixture f(false);f.roster.formation=std::make_shared<EventIntegers>(3,-1);
     initialize_member_system(f.roster,&f.world,f);check(f.nodes.size()==1&&f.roster.formation->size()==3);}
    {Fixture f;auto* cached=f.world.cached_player;initialize_member_system(f.roster,&f.world,f);
     check(f.nodes.size()==2&&f.world.cached_player==cached&&f.nodes[1].entity.get(30)==f.nodes[0].entity.get(30));
     check(f.roster.members->entities().empty());}
    {Fixture f(false);auto& old=f.spawn(2);PlayerWorldState later{&old.entity,&f.groups};
     f.on_added=[&](ComponentSlots&,std::int32_t slot){if(slot==30)f.current_binding=&later;};
     initialize_member_system(f.roster,&f.world,f);
     check(f.nodes.size()==2&&(*f.roster.formation)[10]==2&&!f.world.cached_player&&later.cached_player==&old.entity);}
    {Fixture f(false);MemberSystemInitializer member(f.roster,f);InitList list;list.append(&member);
     MemberBootstrapWorld binding;binding.state=&f.world;InitializationSchedule schedule{&list,true};
     initialize_systems(schedule,&binding);check(!schedule.paused&&f.nodes.size()==1&&(*f.roster.formation)[10]==0);}
    {Fixture f(false);MemberSystemInitializer member(f.roster,f);InitList list;list.append(&member);
     InitializationSchedule schedule{&list,true};MemberBootstrapWorld binding;
     fails([&]{initialize_systems(schedule,&binding);});check(schedule.paused&&f.nodes.empty());
     InitializationWorld wrong;fails([&]{member.init(&wrong);});}
}catch(const std::exception& e){std::cerr<<"uncaught hero bootstrap error: "<<e.what()<<'\n';return 1;}
