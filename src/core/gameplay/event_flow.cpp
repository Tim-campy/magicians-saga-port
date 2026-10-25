#include "event_flow.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t decrement32(std::int32_t value) {
    return value == std::numeric_limits<std::int32_t>::min()
        ? std::numeric_limits<std::int32_t>::max() : value - 1;
}
}

EventObject execute_event_script(EventScript script, std::int32_t start,
                                EventContext context, EventDelayQueue& queue,
                                EventFlowHost& host) {
    EventObject result;
    if (!script) return result;
    // The signed comparison precedes the managed bounds check in the original.
    if (start >= 0 && static_cast<std::size_t>(start) >= script->size()) return result;
    if (start < 0) throw std::out_of_range("negative event script line");
    for (auto line = static_cast<std::size_t>(start); line < script->size(); ++line) {
        const auto row = script->at(line);
        if (!row) throw std::invalid_argument("null event instruction");
        if (row->at(0) != 10) {
            result = host.dispatch(row, context);
            continue;
        }
        // Ordering matters on malformed input and for host side effects.
        const auto id = host.identify_script(script);
        const auto wait = row->at(1);
        const auto talk = host.read_talk_character();
        auto delayed = std::make_shared<DelayedEvent>(DelayedEvent{
            id, static_cast<std::int32_t>(line + 1), wait, -1, talk, context});
        // DelayEvent35499 (0x11c9508) retains args, but clones non-null argi.
        if (context.argi)
            delayed->context.argi = std::make_shared<EventIntegers>(*context.argi);
        if (row->size() >= 3) delayed->hour = row->at(2);
        queue.push_back(delayed);
        return result;
    }
    return result;
}

void process_counted_event_delays(EventDelayQueue& queue, EventFlowHost& host) {
    auto remaining = queue.size();
    std::size_t index = 0;
    while (index < remaining) {
        const auto delayed = queue.at(index);
        if (!delayed) throw std::invalid_argument("null delayed event");
        if (delayed->hour != -1) { ++index; continue; }
        delayed->wait = decrement32(delayed->wait);
        if (delayed->wait > 0) { ++index; continue; }
        queue.erase(queue.begin() + static_cast<EventDelayQueue::difference_type>(index));
        host.restore_talk_character(7, delayed->talk_character);
        --remaining;
        const auto script = host.resolve_script(delayed->script_id);
        execute_event_script(script, delayed->line, delayed->context, queue, host);
        // Original index-- followed by loop increment: inspect the shifted entry.
        // Appended events do not increase the initial traversal limit.
    }
}
}
