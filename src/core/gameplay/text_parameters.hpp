#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace saga::core {
// Host reports a managed System.Exception from ParseInt through this category.
// Native allocation errors and unrelated C++ failures are not swallowed.
class ManagedTextParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
class TextParameterHost {
public:
    virtual ~TextParameterHost() = default;
    virtual std::int32_t parse_decimal(const std::u16string& value) = 0;
    virtual std::int32_t parse_hexadecimal(const std::u16string& value) = 0;
};
struct TextTagNumbers {
    std::int32_t decimal;
    std::uint32_t color_bits;
};
// Parameter text between '=' and '>'; call only for the parameterized path.
// This does not supply a .NET culture/parser or catch the outer layout failure.
TextTagNumbers parse_text_tag_numbers(std::int32_t tag_index,
                                     const std::u16string& parameter,
                                     TextParameterHost& host);
// co/bo extract three bytes before calling Graphics.GetColorOfRGB.
std::uint32_t text_tag_argb(std::uint32_t color_bits);

struct TextMarkupSpan {
    std::int32_t source_begin=0,source_end=0; // Exclusive end in the supplied UTF16 source.
    std::int32_t tag_index=-1; // -1 for ordinary text, otherwise the native TAG index0..18.
    std::u16string_view text;
    std::u16string_view parameter;
    bool has_parameter=false; // Distinguishes an empty '=' parameter from no '='.
};
class TextMarkupHost {
public:
    virtual ~TextMarkupHost()=default;
    // Original String.CompareTo and StartsWith contracts belong to the caller;
    // the scanner does not silently select a culture or ordinal comparison.
    virtual bool equal_tag(std::u16string_view expected,std::u16string_view candidate)=0;
    virtual bool starts_with_tag(std::u16string_view candidate,std::u16string_view prefix)=0;
    virtual void text(const TextMarkupSpan&)=0;
    virtual void tag(const TextMarkupSpan&)=0;
};
// TextLayout._makeTextBlocks scanner, after translation/Chanakya and literal
// <br> -> LF replacement. Views and source indices refer to that supplied text.
// Ordinary callbacks group contiguous units only; an overlong opener creates
// a source gap. Numeric parsing and tag effects remain in the consumer.
void scan_text_markup(std::u16string_view source,TextMarkupHost&);
}
