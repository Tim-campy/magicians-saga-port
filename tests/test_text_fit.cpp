#include "core/gameplay/text_fit.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace saga::core;
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
struct Host : TextFitHost {
    std::int32_t size = 5;
    std::vector<std::int32_t> heights;
    std::size_t height_index = 0;
    std::vector<std::string> calls;
    bool fail_rebuild = false;
    bool replace_effective_size = false;
    bool fail_set = false;
    std::int32_t font_size() override { calls.push_back("size"); return size; }
    void set_font_size(std::int32_t n) override {
        calls.push_back("set:" + std::to_string(n));
        size = replace_effective_size ? 9 : n;
        if (fail_set) throw std::runtime_error("fixture setter failure");
    }
    void rebuild_blocks(TextFitState& s) override {
        calls.push_back("rebuild");
        if (fail_rebuild) throw std::runtime_error("fixture parser failure");
        // Discontinuous layout fixture: reducing the font crosses a wrap boundary.
        s.text_width = size <= 3 ? 40.0F : 90.0F;
    }
    std::int32_t text_height() override {
        calls.push_back("height"); return heights.at(height_index++);
    }
};
int main() {
    TextFitState s{50,60,100,true,true};
    Host h; h.heights = {90,61,61,62};
    fit_text_blocks(s,h);
    check(h.size == 4 && s.height == 62 && h.calls == std::vector<std::string>(
        {"height","size","set:4","rebuild","height","height","height"}),
        "one-pixel tolerance and repeated final height query are preserved");
    s = {50,60,100,true,false}; h = Host{};
    fit_text_blocks(s,h);
    check(h.size == 3 && s.height == 60 && h.calls == std::vector<std::string>(
        {"size","set:4","rebuild","size","set:3","rebuild"}),
        "single line rebuilds at each decrement without querying height");
    s = {50,60,100,true,true}; h = Host{}; h.size = 2;
    h.replace_effective_size = true; h.heights = {90,80,81};
    fit_text_blocks(s,h);
    check(h.size == 9 && s.height == 81 && h.height_index == 3,
        "termination uses requested size one, not effective size");
    for (float height : {0.0F,-1.0F,std::numeric_limits<float>::quiet_NaN()}) {
        s = {50,height,100,true,false}; h = Host{}; fit_text_blocks(s,h);
        check(h.calls.empty(), "height guard applies even to single line");
    }
    s = {50,60,100,false,true}; h = Host{}; fit_text_blocks(s,h);
    check(h.calls.empty(), "disabled auto scale has no effects");
    s = {50,60,100,true,false}; h = Host{}; h.fail_rebuild = true;
    bool threw = false;
    try { fit_text_blocks(s,h); } catch (const std::runtime_error&) { threw = true; }
    check(threw && h.size == 4, "parser failure propagates after size mutation");
    s = {50,60,100,true,false}; h = Host{};
    h.size = std::numeric_limits<std::int32_t>::min();
    // Stop the fixture after the first update to verify wrapping, not a long loop.
    h.fail_rebuild = true;
    try { fit_text_blocks(s,h); } catch (const std::runtime_error&) {}
    check(h.size == std::numeric_limits<std::int32_t>::max(), "32-bit decrement wraps");
    s = {50,60,100,false,true}; h = Host{}; h.size = 10;
    prepare_text_font_size(s,150,h);
    check(h.size == 4 && s.font_size_adjusted && h.calls ==
          std::vector<std::string>({"size","set:4"}),
          "initial correction runs without auto scale and uses two integer divisions");
    prepare_text_font_size(s,50,h);
    check(h.calls.size() == 2, "adjusted flag prevents repeating initial correction");
    s = {50,60,100,true,false}; h = Host{};
    prepare_text_font_size(s,100,h);
    check(h.calls.empty() && !s.font_size_adjusted, "single-line initial correction is skipped");
    s.multiline = true; h.size = -10;
    prepare_text_font_size(s,150,h);
    check(h.size == -4, "negative division truncates toward zero at each step");
    s.font_size_adjusted = false;
    prepare_text_font_size(s,0,h);
    check(h.size == 0 && s.font_size_adjusted, "unguarded ARM SDIV by zero returns zero");
    s.font_size_adjusted = false; h = Host{}; h.size = 21474837;
    prepare_text_font_size(s,100,h);
    check(h.size == -21474835, "initial multiplication wraps before division");
    s.font_size_adjusted = false; h = Host{}; h.fail_set = true;
    threw = false;
    try { prepare_text_font_size(s,100,h); }
    catch (const std::runtime_error&) { threw = true; }
    check(threw && !s.font_size_adjusted, "failed setter leaves adjusted flag unset");
}
