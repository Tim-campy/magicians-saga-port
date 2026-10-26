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
// AppData.CheckTerm(int[]), 0x110b5f8. Fourteen recovered branches; others host-owned.
bool check_event_condition(const EventInstruction& instruction, EventConditionHost& host) {
    host.require_condition_context();
    const auto& row=required(instruction.get());
    switch (row.at(0)) {
    case 1: case 2: case 5: case 31: case 32: {
        const auto opcode=row[0];
        const auto kind=opcode==1 ? ConditionScalar::Money : opcode==2 ? ConditionScalar::Crystals :
            opcode==5 ? ConditionScalar::HeroLevel : opcode==31 ? ConditionScalar::Stamina : ConditionScalar::HighScore;
        const auto raw=host.scalar(kind); // Query precedes argument checks.
        const auto value=(opcode==5 || opcode==31) ? signed32(static_cast<std::uint32_t>(raw)) : raw;
        const auto threshold=row.at(1);
        const auto comparison=row.at(2);
        return compare_event_values(value,threshold,comparison);
    }
    case 15: {
        const auto* quests=host.quests(); // Getter precedes argument/bounds checks.
        const auto id=row.at(1);
        const auto& quest=at(quests,id);
        return (quest.flags & 2U)!=0 || quest.completed;
    }
    case 7: case 16: case 18: case 19: {
        // GetTotalRescueCount / SumAllQuests{Challenge,Clear,Lose}Count.
        // +0xa8 is a QuestRecord, not an array. Every arithmetic step wraps32.
        const auto opcode=row[0];
        const auto& quests=required(host.quests());
        std::uint32_t total=0;
        for (const auto& quest:quests) {
            if (!quest && opcode!=7) continue;
            const auto& record=required(quest.get()).record;
            if (!record) throw std::invalid_argument("null QuestRecord");
            if (opcode==7) total+=static_cast<std::uint32_t>(record->rescued);
            if (opcode==16 || opcode==19) total+=static_cast<std::uint32_t>(record->challenges);
            if (opcode==18) total+=static_cast<std::uint32_t>(record->clears);
            if (opcode==19) total-=static_cast<std::uint32_t>(record->clears);
        }
        if (opcode==7) total+=static_cast<std::uint32_t>(host.rescue_adjustment());
        const auto threshold=row.at(1);
        const auto comparison=row.at(2);
        return compare_event_values(signed32(total),threshold,comparison);
    }
    case 21: {
        // Count town records with BaseData.Check(1), then capacity minus count.
        std::uint32_t occupied=0;
        for (const auto& flags:required(host.town_flags()))
            if ((required(flags.get()) & 1U)!=0) ++occupied;
        const auto available=static_cast<std::uint32_t>(host.town_capacity())-occupied;
        const auto threshold=row.at(1);
        const auto comparison=row.at(2);
        return compare_event_values(signed32(available),threshold,comparison);
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
// AppData.CheckTerm context guard, 0x110b760..0x110b7b4.
void DecodedEventConditionHost::require_condition_context() {
    if (!state_.context_present || !state_.context_character_count)
        throw std::invalid_argument("missing AppData condition context");
    if (*state_.context_character_count==0)
        throw std::out_of_range("empty AppData characters storage");
}
const ConditionCatalog<QuestConditionState>* DecodedEventConditionHost::quests() { return state_.quests.get(); }
const ConditionCatalog<std::int32_t>* DecodedEventConditionHost::area_states() { return state_.area_states.get(); }
const ConditionCatalog<CharacterConditionState>* DecodedEventConditionHost::characters() { return state_.characters.get(); }
const ConditionCatalog<std::uint32_t>* DecodedEventConditionHost::town_flags() { return state_.town_flags.get(); }
EventScript DecodedEventConditionHost::buildings() { return state_.buildings; }
std::int32_t DecodedEventConditionHost::town_capacity() { return state_.town_capacity.value(); }
std::int64_t DecodedEventConditionHost::scalar(ConditionScalar kind) {
    switch (kind) {
    case ConditionScalar::Money: return state_.money.value();
    case ConditionScalar::Crystals: return state_.crystals.value();
    case ConditionScalar::HeroLevel: return state_.hero_level.value();
    case ConditionScalar::Stamina: return state_.stamina.value();
    case ConditionScalar::HighScore: return state_.high_score.value();
    }
    throw std::invalid_argument("unknown condition scalar");
}
// GetTotalRescueCount, 0x1109fe8: Config.DEBUG gates AppData.debugRescueNum.
std::int32_t DecodedEventConditionHost::rescue_adjustment() {
    return state_.debug_enabled.value() ? state_.debug_rescue_count.value() : 0;
}
bool DecodedEventConditionHost::evaluate_other_condition(const EventInstruction&) {
    throw std::logic_error("condition opcode not recovered");
}

}
