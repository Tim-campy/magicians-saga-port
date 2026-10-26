#include "core/gameplay/thread_culture_set.hpp"
#include "core/gameplay/context_switch.hpp"
#include "core/gameplay/async_local_values.hpp"
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
struct Host:ThreadCultureWriteHost,CurrentCultureHost,AsyncExecutionHost {
    std::vector<std::string> calls;
    std::shared_ptr<CultureInstance> thread,ui,user=std::make_shared<CultureInstance>(),user_ui=std::make_shared<CultureInstance>();
    std::shared_ptr<Slot> existing,candidate=std::make_shared<Slot>(),winner=std::make_shared<Slot>();
    bool disable=true,fail_ui=false,fail_make=false,lose=false,no_publish=false;int invariant_calls=0;
    bool real=false;std::shared_ptr<AsyncCultureSlot> real_slot;
    std::shared_ptr<AsyncExecutionState> execution=std::make_shared<AsyncExecutionState>();
    std::shared_ptr<AsyncExecutionState> mutable_execution_context()override{return execution;}
    std::shared_ptr<CultureInstance> thread_culture()override{calls.push_back("thread");return thread;}
    std::shared_ptr<CultureInstance> default_thread_culture()override{return nullptr;}
    std::shared_ptr<CultureInstance> thread_ui_culture()override{calls.push_back("ui");return ui;}
    void write_thread_culture(std::shared_ptr<CultureInstance> value)override{calls.push_back("write");thread=value;}
    void write_thread_ui_culture(std::shared_ptr<CultureInstance> value)override{calls.push_back("write_ui");ui=value;}
    bool disable_async_culture_flow()override{calls.push_back("switch");return disable;}
    std::shared_ptr<AsyncCultureSlot> async_culture_slot()override{calls.push_back("slot");return real?real_slot:existing;}
    std::shared_ptr<AsyncCultureSlot> make_async_culture_slot()override{calls.push_back("make");if(fail_make)throw CultureManagedException("make");if(real)return std::make_shared<ContextCultureSlot>(*this,*this);return candidate;}
    void publish_async_culture_slot(std::shared_ptr<AsyncCultureSlot> value)override{calls.push_back("publish");if(real){if(!real_slot)real_slot=value;return;}check(value==candidate);if(!no_publish)existing=lose?winner:candidate;}
    bool invariant_mode()override{calls.push_back("mode");return fail_ui;}
    std::optional<std::u16string> native_locale_name()override{calls.push_back("name");return u"supplied";}
    std::shared_ptr<CultureInstance> create_specific(const std::u16string&)override{calls.push_back("specific");return user;}
    std::shared_ptr<CultureInstance> invariant_culture()override{calls.push_back("invariant");if(++invariant_calls==2)throw CultureManagedException("second invariant query");return user;}
};
struct NoFailure:ContextNotificationFailureHost{void fail_fast(std::exception_ptr)override{check(false);}};
struct DefaultFlowHost:Host {
    bool disable_async_culture_flow()override{return ThreadCultureWriteHost::disable_async_culture_flow();}
};
int main(){
    auto incoming=std::make_shared<CultureInstance>();
    {DefaultFlowHost h;CurrentCultureState s;h.thread=h.user;h.real=true;
        check(!h.disable_async_culture_flow());
        set_thread_culture(incoming,h,s,h);
        check(h.thread==incoming&&h.real_slot&&h.execution->values->size()==1);
        check(h.execution->notifications->keys.size()==1);}
    {Host h;CurrentCultureState s;h.thread=h.user;h.disable=false;h.real=true;
        set_thread_culture(incoming,h,s,h);
        check(h.thread==incoming && h.execution->values->size()==1 && h.execution->notifications->keys.size()==1);
        auto saved=h.execution->values;auto before=h.calls.size();
        h.real_slot->set(incoming);check(h.calls.size()==before && h.execution->values==saved);
        h.real_slot->set(nullptr);check(!h.thread && (*saved)[0].value==incoming);}
    {Host h;CurrentCultureState s;NoFailure failure;h.thread=h.user;h.disable=false;h.real=true;
        set_thread_culture(incoming,h,s,h);
        ThreadExecutionState execution{h.execution,true};
        auto next=std::make_shared<AsyncExecutionState>(*h.execution);
        next->values=std::make_shared<AsyncLocalValues>(*h.execution->values);
        (*next->values)[0].value=h.user;
        auto saved=set_execution_context(execution,next,false,failure);
        check(h.thread==h.user && execution.context==next);
        undo_execution_context(saved,failure);
        check(h.thread==incoming && execution.context==h.execution && execution.belongs_to_outer_scope);}
    {Host h;CurrentCultureState s;try{set_thread_culture(nullptr,h,s,h);check(false);}catch(const CultureMutationError& e){check(e.kind==CultureMutationFailure::NullValue);}check(h.calls.empty());}
    {Host h;CurrentCultureState s;set_thread_culture(incoming,h,s,h);
        check((h.calls==std::vector<std::string>{"thread","ui","mode","name","specific","write","write_ui","switch","write"}));
        check(h.thread==incoming && h.ui==h.user && s.cached==h.user && !incoming->read_only);}
    {Host h;CurrentCultureState s;h.thread=h.user;set_thread_culture(incoming,h,s,h);
        check((h.calls==std::vector<std::string>{"thread","switch","write"}) && !s.cached);}
    {Host h;CurrentCultureState s;h.ui=h.user_ui;set_thread_culture(incoming,h,s,h);
        check((h.calls==std::vector<std::string>{"thread","ui","switch","write"}) && !s.cached);}
    {Host h;CurrentCultureState s;h.fail_ui=true;
        try{set_thread_culture(incoming,h,s,h);check(false);}catch(const CultureManagedException&){}
        check(h.thread==h.user && !h.ui && !s.cached && h.calls.back()=="invariant");}
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
