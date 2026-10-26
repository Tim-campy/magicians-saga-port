#pragma once
#include "current_culture.hpp"
namespace saga::core {
class AsyncCultureSlot {
public:
    virtual ~AsyncCultureSlot() = default;
    // Original AsyncLocal.Value routes through ExecutionContext.SetLocalValue.
    virtual void set(std::shared_ptr<CultureInstance>) = 0;
};
class ThreadCultureWriteHost:public ThreadCultureHost {
public:
    virtual std::shared_ptr<CultureInstance> thread_ui_culture() = 0;
    virtual void write_thread_culture(std::shared_ptr<CultureInstance>) = 0;
    virtual void write_thread_ui_culture(std::shared_ptr<CultureInstance>) = 0;
    virtual std::shared_ptr<CultureInstance> construct_user_ui_culture() = 0;
    virtual bool disable_async_culture_flow() = 0;
    virtual std::shared_ptr<AsyncCultureSlot> async_culture_slot() = 0;
    // Construct with AsyncLocalSetCurrentCulture notification callback.
    virtual std::shared_ptr<AsyncCultureSlot> make_async_culture_slot() = 0;
    // Publish only if currently null, as Interlocked.CompareExchange(ref, candidate, null).
    virtual void publish_async_culture_slot(std::shared_ptr<AsyncCultureSlot>) = 0;
};
void initialize_thread_cultures(ThreadCultureWriteHost&,CurrentCultureState&,CurrentCultureHost&);
void set_thread_culture(std::shared_ptr<CultureInstance>,ThreadCultureWriteHost&,CurrentCultureState&,CurrentCultureHost&);
// Event's current value is captured before the TLS write; null is allowed here.
void async_local_set_current_culture(std::shared_ptr<CultureInstance>,ThreadCultureWriteHost&);
}
