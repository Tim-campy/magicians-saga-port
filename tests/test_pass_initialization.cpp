#include "core/gameplay/pass_initialization.hpp"
#include <cstdlib>
#include <functional>
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
    std::function<void()> on_query,on_read;
    std::shared_ptr<TownTable> town_records()override{++queries;if(on_query)on_query();return records;}
    std::int32_t saved_town_count()override{++reads;if(on_read)on_read();if(fail_read)throw std::runtime_error("save");return saved;}
    void save_town_count(std::int32_t n)override{++writes;saved=n;}
    void rebuild_town()override{++rebuilds;if(fail_rebuild)throw std::runtime_error("rebuild");}
};
template<class Exception,class F>void fails_with(F f){bool caught=false;try{f();}catch(const Exception&){caught=true;}check(caught);}
EventInstruction building_row(EventIntegers values){return std::make_shared<EventIntegers>(std::move(values));}
TownInitializationState town_state(EventScript source){
    TownInitializationState state;state.building_list=std::move(source);
    state.build_enable=std::make_shared<std::vector<std::uint8_t>>(std::initializer_list<std::uint8_t>{0,1});return state;
}
struct PermissionConditions final:EventConditionHost {
    std::vector<std::string>& calls;std::int64_t money=5;std::function<void()> on_money;
    explicit PermissionConditions(std::vector<std::string>& trace):calls(trace){}
    void require_condition_context()override{calls.push_back("require");}
    std::int64_t scalar(ConditionScalar kind)override{check(kind==ConditionScalar::Money);calls.push_back("money");if(on_money)on_money();return money;}
    const ConditionCatalog<QuestConditionState>* quests()override{throw std::logic_error("unexpected quests");}
    std::int32_t rescue_adjustment()override{throw std::logic_error("unexpected rescue");}
    const ConditionCatalog<std::uint32_t>* town_flags()override{throw std::logic_error("unexpected towns");}
    std::int32_t town_capacity()override{throw std::logic_error("unexpected capacity");}
    const ConditionCatalog<std::int32_t>* area_states()override{throw std::logic_error("unexpected areas");}
    const ConditionCatalog<CharacterConditionState>* characters()override{throw std::logic_error("unexpected characters");}
    EventScript buildings()override{throw std::logic_error("unexpected buildings");}
    bool evaluate_other_condition(const EventInstruction&)override{throw std::logic_error("unexpected condition");}
};
struct Permissions final:TownPermissionHost {
    std::vector<std::string> calls;
    std::shared_ptr<TownTable> records=std::make_shared<TownTable>(1);
    std::shared_ptr<EventConditionHost> context;
    std::shared_ptr<PermissionConditions> conditions=std::make_shared<PermissionConditions>(calls);
    std::function<void()> on_query,on_context;int queries=0;
    Permissions(){records->at(0).emplace();records->at(0)->open_term=std::make_shared<std::vector<EventInstruction>>(1,building_row({1,5,1}));context=conditions;}
    std::shared_ptr<TownTable> town_records()override{calls.push_back("catalog");++queries;if(on_query)on_query();return records;}
    std::shared_ptr<EventConditionHost> condition_context()override{calls.push_back("context");if(on_context)on_context();return context;}
};
void test_town_permissions(){
    {Permissions h;auto state=town_state(nullptr);state.build_enable.reset();
     fails_with<std::invalid_argument>([&]{update_town_permission(state,-1,h);});
     fails_with<std::invalid_argument>([&]{town_permission(state,0);});check(h.calls.empty());
     state.build_enable=std::make_shared<std::vector<std::uint8_t>>(1,std::uint8_t{0});
     for(const auto index:{-1,1}){fails_with<std::out_of_range>([&]{update_town_permission(state,index,h);});
         fails_with<std::out_of_range>([&]{town_permission(state,index);});}check(h.calls.empty());}
    for(const auto byte:{std::uint8_t{1},std::uint8_t{2},std::uint8_t{255}}){
        Permissions h;auto state=town_state(nullptr);state.build_enable->at(0)=byte;h.records.reset();h.context.reset();
        check(town_permission(state,0)==byte&&!update_town_permission(state,0,h)&&h.calls.empty());
    }
    for(const bool empty:{false,true}){
        Permissions h;auto state=town_state(nullptr);h.context.reset();
        h.records->at(0)->open_term=empty?std::make_shared<std::vector<EventInstruction>>():nullptr;
        const auto old=state.build_enable;check(update_town_permission(state,0,h));
        check(state.build_enable==old&&old->at(0)==0&&h.calls==std::vector<std::string>{"catalog"});
    }
    {Permissions h;auto state=town_state(nullptr);h.conditions->money=4;
     check(!update_town_permission(state,0,h)&&state.build_enable->at(0)==0);
     check(h.calls==std::vector<std::string>({"catalog","context","catalog","require","money"}));}
    {Permissions h;auto state=town_state(nullptr);const auto old=state.build_enable;
     h.conditions->on_money=[&]{state.build_enable=std::make_shared<std::vector<std::uint8_t>>(1,2);};
     check(update_town_permission(state,0,h)&&state.build_enable->at(0)==1&&old->at(0)==0);
     check(h.calls==std::vector<std::string>({"catalog","context","catalog","require","money"}));}
    // The second lookup supplies the tested terms, and captures a different table.
    {Permissions h;auto state=town_state(nullptr);const auto old=h.records;
     h.on_context=[&]{h.records=std::make_shared<TownTable>(*old);h.records->at(0)->open_term=std::make_shared<std::vector<EventInstruction>>(1,building_row({1,6,1}));};
     check(!update_town_permission(state,0,h)&&state.build_enable->at(0)==0);
     check(old->at(0)->open_term->at(0)->at(1)==5&&h.queries==2);}
    // Capture the context owner before the second catalog query replaces it.
    {Permissions h;auto state=town_state(nullptr);auto replacement=std::make_shared<PermissionConditions>(h.calls);replacement->money=0;
     h.on_query=[&]{if(h.queries==2)h.context=replacement;};
     check(update_town_permission(state,0,h)&&state.build_enable->at(0)==1);}
    for(const bool empty:{false,true}){
        Permissions h;auto state=town_state(nullptr);
        h.on_query=[&]{if(h.queries==2)h.records->at(0)->open_term=empty?std::make_shared<std::vector<EventInstruction>>():nullptr;};
        check(!update_town_permission(state,0,h)&&state.build_enable->at(0)==0);
        check(h.calls==std::vector<std::string>({"catalog","context","catalog"}));
    }
    // Missing context is checked after the second catalog's bounds and row.
    for(const int failure:{0,1,2,3}){
        Permissions h;auto state=town_state(nullptr);h.context.reset();
        h.on_query=[&]{if(h.queries==2){
            if(failure==0)h.records.reset();else if(failure==1)h.records->clear();
            else if(failure==2)h.records->at(0).reset();else h.records->at(0)->open_term.reset();}};
        if(failure==1)fails_with<std::out_of_range>([&]{update_town_permission(state,0,h);});
        else fails_with<std::invalid_argument>([&]{update_town_permission(state,0,h);});
        check(state.build_enable->at(0)==0&&h.calls==std::vector<std::string>({"catalog","context","catalog"}));
    }
    // Requiring a zero-length replacement still fails after a successful term;
    // earlier callback mutations remain visible, and the old array is untouched.
    for(const bool absent:{false,true}){
        Permissions h;auto state=town_state(nullptr);const auto old=state.build_enable;
        h.conditions->on_money=[&]{old->at(1)=7;state.build_enable=absent?nullptr:std::make_shared<std::vector<std::uint8_t>>();};
        if(absent)fails_with<std::invalid_argument>([&]{update_town_permission(state,0,h);});
        else fails_with<std::out_of_range>([&]{update_town_permission(state,0,h);});
        check(old->at(0)==0&&old->at(1)==7);
    }
    for(const int failure:{0,1,2}){
        Permissions h;auto state=town_state(nullptr);
        if(failure==0)h.records.reset();else if(failure==1)h.records->clear();else h.records->at(0).reset();
        fails([&]{update_town_permission(state,0,h);});check(h.calls==std::vector<std::string>{"catalog"});
    }
}
void test_town_building_cache(){
    {FreshTown h;h.saved=4;
     auto first=building_row({2,8,3,99}),zero=building_row({0,4,5}),last=building_row({2,9,6});
     auto source=std::make_shared<std::vector<EventInstruction>>(std::initializer_list<EventInstruction>{first,zero,last});
     auto s=town_state(source);const auto enabled=s.build_enable;EventScript cache;
     const auto result=get_town_buildings(s,cache,h);
     check(result==cache&&cache->size()==4&&!cache->at(1)&&!cache->at(3));
     check(cache->at(0)!=zero&&cache->at(2)!=first&&cache->at(2)!=last);
     check(*cache->at(0)==EventIntegers({0,4,5})&&*cache->at(2)==EventIntegers({2,9,6}));
     first->at(1)=80;last->at(2)=60;cache->at(0)->at(1)=40;
     check(cache->at(2)->at(2)==6&&zero->at(1)==4);
     source->push_back(building_row({1,10,11}));h.fail_read=true;h.records.reset();
     check(get_town_buildings(s,cache,h)==result&&!cache->at(1));
     check(h.reads==1&&h.queries==0&&h.writes==0&&h.rebuilds==0);
     check(s.building_list==source&&s.build_enable==enabled&&*enabled==std::vector<std::uint8_t>({0,1}));}
    // A failed late row leaves the already published cache usable, without retrying its source.
    for(const auto bad:std::vector<EventInstruction>{nullptr,building_row({}),building_row({0}),building_row({0,7}),
                                                  building_row({-1,7,8}),building_row({3,7,8})}){
        FreshTown h;h.saved=3;auto prefix=building_row({1,4,5});
        auto source=std::make_shared<std::vector<EventInstruction>>(std::initializer_list<EventInstruction>{prefix,bad});
        auto s=town_state(source);EventScript cache;
        fails([&]{get_town_buildings(s,cache,h);});
        check(cache&&cache->size()==3&&!cache->at(0)&&!cache->at(2));
        check(cache->at(1)!=prefix&&*cache->at(1)==EventIntegers({1,4,5}));
        check(s.building_list==source&&source->size()==2&&source->at(1)==bad);
        h.fail_read=true;check(get_town_buildings(s,cache,h)==cache);
        check(h.reads==1&&h.queries==0&&h.writes==0&&h.rebuilds==0);
    }
    {FreshTown h;h.saved=-1;auto s=town_state(std::make_shared<std::vector<EventInstruction>>());EventScript cache;
     fails_with<std::length_error>([&]{get_town_buildings(s,cache,h);});check(!cache&&h.reads==1&&h.queries==0);
     h.saved=1;h.fail_read=true;fails_with<std::runtime_error>([&]{get_town_buildings(s,cache,h);});check(!cache&&h.reads==2);}
    {FreshTown h;h.saved=0;auto s=town_state(nullptr);EventScript cache;
     fails_with<std::invalid_argument>([&]{get_town_buildings(s,cache,h);});check(cache&&cache->empty()&&h.reads==1);}
    {FreshTown h;h.saved=0;auto source=std::make_shared<std::vector<EventInstruction>>();auto s=town_state(source);EventScript cache;
     const auto empty=get_town_buildings(s,cache,h);check(empty==cache&&cache&&cache->empty()&&h.reads==1);
     cache.reset();source->push_back(building_row({0,1,2}));
     fails_with<std::out_of_range>([&]{get_town_buildings(s,cache,h);});check(cache&&cache->empty()&&h.reads==2);}
    {FreshTown h;h.saved=4;TownDefinition extra;extra.id=3;extra.first_build=42;h.records->push_back(extra);
     auto zero=building_row({0,70,5,999}),two=building_row({2,90,7}),last=building_row({2,91,8});
     auto source=std::make_shared<std::vector<EventInstruction>>(std::initializer_list<EventInstruction>{zero,two,last});
     auto s=town_state(source);const auto enabled=s.build_enable;
     EventScript cache=std::make_shared<std::vector<EventInstruction>>(1,building_row({9,9,9}));const auto old=cache;
     h.on_read=[&]{check(!cache);};
     h.on_query=[&]{check(!cache&&source->size()==static_cast<std::size_t>(h.queries+2));};
     reset_town_buildings(s,cache,h);
     check(!cache&&old->size()==1&&*old->at(0)==EventIntegers({9,9,9}));
     check(s.building_list==source&&source->size()==5&&source->at(0)==zero&&source->at(1)==two&&source->at(2)==last);
     check(*zero==EventIntegers({0,70,5,999})&&*two==EventIntegers({2,90,7})&&*last==EventIntegers({2,91,8}));
     check(*source->at(3)==EventIntegers({1,8,0})&&*source->at(4)==EventIntegers({3,42,0}));
     check(s.build_enable==enabled&&*enabled==std::vector<std::uint8_t>({0,1}));
     check(h.reads==1&&h.queries==2&&h.writes==0&&h.rebuilds==0);
     h.on_read={};h.on_query={};const auto projection=get_town_buildings(s,cache,h);
     check(projection->size()==4&&*projection->at(2)==EventIntegers({2,91,8}));
     check(projection->at(0)!=zero&&projection->at(0)->size()==3);
     h.on_read=[&]{check(!cache);};const auto before=*source;
     reset_town_buildings(s,cache,h);
     check(!cache&&*source==before&&h.reads==3&&h.queries==2&&h.writes==0&&h.rebuilds==0);}
    // Reset invalidates the old cache even if reading capacity or allocating the local view fails.
    for(const bool fail_read:{false,true}){
        FreshTown h;h.saved=-1;h.fail_read=fail_read;
        auto source=std::make_shared<std::vector<EventInstruction>>(1,building_row({0,7,8}));auto s=town_state(source);
        EventScript cache=std::make_shared<std::vector<EventInstruction>>(1,source->at(0));const auto old=cache;
        h.on_read=[&]{check(!cache);};fails([&]{reset_town_buildings(s,cache,h);});
        check(!cache&&old->at(0)==source->at(0)&&source->size()==1);
        check(h.reads==1&&h.queries==0&&h.writes==0&&h.rebuilds==0);
    }
    // All source rows are copied before Reset starts filling gaps; copy failure cannot repair any gap.
    for(const auto bad:std::vector<EventInstruction>{nullptr,building_row({}),building_row({0}),building_row({0,7}),
                                                  building_row({-1,7,8}),building_row({3,7,8})}){
        FreshTown h;h.saved=3;auto prefix=building_row({1,4,5});
        auto source=std::make_shared<std::vector<EventInstruction>>(std::initializer_list<EventInstruction>{prefix,bad});
        auto s=town_state(source);EventScript cache=std::make_shared<std::vector<EventInstruction>>(1,prefix);const auto old=cache;
        fails([&]{reset_town_buildings(s,cache,h);});
        check(!cache&&old->at(0)==prefix&&source->size()==2&&source->at(0)==prefix&&source->at(1)==bad);
        check(h.reads==1&&h.queries==0&&h.writes==0&&h.rebuilds==0);
    }
    {FreshTown h;h.saved=3;h.records->at(1).reset();
     auto existing=building_row({2,90,7});auto source=std::make_shared<std::vector<EventInstruction>>(1,existing);
     auto s=town_state(source);const auto enabled=s.build_enable;EventScript cache=std::make_shared<std::vector<EventInstruction>>(1,existing);
     h.on_query=[&]{check(!cache);};fails_with<std::invalid_argument>([&]{reset_town_buildings(s,cache,h);});
     check(!cache&&source->size()==2&&source->at(0)==existing&&*source->at(1)==EventIntegers({0,7,0}));
     check(s.build_enable==enabled&&*enabled==std::vector<std::uint8_t>({0,1}));
     check(h.reads==1&&h.queries==2&&h.writes==0&&h.rebuilds==0);}
    {FreshTown h;h.saved=1;h.records.reset();auto source=std::make_shared<std::vector<EventInstruction>>();auto s=town_state(source);EventScript cache;
     fails_with<std::invalid_argument>([&]{reset_town_buildings(s,cache,h);});
     check(!cache&&source->empty()&&h.reads==1&&h.queries==1&&h.writes==0&&h.rebuilds==0);}
    {FreshTown h;h.saved=4;auto existing=building_row({2,90,7});
     auto source=std::make_shared<std::vector<EventInstruction>>(1,existing);auto s=town_state(source);EventScript cache;
     fails_with<std::out_of_range>([&]{reset_town_buildings(s,cache,h);});
     check(!cache&&source->size()==3&&source->at(0)==existing);
     check(*source->at(1)==EventIntegers({0,7,0})&&*source->at(2)==EventIntegers({1,8,0}));
     check(h.reads==1&&h.queries==3&&h.writes==0&&h.rebuilds==0);}
    {FreshTown h;h.saved=0;auto s=town_state(std::make_shared<std::vector<EventInstruction>>());
     EventScript cache=std::make_shared<std::vector<EventInstruction>>();reset_town_buildings(s,cache,h);
     check(!cache&&h.reads==1&&h.queries==0&&h.writes==0&&h.rebuilds==0);}
    {FreshTown h;h.saved=0;auto s=town_state(nullptr);EventScript cache=std::make_shared<std::vector<EventInstruction>>();
     fails_with<std::invalid_argument>([&]{reset_town_buildings(s,cache,h);});
     check(!cache&&h.reads==1&&h.queries==0&&h.writes==0&&h.rebuilds==0);}
}
int main(){
    test_town_permissions();
    test_town_building_cache();
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
