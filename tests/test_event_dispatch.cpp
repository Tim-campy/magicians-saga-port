#include "core/gameplay/event_dispatch.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

using namespace saga::core;
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
template<class E, class F> void expect(F action, const char* message) {
    try { action(); } catch (const E&) { return; }
    check(false, message);
}
EventInstruction row(EventIntegers values) { return std::make_shared<EventIntegers>(std::move(values)); }
struct Host : EventEffectsHost, EventFlowHost {
    EventProgression state{0, 0, 10000, {}, {}};
    EventDelayQueue queue;
    EventScript script = std::make_shared<std::vector<EventInstruction>>();
    std::vector<std::string> trace;
    std::vector<std::int32_t> limits{10, 20};
    std::size_t reads = 0;
    std::int64_t stamina = 7;
    EventObject token = std::make_shared<int>(1);
    EventObject expected_args;
    bool fail_push = false;
    EventProgression& progression() override { return state; }
    std::int32_t stamina_limit() override { trace.push_back("limit"); return limits.at(reads++); }
    std::int64_t read_stamina() override { trace.push_back("read"); return stamina; }
    void write_stamina(std::int64_t v) override { trace.push_back("write"); stamina = v; }
    EventObject make_talk_form(std::int32_t id, EventObject args) override {
        check(id == 4 && args == expected_args, "talk receives substituted id and shared args");
        trace.push_back("talk"); return token;
    }
    EventObject make_event_form(std::int32_t id, EventObject args) override {
        check(id == 9 && args == expected_args, "event receives id and args");
        trace.push_back("event"); return token;
    }
    void push_form(EventObject form) override {
        check(form == token, "created form passed unchanged to push");
        trace.push_back("push");
        if (fail_push) throw std::runtime_error("test push failure");
    }
    EventObject dispatch(const EventInstruction& instruction, const EventContext& context) override {
        auto result = dispatch_known_event_instruction(instruction, context, *this);
        if (!result.handled) throw std::logic_error("unsupported opcode in test host");
        return result.value;
    }
    std::int32_t identify_script(const EventScript& value) override { return value == script ? 0 : -1; }
    EventScript resolve_script(std::int32_t id) override {
        if (id != 0) throw std::out_of_range("unknown test script");
        return script;
    }
    std::int32_t read_talk_character() override { return 0; }
    void restore_talk_character(std::size_t, std::int32_t) override { trace.push_back("restore"); }
};
int main() {
    check(stamina_limit_from_item_state(0) == 10 && stamina_limit_from_item_state(1) == 20 &&
          stamina_limit_from_item_state(-1) == 20, "limit tests zero, not positive or purchased enum");
    check(event_stamina_balance(7, 15, 10, 20) == 8, "positive percentage truncates");
    check(event_stamina_balance(7, -15, 10, 20) == 6, "negative percentage truncates toward zero");
    check(event_stamina_balance(7, 50, 10, 20) == 12, "separate first and second limits");
    check(event_stamina_balance(7, 100, 20, 10) == 10, "upper clamp uses second limit");
    check(event_stamina_balance(7, -100, 10, 20) == 0, "lower clamp");
    check(event_stamina_balance(7, std::numeric_limits<std::int32_t>::max(), 10, 20) == 7,
          "product wraps32 to -10 before division, producing zero delta");
    check(event_stamina_balance(std::numeric_limits<std::int64_t>::max(), 10, 10, 20) == 0,
          "balance wraps64 before clamps");
    Host h;
    h.expected_args = std::make_shared<int>(3);
    EventContext context{h.expected_args, std::make_shared<EventIntegers>(EventIntegers{4})};
    auto form = dispatch_known_event_instruction(row({8, std::numeric_limits<std::int32_t>::min()}), context, h);
    check(form.handled && form.value == h.token && h.trace == std::vector<std::string>({"talk", "push"}),
          "form created, pushed and returned");
    h.trace.clear();
    dispatch_known_event_instruction(row({31, 50}), {}, h);
    check(h.stamina == 12 && h.trace == std::vector<std::string>({"limit", "read", "limit", "write"}),
          "stamina preserves callback order and two limit reads");
    h.reads = 0; h.trace.clear();
    expect<std::out_of_range>([&] { dispatch_known_event_instruction(row({31}), {}, h); },
                              "missing percent fails after first limit");
    check(h.trace == std::vector<std::string>({"limit"}), "malformed row effect ordering");
    h.trace.clear();
    check(dispatch_known_event_instruction(nullptr, {}, h).handled &&
          dispatch_known_event_instruction(row({}), {}, h).handled, "unit null and empty rows tolerated");
    check(dispatch_known_event_instruction(row({10, 1}), {}, h).handled && h.trace.empty(),
          "unit delay is no-op and does not enqueue");
    check(!dispatch_known_event_instruction(row({999}), {}, h).handled, "unknown opcode not silently handled");
    h.fail_push = true;
    expect<std::runtime_error>([&] { dispatch_known_event_instruction(row({12, 9}), context, h); },
                               "push failure propagates");
    check(h.trace == std::vector<std::string>({"event", "push"}), "creation precedes failed push");

    // Dispatcher plus real numeric effects and delay; UI callbacks only observe.
    h.fail_push = false; h.trace.clear(); h.reads = 0; h.stamina = 7;
    *h.script = {row({1, 5}), row({8, 4}), row({10, 1}), row({31, 50}), row({12, 9})};
    check(execute_event_script(h.script, 0, context, h.queue, h) == h.token,
          "matrix returns last form when interrupted by delay");
    check(h.state.money == 5 && h.stamina == 7 && h.queue.size() == 1, "later effects await continuation");
    process_counted_event_delays(h.queue, h);
    check(h.stamina == 12 && h.queue.empty() && h.trace == std::vector<std::string>(
          {"talk", "push", "restore", "limit", "read", "limit", "write", "event", "push"}),
          "integrated order across delay and presentation boundary");
    std::cout << "event dispatch contracts passed\n";
}
