#include "text_reveal.hpp"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t s32(std::uint32_t value) {
    std::int32_t result;
    std::memcpy(&result, &value, sizeof result);
    return result;
}
std::int64_t s64(std::uint64_t value) {
    std::int64_t result;
    std::memcpy(&result, &value, sizeof result);
    return result;
}
std::int32_t limit(const TextRevealState& s) {
    return s32(static_cast<std::uint32_t>(s.length) * 1000U);
}
std::int64_t arm_divide(std::int64_t n, std::int64_t d) {
    // SDIV in Flush has no managed divide-by-zero guard in this artifact.
    if (d == 0) return 0;
    if (n == std::numeric_limits<std::int64_t>::min() && d == -1) return n;
    return n / d;
}
}
// SetText19198: 0x1bbd420..0x1bbd48c; StringEx.Length13492: 0x1be83d4.
std::int32_t reveal_length(const std::vector<const RevealBlock*>& blocks) {
    std::uint32_t total = 0;
    for (const auto* block : blocks) {
        if (!block) throw std::invalid_argument("null text block");
        if (block->is_pictograph || block->is_button) ++total;
        else {
            if (!block->text) throw std::invalid_argument("null ordinary block text");
            if (block->text->size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
                throw std::length_error("text exceeds managed string length");
            total += static_cast<std::uint32_t>(block->text->size());
        }
    }
    return s32(total);
}
// SetSpeed19210: 0x1bc3568; input is Language.IsShortWords's result.
std::int32_t configured_reveal_speed(std::int32_t requested, bool short_words) {
    return short_words ? requested : s32(static_cast<std::uint32_t>(requested) * 3U) / 2;
}
// Reset19209: 0x1bc3554.
void reset_text_reveal(TextRevealState& s) {
    restart_text_reveal_for_new_text(s);
    s.last_frame = 0;
}
void restart_text_reveal_for_new_text(TextRevealState& s) {
    s.position = 0;
    s.begin_time = 0;
    s.time_offset = 0;
}
// Timing prefix of Frame19208: 0x1bc33d8..0x1bc34ac.
void advance_text_reveal_timing(TextRevealState& s, TextRevealClock& clock) {
    if (s.begin_time == 0)
        s.begin_time = s64(static_cast<std::uint64_t>(clock.current_time_millis()) - 50U);
    const auto previous_frame = s.last_frame;
    const auto current_frame = clock.total_frames();
    if (previous_frame == current_frame)
        s.time_offset = s32(static_cast<std::uint32_t>(s.time_offset) + 50U);
    s.last_frame = clock.total_frames();
    const auto now = clock.current_time_millis();
    const auto elapsed = static_cast<std::uint64_t>(now) +
        static_cast<std::uint64_t>(static_cast<std::int64_t>(s.time_offset)) -
        static_cast<std::uint64_t>(s.begin_time);
    const auto product = s64(elapsed * static_cast<std::uint64_t>(static_cast<std::int64_t>(s.speed)));
    auto position = product > -50 ? product / 50 : 0;
    const auto maximum = limit(s);
    if (position > maximum) position = maximum;
    s.position = s32(static_cast<std::uint32_t>(position));
}
// Flush19219: 0x1bc36b0. Keep both multiplications and divisions in order.
void flush_text_reveal(TextRevealState& s, TextRevealClock& clock) {
    s.position = limit(s);
    const auto now = clock.current_time_millis();
    const auto numerator = static_cast<std::int64_t>(limit(s)) * 1000;
    const auto scaled = arm_divide(numerator, s.speed);
    s.begin_time = s64(static_cast<std::uint64_t>(now) + static_cast<std::uint64_t>(scaled / -20));
}
// IsEnd19220: 0x1bc370c.
bool text_reveal_ended(const TextRevealState& s) {
    return s.position == -1 || s.position >= limit(s);
}
}
