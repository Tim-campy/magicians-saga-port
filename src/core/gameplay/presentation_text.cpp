#include "presentation_text.hpp"
#include <stdexcept>
#include <string_view>

namespace saga::core {
namespace {
constexpr std::u16string_view honorific = u"\u3055\u3093";
constexpr std::u16string_view color_end = u"</co>";
struct HonorificMatch { std::size_t start, keep_end, end; };

// Exact language of (san(</co>)?)san. Leftmost, nonoverlapping matches, like
// the original Regex.Replace(pattern, "$1"); no recursive cleanup.
std::optional<HonorificMatch> find_duplicate(const std::u16string& text, std::size_t from = 0) {
    for (auto at = text.find(honorific, from); at != std::u16string::npos;
         at = text.find(honorific, at + 1)) {
        auto next = at + honorific.size();
        if (text.compare(next, color_end.size(), color_end) == 0) next += color_end.size();
        if (text.compare(next, honorific.size(), honorific) == 0)
            return HonorificMatch{at, next, next + honorific.size()};
    }
    return std::nullopt;
}
std::u16string clean_duplicates(const std::u16string& text) {
    std::u16string result;
    std::size_t from = 0;
    while (const auto match = find_duplicate(text, from)) {
        result.append(text, from, match->keep_end - from);
        from = match->end;
    }
    result.append(text, from, std::u16string::npos);
    return result;
}
std::u16string marker_for(std::size_t index) {
    std::u16string digits;
    do {
        digits.insert(digits.begin(), static_cast<char16_t>(u'0' + index % 10));
        index /= 10;
    } while (index);
    return u"<" + digits + u">";
}
void replace_marker(std::u16string& text, const std::u16string& marker, const std::u16string& value) {
    std::size_t from = 0;
    for (;;) {
        const auto at = text.find(marker, from);
        if (at == std::u16string::npos) return;
        text.replace(at, marker.size(), value);
        from = at + value.size(); // Do not revisit the just-inserted text.
    }
}
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
std::vector<std::u16string> split_text_color_fields(const std::u16string* text) {
    return split(text, u',', true);
}
std::vector<std::u16string> split_trimmed_text_fields(const std::u16string* text,char16_t delimiter) {
    return split(text,delimiter,true);
}
NullablePresentationText substitute_form_arguments(NullablePresentationText text,
                                                   NullablePresentationText args,
                                                   PresentationTextHost& host) {
    if (!text || !args || args->empty()) return text;
    const auto fields = split(&*args, u'\t', false);
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const auto value = host.translate_argument(fields[i]);
        bool clean = false;
        // ELF0x1c18f60: cbz skips regex checks when the property is false.
        if (host.remove_duplicate_honorific() && !find_duplicate(*text)) {
            if (!value) throw std::invalid_argument("null translated argument in honorific check");
            clean = !find_duplicate(*value);
        }
        // Literal Replace returns input unchanged if replacement is null.
        if (value) replace_marker(*text, marker_for(i), *value);
        if (clean) *text = clean_duplicates(*text);
    }
    return text;
}
}
