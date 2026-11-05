#pragma once
#include "select_navigation.hpp"
#include <optional>
#include <string>
#include <string_view>
namespace saga::core {
struct BuildingDefinition {
    std::int32_t id=0;
    std::string name,resource;
    std::int32_t image=0,owner_id=0,type=0,cost=0,base_have=0;
    std::string explanation;
    std::int32_t flags=0;
};
using BuildingTable=std::vector<std::optional<BuildingDefinition>>;
// BuildingData.Load 0x116cca8. Static table fields only; saved have/discovery are separate.
BuildingDefinition& load_building_definition(BuildingDefinition&,std::optional<std::string_view>,const EventIntegerParser&);
BuildingTable load_building_table(std::string_view,bool normalize_crlf,const EventIntegerParser&);
struct BuildingNewGameState {
    std::int32_t discovery_state=0;
    bool is_new=false;
    std::int32_t have=0;
};
using BuildingProgressTable=std::vector<std::optional<BuildingNewGameState>>;
// SubForm.Init type12, 0x11f6914..0x11f6aac: ordered IDs, at most six.
// Exclude flags mask2 in the native profile; discovery does not gate this list.
std::vector<std::int32_t> buildable_building_ids(const BuildingTable&,const BuildingProgressTable&,
                                               std::int32_t excluded_flags);
// BuildingData.GetBuildingPrice0x116c8ec: only type6 reads ResidentUpRate.
// Rates are an explicit APK-derived int32 array (cctor initializer field24275).
std::int32_t building_price(const BuildingDefinition&,const BuildingNewGameState&,
                            const EventInstruction& resident_rates);
// BuildingData.NewGame0x116c868: conditional virtual Find(true), then fresh base_have.
// Native initial mask1: cctor0x116cf8c..0x116cfa0, pair at ELF0x626c98.
// The application owner supplies the mask explicitly.
void reset_building_for_new_game(BuildingNewGameState&,const BuildingDefinition&,std::int32_t find_mask,
                                 const std::function<void(bool)>& find);
void reset_building_for_new_game(BuildingNewGameState&,const BuildingDefinition&,std::int32_t find_mask);
// BuildingData.IsDecoration 0x116c9f0; shared by catalogue consumers and drawing.
inline bool building_is_decoration(std::int32_t type){return type==8||type==12;}
struct KingRewardDefinition {
    std::int32_t id=0;
    std::string name;
    std::int32_t score_value=0,crystal_level=0,timing=0;
    EventScript terms,execs;
    bool received=false; // Saved +0x48; Load does not write it.
};
using KingRewardTable=std::vector<std::optional<KingRewardDefinition>>;
using KingRewardRows=std::vector<std::shared_ptr<KingRewardDefinition>>;
KingRewardDefinition& load_king_reward_definition(KingRewardDefinition&,std::optional<std::string_view>,const EventIntegerParser&);
KingRewardTable load_king_reward_table(std::string_view,bool normalize_crlf,const EventIntegerParser&);
void reset_king_reward_for_new_game(KingRewardDefinition&); // NewGame 0x1171d64 clears only received.
class KingRewardAvailabilityHost {
public:
    virtual ~KingRewardAvailabilityHost()=default;
    virtual std::int64_t high_score()=0;
    virtual std::shared_ptr<KingRewardRows> rewards()=0;
};
// CheckExeKingEvent(false), 0x11169c4: no scripts, reward grants or state writes.
bool king_reward_available(KingRewardAvailabilityHost&);
class KingRewardExecutionHost:public KingRewardAvailabilityHost {
public:
    // Native args = decimal(crystal_level) + TAB + decimal(score_value).
    // Locale/number formatting is an explicit host boundary.
    virtual EventObject format_arguments(std::int32_t crystal_level,std::int32_t score_value)=0;
    virtual EventObject execute_instruction(const EventInstruction&,const EventContext&)=0;
    virtual void push_form(EventObject)=0;
    virtual void add_crystals(std::int32_t)=0;
    virtual void mark_received(KingRewardDefinition&)=0; // Recieved0x1171d6c: received=true.
};
// CheckExeKingEvent0x11169c4: false queries only; true runs all eligible rewards.
// Unit dispatch, not matrix execution: terms/timing and Event counters are unused.
// Exceptions retain earlier effects; received follows scripts and crystal credit.
bool check_and_execute_king_rewards(bool execute,KingRewardExecutionHost&);
using BuildingSlots=std::vector<std::shared_ptr<EventIntegers>>;
struct BuildingMenuRecord {std::int32_t kind;};
using BuildingMenuCatalog=std::vector<std::shared_ptr<BuildingMenuRecord>>;
struct SelectBuildingState {
    std::int32_t selected_chip;
    std::int32_t& camera_x; // Bind to SelectPreludeState.camera_x.
    std::int32_t camera_y;
    float& drag_distance; // Bind to SelectTouchMoveState.drag_distance.
    std::int32_t unlock_info_number,unlock_info_frames;
};
class SelectBuildingHost {
public:
    virtual ~SelectBuildingHost() = default;
    virtual bool pressed(std::uint32_t mask) = 0;
    virtual void play_button_sound() = 0;
    virtual std::shared_ptr<BuildingSlots> building_slots() = 0;
    virtual std::shared_ptr<EventIntegers> save_integers() = 0;
    virtual std::shared_ptr<BuildingMenuCatalog> buildings() = 0;
    virtual bool can_build(std::int32_t index) = 0;
    virtual std::shared_ptr<std::vector<EventObject>> living_members() = 0;
    virtual std::int32_t system_share_value() = 0; // sysSave index at AppData static +0x304.
    virtual EventObject make_shop(std::int32_t kind) = 0;
    virtual void set_shop_selection(EventObject shop,std::int32_t index) = 0; // ShopForm+0x12c.
    virtual EventObject make_build_list(std::int32_t index) = 0;
    virtual EventObject make_human_house(EventObject member,std::int32_t index,bool option) = 0;
    virtual EventObject make_subform(std::int32_t type) = 0;
    virtual EventObject make_share_window(bool first_time) = 0;
};
bool building_is_shop(std::int32_t kind);
void save_select_position(const SelectBuildingState&,SelectBuildingHost&);
// true returns from Update; false continues at the area button, 0x11ecdcc.
bool process_select_building_button(SelectBuildingState&,SelectFormPassState&,SelectBuildingHost&);
struct SelectBuildingUpdate {SelectPreludeSnapshot snapshot;bool returned;};
SelectBuildingUpdate update_select_through_building(SelectUpdateBindings&,SelectBuildingState&,SelectBuildingHost&);
}
