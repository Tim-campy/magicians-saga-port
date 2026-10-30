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
    if(scenario_.reset_timer)reset_stage_timer(owner_.stage_,*this);
    owner_.quest_form_.fade_frame=scenario_.fade_frame;
    if(!owner_.table_quest_){
        auto rows=std::make_shared<std::vector<EventInstruction>>();
        for(const auto& e:scenario_.events)rows->push_back(std::make_shared<EventIntegers>(EventIntegers{e.first,e.second}));
        owner_.stage_.quest->events=std::make_shared<std::vector<EventScript>>(1,std::move(rows));
    }
    const auto& groups=owner_.stage_.quest->events;
    if(!groups||groups->empty()||!groups->at(0))throw std::invalid_argument("stage events require a first group");
    owner_.stage_.event_states=std::make_shared<std::vector<std::uint8_t>>(groups->at(0)->size(),std::uint8_t{0});
}
bool MissionStage::advance(std::int32_t pass,bool enabled){
    if(owner_.transition_pending())return false;
    if(!gameplay_needs_update(3,enabled,pass))return true;
    ++steps_;
    if(frame_pulse_)pulse_=*frame_pulse_;
    else{auto pulse=scenario_.pulses.find(steps_);pulse_=pending_pulse_;pending_pulse_=0;
        if(pulse!=scenario_.pulses.end())pulse_|=pulse->second;}
    if(owner_.stage_.dialog){
        if(owner_.stage_.dialog->kind!=99)unavailable("retry/defeat form lifecycle and recovery");
        // SAGA_STUB(mission-exit-modal): explicit close notification replaces the unbound FormManager gate.
        // Negative answers are valid native cancellations, never a pending sentinel.
        if(!answer_received_){++modal_wait_steps_;return true;}
    }
    if(owner_.stage_.state==1)++owner_.collection_steps_;
    update_stage(owner_.stage_,*this);
    if(!owner_.stage_.dialog)answer_received_=false;
    return true;
}
void MissionStage::begin_frame(std::uint64_t frame){
    if(frame_pulse_)throw std::logic_error("stage input frame already open");
    frame_pulse_=pending_pulse_;pending_pulse_=0;
    const auto at=scenario_.pulses.find(frame);if(at!=scenario_.pulses.end())*frame_pulse_|=at->second;
}
bool MissionStage::exit_dialog_pending()const{return owner_.stage_.dialog&&owner_.stage_.dialog->kind==99&&!answer_received_;}
void MissionStage::answer_dialog(std::int32_t answer){
    if(!exit_dialog_pending())throw std::invalid_argument("no pending exit dialog to answer");
    owner_.stage_.dialog->answer=answer;answer_received_=true;++dialog_answers_;
    owner_.notification_events_.push_back("answer_exit:"+std::to_string(answer));
}
void MissionStage::start(std::int32_t stage,const EventIntegers& formation,StageMapLoader& map,
    std::function<bool()> paused,std::function<void()> resume,std::shared_ptr<AreaDefinition> area){
    if(started_||formation.size()!=16||!owner_.results_||!owner_.stage_.enemies)
        throw std::invalid_argument("stage_start requires fresh entry, sixteen formation slots, results and native Init");
    owner_.members_.formation=std::make_shared<EventIntegers>(formation);
    map_loader_=&map;paused_=std::move(paused);resume_=std::move(resume);
    // The scenario selects a canonical Area.txt row or declares the diagnostic null-area boundary.
    start_stage(owner_.stage_,std::move(area),owner_.stage_.quest,stage,owner_.random_,*this);started_=true;
}
StageCleanupHost& MissionStage::cleanup_host(){return owner_.results_->cleanup_service();}
StageQuestForm* MissionStage::quest_form(){return &owner_.quest_form_;}
void MissionStage::play_stage_bgm(){owner_.notification_events_.push_back("play_stage_bgm");} // SAGA_STUB(mission-stage-audio).
void MissionStage::play_stage_sound(){owner_.notification_events_.push_back("play_stage_sound");} // SAGA_STUB(mission-stage-audio).
PartyCharacters MissionStage::character_table(){return owner_.character_table();}
PartyHumanFactory* MissionStage::human_factory_world(){return owner_.human_factory_world();}
PlayerWorldState* MissionStage::current_world(){return owner_.current_world();}
QuestEntityHost* MissionStage::destruction_host(PlayerWorldState&){return &owner_.world_;}
QuestMemberId MissionStage::entity_id(ComponentSlots& e){return owner_.world_.handle(e);}
PartyCharacter* MissionStage::character(HumanRecordLink& h){return owner_.character(h);}
bool MissionStage::debug_enabled(){return owner_.runtime_.debug;}
bool MissionStage::editor_enabled(){unavailable("DEBUG editor auto-party inputs");}
bool MissionStage::quest_has_auto_party(){return (static_cast<std::uint32_t>(owner_.stage_.quest->flags)&2U)!=0;}
PartyCharacter* MissionStage::random_character(const std::vector<PartyCharacter*>&){unavailable("DEBUG auto-party saved-state writeback");}
PartyDistanceSystem* MissionStage::distance_system(PlayerWorldState&){unavailable("DEBUG auto-party distance service");}
std::shared_ptr<std::vector<std::int32_t>> MissionStage::auto_formation(std::int32_t){unavailable("DEBUG auto-party formation");}
ProjectileCollision& MissionStage::acquire_collision(){return owner_.world_.acquire_collision();}
void MissionStage::release_collision(Component* value){owner_.world_.return_component(32,value);}
std::int32_t MissionStage::formation_left(){return stage_formation_left(static_cast<StageStartHost&>(*this));}
std::int32_t MissionStage::formation_top(){return stage_formation_top(owner_.stage_,static_cast<StageStartHost&>(*this));}
std::int32_t MissionStage::total_parameter(ComponentSlots& e,std::int32_t key){return owner_.human_parameter(e,key);}
void MissionStage::change_animation(ComponentSlots& e,std::int32_t clip,bool reset){owner_.change_animation(e,clip,reset);}
HumanStateHost* MissionStage::human_system(){return static_cast<HumanFrameHost*>(&owner_);}
std::int32_t* MissionStage::sortie_count(HumanRecordLink& h){
    auto at=owner_.saved_.sortie_counts.find(h.character_id);return at==owner_.saved_.sortie_counts.end()?nullptr:&at->second;
}
StageGeometry* MissionStage::stage_map(){return &owner_.geometry_;}
std::int32_t MissionStage::frames_per_second(){return owner_.runtime_.fps;}
StageQuestForm* MissionStage::update_quest_form(){return &owner_.quest_form_;}
DropPlacementWorld* MissionStage::drop_world(){return owner_.drop_world();}
DropCollectionHost& MissionStage::collection_host(){return owner_.collection_host_;}
BossNotificationHost& MissionStage::movement_host(){return owner_;}
bool MissionStage::key_pulse(std::int32_t mask){return (frame_pulse_.value_or(pulse_)&static_cast<std::uint32_t>(mask))!=0;}
void MissionStage::pause_systems(bool value){if(!pause_)unavailable("stage pause/resume lifecycle");pause_(value);}
bool MissionStage::execute_stage_event(std::int32_t id){
    throw std::logic_error("mission service unbound: stage event catalog dispatch id="+std::to_string(id));
}
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
std::shared_ptr<StageDialog> MissionStage::make_exit_form(){
    auto form=std::make_shared<StageDialog>();
    owner_.stage_forms_.push_back(form);owner_.form_names_[form.get()]="exit";
    owner_.notification_events_.push_back("make_exit");answer_received_=false;return form;
} // SAGA_STUB(mission-stage-forms): retained identity; desktop title supplies the explicit exit choices.
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
    out<<",\"exit_dialog_pending\":"<<(exit_dialog_pending()?"true":"false")
       <<",\"exit_dialog_answers\":"<<dialog_answers_<<",\"exit_dialog_wait_steps\":"<<modal_wait_steps_;
    out<<",\"native_stage_started\":"<<(started_?"true":"false");
    if(started_){
        out<<",\"stage_entry_stage\":"<<owner_.stage_.stage<<",\"stage_area\":";
        if(owner_.stage_.area){
            const auto& area=*static_cast<const AreaDefinition*>(owner_.stage_.area.get());
            out<<"{\"id\":"<<area.id<<",\"image\":"<<area.image<<",\"flags\":"<<area.flags<<",\"stages\":";
            if(area.stages){out<<'[';for(std::size_t i=0;i<area.stages->size();++i){if(i)out<<',';out<<area.stages->at(i);}out<<']';}
            else out<<"null";
            out<<'}';
        }else out<<"null";
        out<<",\"quest_form_scale\":"<<owner_.quest_form_.scale<<",\"entry_members\":[";bool first=true;
        for(auto* e:owner_.members_.members->entities()){if(!first)out<<',';first=false;out<<owner_.world_.id(*e);}out<<"],\"sortie_counts\":[";first=true;
        for(const auto& e:owner_.saved_.sortie_counts){if(!first)out<<',';first=false;out<<"{\"id\":"<<e.first<<",\"count\":"<<e.second<<'}';}out<<']';
    }
    out<<",\"stage_steps\":"<<steps_<<",\"remaining_time\":"<<owner_.stage_.remaining_time
       <<",\"boss_entry_enabled\":"<<(enemy_entry_?"true":"false")<<",\"boss_started\":"<<(owner_.stage_.boss_started?"true":"false")
       <<",\"boss_creation_calls\":"<<boss_creation_calls_<<",\"min_z_rate\":"<<owner_.stage_.min_z_rate<<",\"boss_fence\":";
    if(const auto* e=owner_.stage_.boss_fence){out<<"{\"id\":"<<owner_.world_.id(*e)<<",\"destroyed\":"<<(owner_.world_.destroyed(*e)?"true":"false");if(const auto* p=dynamic_cast<ContactPosition*>(e->get(0)))out<<",\"x\":"<<p->x<<",\"y\":"<<p->y<<",\"z\":"<<p->z;out<<'}';}else out<<"null";
}
void MissionHumans::enable_stage_loop(MissionStageScenario scenario){
    if(stage_loop_)throw std::invalid_argument("stage_loop requires one explicit scenario");
    stage_loop_=std::make_unique<MissionStage>(*this,std::move(scenario));
}
void MissionHumans::start_mission(std::int32_t stage,const EventIntegers& formation,StageMapLoader& map,
    std::function<bool()> paused,std::function<void()> resume,std::shared_ptr<AreaDefinition> area){
    if(!stage_loop_)throw std::invalid_argument("stage_start requires stage_loop");
    stage_loop_->start(stage,formation,map,std::move(paused),std::move(resume),std::move(area));
}
bool MissionHumans::advance_stage(std::int32_t pass,bool enabled){
    if(stage_loop_)return stage_loop_->advance(pass,enabled);
    advance_collection(pass,enabled);return true;
}
void MissionHumans::pulse_stage_keys(std::uint32_t mask){
    if(!stage_loop_)throw std::invalid_argument("stage_loop is required for pulse input");stage_loop_->pulse(mask);
}
void MissionHumans::bind_stage_pause(std::function<void(bool)> callback){
    if(!stage_loop_)throw std::invalid_argument("stage_loop is required for pause lifecycle");stage_loop_->bind_pause(std::move(callback));
}
bool MissionHumans::exit_dialog_pending()const{return stage_loop_&&stage_loop_->exit_dialog_pending();}
void MissionHumans::answer_stage_dialog(std::int32_t answer){
    if(!stage_loop_)throw std::invalid_argument("stage_loop is required for dialog answers");stage_loop_->answer_dialog(answer);
}
void MissionHumans::begin_stage_frame(std::uint64_t frame){if(!stage_loop_)throw std::logic_error("native quest frame requires stage_loop");stage_loop_->begin_frame(frame);}
void MissionHumans::end_stage_frame(){stage_loop_->end_frame();}
bool MissionHumans::stage_frame_pulse(std::uint32_t mask)const{return stage_loop_&&stage_loop_->frame_key_pulse(mask);}
void MissionHumans::debug_show_result(){if(!results_)unavailable("DEBUG result providers");results_->grant();}
void MissionHumans::debug_defeat_form(std::int32_t reason){auto form=make_defeat_form(reason);push(form.get());}
void MissionHumans::debug_victory_form(std::int32_t enemy,bool completed){auto* form=make_victory(enemy,completed);push(form);}
}
