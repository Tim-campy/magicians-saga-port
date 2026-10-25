#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace saga::core {
using EventObject = std::shared_ptr<void>;
using EventIntegers = std::vector<std::int32_t>;
using EventInstruction = std::shared_ptr<EventIntegers>;
// Shared identity and null rows are significant. These represent managed arrays:
// callers must keep their lengths fixed and within signed 32-bit bounds.
using EventScript = std::shared_ptr<std::vector<EventInstruction>>;

struct EventContext {
    EventObject args;
    std::shared_ptr<EventIntegers> argi;
};
struct DelayedEvent {
    std::int32_t script_id;
    std::int32_t line;
    std::int32_t wait;
    std::int32_t hour;
    std::int32_t talk_character;
    EventContext context;
};
using EventDelayQueue = std::vector<std::shared_ptr<DelayedEvent>>;

// AppData/form integration boundary, not default or approximated opcode effects.
// identify_script must preserve script identity (see report81); dispatch must
// implement the unit dispatcher, including its argument substitution contract.
class EventFlowHost {
public:
    virtual ~EventFlowHost() = default;
    virtual EventObject dispatch(const EventInstruction&, const EventContext&) = 0;
    virtual std::int32_t identify_script(const EventScript&) = 0;
    virtual EventScript resolve_script(std::int32_t id) = 0;
    virtual std::int32_t read_talk_character() = 0;
    virtual void restore_talk_character(std::size_t slot, std::int32_t value) = 0;
};

// AppData.ExecuteScript34377, ELF VA0x110c8ec. Opcode10 schedules and stops;
// all other rows are delegated. Returns the last unit-dispatch result.
EventObject execute_event_script(EventScript script, std::int32_t start,
                                EventContext context, EventDelayQueue& queue,
                                EventFlowHost& host);

// SelectForm.Update35965, ELF VA0x11ec578..0x11ec6d8. One reached queue pass,
// NOT a frame/second/tick policy. Caller must apply the original form guards.
// Callback mutations are observed through indexed access, not saved iterators.
void process_counted_event_delays(EventDelayQueue& queue, EventFlowHost& host);
}
