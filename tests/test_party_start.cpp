#include "core/gameplay/party_start.hpp"
#include "core/gameplay/animation_pass.hpp"
#include "core/gameplay/stage_cleanup.hpp"
#include <cstdlib>
#include <deque>
#include <iostream>

using namespace saga::core;
void verify(bool v,int line){if(!v){std::cerr<<"party start check failed at "<<line<<'\n';std::abort();}}
#define check(...) verify((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Node {
    std::int32_t id;ComponentEvents events;ComponentSlots entity{35,&events};QuestEntityLifetime lifetime;
    HumanRecordLink human;ContactPosition position;ContactDirection direction;ProjectileSpeed speed;
    EntitySprite sprite;EntityPlayback playback;CombatParameterLinks parameters;Component tag;
    DamageParameter hp{7,8,0,0},energy{9,10,0,0};DamageParameterMap values{{0,&hp},{1,&energy}};
    explicit Node(std::int32_t n):id(n){parameters.values=&values;playback.state={1,-1};sprite.animation={1,36,0};
        entity.add(0,&position).add(1,&speed).add(2,&sprite).add(10,&playback).add(12,&direction)
              .add(14,&human).add(23,&parameters);}
};
struct Fixture final:PartyStartHost,PartyHumanFactory,PartyDistanceSystem,MemberInitializationHost,
                     HumanStateHost,QuestEntityHost,EntityDeletionHost,ComponentObserver {
    std::deque<Node> nodes;std::deque<ProjectileCollision> collisions;std::vector<Component*> released;
    EntityRegistry registry{*this};DynamicSubsets groups{registry,[&](const ComponentSlots& e){return node(e).id;}};
    PlayerWorldState world{nullptr,&groups};MemberRoster roster;
    std::array<PartyCharacter,12> records;std::array<std::int32_t,12> sorties{};
    PartyCharacters table=std::make_shared<std::vector<PartyCharacter*>>();
    std::shared_ptr<std::vector<std::int32_t>> layout=std::make_shared<std::vector<std::int32_t>>(16,1);
    std::vector<HumanCreation> creations;std::vector<std::int32_t> distance_order;
    std::vector<std::int32_t> random_candidates;
    std::vector<std::string> calls;std::vector<QuestMemberId> destroyed;
    bool debug=false,editor=false,auto_party=false,no_factory=false,no_world=false,no_distance=false;
    int quest_queries=0,editor_queries=0,random_queries=0;std::int32_t layout_index=-1,random_id=1;
    std::function<void()> on_factory_world,on_release;std::function<void(HumanRecordLink&)> on_record;
    std::function<void(const HumanCreation&)> on_create;
    Fixture(){for(std::size_t i=0;i<records.size();++i){records[i].id=static_cast<std::int32_t>(i);records[i].flags=i==0?2:4;
            records[i].saved={4,42};table->push_back(&records[i]);}
        spawn(0).entity.add(30,&nodes.front().tag);(*roster.formation)[10]=0;}
    Node& node(const ComponentSlots& e){for(auto& n:nodes)if(&n.entity==&e)return n;throw std::invalid_argument("entity");}
    Node& spawn(std::int32_t character){nodes.emplace_back(static_cast<std::int32_t>(nodes.size()));auto& n=nodes.back();
        n.human.character_id=character;EntityRegistration r{n.id,&n.entity,&n.events};registry.add(&r,false);return n;}
    void added(ComponentSlots& e,std::int32_t i,Component* c)override{groups.added(e,i,c);}
    void removed(ComponentSlots& e,std::int32_t i,Component* c)override{groups.removed(e,i,c);}
    void changed(ComponentSlots& e,std::int32_t i,Component* a,Component* b)override{groups.changed(e,i,a,b);}
    PlayerWorldState* current_world()override{calls.push_back("world");return no_world?nullptr:&world;}
    QuestEntityHost* destruction_host(PlayerWorldState& w)override{check(&w==&world);calls.push_back("destruction");return this;}
    QuestMemberId entity_id(ComponentSlots& e)override{return static_cast<QuestMemberId>(node(e).id);}
    PartyCharacter* character(HumanRecordLink& h)override{if(on_record)on_record(h);return table->at(static_cast<std::size_t>(h.character_id));}
    PartyCharacters character_table()override{calls.push_back("table");return table;}
    PartyHumanFactory* human_factory_world()override{calls.push_back("factory-world");if(on_factory_world)on_factory_world();return no_factory?nullptr:this;}
    ComponentSlots* create_human(const HumanCreation& args)override{
        calls.push_back("create");creations.push_back(args);if(on_create)on_create(args);auto& n=spawn(args.character);
        n.human.formation_x=args.formation_x;n.human.formation_z=args.formation_z;n.direction.direction=args.direction;
        n.position.x=args.x;n.position.y=args.y;n.position.z=args.z;return &n.entity;}
    ProjectileCollision& acquire_collision()override{calls.push_back("collision");collisions.emplace_back();return collisions.back();}
    void release_collision(Component* c)override{calls.push_back("release");released.push_back(c);if(on_release)on_release();}
    bool debug_enabled()override{calls.push_back("debug");return debug;}
    bool editor_enabled()override{++editor_queries;return editor;}
    bool quest_has_auto_party()override{++quest_queries;return auto_party;}
    PartyCharacter* random_character(const std::vector<PartyCharacter*>& candidates)override{
        if(random_queries==0)for(auto* c:candidates)random_candidates.push_back(c->id);
        ++random_queries;return candidates.empty()?nullptr:&records.at(static_cast<std::size_t>(random_id));}
    PartyDistanceSystem* distance_system(PlayerWorldState&)override{calls.push_back("distance-system");return no_distance?nullptr:this;}
    std::int32_t distance(PartyCharacter* data)override{if(!data)throw std::invalid_argument("distance record");
        distance_order.push_back(data->id);return data->id%3;}
    std::shared_ptr<std::vector<std::int32_t>> auto_formation(std::int32_t index)override{layout_index=index;return layout;}
    MemberInitializationHost& member_initialization()override{calls.push_back("initialize");return *this;}
    std::int32_t formation_left()override{return 100;}
    std::int32_t formation_top()override{return 200;}
    std::int32_t total_parameter(ComponentSlots&,std::int32_t k)override{return k==0?100:30;}
    void change_animation(ComponentSlots& e,std::int32_t clip,bool reset)override{saga::core::change_animation(live_animation(e),clip,reset);}
    HumanStateHost* human_system()override{return this;}
    bool playing_cut_in(ComponentSlots&)override{return false;}
    std::int32_t attack_interval(ComponentSlots&)override{return 10;}
    std::int32_t* sortie_count(HumanRecordLink& h)override{return &sorties.at(static_cast<std::size_t>(h.character_id));}
    QuestEntityLifetime& entity_lifetime(QuestMemberId id)override{return nodes.at(static_cast<std::size_t>(id)).lifetime;}
    void delete_from_manager(QuestMemberId id,bool remove)override{destroyed.push_back(id);delete_entity_from_manager(id,remove,*this);}
    void remove_all_components(QuestMemberId id)override{nodes.at(static_cast<std::size_t>(id)).entity.remove_all();}
    void dispose_entity_events(QuestMemberId id)override{nodes.at(static_cast<std::size_t>(id)).events.dispose();}
    void erase_registration(QuestMemberId id)override{registry.erase(static_cast<std::int32_t>(id));}
};
int main()try{
    {Fixture f;auto& old=f.spawn(1);auto& retained=f.spawn(2);f.records[2].flags=0;
     add_party_collision(f.nodes.front().entity,99,1,2,3,true,f);auto* previous=f.nodes.front().entity.get(32);
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
     add_party_collision(e,99,0,0,0,false,f);fails([&]{start_party(f.roster,f);});
     check(!e.contains(32)&&f.roster.members->entities().empty());}
    {Fixture f;f.no_world=true;fails([&]{start_party(f.roster,f);});check(f.collisions.empty());}
    {Fixture f;(*f.roster.formation)[0]=-2;fails([&]{start_party(f.roster,f);});
     check(f.nodes.front().entity.contains(32)&&f.roster.members->entities().size()==1&&f.sorties[0]==0);}
    {Fixture f;f.debug=true;f.auto_party=true;f.no_distance=true;fails([&]{start_party(f.roster,f);});
     check(f.random_queries==8&&f.distance_order.empty()&&f.layout_index==-1&&f.roster.members->entities().size()==1);}
    {Fixture f;f.debug=true;f.auto_party=true;for(auto& r:f.records)r.flags=0;
     fails([&]{start_party(f.roster,f);});check(f.random_queries==8&&f.creations.empty());}
    {Fixture f;auto& e=f.nodes.front().entity;auto& occupied=f.collisions.emplace_back();e.add(32,&occupied);
     add_party_collision(e,12,1,2,3,true,f);check(e.get(32)==&occupied&&f.collisions.back().radius==12);
     e.remove(32);remove_party_collision(e,f);check(f.released.back()==nullptr);}
    {Fixture f;auto& other=f.spawn(1);f.on_record=[&](HumanRecordLink&){other.entity.remove(14);};
     fails([&]{start_party(f.roster,f);});check(f.destroyed.empty()&&f.collisions.empty());}
}catch(const std::exception& e){std::cerr<<"uncaught party start error: "<<e.what()<<'\n';return 1;}
