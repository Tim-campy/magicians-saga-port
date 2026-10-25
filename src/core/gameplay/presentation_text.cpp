#include "presentation_text.hpp"

namespace saga::core {
namespace {
std::vector<std::u16string> split(const std::u16string* text, char16_t delimiter, bool trim) {
    std::vector<std::u16string> result;
    if (!text || text->empty()) return result;
    std::size_t start = 0;
    for (;;) {
        const auto delimiter_at = text->find(delimiter, start);
        auto end = delimiter_at == std::u16string::npos ? text->size() : delimiter_at;
        auto first = start;
        if (trim) {
            // ELF0x1c192e0..0x1c19398: unsigned char comparisons against0x21/0x20,
            // not Char.IsWhiteSpace or the locale-dependent C/C++ whitespace set.
            while (first < end && (*text)[first] <= u' ') ++first;
            while (end > first && (*text)[end - 1] <= u' ') --end;
        }
        result.push_back(text->substr(first, end - first));
        if (delimiter_at == std::u16string::npos) return result;
        start = delimiter_at + 1;
    }
}
}
std::vector<std::u16string> split_form_text(const std::u16string* text) {
    return split(text, u'\t', true);
}
std::vector<std::u16string> split_event_text_fields(const std::u16string* text) {
    return split(text, u'&', false);
}
}
