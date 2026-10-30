#include "core/gameplay/event_progression.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace saga::core;
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
template<class E, class F> void expect(F action, const char* message) {
    try { action(); } catch (const E&) { return; }
    check(false, message);
}
// Test adapter: no UI/runtime emulation. Unsupported effects must fail loudly.
struct Host : EventFlowHost {
    EventProgression state{10, 50000, 10000, {}, {}};
    EventDelayQueue queue;
    EventScript script = std::make_shared<std::vector<EventInstruction>>();
    EventObject dispatch(const EventInstruction& row, const EventContext& context) override {
        if (!apply_event_progression(resolve_event_arguments(*row, context.argi), state))
            throw std::logic_error("effect not implemented by test host");
        return {};
    }
    std::int32_t identify_script(const EventScript& value) override { return value == script ? 0 : -1; }
    EventScript resolve_script(std::int32_t id) override {
        if (id != 0) throw std::out_of_range("unknown test event");
        return script;
    }
    std::int32_t read_talk_character() override { return 0; }
    void restore_talk_character(std::size_t slot, std::int32_t value) override {
        check(slot == 7 && value == 0, "continuation context");
    }
};
int main() {
    check(check_decoded_crystals(-1,0)&&check_decoded_crystals(-1,-1),"nonpositive crystal request succeeds");
    check(check_decoded_crystals(100,100)&&!check_decoded_crystals(99,100),"signed64 crystal threshold");
    check(check_decoded_crystals(std::numeric_limits<std::int64_t>::max(),std::numeric_limits<std::int64_t>::max()),"full signed64 request retained");
    check(!check_decoded_crystals(-1,std::numeric_limits<std::int64_t>::max()),"negative balance cannot satisfy positive request");
    check(subtract_decoded_crystals(143,100,10000)==43&&subtract_decoded_crystals(143,100,9999)==0,"check and scaled debit are separate contracts");
    check(add_decoded_currency(std::numeric_limits<std::int64_t>::max(),1)==std::numeric_limits<std::int64_t>::min(),"decoded money wrap64");
    check(add_decoded_currency(0,-1)==-1,"decoded add has no lower clamp");
    check(add_decoded_currency(0,std::numeric_limits<std::int32_t>::min())==-2147483648LL,"decoded delta sign extension");
    constexpr auto imin = std::numeric_limits<std::int32_t>::min();
    auto args = std::make_shared<EventIntegers>(100, 3);
    args->at(0) = imin; args->at(99) = 8;
    EventIntegers raw{imin, imin, imin + 99, imin + 100, -5};
    const auto resolved = resolve_event_arguments(raw, args);
    check(resolved == EventIntegers({imin, imin, 8, imin + 100, -5}),
          "only argument sentinels0..99 substituted once");
    check(raw[2] == imin + 99, "original row unchanged");
    expect<std::invalid_argument>([&] { resolve_event_arguments({4, imin}, {}); }, "null sentinel args fail");
    expect<std::out_of_range>([&] {
        resolve_event_arguments({4, imin + 99}, std::make_shared<EventIntegers>(1));
    }, "sentinel index checked");

    Host h;
    h.state.areas.push_back(std::make_shared<DiscoveryState>(DiscoveryState{2, false}));
    h.state.quests.push_back(std::make_shared<DiscoveryState>(DiscoveryState{2, false}));
    apply_event_progression({4, 0}, h.state); apply_event_progression({15, 0}, h.state);
    check(h.state.areas[0]->state == 1 && h.state.areas[0]->is_new &&
          h.state.quests[0]->state == 2 && !h.state.quests[0]->is_new,
          "area and quest overrides differ on nonzero undiscovered state");
    h.state.areas[0]->is_new = false;
    apply_event_progression({4, 0}, h.state);
    check(!h.state.areas[0]->is_new, "already found area keeps cleared new flag");
    h.state.quests[0]->state = 0;
    apply_event_progression({15, 0}, h.state);
    check(h.state.quests[0]->state == 1 && h.state.quests[0]->is_new, "quest zero becomes found/new");
    expect<std::out_of_range>([&] { apply_event_progression({4, -1}, h.state); }, "negative area id fails");
    h.state.quests.push_back(nullptr);
    expect<std::invalid_argument>([&] { apply_event_progression({15, 1}, h.state); }, "null record fails");
    check(!apply_event_progression({8, 0}, h.state), "unsupported UI opcode is explicitly unhandled");

    apply_event_progression({1, -20}, h.state);
    check(h.state.money == 0, "subtraction clamps negative result");
    apply_event_progression({1, imin}, h.state);
    check(h.state.money == 2147483648LL, "INT_MIN negation wraps before signed64 subtraction");
    h.state.money = std::numeric_limits<std::int64_t>::max();
    apply_event_progression({1, 1}, h.state);
    check(h.state.money == std::numeric_limits<std::int64_t>::min(), "addition wraps64 without clamp");
    apply_event_progression({1, -1}, h.state);
    check(h.state.money == std::numeric_limits<std::int64_t>::max(), "subtraction wraps before clamp");
    apply_event_progression({2, -2}, h.state);
    check(h.state.crystals == 49998, "mode10000 leaves subtraction amount unscaled");
    h.state.crystal_subtraction_mode = 0;
    apply_event_progression({2, -2}, h.state);
    check(h.state.crystals == 29998, "other mode multiplies subtraction by10000");
    h.state.crystals = 0;
    apply_event_progression({2, -214749}, h.state);
    check(h.state.crystals == 2147477296LL, "scaled32 overflow is sign extended before subtraction");
    apply_event_progression({2, 1}, h.state);
    check(h.state.crystals == 2147477297LL, "positive credit is never scaled");

    // Actual kernels integrated: effect, delay, captured argument, effect.
    h.state.money = 0; h.state.quests[0]->state = 0; h.state.quests[0]->is_new = false;
    for (const auto& row : {EventIntegers{1, 5}, {10, 1}, {15, imin}, {1, -2}})
        h.script->push_back(std::make_shared<EventIntegers>(row));
    auto ids = std::make_shared<EventIntegers>(EventIntegers{0});
    execute_event_script(h.script, 0, {nullptr, ids}, h.queue, h);
    ids->at(0) = 999;
    check(h.state.money == 5 && h.state.quests[0]->state == 0, "effects after delay have not run");
    process_counted_event_delays(h.queue, h);
    check(h.queue.empty() && h.state.money == 3 && h.state.quests[0]->state == 1,
          "resume applies captured id and ordered progression effects");
    std::cout << "event progression contracts passed\n";
}
