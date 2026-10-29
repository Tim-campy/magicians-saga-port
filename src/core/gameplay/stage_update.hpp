#pragma once
#include "stage_start.hpp"
#include "boss_notification.hpp"

namespace saga::core {
class StageRecoveryHost;
class StageBossHost;
struct StageForm {virtual ~StageForm()=default;};
struct StageDialog:StageForm {std::int32_t kind=0,answer=0,reason=0;}; // +0x240/+0xc8/+0x170.
class StageFormStack {
public:virtual ~StageFormStack()=default;virtual void push(StageForm*)=0;
};
// Factories retain forms for their complete stacked lifetime; Push borrows the
// same identity and may receive null. All factories/services are required.
// The ad factory and crystal operations have no success fallback.
class StageGameOverHost {
public:
    virtual ~StageGameOverHost()=default;
    virtual StageFormStack* stage_form_stack()=0;
    virtual std::shared_ptr<StageDialog> make_retry_form(std::int32_t cost)=0;
    virtual std::shared_ptr<StageForm> make_defeat_form(std::int32_t reason)=0;
    virtual void stop_stage_bgm()=0;
};
class StageUpdateHost:public StagePlacementHost,public StageGameOverHost {
public:
    virtual DropCollectionHost& collection_host()=0;
    virtual BossNotificationHost& movement_host()=0;
    virtual StageQuestForm* update_quest_form()=0;
    virtual bool key_pulse(std::int32_t mask)=0; // Resolve current canvas each time.
    virtual void pause_systems(bool)=0; // Resolve current world each time.
    virtual bool execute_stage_event(std::int32_t id)=0; // AppData.ExecEvent result != null.
    virtual StageRecoveryHost& recovery_host()=0;
    virtual StageBossHost& boss_host(); // Default keeps the existing recovery-host binding.
    virtual bool check_crystals(std::int32_t)=0;
    virtual void subtract_crystals(std::int32_t)=0;
    // Japanese/translated message and buttons are bound by the presentation host.
    virtual std::shared_ptr<StageDialog> make_exit_form()=0;
    virtual std::shared_ptr<StageForm> make_insufficient_crystals_form()=0;
    virtual std::shared_ptr<StageForm> make_interstitial_ad()=0;
    virtual void show_quest_result()=0; // Actual grants followed by result presentation.
    virtual PlayerWorldState* update_world()=0;
    // Receives freshly read EquipComponent IDs, never a cached equipment snapshot.
    virtual std::int32_t* equipment_level(EquipmentSlot,std::int32_t id)=0;
    virtual QuestReturnHost& return_host()=0;
};
bool stage_human_alive(const HumanRecordLink&); // HumanComponent.get_isAlive 0x1140040.
void complete_stage_quest(StageQuest&); // QuestData.Complete 0x1172f00.
void raise_stage_equipment_level(std::int32_t&); // EquipmentData/HatData.LevelUp.
void raise_stage_cloth_level(std::int32_t&,std::int32_t amount);
std::int32_t stage_progress_rate(StageStartState&,float z,StagePlacementHost&);
std::int32_t stage_progress_rate(StageStartState&,StagePlacementHost&);
bool execute_stage_events(StageStartState&,StageUpdateHost&);
void update_stage_attack_buttons(StageStartState&);
std::shared_ptr<StageDialog> make_stage_retry_dialog(std::int32_t reason,StageGameOverHost&);
void stage_game_over(StageStartState&,std::int32_t reason,StageGameOverHost&);
// StageSystem.Update 0x11625e8: dialog prelude and all seven states. Compose
// with Start/Retry/StartBossBattle; enemy creation and presentation remain services.
void update_stage(StageStartState&,StageUpdateHost&);
} // namespace saga::core
