#include "execution_context.hpp"
namespace saga::core {
// ExecutionContext.CreateMutableCopy 0x1ecb0a4.
std::shared_ptr<AsyncExecutionState> copy_mutable_execution_context(const AsyncExecutionState& source,ExecutionContextCopyHost& host){
    auto result=std::make_shared<AsyncExecutionState>();
    result->synchronization=source.synchronization;
    result->synchronization_no_flow=source.synchronization_no_flow;
    if(source.logical_call)result->logical_call=host.clone_logical_call(source.logical_call);
    if(source.illogical_call)result->illogical_call=host.clone_illogical_call(source.illogical_call);
    // Read these after both clones, preserving any provider-side changes to the source.
    result->values=source.values;
    result->notifications=source.notifications;
    result->flags=source.flags&2U;
    return result;
}
// Thread.GetMutableExecutionContext 0x1eca7d4.
std::shared_ptr<AsyncExecutionState> mutable_execution_context(ThreadExecutionState& thread,ExecutionContextCopyHost& host){
    auto current=thread.context;
    if(!current){current=std::make_shared<AsyncExecutionState>();thread.context=current;}
    else if(thread.belongs_to_outer_scope){
        current=copy_mutable_execution_context(*current,host);
        thread.context=current;
    }
    thread.belongs_to_outer_scope=false;
    return current;
}
std::shared_ptr<AsyncExecutionState> OwnedAsyncExecutionHost::mutable_execution_context(){
    return core::mutable_execution_context(thread_,copy_);
}
}
