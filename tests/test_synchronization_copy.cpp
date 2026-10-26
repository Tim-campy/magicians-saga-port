#include "core/gameplay/synchronization_copy.hpp"
#include "core/gameplay/context_capture.hpp"
#include <cstdlib>
#include <iostream>
using namespace saga::core;
void check(bool v){if(!v){std::cerr<<"synchronization copy contract failure\n";std::exit(1);}}
struct Calls:CallContextHost{
    std::shared_ptr<CallDataStore> make_store()override{throw std::logic_error("unused");}
    std::optional<std::u16string> string_key(AsyncValue)override{throw std::logic_error("unused");}
    AsyncValue clone_correlation(AsyncValue)override{throw std::logic_error("unused");}
};
struct Failure:ContextNotificationFailureHost{void fail_fast(std::exception_ptr)override{check(false);}};
int main(){
    auto base=std::make_shared<SynchronizationContext>();base->properties=7;
    auto copied=base->create_copy();check(copied!=base&&copied->properties==0);
    auto os=std::make_shared<OsSynchronizationContext>();os->properties=9;os->native_handle=0xfedcba9876543210ULL;
    auto os_copy=std::dynamic_pointer_cast<OsSynchronizationContext>(os->create_copy());
    check(os_copy&&os_copy!=os&&os_copy->properties==0&&os_copy->native_handle==os->native_handle);
    auto queue=std::make_shared<SynchronizationWorkList>();auto value=std::make_shared<int>(1);queue->entries={value};
    auto unity=std::make_shared<QueuedSynchronizationContext>(queue,-123);unity->properties=3;unity->tracked_count=8;
    unity->current_frame->entries={value};unity->current_frame->version=9;
    auto clone=std::dynamic_pointer_cast<QueuedSynchronizationContext>(unity->create_copy());
    check(clone&&clone!=unity&&clone->async_queue==queue&&clone->main_thread_id==-123);
    check(clone->properties==0&&clone->tracked_count==0&&clone->current_frame!=unity->current_frame);
    check(clone->current_frame->entries.empty()&&clone->current_frame->version==0&&clone->current_frame->initial_capacity==20);
    clone->async_queue->entries.push_back(value);check(unity->async_queue->entries.size()==2&&unity->current_frame->entries.size()==1);
    Calls calls;Failure failure;ExecutionContextDomain domain;ThreadExecutionState thread{std::make_shared<AsyncExecutionState>(),false};
    auto original=thread.context;original->synchronization=std::static_pointer_cast<SynchronizationContext>(unity);
    auto captured=capture_execution_context(thread,0,domain,calls,copy_synchronization_context);
    auto sync=std::dynamic_pointer_cast<QueuedSynchronizationContext>(std::static_pointer_cast<SynchronizationContext>(captured->synchronization));
    check(sync&&sync!=unity&&sync->async_queue==queue);
    int invoked=0;
    run_captured_execution_context(thread,captured,[&](AsyncValue v){++invoked;check(v==value&&thread.context==captured&&thread.context->synchronization==sync);},value,failure);
    check(thread.context==original&&invoked==1&&captured->flags==0);
    try{run_captured_execution_context(thread,captured,{},nullptr,failure);check(false);}catch(const CultureManagedException&){}
    check(thread.context==original&&invoked==1);
    auto failed=capture_execution_context(thread,0,domain,calls,copy_synchronization_context);
    try{run_captured_execution_context(thread,failed,{},nullptr,failure);check(false);}catch(const CultureManagedException&){}
    check(failed->flags==0&&thread.context==original);
    for(int i=0;i<2;++i)run_captured_execution_context(thread,domain.preallocated_default,[&](AsyncValue){++invoked;},nullptr,failure);
    check(invoked==3&&domain.preallocated_default->flags==4);
    try{run_captured_execution_context(thread,nullptr,{},nullptr,failure);check(false);}catch(const CultureManagedException&){}
}
