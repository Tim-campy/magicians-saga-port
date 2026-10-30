#pragma once
#include "core/gameplay/stage_update.hpp"
#include "core/gameplay/stage_recovery.hpp"
#include "core/gameplay/enemy_population.hpp"
#include "core/gameplay/quest_catalog.hpp"
#include <map>
#include <optional>
#include <ostream>

namespace saga::desktop {
class MissionHumans;
struct MissionBossScenario {std::int32_t id,level;};
struct MissionStageScenario {
    std::int32_t remaining_time,fade_frame;bool boss_started;
    std::vector<std::pair<std::int32_t,std::int32_t>> events;
    std::map<std::uint64_t,std::uint32_t> pulses;
    std::optional<MissionBossScenario> boss;
    bool reset_timer=false; // Explicit fresh timer from selected QuestData; not saved-state restoration.
};
class MissionStage final:private core::StageUpdateHost,private core::StageBossHost,private core::StageStartHost,
    private core::PartyStartHost,private core::MemberInitializationHost {
public:
    MissionStage(MissionHumans&,MissionStageScenario);
    bool advance(std::int32_t pass,bool enabled);
    void pulse(std::uint32_t mask){if(frame_pulse_)*frame_pulse_|=mask;else pending_pulse_|=mask;}
    void begin_frame(std::uint64_t);
    void end_frame(){frame_pulse_.reset();}
    bool frame_key_pulse(std::uint32_t mask)const{return (frame_pulse_.value_or(0)&mask)!=0;}
    void bind_pause(std::function<void(bool)> callback){pause_=std::move(callback);}
    bool exit_dialog_pending()const;
    void answer_dialog(std::int32_t);
    void report(std::ostream&);
    bool started()const{return started_;}
    void start(std::int32_t,const core::EventIntegers&,core::StageMapLoader&,
               std::function<bool()>,std::function<void()>,std::shared_ptr<core::AreaDefinition>);
private:
    core::StageCleanupHost& cleanup_host()override;
    core::StageMapLoader* current_map_system()override{return map_loader_;}
    core::PartyStartHost& party_host()override{return *this;}
    core::StageQuestForm* quest_form()override;
    bool systems_paused()override{return paused_();}
    void resume_systems()override{resume_();}
    void play_stage_bgm()override;void play_stage_sound()override;
    core::PartyCharacters character_table()override;
    core::PartyHumanFactory* human_factory_world()override;
    core::PlayerWorldState* current_world()override;
    core::QuestEntityHost* destruction_host(core::PlayerWorldState&)override;
    core::QuestMemberId entity_id(core::ComponentSlots&)override;
    core::PartyCharacter* character(core::HumanRecordLink&)override;
    bool debug_enabled()override;
    bool editor_enabled()override;
    bool quest_has_auto_party()override;
    core::PartyCharacter* random_character(const std::vector<core::PartyCharacter*>&)override;
    core::PartyDistanceSystem* distance_system(core::PlayerWorldState&)override;
    std::shared_ptr<std::vector<std::int32_t>> auto_formation(std::int32_t)override;
    core::MemberInitializationHost& member_initialization()override{return *this;}
    core::ProjectileCollision& acquire_collision()override;
    void release_collision(core::Component*)override;
    std::int32_t formation_left()override;
    std::int32_t formation_top()override;
    std::int32_t total_parameter(core::ComponentSlots&,std::int32_t)override;
    void change_animation(core::ComponentSlots&,std::int32_t,bool)override;
    core::HumanStateHost* human_system()override;
    std::int32_t* sortie_count(core::HumanRecordLink&)override;
    std::int32_t frames_per_second()override;
    core::StageGeometry* stage_map()override;
    core::DropPlacementWorld* drop_world()override;
    core::DropCollectionHost& collection_host()override;
    core::BossNotificationHost& movement_host()override;
    core::StageQuestForm* update_quest_form()override;
    bool key_pulse(std::int32_t)override;
    void pause_systems(bool)override;
    bool execute_stage_event(std::int32_t)override;
    core::StageRecoveryHost& recovery_host()override;
    core::StageBossHost& boss_host()override;
    core::StagePlacementHost& boss_coordinates()override;
    core::ComponentSlots* create_stage_boss(core::StageEnemyStart&)override;
    core::StageObjectWorld* stage_object_world()override{return &objects_;}
    core::ProjectileModification& acquire_modification()override;
    void play_boss_bgm()override;
    void play_boss_sound()override;
    bool check_crystals(std::int32_t)override;void subtract_crystals(std::int32_t)override;
    core::StageFormStack* stage_form_stack()override;
    std::shared_ptr<core::StageDialog> make_retry_form(std::int32_t)override;
    std::shared_ptr<core::StageDialog> make_exit_form()override;
    std::shared_ptr<core::StageForm> make_insufficient_crystals_form()override;
    std::shared_ptr<core::StageForm> make_interstitial_ad()override;
    std::shared_ptr<core::StageForm> make_defeat_form(std::int32_t)override;
    void stop_stage_bgm()override;
    void show_quest_result()override;
    core::PlayerWorldState* update_world()override;
    std::int32_t* equipment_level(core::EquipmentSlot,std::int32_t)override;
    core::QuestReturnHost& return_host()override;
    MissionHumans& owner_;MissionStageScenario scenario_;
    std::unique_ptr<core::EnemyStageEntry> enemy_entry_;
    core::StageObjectWorld objects_;
    std::uint64_t boss_creation_calls_=0;
    std::uint64_t steps_=0;std::uint32_t pulse_=0,pending_pulse_=0;
    std::optional<std::uint32_t> frame_pulse_;
    core::StageMapLoader* map_loader_=nullptr;
    std::function<bool()> paused_;std::function<void()> resume_;
    std::function<void(bool)> pause_;
    bool answer_received_=false;
    std::uint64_t modal_wait_steps_=0,dialog_answers_=0;
    bool started_=false;
};
}
