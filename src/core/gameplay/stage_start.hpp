#pragma once
#include "stage_placement.hpp"
#include "drop_collection.hpp"
#include "stage_cleanup.hpp"
#include "party_start.hpp"

namespace saga::core {
struct AttackButtonState {std::int32_t state=0,frame=0;};
void change_attack_button_state(AttackButtonState&,std::int32_t);
struct StageHumanStart {HumanStateHost* transitions=nullptr;}; // Stored HumanSystem, not a fresh world lookup.
struct StageDialog;
class StageMapLoader {
public:virtual ~StageMapLoader()=default;virtual void load_map(std::shared_ptr<StageQuest>)=0;
};
class StageEnemyStart {
public:virtual ~StageEnemyStart()=default;virtual void start_enemies(std::shared_ptr<StageQuest>)=0;
};
struct StageQuestForm {std::int32_t scale=0,fade_frame=0;}; // +0x1dc / +0x128.
struct StageStartState:StagePlacementState {
    EventObject area;std::int32_t stage=0,state=0,remaining_time=0,loop_counter=0;
    std::int32_t total_damage=0,frame=0,min_z_rate=0,max_z_rate=0,last_active_button=0;
    bool boss_started=false,move_front_pushed=false,retried=false;
    std::int32_t attack_attribute=0; // StageSystem.attackAttribute_ +0x70, retained by Start.
    DropCollectionState collection;
    StageHumanStart* humans=nullptr;
    MemberRoster* members=nullptr;
    StageEnemyStart* enemies=nullptr;
    std::shared_ptr<std::vector<AttackButtonState*>> attack_buttons;
    std::shared_ptr<std::vector<std::uint8_t>> event_states;
    std::shared_ptr<StageDialog> dialog; // StageSystem.dialog_ +0x60.
    ComponentSlots* boss_fence=nullptr; // bossBattleFence_ +0xa0; cleanup owns destruction.
    // Use this operation for every drop_ reference replacement so collection
    // always reads the same current object as stage startup/result accounting.
    void replace_rewards(std::shared_ptr<StageRewards> value){rewards_=std::move(value);collection.rewards=rewards_.get();}
    std::shared_ptr<StageRewards> rewards()const{return rewards_;}
private:
    std::shared_ptr<StageRewards> rewards_;
};
class StageTimerHost {
public:
    virtual ~StageTimerHost()=default;
    virtual std::int32_t frames_per_second()=0; // MyConfig.FPS, queried after timeLimit capture.
};
class StageStartHost:public StagePlacementHost,public StageTimerHost {
public:
    virtual StageCleanupHost& cleanup_host()=0;
    virtual StageMapLoader* current_map_system()=0;
    virtual PartyStartHost& party_host()=0;
    virtual StageQuestForm* quest_form()=0; // Current BaseSystem.apdat_.frmQuest_.
    virtual bool systems_paused()=0; // Missing current world/system manager must fault.
    virtual void resume_systems()=0; // Fresh current world, PauseSystems(false).
    virtual void play_stage_bgm()=0;
    virtual void play_stage_sound()=0;
};
void reset_stage_timer(StageStartState&,StageTimerHost&);
// StageSystem.Start 0x1165198: complete orchestration. Map loading and enemy
// startup remain required services; captured quest argument survives field replacement.
void start_stage(StageStartState&,EventObject area,std::shared_ptr<StageQuest> quest,
                 std::int32_t stage,RandomSource&,StageStartHost&);
} // namespace saga::core
