#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace saga::core {
struct RevealBlock {
    bool is_button;
    bool is_pictograph;
    const std::u16string* text;
};
// After parsing TextLayout blocks. This is not a markup parser.
std::int32_t reveal_length(const std::vector<const RevealBlock*>& blocks);
std::int32_t configured_reveal_speed(std::int32_t requested, bool short_words);
struct TextRevealState {
    std::int32_t length;       // TextLayout +0x34, from parsed blocks
    std::int32_t position;     // +0x40
    std::int32_t speed;        // +0x44, effective SetSpeed result
    std::int64_t begin_time;   // +0x48
    std::int32_t time_offset;  // +0x50
    std::int64_t last_frame;   // +0x58
};
class TextRevealClock {
public:
    virtual ~TextRevealClock() = default;
    virtual std::int64_t current_time_millis() = 0;
    virtual std::int32_t total_frames() = 0;
};
void reset_text_reveal(TextRevealState& state);
// SetText's reconstruction path resets fewer fields than Reset.
void restart_text_reveal_for_new_text(TextRevealState& state);
// Only the timing portion of Frame; block blink/drawing counters are separate.
void advance_text_reveal_timing(TextRevealState& state, TextRevealClock& clock);
void flush_text_reveal(TextRevealState& state, TextRevealClock& clock);
bool text_reveal_ended(const TextRevealState& state);
}
