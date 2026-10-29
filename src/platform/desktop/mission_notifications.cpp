#include "mission_humans.hpp"
#include <stdexcept>

namespace saga::desktop {
using namespace core;
void MissionHumans::enable_notifications(MissionNotificationScenario scenario){
    if(notification_scenario_)throw std::invalid_argument("stage_notifications requires one explicit scenario");
    stage_.state=scenario.state;stage_.frame=scenario.frame;stage_.retried=scenario.retried;
    stage_.quest->flags=scenario.quest_flags;stage_.quest->completed=scenario.quest_completed?1:0;
    held_keys_=scenario.held_keys;notification_scenario_=scenario;
}
void MissionHumans::game_over(std::int32_t reason){
    if(!notification_scenario_)throw std::logic_error("mission service unbound: stage death requires explicit stage_notifications");
    stage_game_over(stage_,reason,*this);
}
std::shared_ptr<StageDialog> MissionHumans::make_retry_form(std::int32_t cost){
    auto form=std::make_shared<StageDialog>();form->answer=notification_scenario_.value().retry_answer;
    stage_forms_.push_back(form);form_names_[form.get()]="retry";
    notification_events_.push_back("make_retry:"+std::to_string(cost));return form;
} // SAGA_STUB(mission-stage-forms): retained identity and observations, no drawn dialog or answer selection.
std::shared_ptr<StageForm> MissionHumans::make_defeat_form(std::int32_t reason){
    auto form=std::make_shared<StageForm>();stage_forms_.push_back(form);form_names_[form.get()]="defeat";
    notification_events_.push_back("make_defeat:"+std::to_string(reason));return form;
} // SAGA_STUB(mission-stage-forms).
void MissionHumans::push(StageForm* form){
    const auto it=form_names_.find(form);notification_events_.push_back("push:"+(it==form_names_.end()?std::string(form?"unknown":"null"):it->second));
} // SAGA_STUB(mission-stage-forms).
void MissionHumans::stop_stage_bgm(){notification_events_.push_back("stop_bgm");} // SAGA_STUB(mission-stage-audio).
void MissionHumans::boss_defeated(ComponentSlots& enemy){
    if(!notification_scenario_)throw std::logic_error("mission service unbound: boss notification requires explicit stage_notifications");
    QuestConditionState quest{};
    BossStageView view{stage_.collection.boss_defeated,stage_.max_z_rate,[&]() -> const QuestConditionState* {
        if(!stage_.quest)return nullptr;
        quest.flags=static_cast<std::uint8_t>(stage_.quest->flags);quest.completed=stage_.quest->completed!=0;return &quest;
    }};
    notify_boss_defeat(view,&enemy,records_,*this);
}
void MissionHumans::key_up(std::int32_t mask){
    held_keys_&=~static_cast<std::uint32_t>(mask);notification_events_.push_back("key_up:"+std::to_string(mask));
}
VictoryForm* MissionHumans::make_victory(std::int32_t enemy,bool completed){
    victory_forms_.push_back(std::make_unique<VictoryForm>());
    notification_events_.push_back("make_victory:"+std::to_string(enemy)+":"+(completed?"1":"0"));return victory_forms_.back().get();
} // SAGA_STUB(mission-stage-forms).
void MissionHumans::push(VictoryForm*){notification_events_.push_back("push:victory");} // SAGA_STUB(mission-stage-forms).
MovementControl* MissionHumans::movement_control(std::int32_t group,std::int32_t id){
    notification_events_.push_back("control_lookup:"+std::to_string(group)+":"+std::to_string(id));return input_?input_->movement_control(group,id):nullptr;
} // SAGA_STUB(mission-movement-controls): without input fresh no QuestForm controls are bound.
void MissionHumans::send_touch_release(MovementControl& control,std::int32_t type,std::int32_t event_id){
    if(input_){input_->release(control,type,event_id);return;}
    throw std::logic_error("mission service unbound: QuestForm touch release");
}
void MissionHumans::write_notification_report(std::ostream& out){
    out<<",\"notifications_enabled\":"<<(notification_scenario_?"true":"false");if(!notification_scenario_)return;
    out<<",\"stage_state\":"<<stage_.state<<",\"stage_frame\":"<<stage_.frame
       <<",\"boss_defeated\":"<<(stage_.collection.boss_defeated?"true":"false")
       <<",\"max_z_rate\":"<<stage_.max_z_rate<<",\"held_keys\":"<<held_keys_<<",\"retry_dialog\":";
    if(stage_.dialog)out<<"{\"kind\":"<<stage_.dialog->kind<<",\"reason\":"<<stage_.dialog->reason<<",\"answer\":"<<stage_.dialog->answer<<'}';else out<<"null";
    out<<",\"notification_events\":[";bool first=true;
    for(const auto& entry:notification_events_){if(!first)out<<',';first=false;out<<'"'<<entry<<'"';}out<<']';
}
} // namespace saga::desktop
