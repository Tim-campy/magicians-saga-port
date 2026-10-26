#pragma once
#include "event_flow.hpp"
#include <optional>

namespace saga::core {
struct QuestConditionRecord {
    std::int32_t challenges; // QuestRecord +0x18
    std::int32_t clears;     // +0x1c
    std::int32_t rescued;    // +0x24
};
struct QuestConditionState {
    std::uint8_t flags; // QuestData +0x1c, completion bit 1
    bool completed;    // +0xa4
    std::optional<QuestConditionRecord> record; // nullable QuestRecord +0xa8
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
enum class ConditionScalar { Money, Crystals, HeroField40, Stamina, HighScore };
class EventConditionHost {
public:
    virtual ~EventConditionHost() = default;
    // Catalog sizes must be in 0..INT32_MAX; QuestRecord counters are signed.
    // Original unit evaluator requires AppData singleton and nonempty
    // characters_ storage before reading the opcode. Actual bootstrap remains host-owned.
    virtual void require_condition_context() = 0;
    virtual const ConditionCatalog<QuestConditionState>* quests() = 0;
    // Decoded SecureLong values, or hero CharaData int32 at +0x40.
    virtual std::int64_t scalar(ConditionScalar) = 0;
    // GetTotalRescueCount conditional static bonus; queried after enumeration.
    // Host must preserve the original enable flag and optional bonus read.
    virtual std::int32_t rescue_adjustment() = 0;
    virtual const ConditionCatalog<std::uint32_t>* town_flags() = 0;
    virtual std::int32_t town_capacity() = 0; // sysSave SecureInt slot 19
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
