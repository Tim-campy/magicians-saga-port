#include "event_conditions.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T> const T& required(const T* p) {
    if (!p) throw std::invalid_argument("null condition data");
    return *p;
}
template<class T> const T& at(const ConditionCatalog<T>* catalog, std::int32_t id) {
    return required(required(catalog).at(static_cast<std::size_t>(id)).get());
}
std::int32_t signed32(std::uint32_t value) {
    return value <= 0x7fffffffU ? static_cast<std::int32_t>(value)
        : -1 - static_cast<std::int32_t>(0xffffffffU-value);
}
}
// AppData.Compare, 0x110c1d4, game:0x060001c2.
bool compare_event_values(std::int64_t left, std::int64_t right, std::int32_t code) {
    switch (code) {
    case 0: return left>right;
    case 1: return left>=right;
    case 2: return left<=right;
    case 3: return left<right;
    case 4: return left==right;
    case 5: return left!=right;
    default: return false;
    }
}
// AppData.CheckTerm(int[]), 0x110b5f8. Five recovered branches; others host-owned.
bool check_event_condition(const EventInstruction& instruction, EventConditionHost& host) {
    host.require_condition_context();
    const auto& row=required(instruction.get());
    switch (row.at(0)) {
    case 15: {
        const auto* quests=host.quests(); // Getter precedes argument/bounds checks.
        const auto id=row.at(1);
        const auto& quest=at(quests,id);
        return (quest.flags & 2U)!=0 || quest.completed;
    }
    case 16: {
        // SumAllQuestsChallengeCount, 0x110c284. Null entries skipped; null
        // challenge arrays fail; ADD W wraps before signed comparison.
        const auto& quests=required(host.quests());
        std::uint32_t total=0;
        for (const auto& quest:quests) {
            if (!quest) continue;
            if (!quest->challenge_count) throw std::invalid_argument("null challenge array");
            total+=static_cast<std::uint32_t>(*quest->challenge_count);
        }
        const auto threshold=row.at(1);
        const auto comparison=row.at(2);
        return compare_event_values(signed32(total),threshold,comparison);
    }
    case 4: {
        const auto* areas=host.area_states();
        const auto id=row.at(1);
        return at(areas,id)==1;
    }
    case 22: {
        std::int32_t total=0;
        for (const auto& character:required(host.characters()))
            if (character && character->sub!=0 && (character->flags & 4U)!=0) ++total;
        const auto threshold=row.at(1);
        const auto comparison=row.at(2);
        return compare_event_values(total,threshold,comparison);
    }
    case 20: {
        // Count(Where(...)) enumerates ALL rows, rather than short-circuit Any.
        // Predicate reads building[1] BEFORE instruction[1].
        std::int32_t total=0;
        const auto buildings=host.buildings();
        for (const auto& building:required(buildings.get())) {
            const auto type=required(building.get()).at(1);
            if (type==row.at(1)) ++total;
        }
        return total>0;
    }
    default: return host.evaluate_other_condition(instruction);
    }
}
// AppData.CheckTerm(int[][]), 0x110b534, game:0x060001c1.
bool check_event_conditions(const EventScript& input, EventConditionHost& host) {
    const auto terms=input; // Preserve managed reference across callbacks.
    if (!terms || terms->empty()) return false;
    for (std::size_t i=0;i<terms->size();++i)
        if (!check_event_condition(terms->at(i),host)) return false;
    return true;
}
// AppData.CheckTerm(int), 0x110b3e0, game:0x060001c0.
bool check_event_eligibility(const ConditionCatalog<EventConditionRecord>* records,
                             std::int32_t index, EventConditionHost& host) {
    if (!records || index<0 || static_cast<std::size_t>(index)>=records->size()) return false;
    const auto record=records->at(static_cast<std::size_t>(index));
    if (!record || record->execution_count>0) return false;
    return check_event_conditions(record->terms,host);
}
}
