#include "core/gameplay/thread_culture_set.hpp"
#include "core/gameplay/culture_setters.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace saga::core;
void check(bool b){if(!b){std::cerr<<"thread culture setter contract failure\n";std::exit(1);}}
struct Slot:AsyncCultureSlot {
    int calls=0;bool fail=false;std::shared_ptr<CultureInstance> value;
    void set(std::shared_ptr<CultureInstance> incoming)override{++calls;value=incoming;if(fail)throw CultureManagedException("slot");}
};
struct Host:ThreadCultureWriteHost,CurrentCultureHost {
    std::vector<std::string> calls;
    std::shared_ptr<CultureInstance> thread,ui,user=std::make_shared<CultureInstance>(),user_ui=std::make_shared<CultureInstance>();
    std::shared_ptr<Slot> existing,candidate=std::make_shared<Slot>(),winner=std::make_shared<Slot>();
    bool disable=true,fail_ui=false,fail_make=false,lose=false,no_publish=false;
    std::shared_ptr<CultureInstance> thread_culture()override{calls.push_back("thread");return thread;}
    std::shared_ptr<CultureInstance> default_thread_culture()override{return nullptr;}
    std::shared_ptr<CultureInstance> thread_ui_culture()override{calls.push_back("ui");return ui;}
    void write_thread_culture(std::shared_ptr<CultureInstance> value)override{calls.push_back("write");thread=value;}
    void write_thread_ui_culture(std::shared_ptr<CultureInstance> value)override{calls.push_back("write_ui");ui=value;}
    std::shared_ptr<CultureInstance> construct_user_ui_culture()override{calls.push_back("user_ui");if(fail_ui)throw CultureManagedException("ui");return user_ui;}
    bool disable_async_culture_flow()override{calls.push_back("switch");return disable;}
    std::shared_ptr<AsyncCultureSlot> async_culture_slot()override{calls.push_back("slot");return existing;}
    std::shared_ptr<AsyncCultureSlot> make_async_culture_slot()override{calls.push_back("make");if(fail_make)throw CultureManagedException("make");return candidate;}
    void publish_async_culture_slot(std::shared_ptr<AsyncCultureSlot> value)override{calls.push_back("publish");check(value==candidate);if(!no_publish)existing=lose?winner:candidate;}
    bool invariant_mode()override{calls.push_back("mode");return false;}
    std::optional<std::u16string> native_locale_name()override{calls.push_back("name");return u"supplied";}
    std::shared_ptr<CultureInstance> create_specific(const std::u16string&)override{calls.push_back("specific");return user;}
    std::shared_ptr<CultureInstance> invariant_culture()override{check(false);return {};}
};
int main(){
    auto incoming=std::make_shared<CultureInstance>();
    {Host h;CurrentCultureState s;try{set_thread_culture(nullptr,h,s,h);check(false);}catch(const CultureMutationError& e){check(e.kind==CultureMutationFailure::NullValue);}check(h.calls.empty());}
    {Host h;CurrentCultureState s;set_thread_culture(incoming,h,s,h);
        check((h.calls==std::vector<std::string>{"thread","ui","mode","name","specific","write","user_ui","write_ui","switch","write"}));
        check(h.thread==incoming && h.ui==h.user_ui && s.cached==h.user && !incoming->read_only);}
    {Host h;CurrentCultureState s;h.thread=h.user;set_thread_culture(incoming,h,s,h);
        check((h.calls==std::vector<std::string>{"thread","switch","write"}) && !s.cached);}
    {Host h;CurrentCultureState s;h.ui=h.user_ui;set_thread_culture(incoming,h,s,h);
        check((h.calls==std::vector<std::string>{"thread","ui","switch","write"}) && !s.cached);}
    {Host h;CurrentCultureState s;h.fail_ui=true;
        try{set_thread_culture(incoming,h,s,h);check(false);}catch(const CultureManagedException&){}
        check(h.thread==h.user && !h.ui && s.cached==h.user && h.calls.back()=="user_ui");}
    {Host h;CurrentCultureState s;h.thread=h.user;h.disable=false;h.lose=true;set_thread_culture(incoming,h,s,h);
        check((h.calls==std::vector<std::string>{"thread","switch","slot","make","publish","slot"}));
        check(h.winner->value==incoming && h.candidate->calls==0 && h.thread==h.user);
        async_local_set_current_culture(incoming,h);check(h.thread==incoming);
        async_local_set_current_culture(nullptr,h);check(!h.thread);}
    {Host h;CurrentCultureState s;h.thread=h.user;h.disable=false;h.existing=h.winner;h.winner->fail=true;
        try{set_thread_culture(incoming,h,s,h);check(false);}catch(const CultureManagedException&){}
        check((h.calls==std::vector<std::string>{"thread","switch","slot"}) && h.winner->value==incoming);}
    for(bool missing:{false,true}){Host h;CurrentCultureState s;h.thread=h.user;h.disable=false;h.no_publish=missing;h.fail_make=!missing;
        try{set_thread_culture(incoming,h,s,h);check(false);}catch(const CultureManagedException&){}
        check(h.thread==h.user && h.candidate->calls==0);}
}
