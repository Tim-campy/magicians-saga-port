#include "core/gameplay/context_capture.hpp"
#include <cstdlib>
#include <iostream>
using namespace saga::core;
void check(bool v){if(!v){std::cerr<<"context capture contract failure\n";std::exit(1);}}
struct Calls:CallContextHost{
    std::shared_ptr<CallDataStore> make_store()override{throw std::logic_error("unused store");}
    std::optional<std::u16string> string_key(AsyncValue)override{throw std::logic_error("unused key");}
    AsyncValue clone_correlation(AsyncValue)override{throw std::logic_error("unused clone");}
};
struct Failure:ContextNotificationFailureHost{void fail_fast(std::exception_ptr)override{throw std::logic_error("fatal");}};
struct Key:AsyncLocalKey{
    std::vector<bool> changes;
    void changed(AsyncValue,AsyncValue,bool context_changed)override{changes.push_back(context_changed);}
};
int main(){
    Calls host;Failure failure;ExecutionContextDomain domain;ThreadExecutionState thread;
    auto x=std::make_shared<int>(1),y=std::make_shared<int>(2);
    check(domain.preallocated_default!=domain.default_context&&domain.preallocated_default->flags==4&&domain.default_context->flags==0);
    auto captured=capture_execution_context(thread,0,domain,host,{});check(captured&&captured->flags==1&&captured!=domain.preallocated_default);
    check(capture_execution_context(thread,3,domain,host,{})==domain.preallocated_default);
    thread.context=std::make_shared<AsyncExecutionState>();auto original=thread.context;
    original->flags=2;int copies=0;original->synchronization=x;
    check(!capture_execution_context(thread,0,domain,host,[&](AsyncValue){++copies;return y;}));check(copies==0&&!original->logical_call);
    original->flags=0;original->synchronization_no_flow=y;original->illogical_call=std::make_shared<IllogicalCallContext>();
    auto call=std::make_shared<LogicalCallContext>();call->host_context=x;call->send_headers=y;original->logical_call=call;
    captured=capture_execution_context(thread,0,domain,host,[&](AsyncValue v){check(v==x);++copies;
        original->values=std::make_shared<AsyncLocalValues>();return y;});
    check(copies==1&&captured->synchronization==y&&captured->values==original->values);
    auto cloned=std::static_pointer_cast<LogicalCallContext>(captured->logical_call);
    check(cloned!=call&&cloned->host_context==x&&!cloned->send_headers);
    check(!captured->illogical_call&&!captured->synchronization_no_flow&&captured->flags==1);
    original->logical_call=nullptr;original->values=nullptr;original->notifications=nullptr;
    check(capture_execution_context(thread,3,domain,host,{})==domain.preallocated_default);
    check(bool(original->logical_call)); // Getter lazily initializes even when capture returns default.
    original->values=std::make_shared<AsyncLocalValues>();
    check(capture_execution_context(thread,3,domain,host,{})!=domain.preallocated_default); // Empty dictionary still counts by identity.
    original->values=nullptr;original->synchronization=nullptr;
    auto illogical=std::static_pointer_cast<IllogicalCallContext>(original->illogical_call);illogical->host_context=x;
    check(default_execution_context(*original,false)); // Illogical host is ignored by the predicate.
    captured=capture_execution_context(thread,0,domain,host,{});
    TypedCallContextCopyHost copy_host(host);OwnedAsyncExecutionHost mutable_host(thread,copy_host);auto key=std::make_shared<Key>();
    run_execution_context(thread,captured,[&](AsyncValue argument){
        check(argument==y&&thread.context==original&&thread.belongs_to_outer_scope);
        set_async_local_value(key,x,true,mutable_host);check(thread.context!=original);
    },y,false,failure);
    check(thread.context==original&&!thread.belongs_to_outer_scope&&captured->flags==0);
    check((key->changes==std::vector<bool>{false,true}));
    // Default sentinel is replaced with a fresh target on the full-install route.
    original->values=std::make_shared<AsyncLocalValues>();
    run_execution_context(thread,domain.preallocated_default,[&](AsyncValue){
        check(thread.context!=original&&thread.context!=domain.preallocated_default&&thread.context->flags==0);
    },nullptr,false,failure);
    check(thread.context==original&&domain.preallocated_default->flags==4);
    try{run_execution_context(thread,captured,[&](AsyncValue){throw std::logic_error("body");},nullptr,false,failure);check(false);}catch(const std::logic_error&){}
    check(thread.context==original&&!thread.belongs_to_outer_scope);
    try{run_execution_context(thread,captured,{},nullptr,false,failure);check(false);}catch(const CultureManagedException&){}
    check(thread.context==original);
}
