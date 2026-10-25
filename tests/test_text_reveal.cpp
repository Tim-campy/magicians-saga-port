#include "core/gameplay/text_reveal.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace saga::core;
void check(bool value, const char* label) {
    if (!value) { std::cerr << label << '\n'; std::exit(1); }
}
struct Clock : TextRevealClock {
    std::int64_t now = 1000;
    std::int32_t frame = 1;
    std::string calls;
    std::int64_t current_time_millis() override { calls += 'T'; return now; }
    std::int32_t total_frames() override { calls += 'F'; return frame; }
};
int main() {
    const std::u16string pair = u"\U0001f600", empty, ordinary = u"abc";
    const RevealBlock a{false,false,&ordinary}, b{false,false,&pair}, c{true,false,nullptr},
        d{false,true,nullptr}, both{true,true,&ordinary}, blank{false,false,&empty}, bad{false,false,nullptr};
    check(reveal_length({&a,&b,&c,&d,&both,&blank}) == 8,
          "UTF-16 units, graphics count once, and empty text");
    check(reveal_length({}) == 0, "empty block collection");
    for (const auto* invalid : {static_cast<const RevealBlock*>(nullptr), &bad}) {
        bool failed = false;
        try { reveal_length({invalid}); } catch (const std::invalid_argument&) { failed = true; }
        check(failed, "invalid ordinary blocks fail instead of becoming empty");
    }
    check(configured_reveal_speed(1500,true) == 1500 && configured_reveal_speed(1500,false) == 2250,
          "effective speed follows short-word flag");
    check(configured_reveal_speed(-3,false) == -4 &&
          configured_reveal_speed(std::numeric_limits<std::int32_t>::max(),false) == 1073741822,
          "speed truncates signed division after 32-bit overflow");
    TextRevealState s{10,0,1500,0,0,0};
    Clock clock;
    advance_text_reveal_timing(s,clock);
    check(clock.calls == "TFFT" && s.position == 1500 && s.begin_time == 950 && s.last_frame == 1,
          "initial clock read precedes two frame reads and final clock read");
    clock.calls.clear();
    advance_text_reveal_timing(s,clock);
    check(clock.calls == "FFT" && s.time_offset == 50 && s.position == 3000,
          "repeated application frame adds offset");
    clock.frame = 2; clock.now = 1050;
    advance_text_reveal_timing(s,clock);
    check(s.position == 4500 && s.time_offset == 50, "offset persists across distinct frames");
    clock.frame = 3; clock.now = 5000;
    advance_text_reveal_timing(s,clock);
    check(s.position == 10000 && text_reveal_ended(s), "upper limit uses length times1000");
    clock.frame = 4; clock.now = 800;
    advance_text_reveal_timing(s,clock);
    check(s.position == 0 && !text_reveal_ended(s), "backward clock is not made monotonic");
    restart_text_reveal_for_new_text(s);
    check(s.position == 0 && s.begin_time == 0 && s.time_offset == 0 && s.last_frame == 4,
          "new text reset retains last application frame");
    clock.now = 1000;
    advance_text_reveal_timing(s,clock);
    check(s.position == 3000, "retained frame can cause compensation on first new-text update");
    reset_text_reveal(s);
    check(s.last_frame == 0 && s.speed == 1500 && s.length == 10, "Reset clears timing only");

    s = {1,0,1500,0,0,1}; clock.now = 2000; clock.frame = 2; clock.calls.clear();
    flush_text_reveal(s,clock);
    check(clock.calls == "T" && s.position == 1000 && s.begin_time == 1967 && text_reveal_ended(s),
          "Flush derives beginTime using successive integer divisions");
    advance_text_reveal_timing(s,clock);
    check(s.position == 990 && !text_reveal_ended(s),
          "truncation can put subsequent Frame below end; do not force a sticky completion flag");
    s.speed = 0;
    flush_text_reveal(s,clock);
    check(s.begin_time == 2000 && s.position == 1000, "artifact SDIV zero produces zero quotient");
    s.position = -1; s.length = 123;
    check(text_reveal_ended(s), "minus-one position sentinel");
    s = {2147484,0,1,1000,0,0}; clock.now = 1000; clock.frame = 1;
    advance_text_reveal_timing(s,clock);
    check(s.position == -2147483296 && text_reveal_ended(s), "limit multiplies at width32 before signed clamp");
    s = {10,0,2,std::numeric_limits<std::int64_t>::min(),0,0}; clock.now = 0;
    advance_text_reveal_timing(s,clock);
    check(s.position == 0, "elapsed-times-speed wraps64 without C++ signed overflow");
    std::cout << "text reveal timing contracts passed\n";
}
