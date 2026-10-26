#pragma once
#include "event_catalog.hpp"
namespace saga::core {
class EventExecutionHost : public EventFlowHost {
public:
    // Re-read on every original Data.get_eventData access. Managed array sizes
    // are fixed and <= INT32_MAX; replacement is observable between accesses.
    virtual std::shared_ptr<EventCatalog> events() = 0;
    // Called only when args is null: absent sysSave => null; otherwise decode
    // SecureString slot 0 of sysSave+0x30. Missing array/slot must fail.
    virtual EventObject default_event_args() = 0;
    // Implements the Config diagnostic gate for null/empty effects.
    virtual void empty_event_diagnostic(std::int32_t id) = 0;
};
EventObject execute_indexed_event(std::int32_t id, EventContext context,
                                 EventDelayQueue&, EventExecutionHost&);
EventObject select_and_execute_events(EventDelayQueue&, EventExecutionHost&,
                                      EventConditionHost&);
}
