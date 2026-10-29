#pragma once
#include "core/gameplay/stage_update.hpp"
#include <map>
#include <ostream>

namespace saga::desktop {
class MissionHumans;
struct MissionStageScenario {
    std::int32_t remaining_time,fade_frame;bool boss_started;
    std::vector<std::pair<std::int32_t,std::int32_t>> events;
    std::map<std::uint64_t,std::uint32_t> pulses;
};
class MissionStage final:private core::StageUpdateHost {
public:
    MissionStage(MissionHumans&,MissionStageScenario);
    bool advance(std::int32_t pass,bool enabled);
    void pulse(std::uint32_t mask){pending_pulse_|=mask;}
    void report(std::ostream&);
private:
    core::StageGeometry* stage_map()override;
    core::DropPlacementWorld* drop_world()override;
    core::DropCollectionHost& collection_host()override;
    core::BossNotificationHost& movement_host()override;
    core::StageQuestForm* update_quest_form()override;
    bool key_pulse(std::int32_t)override;
    void pause_systems(bool)override;
    bool execute_stage_event(std::int32_t)override;
    core::StageRecoveryHost& recovery_host()override;
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
    std::uint64_t steps_=0;std::uint32_t pulse_=0,pending_pulse_=0;
};
}
