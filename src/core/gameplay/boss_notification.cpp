#include "boss_notification.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& require(T* p){if(!p)throw std::invalid_argument("missing boss notification input");return *p;}
}
void release_stage_movement(BossNotificationHost& host){
    host.key_up(0x20000);
    host.key_up(0x80000);
    auto* first=host.movement_control(35,0x10011);
    auto* second=host.movement_control(35,0x10013);
    if(first)host.send_touch_release(*first,1,9);
    if(second)host.send_touch_release(*second,1,9);
}
void notify_boss_defeat(BossStageState& stage,ComponentSlots* enemy,const CombatRecordTables& tables,BossNotificationHost& host){
    stage.boss_defeated=true;
    stage.max_z_rate=100;
    host.key_up(0x80000);
    if(enemy){
        auto* stack=host.form_stack(); // Capture before record resolution and form construction.
        const auto& data=require(resolve_combat_enemy(*enemy,tables));
        const auto& quest=require(stage.quest);
        const auto id=data.id;
        const bool completed=(quest.flags&2U)!=0||quest.completed;
        auto* form=host.make_victory(id,completed);
        require(stack).push(form); // Null factory result is passed through to Push.
    }
    release_stage_movement(host);
}
}
