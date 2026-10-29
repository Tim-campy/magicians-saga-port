#pragma once
#include "mission_entities.hpp"
#include "core/gameplay/human_initialization.hpp"
#include "core/gameplay/stage_recovery.hpp"
#include "core/gameplay/human_shoot.hpp"
#include "core/gameplay/enemy_impact.hpp"
#include "core/gameplay/enemy_frame.hpp"
#include "core/gameplay/enemy_initialization.hpp"
#include "core/gameplay/pass_initialization.hpp"
#include <ostream>

namespace saga::desktop {
struct MissionRuntimeConfig {std::int32_t fps;bool debug;std::int32_t seed;std::string backend;};
struct MissionHumanScenario {std::int32_t distance,min_z_rate,max_z_rate,attribute,button;};
struct MissionRewardScenario {std::int32_t defeated,remaining,saved_defeat_count;};
struct MissionEnemyScenario {
    std::int32_t yugami,maximum,total,interval,level;
    std::int32_t created,defeated,remaining,loop_counter,section_limit,section_created;
    bool boss_defeated;
    std::vector<std::pair<std::int32_t,std::int32_t>> pool;
};
// Concrete Human Init/frame services. Stage/map entry and player-input producers remain separate.
class MissionHumans final:private core::HumanFrameHost,private core::HumanInitializationWorld,
    private core::HumanImpactHost,private core::HumanContactHost,private core::StagePlacementHost,
    private core::MemberFormationHost,private core::HumanShootHost,
    private core::ProjectileMotionHost,private core::PassInitializationWorld,private core::CombatEventHost,
    private core::EnemyImpactHost,private core::ImpactEffectHost,private core::ContactCollisionHost,
    private core::EcsCombatPresentation,private core::DamagePresentationHost,private core::EnemyDefeatHost,
    private core::EnemyInitializationWorld,private core::EnemyFrameHost {
public:
    MissionHumans(MissionEntities&,const GameplayCatalog&,MissionProgress&,core::StageGeometry&,MissionRuntimeConfig,MissionHumanScenario);
    MissionHumans(const MissionHumans&)=delete;MissionHumans& operator=(const MissionHumans&)=delete;
    void update(std::int32_t pass=0,bool enabled=true);
    void advance_projectiles(std::int32_t pass=0,bool enabled=true);
    void advance_enemies(std::int32_t pass=0,bool enabled=true);
    void set_state(core::ComponentSlots&,std::int32_t,float speed_z);
    std::uint64_t random_draws()const{return random_.draws;}
    std::uint64_t shots()const{return shots_;}
    std::int32_t total_damage()const{return stage_.total_damage;}
    void enable_rewards(MissionRewardScenario);
    void write_reward_report(std::ostream&);
    void enable_enemies(MissionEnemyScenario);
    void write_enemy_report(std::ostream&);
private:
    core::EntitySubset* enemy_subset(const core::EventIntegers& slots)override{return world_.subset(slots);}
    core::EnemyGaugeService* enemy_gauge_system()override{return nullptr;} // SAGA_STUB(mission-enemy-gauge): no HUD gauge renderer.
    core::StageStartState* enemy_stage_system()override{return &stage_;}
    core::MemberRoster* enemy_member_system()override{return &members_;}
    void subscribe_enemy_impact(core::EntityEvents::Single,std::optional<core::EventHashes>)override;
    void subscribe_enemy_contact(core::EntityEvents::Pair,std::optional<core::EventHashes>)override;
    void subscribe_enemy_human_dead(core::EntityEvents::Single,std::optional<core::EventHashes>)override;
    core::EnemyFactoryWorld* current_enemy_factory()override{return &world_.enemy_factory_world();}
    core::PlayerWorldState* enemy_world()override{return &world_.player_world();}
    core::StageGeometry* enemy_map()override{return &geometry_;}
    core::StagePlacementHost& enemy_coordinates()override{return *this;}
    bool target_destroyed(core::ComponentSlots& e)override{return world_.destroyed(e);}
    std::int32_t system_yugami()override;
    core::AnimationModificationPool& enemy_modifications()override{return static_cast<core::ProjectileLaunchHost&>(world_);}
    core::QuestEntityHost* population_destruction_world()override{return &world_;}
    core::QuestMemberId population_entity_id(core::ComponentSlots& e)override{return world_.handle(e);}
    core::RandomSource& population_random()override{return random_;}
    core::WeightedLotteryState& population_lottery()override{return lottery_;}
    core::EnemyRecords population_records()override{return world_.enemy_records();}
    const core::ParameterMaximumLevels& frame_enemy_maximum_levels()override{return world_.enemy_maximum_levels();}
    std::int32_t enemy_frames_per_second()override{return runtime_.fps;}
    void release_enemy_garbage(core::Component* p)override{world_.return_component(22,p);}
    core::GarbageLifetime& acquire_garbage()override{return world_.acquire_garbage();}
    bool collision_blocked(core::ComponentSlots& e)override{return world_.destroyed(e);}
    void change_animation(core::ComponentSlots& e,std::int32_t clip,std::int32_t backup)override{cut_in_animation(e,clip,backup);}
    void effect(std::int32_t clip,std::int32_t image,std::int32_t loop,float x,float y,float z)override{world_.effect({x,y,z},clip,image,loop!=0);}
    void fly_away(core::ComponentSlots&,std::int32_t,std::int32_t,std::int32_t,std::int32_t,bool)override;
    void human_state(core::ComponentSlots& e,std::int32_t state)override{core::change_human_state(e,state,static_cast<core::HumanFrameHost&>(*this));}
    void enemy_state(core::ComponentSlots& e,std::int32_t state)override{core::change_enemy_state(e,state);}
    std::int32_t progress_rate(float)override;
    void contact_sound()override; // SAGA_STUB(mission-contact-audio): output only.
    struct Random final:core::RandomSource {core::LegacySystemRandom source;std::uint64_t draws=0;
        explicit Random(std::int32_t seed):source(seed){}std::int32_t next()override{++draws;return source.next();}};
    const core::CombatRecordTables& human_records()override{return records_;}
    std::int32_t human_parameter(core::ComponentSlots&,std::int32_t)override;
    core::PlayerWorldState* human_world()override{return &world_.player_world();}
    core::PlayerWorldState* recovery_world()override{return human_world();}
    core::StagePlacementHost& human_coordinates()override{return *this;}
    float human_formation_x(core::ComponentSlots& e)override{return core::member_formation_x(&e,*this);}
    float human_formation_z(core::ComponentSlots& e)override{return core::member_formation_z(&e,*this);}
    bool human_target_destroyed(core::ComponentSlots& e)override{return world_.destroyed(e);}
    std::int32_t human_frames_per_second()override{return runtime_.fps;}
    core::RandomSource& human_random()override{return random_;}
    std::int32_t celebration_frame_period()override;
    void remove_human_modification(core::ComponentSlots&)override;
    bool shoot_human(core::ComponentSlots&,core::ComponentSlots&)override;
    bool playing_cut_in(core::ComponentSlots&)override;
    void change_animation(core::ComponentSlots&,std::int32_t,bool)override;
    std::int32_t attack_interval(core::ComponentSlots& e)override{return core::human_attack_interval(system_,&e,*this);}
    core::EntitySubset* human_subset(const core::EventIntegers& slots)override{return world_.subset(slots);}
    core::StageGeometry* human_map()override{return &geometry_;}
    core::StageStartState* human_stage_system()override{return &stage_;}
    core::EnemyCreationSystem* human_enemy_system()override{return &enemies_;}
    core::MemberRoster* human_member_system()override{return &members_;}
    void subscribe_human_impact(core::EntityEvents::Single,std::optional<core::EventHashes>)override;
    void subscribe_human_contact(core::EntityEvents::Pair,std::optional<core::EventHashes>)override;
    bool destroyed(core::ComponentSlots& e)override{return world_.destroyed(e);}
    core::ComponentSlots& hero()override;
    void cut_in_animation(core::ComponentSlots&,std::int32_t,std::int32_t)override;
    void contact_effect(std::int32_t,std::int32_t,std::int32_t,float,float,float)override;
    void impact_effect(std::int32_t,std::int32_t,std::int32_t,float,float,float)override;
    void send_human_dead(core::ComponentSlots&)override;
    core::StageGeometry* stage_map()override{return &geometry_;}
    core::DropPlacementWorld* drop_world()override;
    core::BulletRecords shoot_bullet_records()override{return world_.bullet_records();}
    core::BulletFactoryHost* shoot_bullet_world()override{return &world_;}
    core::DropEffectWorld* shoot_effect_world()override{return &world_.effect_world();}
    core::ProjectileLaunchHost& shoot_launch_host()override{return world_;}
    core::EventInstruction shoot_attack_animations()override{return attack_clips_;}
    void play_attack_sound(bool)override;
    core::EntitySubset* subset(const std::vector<std::int32_t>& slots)override{return world_.subset(slots);}
    void subscribe_impact(core::EntityEvents::Single,std::optional<core::EventHashes>)override;
    void send_impact(core::ComponentSlots&)override;
    void destroy(core::ComponentSlots& e,bool remove)override{world_.destroy(e,remove);}
    core::ComponentSlots* search_opponent(core::ComponentSlots&)override;
    void read_bullet_data(core::ComponentSlots&)override;
    void create_bullet_impact(core::ComponentSlots&,core::ComponentSlots&)override;
    void bullet_hit(core::ComponentSlots&,core::ComponentSlots*)override;
    core::ComponentSlots* create_effect(std::int32_t,std::int32_t,std::int32_t,core::ProjectilePoint)override;
    core::ComponentSlots* create_emitter(std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,core::ProjectilePoint)override;
    void boss_defeated(core::ComponentSlots&)override;
    core::ProjectileLink& acquire_projectile()override{return world_.acquire_projectile();}
    core::ProjectileModification& acquire_modification()override{return world_.acquire_modification();}
    void return_component(std::int32_t slot,core::Component* p)override{world_.return_component(slot,p);}
    void add_animation(core::ComponentSlots& e,std::int32_t rate,std::int32_t backup)override{world_.add_animation(e,rate,backup);}
    const core::BulletImpactData* bullet_data(core::ComponentSlots&)override;
    std::int32_t attribute_value(core::ComponentSlots& e,std::int32_t attribute)override{return static_cast<std::uint32_t>(attribute)<=2U?human_parameter(e,attribute+10):0;}
    core::ComponentSlots* create_entity(std::int32_t type)override{return &world_.raw_entity(type);}
    core::ContactPosition& acquire_position()override{return world_.acquire_position();}
    core::ProjectileCollision& acquire_collision()override{return world_.acquire_collision();}
    core::ProjectileDamage& acquire_damage()override{return world_.acquire_bullet_damage();}
    void add_garbage(core::ComponentSlots& e,std::int32_t frames)override{core::add_garbage_lifetime(e,frames,static_cast<core::EffectFactoryHost&>(world_));}
    void add_color(core::ComponentSlots&,std::int32_t,std::int32_t,std::int32_t,std::int32_t)override;
    bool damage(core::ComponentSlots*,core::ComponentSlots&,std::int32_t,std::int32_t)override;
    void defeat(core::ComponentSlots&)override;
    std::int32_t& saved_defeat_count(const core::EnemyRecord& row)override{return saved_.enemy_defeat_counts.at(row.id);}
    core::ComponentSlots* create_drop(std::int32_t type,std::int32_t value,core::ProjectilePoint at,std::int32_t scale)override{return world_.drop(type,value,at,scale);}
    void impact(core::ComponentSlots& e,core::ComponentSlots* parent)override{core::create_impact_effect(&e,parent,records_,*this);}
    void miss(core::ComponentSlots&)override;
    void effects(core::ComponentSlots&,const core::DamageNumbers&)override;
    void send_damage(core::ComponentSlots& e,std::int32_t amount)override{core::record_ecs_stage_damage(stage_.total_damage,e,amount);}
    void diagnostics(const core::DamageNumbers&)override;
    void damage_labels(core::ComponentSlots&,const core::DamageNumbers&)override;
    void positional_damage_effect(core::ProjectilePoint at)override{world_.effect(at,0,1,false);}
    MissionEntities& world_;MissionProgress& saved_;core::StageGeometry& geometry_;MissionRuntimeConfig runtime_;Random random_;
    core::CombatRecordTables records_;core::EventIntegers characters_,equipment_,hats_,cloth_;
    core::CombatSavedLevels levels_{characters_,equipment_,hats_,cloth_};
    core::StageStartState stage_;core::AttackButtonState button_;core::MemberRoster members_;
    core::EnemyCreationSystem enemies_;core::HumanFrameSystem system_;
    core::HumanEventServices services_{records_,*this,*this};
    core::EnemyEventServices enemy_services_{records_,world_.enemy_maximum_levels(),random_,*this,*this,*this};
    core::ProjectileSystemState projectiles_;core::EventInstruction attack_clips_=core::initial_attack_animation_ids();
    core::AttributeAffinityTables affinities_=core::initial_attribute_affinity_tables();
    std::uint64_t shots_=0,attack_sounds_=0,damage_presentations_=0;
    std::optional<MissionRewardScenario> reward_scenario_;
    std::optional<MissionEnemyScenario> enemy_scenario_;core::WeightedLotteryState lottery_;
};
} // namespace saga::desktop
