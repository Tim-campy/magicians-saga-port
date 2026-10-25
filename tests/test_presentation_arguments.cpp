#include "core/gameplay/presentation_text.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace saga::core;
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
struct Host : PresentationTextHost {
    bool cleanup = false;
    std::vector<std::u16string> received;
    std::vector<int> order;
    std::function<NullablePresentationText(const std::u16string&)> translator;
    NullablePresentationText translate_argument(const std::u16string& text) override {
        order.push_back(1); received.push_back(text);
        return translator ? translator(text) : NullablePresentationText(text);
    }
    bool remove_duplicate_honorific() override { order.push_back(2); return cleanup; }
};
int main() {
    Host h;
    check(!substitute_form_arguments({}, u"x", h), "null source remains null");
    check(substitute_form_arguments(u"<0>", {}, h) == u"<0>" &&
          substitute_form_arguments(u"<0>", u"", h) == u"<0>" && h.order.empty(),
          "null/empty args return unchanged without callbacks");
    check(substitute_form_arguments(u"<0>|<1>|<2>", u" a \t\tb", h) == u" a ||b",
          "arguments split on TAB without trim, preserving empty middle argument");
    check(h.received == std::vector<std::u16string>({u" a ", u"", u"b"}) &&
          h.order == std::vector<int>({1,2,1,2,1,2}), "translation and property reads alternate");
    h.received.clear(); h.order.clear();
    check(substitute_form_arguments(u"<0><0>", u"x<0>", h) == u"x<0>x<0>",
          "inserted current marker is not revisited");
    check(substitute_form_arguments(u"<0>", u"<1>\tvalue", h) == u"value",
          "later argument can replace a marker introduced earlier");
    check(substitute_form_arguments(u"<0>|<1>", u"a\t", h) == u"a|", "trailing argument preserved");
    check(substitute_form_arguments(u"<00>|<0>", u"x", h) == u"<00>|x", "marker matched literally");
    check(substitute_form_arguments(u"<10>", u"0\t1\t2\t3\t4\t5\t6\t7\t8\t9\tlast", h) == u"last",
          "multi-digit argument index");

    const std::u16string san = u"\u3055\u3093";
    check(substitute_form_arguments(u"<0>" + san, san, h) == san + san,
          "false flag skips cleanup, correcting previous report85 interpretation");
    h.cleanup = true;
    check(substitute_form_arguments(u"<0>" + san, san, h) == san, "true flag cleans introduced duplicate");
    check(substitute_form_arguments(u"<0></co>" + san, san, h) == san + u"</co>",
          "cleanup preserves optional closing color tag captured in group1");
    check(substitute_form_arguments(san + san + u"|<0>" + san, san, h) == san + san + u"|" + san + san,
          "existing source match disables cleanup for the entire substitution");
    check(substitute_form_arguments(u"<0>" + san, san + san, h) == san + san + san,
          "existing argument match disables cleanup");
    check(substitute_form_arguments(u"<0>" + san + u"|<0>" + san, san, h) == san + u"|" + san,
          "all nonoverlapping introduced matches cleaned");
    check(substitute_form_arguments(san + u"<0>" + san, san, h) == san + san,
          "three introduced honorifics collapse only the first nonoverlapping pair");
    // Empty marker replacement joins two formerly separated honorific fragments.
    check(substitute_form_arguments(san + u"<0>" + san, u"\tunused", h) == san,
          "empty argument can introduce a duplicate across the removed marker");
    h.translator = [&](const std::u16string&) -> NullablePresentationText { h.cleanup = true; return san; };
    h.cleanup = false;
    check(substitute_form_arguments(u"<0>" + san, u"input", h) == san,
          "property observed after translator changes it");
    h.translator = [](const std::u16string&) -> NullablePresentationText { return {}; };
    h.cleanup = false;
    check(substitute_form_arguments(u"<0>", u"x", h) == u"<0>", "null translation is unchanged on skip branch");
    h.cleanup = true;
    bool failed = false;
    try { substitute_form_arguments(u"<0>", u"x", h); }
    catch (const std::invalid_argument&) { failed = true; }
    check(failed, "enabled regex argument check rejects null translated value");
    check(substitute_form_arguments(san + san, u"x", h) == san + san,
          "existing source duplicate short circuits null argument regex check");

    // Format before splitting: a translated argument may introduce a new page.
    h.cleanup = false;
    h.translator = [](const std::u16string&) -> NullablePresentationText { return u" one\t two "; };
    const auto formatted = substitute_form_arguments(u"<0>", u"arg", h);
    check(formatted && split_form_text(&*formatted) == std::vector<std::u16string>({u"one", u"two"}),
          "factory formats first, then splits and trims final pages");
    std::cout << "presentation argument contracts passed\n";
}
