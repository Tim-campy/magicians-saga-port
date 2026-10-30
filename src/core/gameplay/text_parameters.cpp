#include "text_parameters.hpp"
#include "presentation_text.hpp"
#include <array>
#include <limits>

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
void scan_text_markup(std::u16string_view source,TextMarkupHost& host) {
    // Native scanner 0x1bbdad4..0x1bbdcc4; TAG initializer 0x1bbc4a8..0x1bbc69c.
    static constexpr std::array<std::u16string_view,19> tags{
        u"<co=>",u"</co>",u"<si=>",u"</si>",u"<st=>",u"</st>",u"<bl=>",u"</bl>",
        u"<po=>",u"<pic=>",u"<bo=>",u"</bo>",u"<btn=>",u"<ff>",u"<kr=>",
        u"<em>",u"</em>",u"<ruby=>",u"</ruby>"};
    if(source.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("markup exceeds managed string length");
    std::size_t at=0,ordinary=0;
    auto emit_text=[&](std::size_t end){
        if(ordinary<end)host.text({static_cast<std::int32_t>(ordinary),static_cast<std::int32_t>(end),-1,source.substr(ordinary,end-ordinary),{},false});
    };
    while(at<source.size()) {
        if(source[at]!=u'<'){++at;continue;}
        const auto closing=source.find(u'>',at);
        if(closing==std::u16string_view::npos){++at;continue;}
        if(closing-at>32){
            // 0x1bbdb0c..0x1bbdb14 -> 0x1bbe854 drops this '<' only.
            emit_text(at);ordinary=++at;continue;
        }
        const auto candidate=source.substr(at,closing-at+1);
        // Earlier ordinary units have reached the consumer before comparison
        // callbacks can fail at this candidate.
        emit_text(at);ordinary=at;
        std::int32_t matched=-1;
        for(std::size_t i=0;i<tags.size();++i) {
            if(host.equal_tag(tags[i],candidate)){matched=static_cast<std::int32_t>(i);break;}
            const auto equal=tags[i].find(u'=');
            if(equal!=std::u16string_view::npos&&candidate.find(u'=')!=std::u16string_view::npos&&
                host.starts_with_tag(candidate,tags[i].substr(0,equal+1))) {
                matched=static_cast<std::int32_t>(i);break;
            }
        }
        if(matched<0){++at;continue;} // Keep the unit; a later nested opener is still scanned.
        const auto equal=candidate.find(u'=');
        const bool parameterized=equal!=std::u16string_view::npos;
        const auto parameter=parameterized?candidate.substr(equal+1,candidate.size()-equal-2):std::u16string_view{};
        host.tag({static_cast<std::int32_t>(at),static_cast<std::int32_t>(closing+1),matched,candidate,parameter,parameterized});
        at=closing+1;ordinary=at;
    }
    emit_text(source.size());
}
}
