#include "core/gameplay/context_switch.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
using namespace saga::core;
void check(bool v){if(!v){std::cerr<<"context switch contract failure\n";std::exit(1);}}
struct Failure:ContextNotificationFailureHost{void fail_fast(std::exception_ptr)override{throw std::logic_error("terminal host signal");}};
struct Key:AsyncLocalKey{
    std::function<void(AsyncValue,AsyncValue)> callback;
    void changed(AsyncValue a,AsyncValue b,bool changed_context)override{check(changed_context);callback(a,b);}
};
auto state(std::shared_ptr<Key> key,AsyncValue value){
    auto s=std::make_shared<AsyncExecutionState>();s->values=std::make_shared<AsyncLocalValues>();
    s->values->push_back({key,value});s->notifications=std::make_shared<AsyncLocalNotifications>();s->notifications->keys={key};return s;
}
int main(){
    auto x=std::make_shared<int>(1),y=std::make_shared<int>(2),z=std::make_shared<int>(3);
    auto key=std::make_shared<Key>();Failure failure;
    auto old=state(key,x),next=state(key,y);old->synchronization=x;old->synchronization_no_flow=y;next->synchronization=z;
    ThreadExecutionState thread{old,true};int calls=0;
    key->callback=[&](AsyncValue a,AsyncValue b){++calls;check(a==x&&b==y&&thread.context==next&&!thread.belongs_to_outer_scope);};
    auto saved=set_execution_context(thread,next,false,failure);
    check(calls==1&&next->synchronization==z&&next->synchronization_no_flow==y);
    key->callback=[&](AsyncValue a,AsyncValue b){++calls;check(a==y&&b==x&&thread.context==old&&thread.belongs_to_outer_scope&&!saved.thread);undo_execution_context(saved,failure);};
    undo_execution_context(saved,failure);undo_execution_context(saved,failure);check(calls==2);
    key->callback=[](AsyncValue,AsyncValue){};
    saved=set_execution_context(thread,next,true,failure);check(next->synchronization==x);undo_execution_context(saved,failure);
    // A managed forward failure rolls back; a managed rollback failure does not replace it.
    calls=0;key->callback=[&](AsyncValue,AsyncValue b){++calls;if(b==y)throw CultureManagedError("forward");throw CultureManagedException("rollback");};
    try{set_execution_context(thread,next,false,failure);check(false);}catch(const CultureManagedError& e){check(std::string(e.what())=="forward");}
    check(calls==2&&thread.context==old&&thread.belongs_to_outer_scope);
    // Native failures lie outside the managed catch, leaving the published state intact.
    key->callback=[](AsyncValue,AsyncValue){throw std::logic_error("host failure");};
    try{set_execution_context(thread,next,false,failure);check(false);}catch(const std::logic_error&){}
    check(thread.context==next&&!thread.belongs_to_outer_scope);
    // Undo uses the live outgoing context, including a reentrant replacement.
    auto third=state(key,z);thread={old,true};calls=0;
    key->callback=[&](AsyncValue,AsyncValue){thread.context=third;};saved=set_execution_context(thread,next,false,failure);
    key->callback=[&](AsyncValue a,AsyncValue b){++calls;check(a==z&&b==x);};undo_execution_context(saved,failure);check(calls==1&&thread.context==old);
    key->callback=[](AsyncValue,AsyncValue){};saved=set_execution_context(thread,next,false,failure);
    key->callback=[](AsyncValue,AsyncValue){throw CultureManagedError("undo");};
    check(!undo_execution_context_no_throw(saved,failure)&&thread.context==old&&!saved.thread);
    check(undo_execution_context_no_throw(saved,failure));
    try{set_execution_context(thread,nullptr,true,failure);check(false);}catch(const CultureManagedException&){}
    check(thread.context==old&&thread.belongs_to_outer_scope);
    // Empty prior context clears both synchronization references when preserving.
    ThreadExecutionState empty;auto target=std::make_shared<AsyncExecutionState>();target->synchronization=x;target->synchronization_no_flow=y;
    auto token=set_execution_context(empty,target,true,failure);check(!target->synchronization&&!target->synchronization_no_flow);
    undo_execution_context(token,failure);check(!empty.context);
}
