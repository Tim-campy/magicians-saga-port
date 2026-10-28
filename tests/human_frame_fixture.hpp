#pragma once
#include "combat_stage_fixture.hpp"
#include "core/gameplay/human_initialization.hpp"

struct FrameServices final:Services,HumanFrameHost {
    HumanFrameSystem system;std::int32_t period=12,shots=0,fps_queries=0;
    std::function<std::int32_t(ComponentSlots&,std::int32_t)> parameters;
    std::function<void()> on_fps;
    std::function<bool(ComponentSlots&,ComponentSlots&)> shooter;
    explicit FrameServices(UpdateFixture& fixture):Services(fixture){
        shop.resize(13);for(auto& row:shop)row=ShopParameterUpgrade{};
        system.transitions=static_cast<HumanFrameHost*>(this);f.state.humans=&system;
    }
    const CombatRecordTables& human_records()override{return records;}
    std::int32_t human_parameter(ComponentSlots& e,std::int32_t key)override{
        return parameters?parameters(e,key):ecs_member_total_parameter(e,key,records,levels,growth);
    }
    PlayerWorldState* human_world()override{return f.enemy_world();}
    StagePlacementHost& human_coordinates()override{return f.enemy_coordinates();}
    float human_formation_x(ComponentSlots& e)override{return member_formation_x(&e,f);}
    float human_formation_z(ComponentSlots& e)override{return member_formation_z(&e,f);}
    bool human_target_destroyed(ComponentSlots& e)override{return f.target_destroyed(e);}
    std::int32_t human_frames_per_second()override{++fps_queries;if(on_fps)on_fps();return f.fps;}
    RandomSource& human_random()override{return f.random;}
    std::int32_t celebration_frame_period()override{return period;}
    void remove_human_modification(ComponentSlots& e)override{remove_recovery_modification(e,f);}
    bool shoot_human(ComponentSlots& e,ComponentSlots& target)override{++shots;if(!shooter)unbound();return shooter(e,target);}
    bool playing_cut_in(ComponentSlots& e)override{return Services::playing_cut_in(e);}
    void change_animation(ComponentSlots& e,std::int32_t clip,bool reset)override{Services::change_animation(e,clip,reset);}
    std::int32_t attack_interval(ComponentSlots& e)override{return human_attack_interval(system,&e,*this);}
};
struct HumanWorld final:HumanInitializationWorld {
    UpdateFixture& f;FrameServices& host;std::vector<std::string> calls;std::string fail;bool null_results=false;
    HumanEventServices services{host.records,host,host};
    HumanWorld(UpdateFixture& fixture,FrameServices& value):f(fixture),host(value){}
    void step(std::string s){calls.push_back(s);if(fail==s)throw std::runtime_error(s);}
    EntitySubset* human_subset(const EventIntegers& all)override{step("subset");check(all==EventIntegers{14});
        return null_results?nullptr:&f.groups.get(std::make_shared<ComponentGroupFilter>(all));}
    StageGeometry* human_map()override{step("map");return null_results?nullptr:f.active_map;}
    StageStartState* human_stage_system()override{step("stage");return null_results?nullptr:&f.state;}
    EnemyCreationSystem* human_enemy_system()override{step("enemies");return null_results?nullptr:&f.enemy_system;}
    MemberRoster* human_member_system()override{step("members");return null_results?nullptr:&f.roster;}
    void subscribe_human_impact(EntityEvents::Single callback,std::optional<EventHashes> hash)override{
        step("subscribe23");host.events.subscribe(23,[this,callback=std::move(callback)](ComponentSlots* e){host.calls.push_back("human23");callback(e);},hash);}
    void subscribe_human_contact(EntityEvents::Pair callback,std::optional<EventHashes> hash)override{
        step("subscribe29");host.events.subscribe(29,[this,callback=std::move(callback)](ComponentSlots* a,ComponentSlots* b){host.calls.push_back("human29");callback(a,b);},hash);}
    void init(std::optional<std::array<EventHashes,2>> hashes=std::nullopt){initialize_humans(host.system,this,services,hashes);}
};
struct HumanFixture {
    UpdateFixture f;FrameServices host{f};HumanWorld world{f,host};World enemy_world{f,host};
    HumanFixture(){world.init();enemy_world.init();f.begin();reset_enemy_population(f.enemy_system,f);f.enemy_system.remaining_enemies=0;f.random.index=0;}
    ComponentSlots& hero(){return *member_hero(&f.world);}
    ComponentSlots& enemy_at(ProjectilePoint p={100,0,140},std::int32_t flags=0){return create_system_enemy(f.enemy_system,{2,2,1,flags,p},f);}
    ComponentSlots& sub(){auto* e=create_sub_character(36,0,1,f.party_host());add_member(f.roster,e);return *e;}
    void update(){update_humans(host.system,host);}
    void no_attack(){for(auto* button:*f.state.attack_buttons)button->state=0;}
    void flat_parameters(std::int32_t value=0){host.parameters=[value](ComponentSlots&,std::int32_t){return value;};}
};
