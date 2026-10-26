#pragma once
#include "async_local_values.hpp"
#include <exception>
namespace saga::core {
class ContextNotificationFailureHost {
public:
    virtual ~ContextNotificationFailureHost() = default;
    // Original Environment.FailFast: production host must enforce its terminal policy.
    // Tests may throw a dedicated sentinel to observe this path without termination.
    virtual void fail_fast(std::exception_ptr) = 0;
};
void notify_async_context_changed(std::shared_ptr<AsyncExecutionState> previous,
                                  std::shared_ptr<AsyncExecutionState> current,
                                  ContextNotificationFailureHost&);
}
