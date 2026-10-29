#pragma once
#include "mission_entities.hpp"
#include "core/gameplay/human_initialization.hpp"
#include "core/gameplay/stage_recovery.hpp"
#include "core/gameplay/human_shoot.hpp"
#include "core/gameplay/enemy_impact.hpp"
#include "core/gameplay/enemy_frame.hpp"
#include "core/gameplay/enemy_initialization.hpp"
#include "core/gameplay/stage_update.hpp"
#include "mission_results.hpp"
#include "mission_stage.hpp"
#include "mission_input.hpp"
#include "core/gameplay/pass_initialization.hpp"
#include "core/gameplay/hero_bootstrap.hpp"
#include <ostream>
#include <stdexcept>

namespace saga::desktop {
struct MissionRuntimeConfig {std::int32_t fps;bool debug;std::int32_t seed;std::string backend;};
struct MissionHumanScenario {
    std::int32_t distance,min_z_rate,max_z_rate,attribute,button;
    std::shared_ptr<core::StageQuest> quest; // Selected static table plus explicit saved state.
    bool stage_entry=false; // Fresh roster for native Start; selected mid-mission actors otherwise.
};
struct MissionRewardScenario {std::int32_t defeated,remaining,saved_defeat_count;};
struct MissionNotificationScenario {
    std::int32_t state,frame;bool retried;std::int32_t quest_flags;bool quest_completed;
    std::uint32_t held_keys;std::int32_t retry_answer;
};
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
    private core::EnemyInitializationWorld,private core::EnemyFrameHost,
    private core::HumanDeathHost,private core::StageGameOverHost,private core::BossNotificationHost,
    private core::StageFormStack,private core::VictoryFormStack,private core::StageInitializationWorld,
    private core::HeroBootstrapHost,private core::PartyHumanFactory {
public:
    MissionHumans(MissionEntities&,const GameplayCatalog&,MissionProgress&,core::StageGeometry&,MissionRuntimeConfig,MissionHumanScenario,bool defer_init=false,core::EnemyGaugeService* gauge=nullptr);
    void initialize_system(std::int32_t factory,core::InitializationWorld* argument);
    void apply_initialized_scenario();
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
    void enable_notifications(MissionNotificationScenario);
    void write_notification_report(std::ostream&);
    void enable_collection(); // Explicit fresh accumulator/list selection, not save restoration or full Stage.Start.
    void advance_collection(std::int32_t pass=0,bool enabled=true);
    void write_collection_report(std::ostream&);
    void enable_results(MissionResultScenario);
    void enable_stage_loop(MissionStageScenario);
    void start_mission(std::int32_t,const core::EventIntegers&,core::StageMapLoader&,
                       std::function<bool()>,std::function<void()>);
    bool advance_stage(std::int32_t pass=0,bool enabled=true);
    bool transition_pending()const;
    void write_result_report(std::ostream&);
    std::shared_ptr<core::StageQuest> quest(){return stage_.quest;}
    core::StageQuestForm& quest_form(){return quest_form_;}
    void enable_input(MissionInputScenario);
    bool input_enabled()const{return static_cast<bool>(input_);}
    void advance_input(std::int32_t pass=0,bool enabled=true);
    void set_input_keys(std::uint32_t);
    void set_input_movement(bool up,bool down){set_input_keys((held_keys_&~0xa0000U)|(up?0x20000U:0U)|(down?0x80000U:0U));}
    void select_input_attack(std::int32_t);
    void observe_input_touch(std::function<void(const core::InputTouch*)>);
    void send_logical_touch(std::int32_t type,std::int32_t group,std::int32_t value);
    std::int32_t selected_attack()const{return stage_.attack_attribute;}
    void write_input_report(std::ostream&);
    bool stage_input_enabled()const{return static_cast<bool>(stage_loop_);}
    void pulse_stage_keys(std::uint32_t);
    void bind_stage_pause(std::function<void(bool)>);
    void construct_member_roster(){members_=core::MemberRoster{};} // MemberSystem ctor post-base lists/formation.
    std::int32_t stage_state()const{return stage_.state;}
    bool exit_dialog_pending()const;
    void answer_stage_dialog(std::int32_t);
private:
    core::StageHumanStart* initialization_humans()override{return &system_;}
    core::MemberRoster* initialization_members()override{return &members_;}
    core::StageEnemyStart* initialization_enemies()override;
    void subscribe_stage_death(std::function<void(core::ComponentSlots*)> f)override{world_.events().subscribe(30,std::move(f));}
    void subscribe_stage_touch(std::function<void(const core::InputTouch*)> f)override{if(!input_)throw std::logic_error("Stage Init requires Input owner");input_->observe_touch(std::move(f));}
    void subscribe_stage_damage(std::function<void(core::ComponentSlots*,std::int32_t)> f)override{
        auto previous=damage_receiver_;damage_receiver_=[previous=std::move(previous),f=std::move(f)](auto* e,auto amount){if(previous)previous(e,amount);f(e,amount);};
    }
    core::PartyCharacters character_table()override{return world_.character_table();}
    core::PlayerWorldState* current_world()override{return &world_.player_world();}
    core::PartyHumanFactory* human_factory_world()override{return this;}
    core::PartyHumanFactory* world_factory(core::PlayerWorldState&)override{return this;}
    core::Component* player_marker()override{return world_.shared_player_marker();}
    core::PartyCharacter* character(core::HumanRecordLink& h)override{return world_.character_table()->at(static_cast<std::size_t>(h.character_id));}
    core::ComponentSlots* create_human(const core::HumanCreation& args)override{return &world_.human(args);}
    friend class MissionResults;friend class MissionStage;
    bool table_quest_=false;
    friend class MissionInput;
    struct CollectionHost final:core::DropCollectionHost {
        MissionHumans& owner;
        explicit CollectionHost(MissionHumans& value):owner(value){}
        core::ComponentSlots* front_line_human()override;
        core::EntitySubset* drop_subset()override{return owner.world_.subset({34});}
        const core::TreasureRecord& treasure(std::int32_t id)override{return owner.world_.treasure(id);}
        core::ComponentSlots* hero()override{return &owner.hero();}
        core::DropEffectWorld* effect_world()override{return &owner.world_.effect_world();}
        core::GarbageLifetime& acquire_garbage()override{return owner.world_.acquire_garbage();}
        void play_collection_sound()override{++owner.collection_sounds_;} // SAGA_STUB(mission-collection-audio).
        void play_reward_jingle()override{++owner.collection_jingles_;} // SAGA_STUB(mission-collection-audio).
    } collection_host_{*this};
    void game_over(std::int32_t reason)override;
    core::StageFormStack* stage_form_stack()override{return this;}
    std::shared_ptr<core::StageDialog> make_retry_form(std::int32_t cost)override;
    std::shared_ptr<core::StageForm> make_defeat_form(std::int32_t reason)override;
    void stop_stage_bgm()override;
    void push(core::StageForm*)override;
    void push(core::VictoryForm*)override;
    void key_up(std::int32_t mask)override;
    core::VictoryFormStack* form_stack()override{return this;}
    core::VictoryForm* make_victory(std::int32_t,bool)override;
    core::MovementControl* movement_control(std::int32_t,std::int32_t)override;
    void send_touch_release(core::MovementControl&,std::int32_t,std::int32_t)override;
    core::EntitySubset* enemy_subset(const core::EventIntegers& slots)override{return world_.subset(slots);}
    core::EnemyGaugeService* enemy_gauge_system()override{return gauge_service_;}
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
    void send_damage(core::ComponentSlots& e,std::int32_t amount)override{if(damage_receiver_)damage_receiver_(&e,amount);else core::record_ecs_stage_damage(stage_.total_damage,e,amount);}
    void diagnostics(const core::DamageNumbers&)override;
    void damage_labels(core::ComponentSlots&,const core::DamageNumbers&)override;
    void positional_damage_effect(core::ProjectilePoint at)override{world_.effect(at,0,1,false);}
    const GameplayCatalog& catalog_;MissionEntities& world_;MissionProgress& saved_;core::StageGeometry& geometry_;MissionRuntimeConfig runtime_;Random random_;
    core::CombatRecordTables records_;core::EventIntegers characters_,equipment_,hats_,cloth_;
    core::CombatSavedLevels levels_{characters_,equipment_,hats_,cloth_};
    core::StageStartState stage_;core::AttackButtonState button_;core::MemberRoster members_;
    core::StageQuestForm quest_form_;
    core::EnemyCreationSystem enemies_;core::HumanFrameSystem system_;
    bool deferred_init_=false;core::EnemyGaugeService* gauge_service_=nullptr;
    std::unique_ptr<core::EnemyStageEntry> default_enemy_entry_;
    std::function<void(core::ComponentSlots*,std::int32_t)> damage_receiver_;
    core::HumanEventServices services_{records_,*this,*this};
    core::EnemyEventServices enemy_services_{records_,world_.enemy_maximum_levels(),random_,*this,*this,*this};
    core::ProjectileSystemState projectiles_;core::EventInstruction attack_clips_=core::initial_attack_animation_ids();
    core::AttributeAffinityTables affinities_=core::initial_attribute_affinity_tables();
    std::uint64_t shots_=0,attack_sounds_=0,damage_presentations_=0;
    std::optional<MissionRewardScenario> reward_scenario_;
    std::optional<MissionEnemyScenario> enemy_scenario_;core::WeightedLotteryState lottery_;
    std::optional<MissionNotificationScenario> notification_scenario_;
    std::uint32_t held_keys_=0;
    std::vector<std::shared_ptr<core::StageForm>> stage_forms_;
    std::vector<std::unique_ptr<core::VictoryForm>> victory_forms_;
    std::map<const core::StageForm*,std::string> form_names_;
    std::vector<std::string> notification_events_; // One ordered presentation stream for both stack projections.
    bool collection_enabled_=false;std::uint64_t collection_steps_=0,collection_sounds_=0,collection_jingles_=0;
    std::unique_ptr<MissionResults> results_;std::unique_ptr<MissionStage> stage_loop_;
    std::optional<core::DropPlacementWorld> placement_world_;
    std::unique_ptr<MissionInput> input_;
};
} // namespace saga::desktop
