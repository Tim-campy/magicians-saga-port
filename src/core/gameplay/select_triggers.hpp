#pragma once
#include "event_execution.hpp"
namespace saga::core {
class SelectTriggerHost {
public:
    virtual ~SelectTriggerHost() = default;
    virtual std::int64_t stamina() = 0;
    virtual std::uint32_t system_flags() = 0; // sysSave, slot selected by SYSSAVEI_FLAG.
    // Decoded save_.intArray (not sysSave_); slot 0 holds the normal-save flags.
    virtual std::shared_ptr<EventIntegers> save_integers() = 0;
};
bool event_already_executed(std::int32_t id,EventExecutionHost&);
// SelectForm.Update 0x11ec948..0x11eca24, after touch movement, before buttons.
void process_select_triggers(SelectTriggerHost&,EventDelayQueue&,EventExecutionHost&);
}
