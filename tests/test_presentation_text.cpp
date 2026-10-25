#include "core/gameplay/presentation_text.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>

using namespace saga::core;
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    std::u16string empty;
    check(split_form_text(nullptr).empty() && split_form_text(&empty).empty(),
          "null/empty input produces zero parts");
    std::u16string text = u" first \r\t\t last\n\t";
    check(split_form_text(&text) == std::vector<std::u16string>({u"first", u"", u"last", u""}),
          "trim each part while retaining empty and final fields");
    text = u"\t";
    check(split_form_text(&text) == std::vector<std::u16string>({u"", u""}),
          "delimiter-only input differs from empty input");
    text = u" \r\n";
    check(split_form_text(&text) == std::vector<std::u16string>({u""}),
          "all trimmed input preserves its empty part");
    text = u"\\\tvalue";
    check(split_form_text(&text) == std::vector<std::u16string>({u"\\", u"value"}),
          "backslash does not escape delimiter in this overload");
    text = u"\u00a0\u3000first\u3000\u00a0";
    check(split_form_text(&text) == std::vector<std::u16string>({text}),
          "NBSP and ideographic spaces are preserved");
    text = std::u16string({u'A', char16_t(0), u'B'});
    check(split_form_text(&text) == std::vector<std::u16string>({text}), "interior NUL is not trimmed");

    // Exhaust all UTF-16 boundary units, including unpaired surrogates. TAB is
    // separately verified above because it is the delimiter, not a trim probe.
    for (std::uint32_t unit = 0; unit <= 0xffffU; ++unit) {
        if (unit == 9) continue;
        const auto character = static_cast<char16_t>(unit);
        text = std::u16string({character, u'X', character});
        const auto parts = split_form_text(&text);
        check(parts.size() == 1 && parts[0] == (unit <= 32 ? u"X" : text),
              "UTF-16 boundary rule must be exactly <=0x20");
    }

    // Form factory sequence: outer trimmed TAB split, inner raw '&' split.
    text = u" 1& hello &unused \t2&\t";
    const auto pages = split_form_text(&text);
    check(pages.size() == 3, "outer split retains trailing empty event segment");
    check(split_event_text_fields(&pages[0]) == std::vector<std::u16string>({u"1", u" hello ", u"unused"}),
          "inner fields are not trimmed or collapsed into one text");
    check(split_event_text_fields(&pages[1]) == std::vector<std::u16string>({u"2", u""}),
          "explicit empty event text remains a second field");
    check(split_event_text_fields(&pages[2]).empty(), "empty event segment has no fields");
    check(split_event_text_fields(nullptr).empty(), "inner null input returns no fields");
    std::cout << "presentation text contracts passed\n";
}
