#include "mission_humans.hpp"
#include "core/gameplay/event_progression.hpp"
#include <stdexcept>

namespace saga::desktop {
using namespace core;
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing mission result input");return *p;}
template<class T>T& field(ComponentSlots& e,int slot){return required(dynamic_cast<T*>(e.get(slot)));}
[[noreturn]]void unavailable(const char* name){throw std::logic_error(std::string("mission service unbound: ")+name);}
}
MissionResults::MissionResults(MissionHumans& owner,MissionResultScenario scenario):owner_(owner),state_(std::move(scenario)){
    if(!owner_.catalog_.levels||!owner_.catalog_.treasures)throw std::invalid_argument("results require Table/Treasure catalogs");
    const std::array<const EquipmentCombatTable*,3> tables{{&owner_.catalog_.equipment,&owner_.catalog_.cloth,&owner_.catalog_.hats}};
    for(std::size_t k=0;k<tables.size();++k)for(const auto& row:*tables[k])if(row)
        inventory_[k].emplace(row->id,state_.inventory_records?state_.inventory_records->at(k).at(row->id):state_.inventory[k]);
    for(const auto& row:owner_.catalog_.characters)if(row)
        recruits_.emplace(row->id,state_.recruit_records?state_.recruit_records->at(row->id):state_.recruits);
    owner_.stage_.quest->rescued_prisoners=state_.rescued;
}
std::optional<QuestMemberId> MissionResults::next(Cursor& cursor){
    auto& list=required(cursor.list.get());if(list.version()!=cursor.version)throw std::runtime_error("result member list changed");
    if(cursor.index==list.entities().size())return std::nullopt;
    return owner_.world_.handle(required(list.entities().at(cursor.index++)));
}
StageRewards& MissionResults::drops(){return required(owner_.stage_.rewards().get());}
std::optional<std::int32_t> MissionResults::next_treasure(){
    const auto& ids=required(granting_.get()).treasure_ids;
    if(treasure_index_==ids.size())return std::nullopt;return ids.at(treasure_index_++);
}
const TreasureRecord& MissionResults::treasure(std::int32_t id){return owner_.world_.treasure(id);}
ItemStock& MissionResults::item(RewardItem kind,std::int32_t id){return inventory_.at(static_cast<std::size_t>(kind)).at(id);}
RecruitmentState& MissionResults::character(std::int32_t id){return recruits_.at(id);}
void MissionResults::add_money(std::int32_t amount){state_.money=add_decoded_currency(state_.money,amount);}
void MissionResults::add_crystals(std::int32_t amount){state_.crystals=add_decoded_currency(state_.crystals,amount);}
std::int32_t& MissionResults::rescued_prisoners(){return required(owner_.stage_.quest.get()).rescued_prisoners;}
QuestExperienceHost& MissionResults::members(){auto list=owner_.members_.members;grant_cursor_={list,required(list.get()).version(),0};return *this;}
std::int32_t MissionResults::defeated_enemies(){return owner_.enemies_.defeated_enemies;}
std::int32_t MissionResults::collected_prisoners(){return drops().prisoners;}
std::int32_t MissionResults::total_damage(){return owner_.stage_.total_damage;}
MemberExperience& MissionResults::human(QuestMemberId id){return field<HumanRecordLink>(owner_.world_.by_handle(id),14);}
std::int32_t MissionResults::parameter_zero(QuestMemberId id){return required(resolve_combat_parameter(owner_.world_.by_handle(id),0)).value;}
std::int32_t MissionResults::table_count(){return static_cast<std::int32_t>(owner_.catalog_.levels->size());}
std::int32_t MissionResults::next_experience(MemberExperience& captured,std::int32_t index){
    auto& human=static_cast<HumanRecordLink&>(captured);
    TableExperienceHost table(*owner_.catalog_.levels,owner_.catalog_.characters,human.character_id);return table.next_experience(index);
}
void MissionResults::grant(){
    granting_=owner_.stage_.rewards();treasure_index_=0;result_=grant_quest_rewards(*this);
    auto form=std::make_shared<StageForm>();owner_.stage_forms_.push_back(form);owner_.form_names_[form.get()]="result";
    owner_.notification_events_.push_back("make_result");owner_.push(form.get()); // SAGA_STUB(mission-stage-forms): grants precede presentation.
}
MemberFinishHost& MissionResults::finish_members_host(){finish_cursors_={};return *this;}
std::optional<QuestMemberId> MissionResults::next_finish_member(FinishMemberList kind){
    const auto index=kind==FinishMemberList::members?0U:1U;auto& cursor=finish_cursors_.at(index);
    if(!cursor.list){auto list=index==0?owner_.members_.members:owner_.members_.sub_members;cursor={list,required(list.get()).version(),0};}
    return next(cursor);
}
SavedCharacterProgression* MissionResults::saved_character(MemberExperience& captured){
    const auto id=static_cast<HumanRecordLink&>(captured).character_id;auto at=owner_.saved_.characters.find(id);return at==owner_.saved_.characters.end()?nullptr:&at->second;
}
void MissionResults::clear_finish_members(FinishMemberList kind){required((kind==FinishMemberList::members?owner_.members_.members:owner_.members_.sub_members).get()).clear();}
QuestEntityLifetime& MissionResults::entity_lifetime(QuestMemberId id){return owner_.world_.entity_lifetime(id);}
void MissionResults::delete_from_manager(QuestMemberId id,bool remove){owner_.world_.delete_from_manager(id,remove);}
std::vector<std::optional<QuestMemberId>> MissionResults::snapshot_group(std::int32_t slot){
    std::vector<std::optional<QuestMemberId>> copy;auto cursor=owner_.world_.subset({slot})->cursor();
    while(const auto entry=cursor.next())copy.push_back(*entry?std::optional<QuestMemberId>(owner_.world_.handle(**entry)):std::nullopt);return copy;
}
QuestEntityHost& MissionResults::destruction_host(){return owner_.world_;}
std::optional<QuestMemberId> MissionResults::extra_entity(){return owner_.stage_.boss_fence?std::optional<QuestMemberId>(owner_.world_.handle(*owner_.stage_.boss_fence)):std::nullopt;}
void MissionResults::clear_extra_entity(){owner_.stage_.boss_fence=nullptr;}
void MissionResults::clear_collecting_drops(){required(owner_.stage_.collection.collecting.get()).clear();}
void MissionResults::make_transition(std::int32_t mode,std::function<void()> callback,bool,std::uint32_t){
    owner_.notification_events_.push_back("transition:"+std::to_string(mode));transition_=std::move(callback);
} // SAGA_STUB(mission-return-transition): retain callback; explicit scene policy holds old mission here.
std::int32_t MissionResults::return_area_id(){
    if(!owner_.stage_.area)unavailable("town-return area");
    return static_cast<const AreaDefinition*>(owner_.stage_.area.get())->id;
}
std::int32_t MissionResults::return_stage(){
    if(!owner_.stage_loop_||!owner_.stage_loop_->started())unavailable("town-return stage");
    return owner_.stage_.stage;
}
void MissionResults::change_to_town_form(bool){unavailable("town form switch");}
void MissionResults::send_town_return(std::int32_t,const TownReturnEvent&){unavailable("town event31");}
void MissionHumans::enable_results(MissionResultScenario scenario){
    if(results_||!collection_enabled_)throw std::invalid_argument("results require one explicit state and collection fresh");
    results_=std::make_unique<MissionResults>(*this,std::move(scenario));
}
bool MissionHumans::transition_pending()const{return results_&&results_->transition_pending();}
void MissionHumans::write_result_report(std::ostream& out){
    out<<",\"stage_loop_enabled\":"<<(stage_loop_?"true":"false");if(stage_loop_)stage_loop_->report(out);
    out<<",\"results_enabled\":"<<(results_?"true":"false");if(results_)results_->report(out);
}
void MissionResults::report(std::ostream& out){
    out<<",\"result_granted\":"<<(result_?"true":"false")<<",\"transition_pending\":"<<(transition_pending()?"true":"false")
       <<",\"quest_completed\":"<<(owner_.stage_.quest->completed?"true":"false")<<",\"wallet\":{\"money\":"<<state_.money<<",\"crystals\":"<<state_.crystals<<"},\"rescued_prisoners\":"<<rescued_prisoners();
    const auto& q=state_.record;out<<",\"quest_record\":{\"best_experience\":"<<q.best_experience<<",\"best_money\":"<<q.best_money<<",\"challenges\":"<<q.challenges<<",\"clears\":"<<q.clears<<",\"defeated\":"<<q.defeated<<",\"rescued\":"<<q.rescued<<",\"best_damage\":"<<q.best_damage<<'}';
    out<<",\"saved_characters\":[";bool first=true;for(const auto& entry:owner_.saved_.characters){if(!first)out<<',';first=false;out<<"{\"id\":"<<entry.first<<",\"level\":"<<entry.second.level<<",\"experience\":"<<entry.second.experience<<'}';}out<<']';
    if(auto* hero=member_hero(&owner_.world_.player_world())){const auto& ids=*field<EquipmentRecordLinks>(*hero,8).ids;
        out<<",\"result_gear_levels\":{\"weapon\":"<<owner_.saved_.equipment_levels.at(ids.at(static_cast<std::size_t>(EquipmentSlot::weapon)))
           <<",\"hat\":"<<owner_.saved_.hat_levels.at(ids.at(static_cast<std::size_t>(EquipmentSlot::hat)))
           <<",\"cloth\":"<<owner_.saved_.cloth_levels.at(ids.at(static_cast<std::size_t>(EquipmentSlot::cloth)))<<'}';}
    out<<",\"inventory_changes\":[";first=true;for(std::size_t k=0;k<inventory_.size();++k)for(const auto& e:inventory_[k]){
        const auto& initial=state_.inventory_records?state_.inventory_records->at(k).at(e.first):state_.inventory[k];const auto& v=e.second;if(v.discovery_state==initial.discovery_state&&v.is_new==initial.is_new&&v.stock==initial.stock)continue;
        if(!first)out<<',';first=false;out<<"{\"kind\":"<<k<<",\"id\":"<<e.first<<",\"found\":"<<v.discovery_state<<",\"is_new\":"<<(v.is_new?"true":"false")<<",\"stock\":"<<v.stock<<'}';}out<<']';
    out<<",\"recruit_changes\":[";first=true;for(const auto& e:recruits_){const auto& v=e.second;const auto& a=state_.recruit_records?state_.recruit_records->at(e.first):state_.recruits;if(v.discovery_state==a.discovery_state&&v.is_new==a.is_new&&v.recruited==a.recruited)continue;
        if(!first)out<<',';first=false;out<<"{\"id\":"<<e.first<<",\"found\":"<<v.discovery_state<<",\"is_new\":"<<(v.is_new?"true":"false")<<",\"recruited\":"<<v.recruited<<'}';}out<<']';
    if(!result_)return;const auto& r=*result_;out<<",\"quest_result\":{\"experience\":"<<r.experience<<",\"money\":"<<r.money<<",\"crystals\":"<<r.crystals<<",\"prisoners\":"<<r.prisoners<<",\"members\":[";first=true;
    for(const auto& m:r.members){if(!first)out<<',';first=false;out<<"{\"handle\":"<<m.character<<",\"levels_gained\":"<<m.levels_gained<<",\"added_experience\":"<<m.added_experience<<",\"display_level\":"<<m.display_level<<",\"display_experience\":"<<m.display_experience<<'}';}out<<"]}";
}
}
