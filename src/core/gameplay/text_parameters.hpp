#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>

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
}
