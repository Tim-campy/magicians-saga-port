#include "text_parameters.hpp"
#include "presentation_text.hpp"

namespace saga::core {
// _makeTextBlocks19200, ELF0x1bbe414..0x1bbe4d8. Local handlers at
// 0x1bbf300/0x1bbf2bc; comma-component failures follow the outer exception path.
TextTagNumbers parse_text_tag_numbers(std::int32_t tag_index,
                                     const std::u16string& parameter,
                                     TextParameterHost& host) {
    TextTagNumbers result{0,0};
    try { result.decimal = host.parse_decimal(parameter); }
    catch (const ManagedTextParseError&) {}
    try { result.color_bits = static_cast<std::uint32_t>(host.parse_hexadecimal(parameter)); }
    catch (const ManagedTextParseError&) {}
    if ((tag_index == 0 || tag_index == 10) && result.color_bits == 0 &&
        parameter.find(u',') != std::u16string::npos) {
        const auto fields = split_text_color_fields(&parameter);
        result.color_bits = static_cast<std::uint32_t>(host.parse_decimal(fields.at(0))) << 16;
        if (fields.size() >= 2)
            result.color_bits |= static_cast<std::uint32_t>(host.parse_decimal(fields[1])) << 8;
        if (fields.size() >= 3)
            result.color_bits |= static_cast<std::uint32_t>(host.parse_decimal(fields[2]));
        // Fields beyond the third are not read by this path.
    }
    return result;
}
// Tag caller masks the channels before Graphics.GetColorOfRGB17946 (0x1b62c34).
std::uint32_t text_tag_argb(std::uint32_t bits) {
    return 0xff000000U | (bits & 0x00ffffffU);
}
}
