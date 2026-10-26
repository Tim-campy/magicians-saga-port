#pragma once
#include "select_navigation.hpp"
namespace saga::core {
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
