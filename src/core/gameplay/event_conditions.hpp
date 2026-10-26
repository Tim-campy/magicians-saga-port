#pragma once
#include "event_flow.hpp"
#include <optional>

namespace saga::core {
struct QuestConditionState {
    std::uint8_t flags; // QuestData +0x1c, completion bit 1
    bool completed;    // +0xa4
    std::optional<std::int32_t> challenge_count; // nullable array +0xa8: Length
};
struct CharacterConditionState {
    std::int32_t sub;  // CharaData +0x50
    std::uint8_t flags; // +0x1c, recruitment bit 2
};
template<class T> using ConditionCatalog = std::vector<std::shared_ptr<T>>;
struct EventConditionRecord {
    std::int32_t execution_count;
    EventScript terms;
};
class EventConditionHost {
public:
    virtual ~EventConditionHost() = default;
    // Catalog sizes and challenge lengths must be in 0..INT32_MAX.
    // Original unit evaluator requires AppData singleton and nonempty
    // characters_ storage before reading the opcode. Actual bootstrap remains host-owned.
    virtual void require_condition_context() = 0;
    virtual const ConditionCatalog<QuestConditionState>* quests() = 0;
    virtual const ConditionCatalog<std::int32_t>* area_states() = 0;
    virtual const ConditionCatalog<CharacterConditionState>* characters() = 0;
    // TownSystem.GetBuildingList output, including null gaps and row identity.
    virtual EventScript buildings() = 0;
    // Remaining original opcodes are not yet recovered; never default to true.
    virtual bool evaluate_other_condition(const EventInstruction&) = 0;
};
bool compare_event_values(std::int64_t left, std::int64_t right, std::int32_t code);
bool check_event_condition(const EventInstruction&, EventConditionHost&);
bool check_event_conditions(const EventScript&, EventConditionHost&);
bool check_event_eligibility(const ConditionCatalog<EventConditionRecord>*,
                             std::int32_t index, EventConditionHost&);
}
