#include "core/gameplay/text_parameters.hpp"
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace saga::core;
void check(bool ok, const char* label) {
    if (!ok) { std::cerr << label << '\n'; std::exit(1); }
}
struct Host : TextParameterHost {
    std::vector<std::u16string> calls;
    bool native_failure = false;
    // Explicit fixture values, not a replacement production numeric parser.
    std::int32_t parse_decimal(const std::u16string& value) override {
        calls.push_back(u"D:"+value);
        if (native_failure) throw std::logic_error("native failure");
        if (value == u"1") return 1;
        if (value == u"2") return 2;
        if (value == u"3") return 3;
        if (value == u"256") return 256;
        if (value == u"-1") return -1;
        if (value == u"0") return 0;
        if (value == u"10") return 10;
        throw ManagedTextParseError("fixture decimal failure");
    }
    std::int32_t parse_hexadecimal(const std::u16string& value) override {
        calls.push_back(u"H:"+value);
        if (value == u"4a86e8") return 0x4a86e8;
        if (value == u"10") return 16;
        if (value == u"0") return 0;
        throw ManagedTextParseError("fixture hex failure");
    }
};
int main() {
    Host h;
    auto n = parse_text_tag_numbers(0,u"4a86e8",h);
    check(n.decimal == 0 && text_tag_argb(n.color_bits) == 0xff4a86e8U &&
          h.calls == std::vector<std::u16string>({u"D:4a86e8",u"H:4a86e8"}),
          "decimal failure resumes hex parse");
    n = parse_text_tag_numbers(0,u"10",h);
    check(n.decimal == 10 && n.color_bits == 16, "decimal and hex results are distinct");
    h.calls.clear();
    n = parse_text_tag_numbers(10,u" 1 , 2 , 3 ,ignored",h);
    check(n.color_bits == 0x010203 && h.calls == std::vector<std::u16string>(
          {u"D: 1 , 2 , 3 ,ignored",u"H: 1 , 2 , 3 ,ignored",u"D:1",u"D:2",u"D:3"}),
          "fallback trims channels, preserves order and ignores fourth field");
    n = parse_text_tag_numbers(0,u"1,256,0",h);
    check(text_tag_argb(n.color_bits) == 0xff010000U, "combine before extracting bytes, no channel clamp");
    n = parse_text_tag_numbers(0,u"1,2,-1",h);
    check(text_tag_argb(n.color_bits) == 0xffffffffU, "negative blue OR affects all low24 bits");
    n = parse_text_tag_numbers(0,u"1,2",h);
    check(n.color_bits == 0x010200, "two fields leave low byte zero");
    h.calls.clear();
    n = parse_text_tag_numbers(8,u"1,2,3",h);
    check(n.color_bits == 0 && h.calls.size() == 2, "non-color tags do not use RGB fallback");
    n = parse_text_tag_numbers(0,u"invalid",h);
    check(n.decimal == 0 && n.color_bits == 0, "two local failures with no comma remain zero");
    h.calls.clear();
    bool propagated = false;
    try { parse_text_tag_numbers(0,u"1,,3",h); }
    catch (const ManagedTextParseError&) { propagated = true; }
    check(propagated && h.calls.back() == u"D:" && h.calls.size() == 4,
          "component error propagates and third channel is not parsed");
    h.native_failure = true; h.calls.clear(); propagated = false;
    try { parse_text_tag_numbers(0,u"10",h); }
    catch (const std::logic_error&) { propagated = true; }
    check(propagated && h.calls.size() == 1, "unrelated native exceptions are not swallowed");
    std::cout << "text numeric parameter contracts passed\n";
}
