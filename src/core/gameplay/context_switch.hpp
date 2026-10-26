#pragma once
#include "execution_context.hpp"
#include "context_notifications.hpp"
namespace saga::core {
struct ExecutionContextSwitch {
    std::shared_ptr<AsyncExecutionState> previous;
    bool previous_belongs_to_outer_scope=false;
    // Explicit thread storage must outlive this switch token. Copying matches the value-type original.
    ThreadExecutionState* thread=nullptr;
};
ExecutionContextSwitch set_execution_context(ThreadExecutionState&,std::shared_ptr<AsyncExecutionState>,
                                             bool preserve_synchronization,ContextNotificationFailureHost&);
void undo_execution_context(ExecutionContextSwitch&,ContextNotificationFailureHost&);
// Catches managed System.Object failures only; native/host failures can escape.
bool undo_execution_context_no_throw(ExecutionContextSwitch&,ContextNotificationFailureHost&);
}
