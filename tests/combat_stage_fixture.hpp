#pragma once
#include "stage_update_fixture.hpp"
#include "core/gameplay/enemy_initialization.hpp"
#include "core/gameplay/pass_initialization.hpp"
#include "core/ecs/update_scheduler.hpp"
#include "core/gameplay/human_contact.hpp"

template<class T>T& field(ComponentSlots& e,std::int32_t slot){auto* p=dynamic_cast<T*>(e.get(slot));check(p);return *p;}
struct Services:EnemyImpactHost,ContactCollisionHost,CombatEventHost,EnemyDefeatHost,HumanImpactHost,HumanContactHost,
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
    std::function<void(ComponentSlots&,ComponentSlots*)> on_impact;
    std::function<void(ComponentSlots&)> on_read_bullet;
    std::function<void(ComponentSlots&,ComponentSlots&)> on_bullet_impact;
    std::function<ComponentSlots*(std::int32_t,std::int32_t,std::int32_t,ProjectilePoint)> on_create_effect;
    std::function<ComponentSlots*(std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,ProjectilePoint)> on_create_emitter;
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
    void impact(ComponentSlots& e,ComponentSlots* parent)override{if(on_impact)on_impact(e,parent);else unbound();}
    void read_bullet_data(ComponentSlots& e)override{if(on_read_bullet)on_read_bullet(e);else unbound();}
    void create_bullet_impact(ComponentSlots& e,ComponentSlots& parent)override{if(on_bullet_impact)on_bullet_impact(e,parent);else unbound();}
    void bullet_hit(ComponentSlots& source,ComponentSlots* target)override{on_projectile_hit(&source,target,*this);}
    ComponentSlots* create_effect(std::int32_t a,std::int32_t b,std::int32_t c,ProjectilePoint p)override{
        if(on_create_effect)return on_create_effect(a,b,c,p);unbound();}
    ComponentSlots* create_emitter(std::int32_t a,std::int32_t b,std::int32_t c,std::int32_t d,std::int32_t e,ProjectilePoint p)override{
        if(on_create_emitter)return on_create_emitter(a,b,c,d,e,p);unbound();}
    void boss_defeated(ComponentSlots&)override{unbound();}
    bool collision_blocked(ComponentSlots& e)override{return destroyed(e);}
    void cut_in_animation(ComponentSlots& e,std::int32_t clip,std::int32_t backup)override{saga::core::cut_in_animation(live_animation(e),clip,backup);}
    void change_animation(ComponentSlots& e,std::int32_t clip,std::int32_t backup)override{saga::core::cut_in_animation(live_animation(e),clip,backup);}
    void effect(std::int32_t,std::int32_t,std::int32_t,float,float,float)override{calls.push_back("contact-presentation");}
    void fly_away(ComponentSlots& e,std::int32_t a,std::int32_t b,std::int32_t c,std::int32_t d,bool modify)override{
        saga::core::fly_away(e,a,b,c,d,modify,f.random,static_cast<EnemyImpactHost&>(*this));}
    void human_state(ComponentSlots& e,std::int32_t state)override{change_human_state(e,state,static_cast<HumanImpactHost&>(*this));}
    void enemy_state(ComponentSlots& e,std::int32_t state)override{change_enemy_state(e,state);}
    std::int32_t progress_rate(float z)override{return stage_progress_rate(f.state,z,f.enemy_coordinates());}
    ComponentSlots& hero()override{return *member_hero(&f.world);}
    void contact_effect(std::int32_t,std::int32_t,std::int32_t,float,float,float)override{calls.push_back("human-contact-presentation");}
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
