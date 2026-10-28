#include "party_fixture.hpp"
int main()try{
    {Fixture f;auto& old=f.spawn(1);auto& retained=f.spawn(2);f.records[2].flags=0;
     add_party_collision(f.nodes.front().entity,99,1,2,3,true,static_cast<PartyStartHost&>(f));auto* previous=f.nodes.front().entity.get(32);
     (*f.roster.formation)[0]=1;(*f.roster.formation)[15]=2;start_party(f.roster,f);
     check(old.lifetime.destroyed&&!f.registry.contains(old.id)&&!old.entity.contains(14));
     check(!retained.lifetime.destroyed&&f.registry.contains(retained.id));
     check(f.released==std::vector<Component*>{previous});auto& c=f.collisions.back();
     check(c.radius==12&&c.offset_x==0&&c.offset_y==5&&c.offset_z==0&&!c.visited);
     check(f.creations.size()==2&&f.creations[0].character==1&&f.creations[1].character==2);
     const auto& a=f.creations[1];check(a.direction==0&&a.level==4&&a.experience==42&&a.formation_x==108&&a.formation_z==108&&a.x==0&&a.y==0&&a.z==0);
     auto& hero=f.nodes.front();check(hero.human.formation_index==10&&hero.human.formation_x==72&&hero.human.formation_z==72);
     check(hero.position.x==172&&hero.position.z==272&&hero.hp.value==100&&f.sorties[0]==1);
     check(f.roster.members->entities().size()==3&&f.roster.sub_members->entities().size()==2);
     check(f.quest_queries==0&&f.editor_queries==0&&f.random_queries==0&&f.distance_order.empty());
     check(front_line_human(&f.roster.members->entities())==&f.nodes[3].entity);}
    {Fixture f;add_member(f.roster,&f.nodes.front().entity);start_party(f.roster,f);
     check(f.roster.members->entities().size()==2&&f.sorties[0]==2); // No implicit list clear/dedup for hero.
     start_party(f.roster,f);check(f.roster.members->entities().size()==3&&f.sorties[0]==5);}
    {Fixture f;(*f.roster.formation)[0]=1;f.on_create=[&](const HumanCreation&){f.roster.formation=std::make_shared<std::vector<std::int32_t>>(2,-1);};
     start_party(f.roster,f);check(f.creations.size()==1&&f.nodes.front().human.formation_index==0);}
    {Fixture f;f.debug=true;start_party(f.roster,f);check(f.quest_queries==1&&f.random_queries==0&&f.editor_queries==0);}
    {Fixture f;f.debug=true;f.editor=true;f.auto_party=true;f.records[1].flags|=0x100;f.records[2].flags|=0x100;
     start_party(f.roster,f);check(f.random_queries==6&&f.layout_index==10&&f.editor_queries==1);
     check(f.records[1].saved.level==1200&&f.records[2].saved.level==1200&&f.records[0].saved.level==4);
     check(f.distance_order==std::vector<std::int32_t>({1,2,1,1,1,1,1,1,0}));
     check(f.random_candidates==std::vector<std::int32_t>({1,2,3,4,5,6,7,8,9,10,11}));
     check(f.nodes.front().human.formation_index==0&&f.creations.size()==8);
     check(f.creations.front().character==1&&f.creations.back().character==2); // Stable ascending distance.
     check(f.sorties[0]==1&&f.sorties[1]==7&&f.sorties[2]==1&&f.roster.members->entities().size()==9);
     check((*f.layout)[0]==1&&(*f.layout)[8]==1&&(*f.roster.formation)[10]==0);}
    {Fixture f;f.debug=true;f.auto_party=true;for(auto& r:f.records)if(r.id>0)r.flags|=0x100;
     f.table->at(2)=nullptr; // Null catalog gaps are skipped by the first predicate.
     f.layout->clear();start_party(f.roster,f);check(f.random_queries==0&&f.creations.size()==8);
     check(f.distance_order==std::vector<std::int32_t>({1,3,4,5,6,7,8,9,0}));
     for(const auto& c:f.creations)check(c.formation_x==-36&&c.formation_z==0);
     check(f.nodes.front().human.formation_index==-1&&f.nodes.front().position.x==64);}
    {Fixture f;f.on_factory_world=[&]{f.records[1].saved={77,88};(*f.table)[1]=&f.records[2];};
     auto* e=create_sub_character(12,24,1,f);check(e&&f.creations.back().character==1&&f.creations.back().level==77&&f.creations.back().experience==88);}
    {Fixture f;f.no_factory=true;fails([&]{create_sub_character(0,0,1,f);});check(f.creations.empty());
     f.table->at(1)=nullptr;fails([&]{create_sub_character(0,0,1,f);});fails([&]{create_sub_character(0,0,-1,f);});}
    {Fixture f;f.on_release=[](){throw std::runtime_error("release");};auto& e=f.nodes.front().entity;
     add_party_collision(e,99,0,0,0,false,static_cast<PartyStartHost&>(f));fails([&]{start_party(f.roster,f);});
     check(!e.contains(32)&&f.roster.members->entities().empty());}
    {Fixture f;f.no_world=true;fails([&]{start_party(f.roster,f);});check(f.collisions.empty());}
    {Fixture f;(*f.roster.formation)[0]=-2;fails([&]{start_party(f.roster,f);});
     check(f.nodes.front().entity.contains(32)&&f.roster.members->entities().size()==1&&f.sorties[0]==0);}
    {Fixture f;f.debug=true;f.auto_party=true;f.no_distance=true;fails([&]{start_party(f.roster,f);});
     check(f.random_queries==8&&f.distance_order.empty()&&f.layout_index==-1&&f.roster.members->entities().size()==1);}
    {Fixture f;f.debug=true;f.auto_party=true;for(auto& r:f.records)r.flags=0;
     fails([&]{start_party(f.roster,f);});check(f.random_queries==8&&f.creations.empty());}
    {Fixture f;auto& e=f.nodes.front().entity;auto& occupied=f.collisions.emplace_back();e.add(32,&occupied);
     add_party_collision(e,12,1,2,3,true,static_cast<PartyStartHost&>(f));check(e.get(32)==&occupied&&f.collisions.back().radius==12);
     e.remove(32);remove_party_collision(e,static_cast<PartyStartHost&>(f));check(f.released.back()==nullptr);}
    {Fixture f;auto& other=f.spawn(1);f.on_record=[&](HumanRecordLink&){other.entity.remove(14);};
     fails([&]{start_party(f.roster,f);});check(f.destroyed.empty()&&f.collisions.empty());}
}catch(const std::exception& e){std::cerr<<"uncaught party start error: "<<e.what()<<'\n';return 1;}
