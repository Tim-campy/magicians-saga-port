#include "mission_humans.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {[[noreturn]]void unavailable(const char* service){throw std::logic_error(std::string("mission service unbound: ")+service);}}
MissionStage::MissionStage(MissionHumans& owner,MissionStageScenario scenario):owner_(owner),scenario_(std::move(scenario)){
    if(!owner_.collection_enabled_||!owner_.notification_scenario_)throw std::invalid_argument("stage_loop requires collection fresh and stage_notifications");
    owner_.stage_.remaining_time=scenario_.remaining_time;owner_.stage_.boss_started=scenario_.boss_started;
    form_.fade_frame=scenario_.fade_frame;
    auto rows=std::make_shared<std::vector<EventInstruction>>();
    for(const auto& e:scenario_.events)rows->push_back(std::make_shared<EventIntegers>(EventIntegers{e.first,e.second}));
    owner_.stage_.quest->events=std::make_shared<std::vector<EventScript>>(1,std::move(rows));
    owner_.stage_.event_states=std::make_shared<std::vector<std::uint8_t>>(scenario_.events.size(),std::uint8_t{0});
}
bool MissionStage::advance(std::int32_t pass,bool enabled){
    if(owner_.transition_pending())return false;
    if(!gameplay_needs_update(3,enabled,pass))return true;
    if(owner_.stage_.dialog)unavailable("stage dialog pause/input flow");
    ++steps_;auto pulse=scenario_.pulses.find(steps_);pulse_=pulse==scenario_.pulses.end()?0:pulse->second;
    if(owner_.stage_.state==1)++owner_.collection_steps_;
    update_stage(owner_.stage_,*this);return true;
}
StageGeometry* MissionStage::stage_map(){return &owner_.geometry_;}
DropPlacementWorld* MissionStage::drop_world(){return owner_.drop_world();}
DropCollectionHost& MissionStage::collection_host(){return owner_.collection_host_;}
BossNotificationHost& MissionStage::movement_host(){return owner_;}
bool MissionStage::key_pulse(std::int32_t mask){return (pulse_&static_cast<std::uint32_t>(mask))!=0;}
void MissionStage::pause_systems(bool){unavailable("stage pause/resume input");}
bool MissionStage::execute_stage_event(std::int32_t){unavailable("stage event catalog dispatch");}
StageRecoveryHost& MissionStage::recovery_host(){unavailable("stage recovery/boss startup");}
bool MissionStage::check_crystals(std::int32_t){unavailable("crystal retry decision");}
void MissionStage::subtract_crystals(std::int32_t){unavailable("crystal retry subtraction");}
StageFormStack* MissionStage::stage_form_stack(){return owner_.stage_form_stack();}
std::shared_ptr<StageDialog> MissionStage::make_retry_form(std::int32_t cost){return owner_.make_retry_form(cost);}
std::shared_ptr<StageDialog> MissionStage::make_exit_form(){unavailable("stage exit dialog");}
std::shared_ptr<StageForm> MissionStage::make_insufficient_crystals_form(){unavailable("insufficient crystals dialog");}
std::shared_ptr<StageForm> MissionStage::make_interstitial_ad(){unavailable("retry ad request");}
std::shared_ptr<StageForm> MissionStage::make_defeat_form(std::int32_t reason){return owner_.make_defeat_form(reason);}
void MissionStage::stop_stage_bgm(){owner_.stop_stage_bgm();}
void MissionStage::show_quest_result(){if(!owner_.results_)unavailable("result state providers");owner_.results_->grant();}
PlayerWorldState* MissionStage::update_world(){return &owner_.world_.player_world();}
std::int32_t* MissionStage::equipment_level(EquipmentSlot kind,std::int32_t id){
    if(kind==EquipmentSlot::weapon)return &owner_.saved_.equipment_levels.at(id);
    if(kind==EquipmentSlot::hat)return &owner_.saved_.hat_levels.at(id);
    if(kind==EquipmentSlot::cloth)return &owner_.saved_.cloth_levels.at(id);
    unavailable("stage gear slot");
}
QuestReturnHost& MissionStage::return_host(){if(!owner_.results_)unavailable("return state providers");return *owner_.results_;}
void MissionStage::report(std::ostream& out){out<<",\"stage_steps\":"<<steps_<<",\"remaining_time\":"<<owner_.stage_.remaining_time;}
void MissionHumans::enable_stage_loop(MissionStageScenario scenario){
    if(stage_loop_)throw std::invalid_argument("stage_loop requires one explicit scenario");
    stage_loop_=std::make_unique<MissionStage>(*this,std::move(scenario));
}
bool MissionHumans::advance_stage(std::int32_t pass,bool enabled){
    if(stage_loop_)return stage_loop_->advance(pass,enabled);
    advance_collection(pass,enabled);return true;
}
}
