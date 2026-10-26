#pragma once
#include "select_building.hpp"
#include "town_bubble.hpp"
namespace saga::core {
struct SelectTownButtonsState {std::int32_t selected_area;std::int32_t resident_present_value;};
class SelectTownButtonsHost:public TownBubbleHost {
public:
    virtual EventObject area_form() = 0; // Requires AppData, nullable frmSelectStage_.
    virtual EventObject make_obstacle(std::int32_t chip) = 0;
    virtual EventObject make_store_invite() = 0;
    virtual std::int32_t sum_quest_challenges() = 0;
    virtual std::shared_ptr<EventIntegers> system_save_integers() = 0;
    virtual std::int32_t store_limit_slot() = 0; // SYSSAVEI_STORE_KAIRO_LIMIT, no guessed index.
    virtual void add_money(std::int32_t amount) = 0;
    // Original localized formatting literal and popup type 3/title/option 0 remain host data.
    virtual FormText format_present_value(std::int32_t amount) = 0;
    virtual EventObject make_present_popup(FormText formatted) = 0;
};
// true returns from Update; false continues at selected-chip guidance, 0x11ed144.
bool process_select_town_buttons(SelectTownButtonsState&,SelectBuildingState&,SelectFormPassState&,
    TownBubbleState&,SelectBuildingHost&,SelectTownButtonsHost&);
SelectBuildingUpdate update_select_through_town_buttons(SelectUpdateBindings&,SelectBuildingState&,
    SelectBuildingHost&,SelectTownButtonsState&,TownBubbleState&,SelectTownButtonsHost&);
}
