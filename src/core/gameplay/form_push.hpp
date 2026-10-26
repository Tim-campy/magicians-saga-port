#pragma once
#include "event_flow.hpp"
#include <string>
namespace saga::core {
using FormText = std::shared_ptr<const std::string>;
class FormStackHost {
public:
    virtual ~FormStackHost() = default;
    virtual void push(EventObject parent,EventObject child,bool option) = 0;
};
struct FormPushState {
    EventObject identity;
    FormStackHost* manager;
    EventObject dialog;
};
class DialogFactory {
public:
    virtual ~DialogFactory() = default;
    // SubForm.MakeDialog overloads; preserves null text semantics in the host.
    virtual EventObject text(FormText) = 0;
    virtual EventObject choice(FormText,FormText) = 0;
    // Host includes MakeDialog's +0x13c assignment and null-result failure.
    virtual EventObject choice_with_value(FormText,FormText,std::int32_t) = 0;
};
bool push_base_form(FormPushState&,EventObject child);
bool push_select_form(FormPushState&,EventObject child,bool& auto_save,bool& event_check);
bool push_text_dialog(FormPushState&,DialogFactory&,FormText);
bool push_choice_dialog(FormPushState&,DialogFactory&,FormText,FormText);
bool push_choice_dialog(FormPushState&,DialogFactory&,FormText,FormText,std::int32_t value);
// SelectForm.Update's dialog check only, after its earlier common updates.
bool consume_dialog_update_gate(FormPushState&);
}
