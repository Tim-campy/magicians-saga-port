#include "quest_catalog.hpp"
#include "table_catalog.hpp"
#include "quest_rewards.hpp"
#include "event_progression.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
void read(AreaDefinition& r,detail::TableTokens& t) {
    r.id=t.integer();r.name=t.take();r.image=t.integer();
    auto stages=t.integers();
    r.stages=stages?std::make_shared<EventIntegers>(std::move(*stages)):nullptr;
    r.flags=t.integer();
}
StageScriptGroups groups(detail::TableTokens& t) {
    const auto count=t.integer();
    if(count<0)return nullptr;
    auto result=std::make_shared<std::vector<EventScript>>();
    for(std::int32_t i=0;i<count;++i){
        auto group=t.script();
        if(!group)throw std::length_error("negative quest group size");
        result->push_back(std::move(group));
    }
    return result;
}
void read(QuestDefinition& r,detail::TableTokens& t) {
    r.id=t.integer();r.name=t.take();r.image=t.integer();r.resource=t.take();
    auto chips=t.integers();r.stage.chip_images=chips?std::make_shared<EventIntegers>(std::move(*chips)):nullptr;
    t.take();t.take(); // Native skip(), not integer parsing.
    auto& q=r.stage;
    q.base_level=t.integer();q.boss_level=t.integer();q.distance=t.integer();q.time_limit=t.integer();
    q.enemy_interval=t.integer();q.enemy_maximum=t.integer();q.enemy_total_limit=t.integer();q.money_rate=t.integer();
    q.normal_enemies=groups(t);q.fixed_enemies=groups(t);q.boss_id=t.integer();
    q.events=groups(t);q.fixed_treasure=groups(t);q.prisoner_maximum=t.integer();
    q.reward_treasure=t.integer();q.cleared_reward_treasure=t.integer();q.flags=t.integer();r.need_stamina=t.integer();
}
}
AreaDefinition& load_area_definition(AreaDefinition& receiver,std::optional<std::string_view> line,const EventIntegerParser& parser){
    // AreaData.Load 0x116c414: preserve the receiver on null/empty and retain earlier stores on failure.
    if(line&&!line->empty()){detail::TableTokens t(*line,parser);read(receiver,t);}
    return receiver;
}
AreaTable load_area_table(std::string_view text,bool normalize,const EventIntegerParser& parser){
    return detail::load_table_records<AreaDefinition>(text,normalize,parser,[](detail::TableTokens& t){AreaDefinition r;read(r,t);return r;});
}
void reset_area_progress(AreaProgressState& state,const AreaDefinition& definition){
    // AreaData.oldNewGame 0x116bf9c. Preserve the temporary allocations and each
    // publication before allocating its rows. Standard shared owners replace
    // managed array allocation categories/lifetimes; no CLR allocator is emulated.
    const auto dimensions=std::make_shared<std::vector<EventInstruction>>(3);
    static_cast<void>(std::make_shared<EventIntegers>(3,0));
    state.stage_detail=std::make_shared<std::vector<EventInstruction>>(dimensions->size());
    for(std::size_t row=0;row<state.stage_detail->size();++row){
        state.stage_detail->at(row)=std::make_shared<EventIntegers>(6,0);
        for(std::size_t column=0;column<6;++column)state.stage_detail->at(row)->at(column)=0;
    }
    state.stage_total_damage=std::make_shared<std::vector<EventInstruction>>(dimensions->size());
    for(std::size_t row=0;row<state.stage_total_damage->size();++row){
        state.stage_total_damage->at(row)=std::make_shared<EventIntegers>(2,0);
        for(std::size_t column=0;column<2;++column)state.stage_total_damage->at(row)->at(column)=0;
    }
    state.area_detail=std::make_shared<EventIntegers>(6,0);
    for(std::size_t column=0;column<6;++column)state.area_detail->at(column)=0;
    state.arrival_stage=0;
    static_cast<void>(std::make_shared<EventIntegers>(dimensions->size(),0));
    state.discovery_state=static_cast<std::int32_t>(static_cast<std::uint32_t>(definition.flags)&1U);
}
void reset_area_for_new_game(AreaProgressState& state,const AreaDefinition& definition,
                             const std::function<void(bool)>& find){
    if((static_cast<std::uint32_t>(definition.flags)&1U)!=0){
        if(find)find(true);else find_data_record(state.discovery_state,state.is_new,true);
    }
    reset_area_progress(state,definition);
}
EventScript initial_quest_stage_detail(){
    auto rows=std::make_shared<std::vector<EventInstruction>>();rows->reserve(99);
    for(std::size_t row=0;row<99;++row)rows->push_back(std::make_shared<EventIntegers>(99,0));
    return rows;
}
QuestDefinition& load_quest_definition(QuestDefinition& receiver,std::optional<std::string_view> line,const EventIntegerParser& parser){
    if(line&&!line->empty()){detail::TableTokens t(*line,parser);read(receiver,t);}
    return receiver;
}
QuestTable load_quest_table(std::string_view text,bool normalize,const EventIntegerParser& parser){
    return detail::load_table_records<QuestDefinition>(text,normalize,parser,[](detail::TableTokens& t){QuestDefinition r;read(r,t);return r;});
}
void find_quest_record(QuestDefinition& receiver,bool){
    find_quest_discovery(receiver.discovery_state,receiver.is_new,false);
}
void reset_quest_for_new_game(QuestDefinition& receiver,const std::function<void(bool)>& find,
                             const std::function<std::shared_ptr<QuestRecord>()>& allocate){
    if((receiver.stage.flags&1)!=0){if(find)find(true);else find_quest_record(receiver,true);}
    receiver.stage.completed=0;
    // QuestRecord..ctor 0x11ce190 only calls Object..ctor: seven CLR-zero int32 fields.
    auto record=allocate?allocate():std::make_shared<QuestRecord>();
    receiver.record=std::move(record);
}
void increment_quest_detail_count(QuestDefinition& receiver,std::int32_t stage){
    if(!receiver.stage_detail)throw std::invalid_argument("missing quest stage-detail array");
    const auto row=receiver.stage_detail->at(static_cast<std::size_t>(stage));
    if(!row)throw std::invalid_argument("missing quest stage-detail row");
    auto& count=row->at(3);
    count=count==std::numeric_limits<std::int32_t>::max()?std::numeric_limits<std::int32_t>::min():count+1;
}
MissionSelectionRecords select_mission_records(AreaTable& areas,QuestTable& quests,std::int32_t area_slot8,std::int32_t quest_slot9){
    // AppData.StartExplore 0x11152c4..0x11152d4 validates the cache length,
    // not the area row or Area.stages; QuestForm then indexes the global quest cache.
    if(static_cast<std::size_t>(area_slot8)>=areas.size())throw std::out_of_range("mission area slot8 outside catalog");
    auto& quest=quests.at(static_cast<std::size_t>(quest_slot9));
    auto& area=areas.at(static_cast<std::size_t>(area_slot8));
    return {area?&*area:nullptr,quest?&*quest:nullptr};
}
} // namespace saga::core
