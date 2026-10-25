#include "core/gameplay/event_flow.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace saga::core;
void check(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
template<class Exception, class F> void expect(F call, const char* message) {
    try { call(); } catch (const Exception&) { return; }
    check(false, message);
}
EventScript script(std::initializer_list<EventIntegers> rows) {
    auto result = std::make_shared<std::vector<EventInstruction>>();
    for (const auto& row : rows) result->push_back(std::make_shared<EventIntegers>(row));
    return result;
}
struct Host : EventFlowHost {
    EventDelayQueue queue;
    std::vector<EventScript> scripts;
    std::vector<std::string> trace;
    std::vector<EventContext> received;
    EventObject result = std::make_shared<int>(17);
    std::int32_t talk = 42;
    std::function<void()> on_dispatch;
    std::function<void()> on_restore;
    EventObject dispatch(const EventInstruction& row, const EventContext& context) override {
        trace.push_back("dispatch:" + std::to_string(row->at(0)));
        received.push_back(context);
        if (on_dispatch) on_dispatch();
        return result;
    }
    std::int32_t identify_script(const EventScript& value) override {
        trace.push_back("identify");
        for (std::size_t i = 0; i < scripts.size(); ++i)
            if (scripts[i] == value) return static_cast<std::int32_t>(i);
        return -1;
    }
    EventScript resolve_script(std::int32_t id) override {
        trace.push_back("resolve:" + std::to_string(id));
        if (id > 999) return {};
        return scripts.at(static_cast<std::size_t>(id % 1000));
    }
    std::int32_t read_talk_character() override { trace.push_back("read-talk"); return talk; }
    void restore_talk_character(std::size_t slot, std::int32_t value) override {
        check(slot == 7, "queue restores slot7, not SubForm slot5");
        talk = value;
        trace.push_back("restore");
        if (on_restore) on_restore();
    }
};
int main() {
    Host h;
    h.scripts = {script({{1, 10}, {10, 2}, {2, 20}, {10, 1}, {15, 4}})};
    EventContext context{std::make_shared<int>(9), std::make_shared<EventIntegers>(EventIntegers{7})};
    const auto first = execute_event_script(h.scripts[0], 0, context, h.queue, h);
    check(first == h.result && h.queue.size() == 1, "delay returns last dispatch result");
    check(h.trace == std::vector<std::string>({"dispatch:1", "identify", "read-talk"}),
          "matrix stops at delay before later effects");
    const auto initial = h.queue.at(0);
    check(initial->line == 2 && initial->hour == -1 && initial->script_id == 0,
          "continuation starts after delay and uses identity id");
    check(initial->context.args == context.args && initial->context.argi != context.argi,
          "object context shared, integer args copied");
    context.argi->at(0) = 99;
    h.trace.clear();
    process_counted_event_delays(h.queue, h);
    check(initial->wait == 1 && h.trace.empty(), "non-due event only decrements");
    h.on_restore = [&] { check(h.queue.empty(), "due item removed before restore"); };
    process_counted_event_delays(h.queue, h);
    check(h.trace == std::vector<std::string>({"restore", "resolve:0", "dispatch:2", "identify", "read-talk"}),
          "restore precedes resolution and continuation effects");
    check(h.received.back().argi->at(0) == 7 && h.received.back().args == context.args,
          "resume receives captured integer arguments and shared object");
    check(h.queue.size() == 1 && h.queue[0]->wait == 1 && h.queue[0]->line == 4,
          "new delay appended during pass is not consumed in same pass");
    h.trace.clear();
    process_counted_event_delays(h.queue, h);
    check(h.queue.empty() && h.trace.back() == "dispatch:15", "next pass completes continuation");

    // Mixed queue: timed entries untouched, due entries adjacent, signed wrap.
    Host mixed;
    mixed.scripts = {script({{4, 1}})};
    auto add = [&](std::int32_t wait, std::int32_t hour) {
        auto value = std::make_shared<DelayedEvent>(DelayedEvent{0, 0, wait, hour, 3, {}});
        mixed.queue.push_back(value); return value;
    };
    const auto hourly = add(0, 5);
    const auto wrap = add(std::numeric_limits<std::int32_t>::min(), -1);
    add(0, -1); add(-2, -1);
    process_counted_event_delays(mixed.queue, mixed);
    check(mixed.queue.size() == 2 && mixed.received.size() == 2,
          "adjacent removals do not skip original entries");
    check(hourly->wait == 0 && wrap->wait == std::numeric_limits<std::int32_t>::max(),
          "hour excludes countdown; INT_MIN wraps to INT_MAX");

    Host bad;
    auto malformed = script({{10}});
    expect<std::out_of_range>([&] { execute_event_script(malformed, 0, {}, bad.queue, bad); },
                              "delay requires wait argument");
    check(bad.trace == std::vector<std::string>({"identify"}) && bad.queue.empty(),
          "identify precedes malformed wait access; no queued effect");
    check(!execute_event_script({}, -1, {}, bad.queue, bad), "null script exits even for negative start");
    expect<std::out_of_range>([&] { execute_event_script(script({}), -1, {}, bad.queue, bad); },
                              "non-null script checks negative index");
    check(!execute_event_script(malformed, 1, {}, bad.queue, bad), "start at end returns null");
    auto null_row = std::make_shared<std::vector<EventInstruction>>(1);
    expect<std::invalid_argument>([&] { execute_event_script(null_row, 0, {}, bad.queue, bad); },
                                  "null row rejected");
    expect<std::out_of_range>([&] { execute_event_script(script({{}}), 0, {}, bad.queue, bad); },
                              "empty row rejected");
    // Equal contents do not provide the registered script identity.
    bad.scripts = {script({{10, 0}})};
    execute_event_script(script({{10, 0}}), 0, {}, bad.queue, bad);
    check(bad.queue[0]->script_id == -1 && !bad.queue[0]->context.argi,
          "unknown identity still schedules with -1 and null args remain null");
    expect<std::out_of_range>([&] { process_counted_event_delays(bad.queue, bad); },
                              "resolver failure propagates");
    check(bad.queue.empty() && bad.trace.back() == "resolve:-1",
          "resolution failure does not restore removed entry");

    Host hours;
    auto explicit_hour = script({{10, 0, 9}});
    execute_event_script(explicit_hour, 0, {nullptr, std::make_shared<EventIntegers>()}, hours.queue, hours);
    check(hours.queue[0]->hour == 9 && hours.queue[0]->context.argi && hours.queue[0]->context.argi->empty(),
          "explicit hour stored; empty args distinct from null");
    process_counted_event_delays(hours.queue, hours);
    check(hours.queue.size() == 1 && hours.queue[0]->wait == 0, "counted pass leaves explicit hour alone");

    // Arbitrary callback mutation is observable: do not silently snapshot queue.
    Host replacement;
    auto active = script({{1}, {2}});
    EventContext active_context{std::make_shared<int>(1), {}};
    const auto original_args = active_context.args;
    replacement.on_dispatch = [&] {
        active = script({{15}});
        active_context.args = std::make_shared<int>(2);
    };
    execute_event_script(active, 0, active_context, replacement.queue, replacement);
    check(replacement.trace == std::vector<std::string>({"dispatch:1", "dispatch:2"}) &&
          replacement.received.back().args == original_args,
          "caller replacing references does not replace captured arguments");

    Host mutation;
    mutation.scripts = {script({{1}})};
    for (int i = 0; i != 2; ++i)
        mutation.queue.push_back(std::make_shared<DelayedEvent>(DelayedEvent{0, 0, 1, -1, 0, {}}));
    mutation.on_dispatch = [&] { mutation.queue.clear(); };
    expect<std::out_of_range>([&] { process_counted_event_delays(mutation.queue, mutation); },
                              "callback invalidating remaining index is not hidden");
    std::cout << "event flow contracts passed\n";
}
