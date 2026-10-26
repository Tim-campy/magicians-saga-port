#include "select_town_buttons.hpp"
#include <limits>
#include <stdexcept>
namespace saga::core {
namespace {
template<class T> T& required(const std::shared_ptr<T>& value) {
    if(!value)throw std::invalid_argument("null town button data");
    return *value;
}
std::int32_t first_human(std::int32_t state,TownBubbleHost& host) {
    for(std::int32_t i=0;;++i) {
        const auto rows=host.town_humans();
        const auto size=required(rows).size();
        if(size>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("town list exceeds int32");
        if(static_cast<std::size_t>(i)>=size)return -1;
        if(required(rows->at(static_cast<std::size_t>(i))).at(8)==state)return i;
    }
}
void set_human_state(std::int32_t index,std::int32_t value,TownBubbleHost& host) {
    const auto row=required(host.town_humans()).at(static_cast<std::size_t>(index));
    required(row).at(8)=value;
}
}
bool process_select_town_buttons(SelectTownButtonsState& state,SelectBuildingState& building,
    SelectFormPassState& form,TownBubbleState& bubble,SelectBuildingHost& inputs,SelectTownButtonsHost& host) {
    if(inputs.pressed(0x10000000U)) {
        inputs.play_button_sound();
        save_select_position(building,inputs);
        const auto save=inputs.save_integers();
        required(save).at(8)=state.selected_area;
        state.selected_area=-1;
        const auto child=host.area_form();
        push_select_form(form.form,child,form.auto_save,form.events.event_check_requested);
        return true;
    }
    if(inputs.pressed(0x20000000U)) {
        building.unlock_info_number=-1;building.unlock_info_frames=-1;
        inputs.play_button_sound();
        const auto child=host.make_obstacle(building.selected_chip);
        push_select_form(form.form,child,form.auto_save,form.events.event_check_requested);
        building.selected_chip=-1;
        return true;
    }
    if(inputs.pressed(0x40000000U)) {
        const auto child=host.make_store_invite();
        push_select_form(form.form,child,form.auto_save,form.events.event_check_requested);
        const auto index=first_human(1,host);
        if(index!=-1)set_human_state(index,2,host);
        const auto count=host.sum_quest_challenges();
        const auto save=host.system_save_integers();
        const auto slot=host.store_limit_slot();
        required(save).at(static_cast<std::size_t>(slot))=count;
        bubble.index=-1;bubble.frames=0;
        static const auto empty=std::make_shared<const std::string>();
        bubble.text=empty;
        return true;
    }
    if(inputs.pressed(0x02000000U)) {
        const auto index=first_human(3,host);
        if(index!=-1) {
            set_human_state(index,0,host);
            host.add_money(state.resident_present_value);
            const auto formatted=host.format_present_value(state.resident_present_value); // Re-read after credit.
            const auto child=host.make_present_popup(formatted);
            push_select_form(form.form,child,form.auto_save,form.events.event_check_requested);
            set_town_bubble(bubble,index,false,host);
        }
        return true; // No eligible resident still consumes this button branch.
    }
    return false;
}
SelectBuildingUpdate update_select_through_town_buttons(SelectUpdateBindings& bindings,SelectBuildingState& building,
    SelectBuildingHost& inputs,SelectTownButtonsState& state,TownBubbleState& bubble,SelectTownButtonsHost& host) {
    const auto prior=update_select_through_building(bindings,building,inputs);
    if(prior.returned)return prior;
    return {prior.snapshot,process_select_town_buttons(state,building,*bindings.state,bubble,inputs,host)};
}
}
