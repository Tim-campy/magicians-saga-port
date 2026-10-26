#pragma once
#include "current_culture.hpp"
#include "thread_culture_set.hpp"
#include <vector>
namespace saga::core {
using AsyncValue=std::shared_ptr<void>;
class AsyncLocalKey {
public:
    virtual ~AsyncLocalKey() = default;
    virtual void changed(AsyncValue previous,AsyncValue current,bool context_changed) = 0;
};
struct AsyncLocalEntry {std::shared_ptr<AsyncLocalKey> key;AsyncValue value;};
using AsyncLocalValues=std::vector<AsyncLocalEntry>;
struct AsyncLocalNotifications {
    std::vector<std::shared_ptr<AsyncLocalKey>> keys;
    std::uint32_t version=0;
};
struct AsyncExecutionState {
    AsyncValue synchronization,synchronization_no_flow,logical_call,illogical_call;
    std::uint32_t flags=0;
    std::shared_ptr<AsyncLocalValues> values;
    std::shared_ptr<AsyncLocalNotifications> notifications;
};
class AsyncExecutionHost {
public:
    virtual ~AsyncExecutionHost() = default;
    // Thread.GetMutableExecutionContext: must honor ownership/copy flags before return.
    virtual std::shared_ptr<AsyncExecutionState> mutable_execution_context() = 0;
};
// Identity keys/values for the recovered AsyncLocal objects. No dictionary enumeration API.
void set_async_local_value(std::shared_ptr<AsyncLocalKey>,AsyncValue,bool notify,AsyncExecutionHost&);
// Hosts must outlive the slot. Values for this typed key must be CultureInstance pointers.
class ContextCultureSlot:public AsyncCultureSlot,public AsyncLocalKey,
    public std::enable_shared_from_this<ContextCultureSlot> {
public:
    ContextCultureSlot(AsyncExecutionHost& execution,ThreadCultureWriteHost& thread):execution_(execution),thread_(thread){}
    void set(std::shared_ptr<CultureInstance>)override;
    void changed(AsyncValue previous,AsyncValue current,bool context_changed)override;
private:
    AsyncExecutionHost& execution_;
    ThreadCultureWriteHost& thread_;
};
}
