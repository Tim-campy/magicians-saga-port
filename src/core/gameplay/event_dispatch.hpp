#pragma once
#include "event_progression.hpp"

namespace saga::core {
// Decoded arithmetic only. Does not load, grant or change a billing item state.
std::int32_t stamina_limit_from_item_state(std::int32_t state);
std::int64_t event_stamina_balance(std::int64_t balance, std::int32_t percent,
                                  std::int32_t percent_limit, std::int32_t clamp_limit);

class EventEffectsHost {
public:
    virtual ~EventEffectsHost() = default;
    virtual EventProgression& progression() = 0;
    virtual std::int32_t stamina_limit() = 0;
    virtual std::int64_t read_stamina() = 0;
    virtual void write_stamina(std::int64_t value) = 0;
    virtual EventObject make_talk_form(std::int32_t id, EventObject args) = 0;
    virtual EventObject make_event_form(std::int32_t id, EventObject args) = 0;
    virtual void push_form(EventObject form) = 0;
};
struct EventDispatchResult {
    bool handled;
    EventObject value;
};

// AppData.ExecuteScript34379: opcodes found in extracted Event.txt (1,2,4,8,
// 10,12,15,31). Opcode10 is a no-op HERE; the matrix executor handles its delay.
// Other opcodes are explicitly unhandled, not silently substituted with no-ops.
EventDispatchResult dispatch_known_event_instruction(EventInstruction row,
                                                    EventContext context,
                                                    EventEffectsHost& host);
}
