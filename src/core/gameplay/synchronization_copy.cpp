#include "synchronization_copy.hpp"
namespace saga::core {
// SynchronizationContext.CreateCopy 0x1ecbe38: fresh base object, not MemberwiseClone.
std::shared_ptr<SynchronizationContext> SynchronizationContext::create_copy() const {
    return std::make_shared<SynchronizationContext>();
}
// OSSpecificSynchronizationContext.CreateCopy 0x1ecc024.
std::shared_ptr<SynchronizationContext> OsSynchronizationContext::create_copy() const {
    auto result=std::make_shared<OsSynchronizationContext>();result->native_handle=native_handle;return result;
}
// UnitySynchronizationContext..ctor(List<WorkRequest>,int32) 0x23c092c.
QueuedSynchronizationContext::QueuedSynchronizationContext(std::shared_ptr<SynchronizationWorkList> queue,std::int32_t thread_id)
    :async_queue(std::move(queue)),current_frame(std::make_shared<SynchronizationWorkList>()),main_thread_id(thread_id){
    current_frame->entries.reserve(20);
}
// UnitySynchronizationContext.CreateCopy 0x23c0e70 shares async queue and main thread id only.
std::shared_ptr<SynchronizationContext> QueuedSynchronizationContext::create_copy() const {
    return std::make_shared<QueuedSynchronizationContext>(async_queue,main_thread_id);
}
AsyncValue copy_synchronization_context(AsyncValue value){
    if(!value)throw CultureManagedException("null synchronization context");
    return std::static_pointer_cast<SynchronizationContext>(value)->create_copy();
}
// GetThreadLocalContext -> 0x1ecbce0 -> native 0xfcf1ac returns null in this APK.
AsyncValue native_synchronization_fallback(){return nullptr;}
// SynchronizationContext.Current 0x1ec3658; CurrentExplicit 0x1ecbe8c aliases it.
AsyncValue current_synchronization_context(const ThreadExecutionState& thread,const SynchronizationFallback& fallback){
    if(thread.context&&thread.context->synchronization)return thread.context->synchronization;
    return fallback();
}
// CurrentNoFlow 0x1ecbcb0 reads its own slot, never the flowing slot.
AsyncValue current_no_flow_synchronization_context(const ThreadExecutionState& thread,const SynchronizationFallback& fallback){
    if(thread.context&&thread.context->synchronization_no_flow)return thread.context->synchronization_no_flow;
    return fallback();
}
// SetSynchronizationContext 0x1ecbc3c obtains a mutable context before both stores.
void set_synchronization_context(ThreadExecutionState& thread,AsyncValue value,ExecutionContextCopyHost& host){
    auto context=mutable_execution_context(thread,host);
    context->synchronization=value;
    context->synchronization_no_flow=std::move(value);
}
}
