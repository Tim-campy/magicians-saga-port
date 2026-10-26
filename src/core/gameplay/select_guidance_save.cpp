#include "select_guidance_save.hpp"
#include <stdexcept>
namespace saga::core {
void process_select_guidance_save(SelectBuildingState& state,SelectFormPassState& form,SelectGuidanceSaveHost& host) {
    if(state.selected_chip==999) {
        state.unlock_info_number=999;
        state.unlock_info_frames=3;
    } else if(state.selected_chip!=-1) {
        const auto slots=host.building_slots();
        if(!slots)throw std::invalid_argument("missing building slots");
        const auto index=state.selected_chip;
        const auto row=slots->at(static_cast<std::size_t>(index));
        if(row && row->at(1)==-1 && !host.can_build(index)) {
            state.unlock_info_frames=60;
            state.unlock_info_number=state.selected_chip; // Re-read after CheckEnable.
            state.selected_chip=-1;
        }
    }
    if(form.auto_save && host.is_top_form()) {
        try {
            host.save_game(true,true);
        } catch(const SelectSaveError& error) {
            host.report_autosave_error(error);
        }
        form.auto_save=false; // Includes a handled save error, but not a reporting failure.
    }
    if(host.soft_label(19)) {
        try {
            host.save_game(false,true);
            host.save_system();
            host.show_offerwall();
        } catch(const SelectSaveError& error) {
            if(host.print_enabled())host.report_offerwall_error(error);
        }
    }
}
SelectBuildingUpdate update_select_through_guidance_save(SelectUpdateBindings& bindings,SelectBuildingState& building,
    SelectBuildingHost& inputs,SelectTownButtonsState& town,TownBubbleState& bubble,
    SelectTownButtonsHost& town_host,SelectGuidanceSaveHost& host) {
    const auto prior=update_select_through_town_buttons(bindings,building,inputs,town,bubble,town_host);
    if(!prior.returned)process_select_guidance_save(building,*bindings.state,host);
    return prior; // Unconsumed path now continues at MoveCursor, preserving the snapshots.
}
}
