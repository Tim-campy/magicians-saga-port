#include "form_push.hpp"
namespace saga::core {
// FormBase.Push, 0x1bda598: absent manager is tolerated, child is not filtered.
bool push_base_form(FormPushState& state,EventObject child) {
    if (state.manager) state.manager->push(state.identity,child,false);
    return true;
}
// SelectForm.Push, 0x11eec34: STRH 0x0101 to static +0x30/+0x31.
bool push_select_form(FormPushState& state,EventObject child,bool& auto_save,bool& event_check) {
    auto_save=true;event_check=true;
    return push_base_form(state,child);
}
// MyFormBase.Push(string), 0x11d7498: deliberately does not write dialog_.
bool push_text_dialog(FormPushState& state,DialogFactory& factory,FormText text) {
    return push_base_form(state,factory.text(text));
}
// MyFormBase.Push(string,string), 0x11d750c.
bool push_choice_dialog(FormPushState& state,DialogFactory& factory,FormText text,FormText choice) {
    const auto child=factory.choice(text,choice);
    state.dialog=child;
    return push_base_form(state,child);
}
// MyFormBase.Push(string,string,int), 0x11d758c.
bool push_choice_dialog(FormPushState& state,DialogFactory& factory,FormText text,FormText choice,std::int32_t value) {
    const auto child=factory.choice_with_value(text,choice,value);
    state.dialog=child;
    return push_base_form(state,child);
}
// SelectForm.Update, 0x11ebde8..0x11ebe18; true means return from this update.
bool consume_dialog_update_gate(FormPushState& state) {
    if (!state.dialog) return false;
    state.dialog.reset();
    return true;
}
}
