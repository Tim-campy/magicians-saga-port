#include "stage_update_fixture.hpp"
#include "core/gameplay/enemy_initialization.hpp"
#include "core/gameplay/pass_initialization.hpp"
#include "core/ecs/update_scheduler.hpp"

template<class T>T& field(ComponentSlots& e,std::int32_t slot){auto* p=dynamic_cast<T*>(e.get(slot));check(p);return *p;}
struct Services final:EnemyImpactHost,ContactCollisionHost,CombatEventHost,EnemyDefeatHost,HumanImpactHost,
                      ProjectileMotionHost,EcsCombatPresentation {
    UpdateFixture& f;EntityEvents events;std::vector<std::string> calls;
    CharacterProgressionTable characters{12};EquipmentCombatTable gear{16},hats{1},cloth{1};
    CombatRecordTables records{characters,*f.enemy_pools.records,gear,hats,cloth};
    EventIntegers character_levels=EventIntegers(12,1),gear_levels=EventIntegers(16,1),hat_levels{1},cloth_levels{1};
    CombatSavedLevels levels{character_levels,gear_levels,hat_levels,cloth_levels};
    ShopParameterTable shop;TableCharacterParameterHost growth{f.maxima,shop};
    CombatConfiguration config;AttributeAffinityTables affinities=initial_attribute_affinity_tables();
    std::array<std::int32_t,3> defeat_counts{};std::deque<ProjectileLink> projectiles;
    std::vector<std::int32_t> returned;std::int32_t searches=0,guards=0,damage_amount=0;
    std::function<void(ComponentSlots&)> on_search;
    EnemyEventServices bindings{records,f.enemy_pools.maxima,f.random,*this,*this,*this};
    explicit Services(UpdateFixture& fixture):f(fixture){
        for(std::size_t i=0;i<characters.size();++i){characters[i].emplace();characters[i]->flags=i==0?2:4;
            characters[i]->parameters=f.records[i].parameters.values;}
        for(auto* table:{&gear,&hats,&cloth})for(auto& record:*table){record.emplace();record->parameters=std::make_shared<std::vector<EventInstruction>>();
            for(int i=0;i<13;++i)record->parameters->push_back(std::make_shared<EventIntegers>(EventIntegers{0,0}));}
        f.enemy_pools.records->at(2)->experience=EventIntegers{10,10};
        f.enemy_pools.records->at(2)->treasures=EnemyTreasureGroups{{}};
    }
    [[noreturn]] static void unbound(){throw std::runtime_error("required presentation/catalog service");}
    bool destroyed(ComponentSlots& e)override{++guards;return f.node(e).lifetime.destroyed;}
    void destroy(ComponentSlots& e,bool remove)override{destroy_quest_entity(f.entity_id(e),remove,static_cast<Fixture&>(f));}
    ComponentSlots* search_opponent(ComponentSlots& e)override{++searches;if(on_search)on_search(e);return search_enemy_target(f.enemy_system,&e,f);}
    ProjectileLink& acquire_projectile()override{projectiles.emplace_back();return projectiles.back();}
    ProjectileModification& acquire_modification()override{return f.acquire_modification();}
    void add_animation(ComponentSlots& e,std::int32_t rate,std::int32_t backup)override{add_animation_component(e,rate,backup,static_cast<EffectFactoryHost&>(f));}
    void return_component(std::int32_t slot,Component*)override{returned.push_back(slot);}
    std::int32_t& saved_defeat_count(const EnemyRecord& r)override{return defeat_counts.at(static_cast<std::size_t>(r.id));}
    ComponentSlots* create_drop(std::int32_t type,std::int32_t value,ProjectilePoint p,std::int32_t scale)override{
        return saga::core::create_drop(type,value,p,scale,f.treasures,f.ids,static_cast<DropFactoryHost&>(f));}
    bool damage(ComponentSlots* attacker,ComponentSlots& receiver,std::int32_t amount,std::int32_t attribute)override{
        if(!attacker)throw std::invalid_argument("required damage attacker");
        EcsDamageHost host(*attacker,receiver,records,levels,growth,f.enemy_pools.maxima,affinities,config,*this);
        return apply_combat_damage(amount,attribute,f.random,host);
    }
    void defeat(ComponentSlots& e)override{defeat_enemy(e,records,f.enemy_system,f.random,*this);}
    void impact(ComponentSlots&,ComponentSlots*)override{unbound();}
    void read_bullet_data(ComponentSlots&)override{unbound();}
    void create_bullet_impact(ComponentSlots&,ComponentSlots&)override{unbound();}
    void bullet_hit(ComponentSlots& source,ComponentSlots* target)override{on_projectile_hit(&source,target,*this);}
    ComponentSlots* create_effect(std::int32_t,std::int32_t,std::int32_t,ProjectilePoint)override{unbound();}
    ComponentSlots* create_emitter(std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,ProjectilePoint)override{unbound();}
    void boss_defeated(ComponentSlots&)override{unbound();}
    bool collision_blocked(ComponentSlots& e)override{return destroyed(e);}
    void cut_in_animation(ComponentSlots& e,std::int32_t clip,std::int32_t backup)override{saga::core::cut_in_animation(live_animation(e),clip,backup);}
    void change_animation(ComponentSlots& e,std::int32_t clip,std::int32_t backup)override{saga::core::cut_in_animation(live_animation(e),clip,backup);}
    void effect(std::int32_t,std::int32_t,std::int32_t,float,float,float)override{calls.push_back("contact-presentation");}
    void fly_away(ComponentSlots& e,std::int32_t a,std::int32_t b,std::int32_t c,std::int32_t d,bool modify)override{
        saga::core::fly_away(e,a,b,c,d,modify,f.random,static_cast<EnemyImpactHost&>(*this));}
    void human_state(ComponentSlots& e,std::int32_t state)override{change_human_state(e,state,*this);}
    void enemy_state(ComponentSlots& e,std::int32_t state)override{change_enemy_state(e,state);}
    std::int32_t progress_rate(float z)override{return stage_progress_rate(f.state,z,f.enemy_coordinates());}
    ComponentSlots& hero()override{return *member_hero(&f.world);}
    void contact_sound()override{calls.push_back("contact-sound");}
    bool playing_cut_in(ComponentSlots& e)override{return f.playing_cut_in(e);}
    void change_animation(ComponentSlots& e,std::int32_t clip,bool reset)override{f.Fixture::change_animation(e,clip,reset);}
    std::int32_t attack_interval(ComponentSlots& e)override{return f.attack_interval(e);}
    void impact_effect(std::int32_t,std::int32_t,std::int32_t,float,float,float)override{calls.push_back("human-presentation");}
    void send_human_dead(ComponentSlots& e)override{events.send(30,&e);}
    void send_impact(ComponentSlots& e)override{events.send(23,&e);}
    void miss(ComponentSlots&)override{calls.push_back("miss");}
    void effects(ComponentSlots&,const DamageNumbers& n)override{damage_amount=n.amount;}
    void send_damage(ComponentSlots& e,std::int32_t amount)override{record_ecs_stage_damage(f.state.total_damage,e,amount);}
    void diagnostics(const DamageNumbers&)override{check(false);}
};
struct World final:EnemyInitializationWorld,PassInitializationWorld {
    UpdateFixture& f;Services& host;EnemyGaugeService gauge;EnemyCreationSystem& state;
    std::vector<std::string> calls;std::string fail;bool null_lookups=false;
    std::function<void(const std::string&)> on_call;
    World(UpdateFixture& fixture,Services& services):f(fixture),host(services),state(f.enemy_system){}
    void step(std::string s){calls.push_back(s);if(on_call)on_call(s);if(fail==s)throw std::runtime_error(s);}
    EntitySubset* enemy_subset(const EventIntegers& all)override{step("subset");check(all==EventIntegers{24});
        return null_lookups?nullptr:&f.groups.get(std::make_shared<ComponentGroupFilter>(all));}
    EnemyGaugeService* enemy_gauge_system()override{step("gauge");return null_lookups?nullptr:&gauge;}
    StageStartState* enemy_stage_system()override{step("stage");return null_lookups?nullptr:&f.state;}
    MemberRoster* enemy_member_system()override{step("members");return null_lookups?nullptr:&f.roster;}
    void subscribe_enemy_impact(EntityEvents::Single callback,std::optional<EventHashes> hash)override{step("subscribe23");
        host.events.subscribe(23,[this,callback=std::move(callback)](ComponentSlots* e){host.calls.push_back("enemy23");callback(e);},hash);}
    void subscribe_enemy_contact(EntityEvents::Pair callback,std::optional<EventHashes> hash)override{step("subscribe29");
        host.events.subscribe(29,[this,callback=std::move(callback)](ComponentSlots* a,ComponentSlots* b){host.calls.push_back("enemy29");callback(a,b);},hash);}
    void subscribe_enemy_human_dead(EntityEvents::Single callback,std::optional<EventHashes> hash)override{step("subscribe30");
        host.events.subscribe(30,[this,callback=std::move(callback)](ComponentSlots* e){host.calls.push_back("enemy30");callback(e);},hash);}
    EntitySubset* subset(const EventIntegers& all)override{return &f.groups.get(std::make_shared<ComponentGroupFilter>(all));}
    void subscribe_impact(EntityEvents::Single callback,std::optional<EventHashes> hash)override{
        host.events.subscribe(23,[this,callback=std::move(callback)](ComponentSlots* e){host.calls.push_back("projectile23");callback(e);},hash);}
    void init(std::optional<std::array<EventHashes,3>> hashes=std::nullopt){initialize_enemies(state,this,host.bindings,hashes);}
};
struct FixtureWithEvents {UpdateFixture f;Services host{f};World world{f,host};};
struct DeathStage final:HumanDeathHost {UpdateFixture& f;explicit DeathStage(UpdateFixture& value):f(value){}
    void game_over(std::int32_t reason)override{stage_game_over(f.state,reason,f);}};
int main()try{
    {FixtureWithEvents t;auto& s=t.f.enemy_system;auto q=std::make_shared<StageQuest>();s.quest=q;
     s.created_enemies=9;s.defeated_enemies=8;s.loop_counter=7;s.remaining_enemies=6;
     auto previous=s.maximum_by_section;t.world.on_call=[&](const std::string& step){if(step=="subscribe23"){
         check(s.subset&&s.gauge==&t.world.gauge&&s.stage==&t.f.state&&s.members==&t.f.roster);
         check(s.maximum_by_section!=previous&&s.maximum_by_section!=s.created_by_section&&*s.maximum_by_section==EventIntegers(10,0)&&*s.created_by_section==EventIntegers(10,0));}};
     t.f.no_world=true; // Init must keep using its supplied world, even with no current global world.
     t.world.init();check(t.world.calls==std::vector<std::string>{"subset","gauge","stage","members","subscribe23","subscribe29","subscribe30"});
     check(s.quest==q&&s.created_enemies==9&&s.defeated_enemies==8&&s.loop_counter==7&&s.remaining_enemies==6);}
    {FixtureWithEvents t;auto old=t.f.enemy_system.subset;fails([&]{initialize_enemies(t.f.enemy_system,nullptr,t.host.bindings);});
     check(t.f.enemy_system.subset==old&&t.world.calls.empty());t.world.null_lookups=true;t.world.init();
     check(!t.f.enemy_system.subset&&!t.f.enemy_system.gauge&&!t.f.enemy_system.stage&&!t.f.enemy_system.members);
     t.host.events.send(23,nullptr);fails([&]{t.host.events.send(30,nullptr);});}
    for(const auto& stop:std::vector<std::string>{"subset","gauge","stage","members","subscribe23","subscribe29","subscribe30"}){
     FixtureWithEvents t;auto& s=t.f.enemy_system;auto old=s.maximum_by_section;auto* old_subset=s.subset;
     s.stage=nullptr;s.members=nullptr;t.world.fail=stop;fails([&]{t.world.init();});
     if(stop=="subset")check(s.subset==old_subset);
     if(stop=="stage"||stop=="members")check(s.gauge==&t.world.gauge);
     if(stop=="members")check(s.stage==&t.f.state&&!s.members);
     const bool subscribed=stop=="subscribe23"||stop=="subscribe29"||stop=="subscribe30";
     check((s.maximum_by_section!=old)==subscribed);
     auto& e=t.f.spawn(1).entity;t.host.events.send(23,&e);
     check(t.host.guards==(stop=="subscribe29"||stop=="subscribe30"?1:0));}
    // Repeat Init replaces arrays but retains subscriptions, including after a later failure.
    {FixtureWithEvents t;std::array<EventHashes,3> hashes{{{1,101},{2,102},{3,103}}};t.world.init(hashes);
     auto first=t.f.enemy_system.maximum_by_section;t.world.init();check(t.f.enemy_system.maximum_by_section!=first);
     auto& e=t.f.spawn(1).entity;t.host.events.send(23,&e);check(t.host.guards==2);
     check(t.host.events.unsubscribe_single(23,1));t.host.events.send(23,&e);check(t.host.guards==3);
     check(t.host.events.unsubscribe_pair(29,2)&&t.host.events.unsubscribe_single(30,3));}
    // Actual collision -> recoil -> live Enemy/Move passes -> walking, without consuming RNG while invincible.
    {FixtureWithEvents t;t.world.init();auto& f=t.f;f.begin();reset_enemy_population(f.enemy_system,f);f.enemy_system.remaining_enemies=0;
     auto& enemy=create_system_enemy(f.enemy_system,{2,2,1,0,{40,0,50}},f);auto* hero=member_hero(&f.world);
     field<HumanRecordLink>(*hero,14).invincible_time=1;f.random.index=0;t.host.events.send(29,&enemy,hero);
     check(field<EnemyRecordLink>(enemy,24).state==3&&field<ProjectileSpeed>(enemy,1).z==-10&&field<ContactPosition>(enemy,0).z==40);
     auto* move=t.world.subset({0,1});bool enabled=true;
     GameplayUpdate enemies(5,enabled,[&](std::int32_t){update_enemies(f.enemy_system,f);});
     GameplayUpdate movement(11,enabled,[&](std::int32_t){move_subset(*move);});
     std::vector<UpdateSystem*> list{&enemies,&movement};UpdateSchedule schedule{&list,nullptr};
     for(int i=0;i<20;++i)update_systems(schedule,1);
     check(field<EnemyRecordLink>(enemy,24).state==3&&field<ContactDirection>(enemy,12).locked);
     update_systems(schedule,1);check(field<EnemyRecordLink>(enemy,24).state==1&&field<EnemyRecordLink>(enemy,24).frame==1);
     check(!field<ContactDirection>(enemy,12).locked&&field<ContactPosition>(enemy,0).z<1&&f.random.index==0);}
    // The registered contact handler uses actual numeric damage, shared defeat counters and real EXP drops.
    {FixtureWithEvents t;t.world.init();auto& f=t.f;f.begin();reset_enemy_population(f.enemy_system,f);
     auto& attacker=create_system_enemy(f.enemy_system,{2,2,1,0,{0,0,50}},f);
     auto& target=create_system_enemy(f.enemy_system,{2,2,1,0,{0,0,50}},f);resolve_combat_parameter(target,0)->value=1;
     auto& source=create_entity(f.ids,0,false,static_cast<Fixture&>(f));ProjectileDamage area;area.parent=&attacker;area.value=1000;area.attribute=0;source.add(33,&area);
     f.random.index=0;f.random.values={0,5,99,0,0,0};t.host.events.send(29,&source,&target);
     check(resolve_combat_parameter(target,0)->value==0&&field<EnemyRecordLink>(target,24).state==4&&target.contains(27));
     check(t.host.damage_amount==433&&f.state.total_damage==433&&f.enemy_system.defeated_enemies==1&&f.enemy_system.remaining_enemies==9);
     check(t.host.defeat_counts[2]==1&&f.random.index==6&&f.nodes.back().drop.type==0&&f.nodes.back().drop.value==10);}
    // Human landing sends nested death before Enemy impact/Projectile cleanup; real target selection observes state 5.
    {FixtureWithEvents t;auto& f=t.f;DeathStage stage(f);
     t.host.events.subscribe(30,[&](ComponentSlots* e){t.host.calls.push_back("stage30");stage_human_dead(*e,t.host.records,stage);});
     t.host.events.subscribe(23,[&](ComponentSlots* e){t.host.calls.push_back("human23");human_projectile_impact(e,t.host);});
     t.world.init();ProjectileSystemState projectile;initialize_projectiles(projectile,&t.world,t.host);
     f.begin();reset_enemy_population(f.enemy_system,f);auto* hero=member_hero(&f.world);
     auto* sub=create_sub_character(0,0,1,f.party_host());add_member(f.roster,sub);
     field<ContactPosition>(*sub,0).x=1000;field<ContactPosition>(*sub,0).z=1000;
     auto& normal=create_system_enemy(f.enemy_system,{2,2,1,0,{0,0,50}},f);
     auto& boss=create_system_enemy(f.enemy_system,{2,2,1,1,{0,0,50}},f);
     field<HumanRecordLink>(*hero,14).state=4;auto& p=field<ContactPosition>(*hero,0);
     launch_projectile(*hero,{p.x,p.y,p.z},{p.x,p.y,p.z},4,nullptr,false,false,200,static_cast<EnemyImpactHost&>(t.host));
     t.host.calls.clear();f.random.index=0;projectile.pass->update(*projectile.subset,t.host);projectile.pass->update(*projectile.subset,t.host);
     check(t.host.calls==std::vector<std::string>{"human23","human-presentation","stage30","enemy30","enemy23","projectile23"});
     check(field<HumanRecordLink>(*hero,14).state==5&&!hero->contains(27)&&t.host.returned.back()==27);
     check(field<EnemyRecordLink>(normal,24).opponent==sub&&field<EnemyRecordLink>(boss,24).opponent==hero&&t.host.searches==2);
     check(f.state.dialog&&f.state.dialog->kind==98&&f.state.dialog->reason==1&&f.random.index==0);}
    // Death reads the current stored subset, and captured-component publication survives replacement during SearchTarget.
    {FixtureWithEvents t;t.world.init();auto& f=t.f;f.begin();reset_enemy_population(f.enemy_system,f);
     auto* dead=member_hero(&f.world);field<HumanRecordLink>(*dead,14).state=5;
     auto* sub=create_sub_character(0,0,1,f.party_host());add_member(f.roster,sub);
     auto& enemy=create_system_enemy(f.enemy_system,{2,2,1,0,{0,0,50}},f);auto& old=field<EnemyRecordLink>(enemy,24);old.opponent=dead;
     EnemyRecordLink replacement;replacement.enemy_id=2;
     t.host.on_search=[&](ComponentSlots& e){e.change(24,&replacement);};t.host.events.send(30,dead);
     check(old.opponent==sub&&!replacement.opponent);
     EntitySubset empty([](ComponentSlots*){return true;},[](const ComponentSlots&){return 0;});f.enemy_system.subset=&empty;
     t.host.events.send(30,dead);check(t.host.searches==1);}
}catch(const std::exception& e){std::cerr<<"uncaught enemy initialization error: "<<e.what()<<'\n';return 1;}
