#include "select_building.hpp"
#include "table_catalog.hpp"
#include "stage_rewards.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
namespace saga::core {
namespace {
void read_building(BuildingDefinition& row,detail::TableTokens& tokens){
    row.id=tokens.integer();row.name=tokens.take();row.resource=tokens.take();
    row.image=tokens.integer();row.owner_id=tokens.integer();row.type=tokens.integer();
    row.cost=tokens.integer();row.base_have=tokens.integer();row.explanation=tokens.take();row.flags=tokens.integer();
}
void read_king_reward(KingRewardDefinition& row,detail::TableTokens& tokens){
    row.id=tokens.integer();row.name=tokens.take();row.score_value=tokens.integer();
    row.crystal_level=tokens.integer();row.timing=tokens.integer();
    row.terms=tokens.script();row.execs=tokens.script();
}
template<class T> T& required(const std::shared_ptr<T>& value) {
    if(!value)throw std::invalid_argument("null building menu data");
    return *value;
}
std::int32_t saved_distance(float value) {
    if(std::isnan(value))return 0;
    if(value==std::numeric_limits<float>::infinity() || value<=-2147483648.0F)return std::numeric_limits<std::int32_t>::min();
    if(value>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();
    return static_cast<std::int32_t>(value);
}
std::int32_t kind(std::int32_t id,SelectBuildingHost& host) {
    const auto row=required(host.buildings()).at(static_cast<std::size_t>(id));
    return required(row).kind;
}
}
BuildingDefinition& load_building_definition(BuildingDefinition& receiver,std::optional<std::string_view> line,const EventIntegerParser& parser){
    if(line&&!line->empty()){detail::TableTokens tokens(*line,parser);read_building(receiver,tokens);}
    return receiver;
}
BuildingTable load_building_table(std::string_view text,bool normalize,const EventIntegerParser& parser){
    return detail::load_table_records<BuildingDefinition>(text,normalize,parser,[](detail::TableTokens& tokens){
        BuildingDefinition row;read_building(row,tokens);return row;
    });
}
void reset_building_for_new_game(BuildingNewGameState& state,const BuildingDefinition& record,std::int32_t mask,
                                 const std::function<void(bool)>& find){
    if((static_cast<std::uint32_t>(record.flags)&static_cast<std::uint32_t>(mask))!=0)find(true);
    state.have=record.base_have;
}
void reset_building_for_new_game(BuildingNewGameState& state,const BuildingDefinition& record,std::int32_t mask){
    reset_building_for_new_game(state,record,mask,[&state](bool fresh){find_data_record(state.discovery_state,state.is_new,fresh);});
}
std::vector<std::int32_t> buildable_building_ids(const BuildingTable& records,const BuildingProgressTable& progress,
                                               std::int32_t excluded_flags){
    std::vector<std::int32_t> result;
    result.reserve(6);
    for(std::size_t slot=0;slot<records.size();++slot){
        const auto& row=records.at(slot);
        if(!row)continue;
        if((static_cast<std::uint32_t>(row->flags)&static_cast<std::uint32_t>(excluded_flags))!=0)continue;
        if(progress.at(slot).value().have<1)continue;
        result.push_back(row->id);
        if(result.size()>5)break;
    }
    return result;
}
std::int32_t building_price(const BuildingDefinition& record,const BuildingNewGameState& progress,
                            const EventInstruction& resident_rates){
    if(record.type!=6)return record.cost;
    const auto signed32=[](std::uint32_t bits){return bits<=0x7fffffffU?static_cast<std::int32_t>(bits)
        :-1-static_cast<std::int32_t>(0xffffffffU-bits);};
    const auto base_have=record.base_have,have=progress.have;
    const auto count=required(resident_rates).size();
    if(count>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("resident rate count exceeds int32");
    const auto used=signed32(static_cast<std::uint32_t>(base_have)-static_cast<std::uint32_t>(have));
    const auto maximum=static_cast<std::int32_t>(count)-1;
    // Parameter.Clamp0x10eda50 tests the lower bound first, including when
    // an empty input makes maximum=-1. The subsequent array read still fails.
    const auto index=used<0?0:used>maximum?maximum:used;
    const auto cost=record.cost;
    const auto rate=resident_rates->at(static_cast<std::size_t>(index));
    const auto product=signed32(static_cast<std::uint32_t>(cost)*static_cast<std::uint32_t>(rate));
    return product/100; // Signed division truncates toward zero after wrap32.
}
KingRewardDefinition& load_king_reward_definition(KingRewardDefinition& receiver,std::optional<std::string_view> line,const EventIntegerParser& parser){
    if(line&&!line->empty()){detail::TableTokens tokens(*line,parser);read_king_reward(receiver,tokens);}
    return receiver;
}
KingRewardTable load_king_reward_table(std::string_view text,bool normalize,const EventIntegerParser& parser){
    return detail::load_table_records<KingRewardDefinition>(text,normalize,parser,[](detail::TableTokens& tokens){
        KingRewardDefinition row;read_king_reward(row,tokens);return row;
    });
}
void reset_king_reward_for_new_game(KingRewardDefinition& row){row.received=false;}
namespace {
bool check_king_rewards(KingRewardAvailabilityHost& host,KingRewardExecutionHost* execution){
    const auto bits=static_cast<std::uint32_t>(host.high_score());
    const auto score=bits<=0x7fffffffU?static_cast<std::int32_t>(bits):-1-static_cast<std::int32_t>(0xffffffffU-bits);
    const auto count=[&](){
        const auto length=required(host.rewards()).size();
        if(length>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("king reward count exceeds int32");
        return static_cast<std::int32_t>(length);
    };
    bool granted=false;
    for(std::int32_t index=0;index<count();++index){
        const auto received=required(required(host.rewards()).at(static_cast<std::size_t>(index))).received;
        if(received)continue;
        const auto threshold=required(required(host.rewards()).at(static_cast<std::size_t>(index))).score_value;
        if(threshold>score)continue;
        if(!execution)return true;
        for(std::size_t line=0;;++line){
            // Native re-fetches both the length and record before testing the
            // index. A callback can replace the row or script between passes.
            const auto script=required(required(host.rewards()).at(static_cast<std::size_t>(index))).execs;
            const auto length=required(script).size();
            if(length>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("king script count exceeds int32");
            const auto row=required(host.rewards()).at(static_cast<std::size_t>(index));
            required(row);
            if(line>=length){
                execution->add_crystals(row->crystal_level);
                const auto received_row=required(host.rewards()).at(static_cast<std::size_t>(index));
                execution->mark_received(required(received_row));
                break;
            }
            const auto instruction=required(row->execs).at(line);
            const auto crystals=required(required(host.rewards()).at(static_cast<std::size_t>(index))).crystal_level;
            const auto value=required(required(host.rewards()).at(static_cast<std::size_t>(index))).score_value;
            const auto args=execution->format_arguments(crystals,value);
            const auto child=execution->execute_instruction(instruction,EventContext{args,{}});
            if(child)execution->push_form(child);
        }
        granted=true;
    }
    return granted;
}
}
bool king_reward_available(KingRewardAvailabilityHost& host){return check_king_rewards(host,nullptr);}
bool check_and_execute_king_rewards(bool execute,KingRewardExecutionHost& host){
    return check_king_rewards(host,execute?&host:nullptr);
}
// BuildingData.IsShop, 0x116c9c4.
bool building_is_shop(std::int32_t value) {return (value>=1 && value<=5) || value==9 || value==10;}
// SelectForm.Update 0x11ecbe0..0x11ecce4. Capture each array before its value read.
void save_select_position(const SelectBuildingState& state,SelectBuildingHost& host) {
    auto save=host.save_integers();required(save).at(11)=state.camera_x;
    save=host.save_integers();required(save).at(12)=state.camera_y;
    save=host.save_integers();const auto distance=saved_distance(state.drag_distance);required(save).at(10)=distance;
}
bool process_select_building_button(SelectBuildingState& state,SelectFormPassState& form,SelectBuildingHost& host) {
    if(!host.pressed(0x08000000U))return false;
    host.play_button_sound();
    const auto slots=host.building_slots();
    if(state.selected_chip<0)return false;
    if(static_cast<std::size_t>(state.selected_chip)>=required(slots).size())return false;
    save_select_position(state,host);
    const auto row=slots->at(static_cast<std::size_t>(state.selected_chip));
    if(row) {
        const auto id=row->at(1);
        EventObject child;
        bool push=false;
        if(id==-1) {
            if(host.can_build(state.selected_chip)) {
                state.unlock_info_number=-1;state.unlock_info_frames=-1;
                child=host.make_build_list(state.selected_chip);push=true;
            }
        } else {
            const auto shop=building_is_shop(kind(id,host));
            const auto type=kind(id,host); // Original always fetches again, even for a shop.
            if(shop) {child=host.make_shop(type);push=true;}
            else if(type==6) {
                const auto members=host.living_members();
                const auto index=state.selected_chip;
                const auto member=required(members).at(static_cast<std::size_t>(index));
                if(member)child=host.make_human_house(member,index,false);
                else {
                    child=host.make_shop(6);
                    if(!child)throw std::invalid_argument("null house shop");
                    host.set_shop_selection(child,state.selected_chip);
                }
                push=true;
            } else if(kind(id,host)==7) {child=host.make_subform(15);push=true;}
            else if(kind(id,host)==12) {const auto value=host.system_share_value();child=host.make_share_window(value==0);push=true;}
        }
        if(push)push_select_form(form.form,child,form.auto_save,form.events.event_check_requested);
    }
    state.selected_chip=-1;
    return true;
}
SelectBuildingUpdate update_select_through_building(SelectUpdateBindings& bindings,SelectBuildingState& state,SelectBuildingHost& host) {
    const auto prior=update_select_through_navigation(bindings);
    if(prior.result==SelectNavigationResult::ReturnTrue)return {prior.snapshot,true};
    return {prior.snapshot,process_select_building_button(state,*bindings.state,host)};
}
}
