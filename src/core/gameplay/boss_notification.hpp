#pragma once
#include "combat_records.hpp"
#include "event_conditions.hpp"
#include <functional>

namespace saga::core {
// Host-owned opaque UI objects; no platform types enter the gameplay core.
struct VictoryForm {};
struct MovementControl {};
class VictoryFormStack {
public:
    virtual ~VictoryFormStack()=default;
    virtual void push(VictoryForm*)=0;
};
struct BossStageState {
    bool boss_defeated=false; // StageSystem +0x85.
    std::int32_t max_z_rate=0; // +0xac; not a timer.
    const QuestConditionState* quest=nullptr; // +0x30; borrowed live state.
};
// Bind actual StageSystem fields; read current quest after the earlier UI callbacks.
struct BossStageView {
    bool& boss_defeated;
    std::int32_t& max_z_rate;
    std::function<const QuestConditionState*()> quest;
};
class BossNotificationHost {
public:
    virtual ~BossNotificationHost()=default;
    // Resolve the current canvas/surface for each call, not once per operation.
    virtual void key_up(std::int32_t mask)=0;
    virtual VictoryFormStack* form_stack()=0;
    virtual VictoryForm* make_victory(std::int32_t enemy_id,bool quest_completed)=0;
    virtual MovementControl* movement_control(std::int32_t group,std::int32_t id)=0;
    // Host captures current world, constructs TouchEvent(type,control), then sends event_id.
    // TouchEvent reset/payload fields and concrete dispatch remain presentation services.
    virtual void send_touch_release(MovementControl&,std::int32_t type,std::int32_t event_id)=0;
};
// StageSystem.ReleaseMoveKeys 0x1163fa0. Both controls are captured before dispatch.
void release_stage_movement(BossNotificationHost&);
// StageSystem.OnBossDefeat 0x11663e4. A null enemy skips only the victory form.
void notify_boss_defeat(BossStageState&,ComponentSlots* enemy,const CombatRecordTables&,BossNotificationHost&);
void notify_boss_defeat(BossStageView,ComponentSlots* enemy,const CombatRecordTables&,BossNotificationHost&);
}
