#pragma once
#include "select_town_buttons.hpp"
#include <exception>
namespace saga::core {
// Host adapters translate only original System.Exception failures into this type.
// Their diagnostic payload belongs to the adapter, not to the portable controller.
class SelectSaveError : public std::exception {};
class SelectGuidanceSaveHost {
public:
    virtual ~SelectGuidanceSaveHost() = default;
    virtual std::shared_ptr<BuildingSlots> building_slots() = 0;
    virtual bool can_build(std::int32_t index) = 0;
    virtual bool is_top_form() = 0;
    virtual void save_game(bool automatic,bool option) = 0;
    virtual void report_autosave_error(const SelectSaveError&) = 0;
    virtual bool soft_label(std::int32_t index) = 0;
    virtual void save_system() = 0;
    virtual void show_offerwall() = 0;
    virtual bool print_enabled() = 0;
    virtual void report_offerwall_error(const SelectSaveError&) = 0;
};
// SelectForm.Update 0x11ed144..0x11ed520. Call only after all button branches fall through.
// Continues at MoveCursor; this function does not yet perform the late timer/review pass.
void process_select_guidance_save(SelectBuildingState&,SelectFormPassState&,SelectGuidanceSaveHost&);
SelectBuildingUpdate update_select_through_guidance_save(SelectUpdateBindings&,SelectBuildingState&,
    SelectBuildingHost&,SelectTownButtonsState&,TownBubbleState&,SelectTownButtonsHost&,SelectGuidanceSaveHost&);
}
