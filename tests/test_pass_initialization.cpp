#include "core/gameplay/pass_initialization.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host final:CombatEventHost {
    int destroyed_count=0;
    bool destroyed(ComponentSlots&)override{return destroyed_count!=0;}
    void destroy(ComponentSlots&,bool remove)override{check(remove);++destroyed_count;}
    ComponentSlots* search_opponent(ComponentSlots&)override{check(false);return nullptr;}
};
struct World final:PassInitializationWorld {
    std::vector<std::vector<std::int32_t>> requests;
    std::vector<std::int32_t> slots{0,1};SubsetFilter filter{&slots};
    EntitySubset group{filter,[](const ComponentSlots&){return 1;}};
    EntityEvents events;ProjectileSystemState* observing=nullptr;
    bool fail_subset=false,fail_subscription=false,null_subset=false;
    std::size_t fail_request=0;
    EntitySubset* subset(const std::vector<std::int32_t>& all)override{
        requests.push_back(all);if(fail_subset||requests.size()==fail_request)throw std::runtime_error("subset");
        return null_subset?nullptr:&group; // Synthetic world service, not dynamic GetSubSet.
    }
    void subscribe_impact(EntityEvents::Single callback,std::optional<EventHashes> hashes)override{
        check(observing&&observing->pass.has_value());
        check(observing->subset==(null_subset?nullptr:&group));
        if(fail_subscription)throw std::runtime_error("subscribe");events.subscribe(23,std::move(callback),hashes);
    }
};
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct FreshTown final:FreshTownHost {
    std::shared_ptr<TownTable> records=std::make_shared<TownTable>(load_town_table("0\t7\t0\t-1\t1\n1\t8\t1\t1\t99\t0\t0\n2\t9\t-1\t0\t1",false,parse_event_decimal));
    std::int32_t saved=-5;int reads=0,writes=0,rebuilds=0,queries=0;bool fail_read=false,fail_rebuild=false;
    std::shared_ptr<TownTable> town_records()override{++queries;return records;}
    std::int32_t saved_town_count()override{++reads;if(fail_read)throw std::runtime_error("save");return saved;}
    void save_town_count(std::int32_t n)override{++writes;saved=n;}
    void rebuild_town()override{++rebuilds;if(fail_rebuild)throw std::runtime_error("rebuild");}
};
int main(){
    {FreshTown h;TownInitializationState s;s.building_list=std::make_shared<std::vector<EventInstruction>>();
     initialize_fresh_town(s,h);check(*s.build_enable==std::vector<std::uint8_t>({1,0,1}));
     check(h.saved==2&&h.writes==1&&h.reads==1&&h.rebuilds==0&&h.queries==13);
     check(s.building_list->size()==2&&*s.building_list->at(0)==EventIntegers({0,7,0})&&*s.building_list->at(1)==EventIntegers({2,9,0}));
     h.saved=99;h.writes=0;initialize_fresh_town(s,h);check(h.saved==99&&h.writes==0&&s.building_list->size()==2);
     h.fail_read=true;fails([&]{initialize_fresh_town(s,h);});check(s.building_list->size()==2&&s.build_enable->at(2)==1);}
    {FreshTown h;TownInitializationState s;s.building_list=std::make_shared<std::vector<EventInstruction>>();
     h.records->at(1).reset();fails([&]{initialize_fresh_town(s,h);});check(h.reads==0&&s.build_enable->at(0)==1&&s.building_list->size()==1);}
    {FreshTown h;TownInitializationState s;s.building_list=std::make_shared<std::vector<EventInstruction>>();
     for(const auto row:{EventIntegers{4,1,0},EventIntegers{4,2,0},EventIntegers{4,3,0},EventIntegers{5,8,0}})s.building_list->push_back(std::make_shared<EventIntegers>(row));
     set_town_building(s,4,9,5,true,h);check(s.building_list->size()==3&&*s.building_list->at(0)==EventIntegers({4,2,0})&&h.rebuilds==0);
     set_town_building(s,5,8,0,false,h);check(h.rebuilds==1&&s.building_list->size()==3);
     h.fail_rebuild=true;fails([&]{set_town_building(s,6,2,0,false,h);});check(h.rebuilds==2&&s.building_list->size()==4);}
    {TownDefinition r;load_town_definition(r,"3\t8\t0\t-1\t5\textra",parse_event_decimal);auto open=r.open_term;
     check(r.id==3&&r.first_build==8&&open&&open->empty()&&!r.rock_term&&r.flags==5);
     load_town_definition(r,std::nullopt,parse_event_decimal);check(r.open_term==open);
     fails([&]{load_town_definition(r,"4\t9\t1\t1\t77\tbad",parse_event_decimal);});
     check(r.id==4&&r.first_build==9&&r.open_term!=open&&r.open_term->at(0)->at(0)==77&&r.flags==5);}
    {World w;EntitySubset* rotation=&w.group;initialize_rotation(rotation,&w);
     check(rotation==&w.group&&w.requests==std::vector<std::vector<std::int32_t>>{{12,1,2,7}});
     w.fail_subset=true;fails([&]{initialize_rotation(rotation,&w);});check(rotation==&w.group);
     fails([&]{initialize_rotation(rotation,nullptr);});check(rotation==&w.group);
     w.fail_subset=false;w.null_subset=true;initialize_rotation(rotation,&w);check(!rotation);}
    {World w;BattleSystemState battle;initialize_battle(battle,&w);
     check(w.requests==std::vector<std::vector<std::int32_t>>{{20,14},{20,16}});
     check(battle.humans==&w.group&&battle.monsters==&w.group);
     ComponentSlots e(35);Component value;e.add(0,&value);e.add(1,&value);w.group.add(&e);
     battle.monsters=nullptr;update_battle(battle);check(w.group.size()==1&&e.get(0)==&value);
     battle.monsters=&w.group;w.null_subset=true;w.fail_request=w.requests.size()+2;
     fails([&]{initialize_battle(battle,&w);});check(!battle.humans&&battle.monsters==&w.group);
     fails([&]{update_battle(battle);});fails([&]{initialize_battle(battle,nullptr);});
     w.fail_request=0;initialize_battle(battle,&w);check(!battle.humans&&!battle.monsters);}
    World world;Host host;ProjectileSystemState projectile;CollisionSystemState collision;
    EntitySubset* move=nullptr;EntitySubset* garbage=nullptr;world.observing=&projectile;
    initialize_projectiles(projectile,&world,host,EventHashes{1,101});
    initialize_movement(move,&world);initialize_collisions(collision,&world);initialize_garbage(garbage,&world);
    check(world.requests==std::vector<std::vector<std::int32_t>>{{27,0,1},{0,1},{32,0},{22}});
    check(move==&world.group&&garbage==&world.group&&collision.subset==&world.group&&collision.pass);
    ComponentSlots emitter(35);Component tag;emitter.add(31,&tag);
    world.events.send(23,&emitter);check(host.destroyed_count==1);
    // Reinitialization subscribes another receiver; Init does not unsubscribe the first.
    initialize_projectiles(projectile,&world,host,EventHashes{1,102});
    world.events.send(23,&emitter);check(host.destroyed_count==3);
    check(world.events.unsubscribe_single(23,1));world.events.send(23,&emitter);check(host.destroyed_count==4);
    world.fail_subset=true;auto* old=projectile.subset;
    fails([&]{initialize_projectiles(projectile,&world,host);});check(projectile.subset==old&&projectile.pass);
    fails([&]{initialize_movement(move,nullptr);});check(move==&world.group);
    world.fail_subset=false;world.null_subset=true;world.fail_subscription=true;
    fails([&]{initialize_projectiles(projectile,&world,host);});check(!projectile.subset&&projectile.pass);
    initialize_collisions(collision,&world);check(!collision.subset&&collision.pass);
    initialize_garbage(garbage,&world);check(!garbage);
}
