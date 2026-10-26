#pragma once
#include "event_catalog.hpp"
namespace saga::core {
class EventExecutionHost : public EventFlowHost {
public:
    // Re-read on every original Data.get_eventData access. Managed array sizes
    // are fixed and <= INT32_MAX; replacement is observable between accesses.
    virtual std::shared_ptr<EventCatalog> events() = 0;
    // Called only when args is null: absent normal save_ => null; otherwise decode
    // SecureString slot 0 of save_+0x30. Missing array/slot must fail.
    virtual EventObject default_event_args() = 0;
    // Implements the Config diagnostic gate for null/empty effects.
    virtual void empty_event_diagnostic(std::int32_t id) = 0;
};
EventObject execute_indexed_event(std::int32_t id, EventContext context,
                                 EventDelayQueue&, EventExecutionHost&);
EventObject select_and_execute_events(EventDelayQueue&, EventExecutionHost&,
                                      EventConditionHost&);
enum class EventDiagnostic { EmptyScript, UnknownScript };
struct DecodedEventExecutionState {
    std::shared_ptr<EventCatalog> catalog;
    std::optional<bool> normal_save_present;
    // Values after SecureString conversion; a null slot converts to null.
    std::shared_ptr<std::vector<EventObject>> normal_save_strings;
    std::optional<std::size_t> quest_data_count; // AppData.questData_ +0x1e8
    std::optional<bool> print_enabled; // Config.PRINT +0x17
    std::function<void(EventDiagnostic,std::int32_t)> diagnostic;
};
// Connects decoded storage to execution and catalog lookup; effect dispatch and
// talk-character state remain delegated to the existing flow host.
class DecodedEventExecutionHost final : public EventExecutionHost {
public:
    DecodedEventExecutionHost(DecodedEventExecutionState& state,EventFlowHost& flow)
        : state_(state),flow_(flow) {}
    std::shared_ptr<EventCatalog> events() override;
    EventObject default_event_args() override;
    void empty_event_diagnostic(std::int32_t id) override;
    EventObject dispatch(const EventInstruction&,const EventContext&) override;
    std::int32_t identify_script(const EventScript&) override;
    EventScript resolve_script(std::int32_t id) override;
    std::int32_t read_talk_character() override;
    void restore_talk_character(std::size_t slot,std::int32_t value) override;
private:
    void diagnose(EventDiagnostic,std::int32_t id);
    DecodedEventExecutionState& state_;
    EventFlowHost& flow_;
};

}
