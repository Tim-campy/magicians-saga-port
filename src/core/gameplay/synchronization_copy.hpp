#pragma once
#include "async_local_values.hpp"
namespace saga::core {
// Copy-only projection. Dispatch, thread affinity and queue execution remain host responsibilities.
class SynchronizationContext {
public:
    virtual ~SynchronizationContext() = default;
    std::uint32_t properties=0;
    virtual std::shared_ptr<SynchronizationContext> create_copy() const;
};
class OsSynchronizationContext : public SynchronizationContext {
public:
    std::uint64_t native_handle=0;
    std::shared_ptr<SynchronizationContext> create_copy() const override;
};
struct SynchronizationWorkList {
    // Opaque work-request storage; not a claim about its payload format or execution.
    std::vector<AsyncValue> entries;
    std::uint32_t version=0;
    std::int32_t initial_capacity=20;
};
class QueuedSynchronizationContext : public SynchronizationContext {
public:
    QueuedSynchronizationContext(std::shared_ptr<SynchronizationWorkList> queue,std::int32_t thread_id);
    std::shared_ptr<SynchronizationWorkList> async_queue,current_frame;
    std::int32_t main_thread_id=0,tracked_count=0;
    std::shared_ptr<SynchronizationContext> create_copy() const override;
};
// Erased values must have been stored through a SynchronizationContext base pointer.
AsyncValue copy_synchronization_context(AsyncValue);
}
