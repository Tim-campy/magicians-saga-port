#include "select_navigation.hpp"
#include <stdexcept>
namespace saga::core {
SelectNavigationUpdate update_select_through_navigation(SelectUpdateBindings& b) {
    if(!b.state)throw std::invalid_argument("missing SelectForm owner");
    const auto prefix=update_select_form_prefix(b.prelude,b.prelude_host,*b.state,b.pass_host,
        b.clouds,b.markers,b.queue,b.execution,b.conditions,b.movement,b.movement_host,b.triggers);
    if(prefix.pass==SelectPassResult::ReturnTrue)return {prefix.snapshot,SelectNavigationResult::ReturnTrue};
    return {prefix.snapshot,process_select_navigation(b.state,b.navigation)};
}
// SelectForm.<Update>b__117_0, 0x11f5a2c: clear before the required AppData access.
void complete_select_room_transition(SelectFormPassState& state,SelectNavigationHost& host) {
    state.markers.weapon_new=false;
    const auto room=host.room_form();
    push_select_form(state.form,room,state.auto_save,state.events.event_check_requested);
}
SelectNavigationResult process_select_navigation(std::shared_ptr<SelectFormPassState> state,std::shared_ptr<SelectNavigationHost> host) {
    if(!state || !host)throw std::invalid_argument("missing navigation owner");
    if(host->pressed(0x04000000U)) {
        host->play_button_sound();
        auto action=[state,host] {complete_select_room_transition(*state,*host);};
        // GetColorOfRGB(48,48,48): clamped channels packed with alpha 0xff.
        host->make_transition(3,true,0xff303030U,std::move(action));
        return SelectNavigationResult::ReturnTrue;
    }
    if(host->pressed(0x01000000U)) {
        const auto shop=host->make_shop(11);
        push_select_form(state->form,shop,state->auto_save,state->events.event_check_requested);
        return SelectNavigationResult::ReturnTrue;
    }
    return SelectNavigationResult::ContinueBuildingButtons;
}
}
