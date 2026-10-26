#pragma once
#include "call_context.hpp"
#include "context_switch.hpp"
#include <functional>
namespace saga::core {
struct ExecutionContextDomain {
    ExecutionContextDomain();
    std::shared_ptr<AsyncExecutionState> preallocated_default,default_context;
};
// Call-context fields must contain their matching portable types.
bool logical_call_has_info(const std::shared_ptr<LogicalCallContext>&);
bool default_execution_context(const AsyncExecutionState&,bool ignore_synchronization);
// Options bit 0 ignores synchronization; bit 1 enables default-context optimization.
std::shared_ptr<AsyncExecutionState> capture_execution_context(ThreadExecutionState&,std::uint32_t options,
    ExecutionContextDomain&,CallContextHost&,const std::function<AsyncValue(AsyncValue)>& clone_synchronization);
// Recovered four-argument Run/RunInternal route, with explicit thread and failure host.
void run_execution_context(ThreadExecutionState&,std::shared_ptr<AsyncExecutionState>,
    const std::function<void(AsyncValue)>& callback,AsyncValue state,bool preserve_synchronization,
    ContextNotificationFailureHost&);
}
