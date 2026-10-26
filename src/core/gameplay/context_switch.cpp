#include "context_switch.hpp"
namespace saga::core {
// ExecutionContextSwitcher.Undo 0x1ec9f14.
void undo_execution_context(ExecutionContextSwitch& saved,ContextNotificationFailureHost& failure){
    auto thread=saved.thread;
    if(!thread)return;
    const auto restored=saved.previous;
    const auto outgoing=thread->context;
    thread->context=restored;
    thread->belongs_to_outer_scope=saved.previous_belongs_to_outer_scope;
    saved.thread=nullptr;
    notify_async_context_changed(outgoing,restored,failure);
}
// ExecutionContextSwitcher.UndoNoThrow 0x1ec9e8c: System.Object category.
bool undo_execution_context_no_throw(ExecutionContextSwitch& saved,ContextNotificationFailureHost& failure){
    try{undo_execution_context(saved,failure);return true;}
    catch(const CultureManagedError&){return false;}
}
// ExecutionContext.SetExecutionContext 0x1ecad0c; current thread is an explicit parameter.
ExecutionContextSwitch set_execution_context(ThreadExecutionState& thread,std::shared_ptr<AsyncExecutionState> next,
                                             bool preserve_synchronization,ContextNotificationFailureHost& failure){
    ExecutionContextSwitch saved{thread.context,thread.belongs_to_outer_scope,&thread};
    // Null-target failure precedes publication and is outside the managed rollback scope.
    if(!next)throw CultureManagedException("null execution context");
    if(preserve_synchronization)next->synchronization=saved.previous?saved.previous->synchronization:nullptr;
    next->synchronization_no_flow=saved.previous?saved.previous->synchronization_no_flow:nullptr;
    thread.context=next;
    thread.belongs_to_outer_scope=false;
    // RuntimeHelpers.PrepareConstrainedRegions 0x1d89248 is RET in this artifact.
    try{notify_async_context_changed(saved.previous,next,failure);}
    catch(const CultureManagedError&){
        undo_execution_context_no_throw(saved,failure);
        throw;
    }
    return saved;
}
}
