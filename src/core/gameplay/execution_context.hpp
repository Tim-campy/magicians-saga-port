#pragma once
#include "async_local_values.hpp"
namespace saga::core {
class ExecutionContextCopyHost {
public:
    virtual ~ExecutionContextCopyHost() = default;
    // Runtime call-context clones, including their managed type checks.
    virtual AsyncValue clone_logical_call(AsyncValue) = 0;
    virtual AsyncValue clone_illogical_call(AsyncValue) = 0;
};
struct ThreadExecutionState {
    std::shared_ptr<AsyncExecutionState> context;
    bool belongs_to_outer_scope=false;
};
std::shared_ptr<AsyncExecutionState> copy_mutable_execution_context(const AsyncExecutionState&,ExecutionContextCopyHost&);
std::shared_ptr<AsyncExecutionState> mutable_execution_context(ThreadExecutionState&,ExecutionContextCopyHost&);
class OwnedAsyncExecutionHost:public AsyncExecutionHost {
public:
    OwnedAsyncExecutionHost(ThreadExecutionState& thread,ExecutionContextCopyHost& copy):thread_(thread),copy_(copy){}
    std::shared_ptr<AsyncExecutionState> mutable_execution_context()override;
private:
    ThreadExecutionState& thread_;
    ExecutionContextCopyHost& copy_;
};
}
