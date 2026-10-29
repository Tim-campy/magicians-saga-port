#pragma once
#include "mission_entities.hpp"
#include "core/gameplay/human_initialization.hpp"
#include "core/gameplay/stage_recovery.hpp"

namespace saga::desktop {
struct MissionRuntimeConfig {std::int32_t fps;bool debug;std::int32_t seed;std::string backend;};
struct MissionHumanScenario {std::int32_t distance,min_z_rate,max_z_rate,attribute,button;};
// Concrete Human Init/frame services. Stage/map entry and player-input producers remain separate.
class MissionHumans final:private core::HumanFrameHost,private core::HumanInitializationWorld,
    private core::HumanImpactHost,private core::HumanContactHost,private core::StagePlacementHost,
    private core::MemberFormationHost {
public:
    MissionHumans(MissionEntities&,const GameplayCatalog&,MissionProgress&,core::StageGeometry&,MissionRuntimeConfig,MissionHumanScenario);
    MissionHumans(const MissionHumans&)=delete;MissionHumans& operator=(const MissionHumans&)=delete;
    void update(std::int32_t pass=0,bool enabled=true);
    void set_state(core::ComponentSlots&,std::int32_t,float speed_z);
    std::uint64_t random_draws()const{return random_.draws;}
private:
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
    MissionEntities& world_;MissionProgress& saved_;core::StageGeometry& geometry_;MissionRuntimeConfig runtime_;Random random_;
    core::CombatRecordTables records_;core::EventIntegers characters_,equipment_,hats_,cloth_;
    core::CombatSavedLevels levels_{characters_,equipment_,hats_,cloth_};
    core::StageStartState stage_;core::AttackButtonState button_;core::MemberRoster members_;
    core::EnemyCreationSystem enemies_;core::HumanFrameSystem system_;
    core::HumanEventServices services_{records_,*this,*this};
};
} // namespace saga::desktop
