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
}
