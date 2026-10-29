#include "mission_humans.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {[[noreturn]]void unavailable(const char* service){throw std::logic_error(std::string("mission service unbound: ")+service);}}
MissionStage::MissionStage(MissionHumans& owner,MissionStageScenario scenario):owner_(owner),scenario_(std::move(scenario)),objects_{owner.world_.effect_world().ids,owner.world_.effect_world().factory}{
    if(!owner_.collection_enabled_||!owner_.notification_scenario_)throw std::invalid_argument("stage_loop requires collection fresh and stage_notifications");
    if(scenario_.boss){
        if(!owner_.enemy_scenario_)throw std::invalid_argument("stage_boss requires enemy_system/enemy_population/enemy_pool");
        owner_.stage_.quest->boss_id=scenario_.boss->id;owner_.stage_.quest->boss_level=scenario_.boss->level;
        enemy_entry_=std::make_unique<EnemyStageEntry>(owner_.enemies_,static_cast<EnemyPopulationHost&>(owner_));
        owner_.stage_.enemies=enemy_entry_.get();
    }
    owner_.stage_.remaining_time=scenario_.remaining_time;owner_.stage_.boss_started=scenario_.boss_started;
    owner_.quest_form_.fade_frame=scenario_.fade_frame;
    auto rows=std::make_shared<std::vector<EventInstruction>>();
    for(const auto& e:scenario_.events)rows->push_back(std::make_shared<EventIntegers>(EventIntegers{e.first,e.second}));
    owner_.stage_.quest->events=std::make_shared<std::vector<EventScript>>(1,std::move(rows));
    owner_.stage_.event_states=std::make_shared<std::vector<std::uint8_t>>(scenario_.events.size(),std::uint8_t{0});
}
bool MissionStage::advance(std::int32_t pass,bool enabled){
    if(owner_.transition_pending())return false;
    if(!gameplay_needs_update(3,enabled,pass))return true;
    if(owner_.stage_.dialog)unavailable("stage dialog pause/input flow");
    ++steps_;auto pulse=scenario_.pulses.find(steps_);pulse_=pending_pulse_;pending_pulse_=0;
    if(pulse!=scenario_.pulses.end())pulse_|=pulse->second;
    if(owner_.stage_.state==1)++owner_.collection_steps_;
    update_stage(owner_.stage_,*this);return true;
}
StageGeometry* MissionStage::stage_map(){return &owner_.geometry_;}
StageQuestForm* MissionStage::update_quest_form(){return &owner_.quest_form_;}
DropPlacementWorld* MissionStage::drop_world(){return owner_.drop_world();}
DropCollectionHost& MissionStage::collection_host(){return owner_.collection_host_;}
BossNotificationHost& MissionStage::movement_host(){return owner_;}
bool MissionStage::key_pulse(std::int32_t mask){return (pulse_&static_cast<std::uint32_t>(mask))!=0;}
void MissionStage::pause_systems(bool){unavailable("stage pause/resume input");}
bool MissionStage::execute_stage_event(std::int32_t){unavailable("stage event catalog dispatch");}
StageRecoveryHost& MissionStage::recovery_host(){unavailable("stage retry recovery");}
StageBossHost& MissionStage::boss_host(){if(!enemy_entry_)unavailable("stage_boss inputs");return *this;}
StagePlacementHost& MissionStage::boss_coordinates(){return static_cast<StagePlacementHost&>(owner_);}
ComponentSlots* MissionStage::create_stage_boss(StageEnemyStart& receiver){
    if(&receiver!=enemy_entry_.get())throw std::invalid_argument("foreign stage enemy system");
    ++boss_creation_calls_;return &create_boss_enemy(owner_.enemies_,static_cast<EnemyCreationHost&>(owner_));
}
ProjectileModification& MissionStage::acquire_modification(){return owner_.world_.acquire_modification();}
void MissionStage::play_boss_bgm(){owner_.notification_events_.push_back("play_boss_bgm");} // SAGA_STUB(mission-stage-audio).
void MissionStage::play_boss_sound(){owner_.notification_events_.push_back("play_boss_sound");} // SAGA_STUB(mission-stage-audio).
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
void MissionStage::report(std::ostream& out){
    out<<",\"stage_steps\":"<<steps_<<",\"remaining_time\":"<<owner_.stage_.remaining_time
       <<",\"boss_entry_enabled\":"<<(enemy_entry_?"true":"false")<<",\"boss_started\":"<<(owner_.stage_.boss_started?"true":"false")
       <<",\"boss_creation_calls\":"<<boss_creation_calls_<<",\"min_z_rate\":"<<owner_.stage_.min_z_rate<<",\"boss_fence\":";
    if(const auto* e=owner_.stage_.boss_fence){out<<"{\"id\":"<<owner_.world_.id(*e)<<",\"destroyed\":"<<(owner_.world_.destroyed(*e)?"true":"false");if(const auto* p=dynamic_cast<ContactPosition*>(e->get(0)))out<<",\"x\":"<<p->x<<",\"y\":"<<p->y<<",\"z\":"<<p->z;out<<'}';}else out<<"null";
}
void MissionHumans::enable_stage_loop(MissionStageScenario scenario){
    if(stage_loop_)throw std::invalid_argument("stage_loop requires one explicit scenario");
    stage_loop_=std::make_unique<MissionStage>(*this,std::move(scenario));
}
bool MissionHumans::advance_stage(std::int32_t pass,bool enabled){
    if(stage_loop_)return stage_loop_->advance(pass,enabled);
    advance_collection(pass,enabled);return true;
}
void MissionHumans::pulse_stage_keys(std::uint32_t mask){
    if(!stage_loop_)throw std::invalid_argument("stage_loop is required for pulse input");stage_loop_->pulse(mask);
}
}
