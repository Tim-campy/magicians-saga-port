#include "core/gameplay/form_push.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool v,const char* m) {if (!v) {std::cerr<<m;std::exit(1);}}
struct Stack:FormStackHost {
    FormPushState* state=nullptr;
    EventObject seen;
    int calls=0;bool fail=false,observe_dialog=false;
    void push(EventObject parent,EventObject child,bool option) override {
        ++calls;check(parent==state->identity && !option,"parent and manager option");
        if (observe_dialog) check(state->dialog==child,"dialog assigned before manager push");
        seen=child;
        if(fail) throw std::runtime_error("manager failure");
    }
};
struct Factory:DialogFactory {
    EventObject child=std::make_shared<int>(1);bool fail=false;
    int calls=0;std::int32_t value=0;FormText first,second;
    EventObject text(FormText a) override {++calls;first=a;if(fail)throw std::runtime_error("factory failure");return child;}
    EventObject choice(FormText a,FormText b) override {second=b;return text(a);}
    EventObject choice_with_value(FormText a,FormText b,std::int32_t n) override {value=n;return choice(a,b);}
};
int main() {
    auto identity=std::make_shared<int>(0),old=std::make_shared<int>(2);
    FormPushState state{identity,nullptr,old};
    check(push_base_form(state,nullptr),"absent manager still returns true");
    Stack stack;stack.state=&state;state.manager=&stack;
    Factory factory;auto text=std::make_shared<const std::string>("synthetic");
    check(push_text_dialog(state,factory,text) && state.dialog==old,"single-text overload leaves old dialog");
    check(factory.first==text && stack.seen==factory.child,"factory input and child identity");
    stack.observe_dialog=true;
    check(push_choice_dialog(state,factory,nullptr,text) && state.dialog==factory.child,"choice stores dialog, null text passed through");
    check(push_choice_dialog(state,factory,text,nullptr,-3) && factory.value==-3,"explicit factory value");
    check(consume_dialog_update_gate(state) && !consume_dialog_update_gate(state),"dialog gate consumed once");
    state.dialog=old;factory.fail=true;
    try {push_choice_dialog(state,factory,text,text);check(false,"expected factory failure");}catch(const std::runtime_error&) {}
    check(state.dialog==old,"factory failure leaves previous dialog");factory.fail=false;
    stack.fail=true;
    try {push_choice_dialog(state,factory,text,text);check(false,"expected manager failure");}catch(const std::runtime_error&) {}
    check(state.dialog==factory.child,"manager failure keeps assigned dialog");
    bool autosave=false,event_check=false;stack.observe_dialog=false;
    try {push_select_form(state,nullptr,autosave,event_check);check(false,"expected manager failure");}catch(const std::runtime_error&) {}
    check(autosave && event_check,"select flags set before manager failure");
    stack.fail=false;factory.child=nullptr;
    check(push_choice_dialog(state,factory,text,text) && !state.dialog && !stack.seen,"null factory result forwarded by two-text wrapper");
}
