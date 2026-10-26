#include "context_capture.hpp"
namespace saga::core {
// ExecutionContext..cctor 0x1ecb5cc: two distinct instances, flags 4 and 0.
ExecutionContextDomain::ExecutionContextDomain():preallocated_default(std::make_shared<AsyncExecutionState>()),default_context(std::make_shared<AsyncExecutionState>()){
    preallocated_default->flags=4;
}
// LogicalCallContext.HasInfo 0x1d56b88. Header arrays/correlation flag do not count.
bool logical_call_has_info(const std::shared_ptr<LogicalCallContext>& value){
    if(!value)return false;
    if(value->remoting&&value->remoting->logical_call_id)return true;
    if(value->security&&value->security->principal)return true;
    if(value->host_context)return true;
    auto data=value->data;return data&&data->count()>0;
}
// ExecutionContext.IsDefaultFTContext 0x1ecac60.
bool default_execution_context(const AsyncExecutionState& value,bool ignore_synchronization){
    if(!ignore_synchronization&&value.synchronization)return false;
    if(logical_call_has_info(std::static_pointer_cast<LogicalCallContext>(value.logical_call)))return false;
    auto illogical=std::static_pointer_cast<IllogicalCallContext>(value.illogical_call);
    auto data=illogical?illogical->data:nullptr;
    return !data||data->count()<=0; // Illogical host reference is not tested.
}
namespace {
std::shared_ptr<LogicalCallContext> logical(AsyncExecutionState& value){
    if(!value.logical_call)value.logical_call=std::make_shared<LogicalCallContext>();
    return std::static_pointer_cast<LogicalCallContext>(value.logical_call);
}
}
// ExecutionContext.Capture(ref StackCrawlMark, CaptureOptions) 0x1ecb1cc.
std::shared_ptr<AsyncExecutionState> capture_execution_context(ThreadExecutionState& thread,std::uint32_t options,
    ExecutionContextDomain& domain,CallContextHost& host,const std::function<AsyncValue(AsyncValue)>& clone_synchronization){
    auto source=thread.context;
    AsyncValue synchronization;
    std::shared_ptr<LogicalCallContext> call;
    std::shared_ptr<AsyncLocalValues> values;
    std::shared_ptr<AsyncLocalNotifications> notifications;
    if(source){
        if(source->flags&2U)return nullptr;
        if(!(options&1U)&&source->synchronization){
            if(!clone_synchronization)throw std::invalid_argument("SynchronizationContext clone provider required");
            synchronization=clone_synchronization(source->synchronization);
        }
        if(logical_call_has_info(logical(*source)))call=copy_logical_call_context(*logical(*source),host);
        values=source->values;notifications=source->notifications;
    }
    if((options&2U)&&!synchronization){
        const bool has_info=logical_call_has_info(call); // Before checking dictionary/list presence.
        if(!notifications&&!values&&!has_info)return domain.preallocated_default;
    }
    auto result=std::make_shared<AsyncExecutionState>();
    result->synchronization=synchronization;result->logical_call=call;
    result->values=values;result->notifications=notifications;result->flags=1;
    return result;
}
// ExecutionContext.Run 0x1ec6f6c.
void run_captured_execution_context(ThreadExecutionState& thread,std::shared_ptr<AsyncExecutionState> target,
    const std::function<void(AsyncValue)>& callback,AsyncValue state,ContextNotificationFailureHost& failure){
    if(!target)throw CultureManagedException("null execution context capture");
    if(!(target->flags&5U))throw CultureManagedException("execution context capture already consumed");
    run_execution_context(thread,target,callback,state,false,failure);
}
// ExecutionContext.RunInternal 0x1eca9d0 (four-argument Run 0x1eca954 delegates here).
void run_execution_context(ThreadExecutionState& thread,std::shared_ptr<AsyncExecutionState> target,
    const std::function<void(AsyncValue)>& callback,AsyncValue state,bool preserve_synchronization,
    ContextNotificationFailureHost& failure){
    if(!target)throw CultureManagedException("null execution context");
    if(!(target->flags&4U))target->flags&=~1U;
    ExecutionContextSwitch saved;
    try{
        auto current=thread.context;
        if((!current||default_execution_context(*current,preserve_synchronization))&&
            default_execution_context(*target,preserve_synchronization)&&
            (current?current->values:nullptr)==target->values){
            // EstablishCopyOnWriteScope 0x1ecacd8 re-reads the thread after predicates.
            saved={thread.context,thread.belongs_to_outer_scope,&thread};
            thread.belongs_to_outer_scope=true;
        }else{
            if(target->flags&4U)target=std::make_shared<AsyncExecutionState>();
            saved=set_execution_context(thread,target,preserve_synchronization,failure);
        }
        if(!callback)throw CultureManagedException("null context callback");
        callback(state);
    }catch(...){undo_execution_context(saved,failure);throw;}
    undo_execution_context(saved,failure);
}
}
