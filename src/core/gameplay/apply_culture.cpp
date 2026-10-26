#include "apply_culture.hpp"
#include "assets/culture_signs.hpp"
namespace saga::core {
// The extractor proves valid UTF-8 for every reachable sign. Reject corrupted
// inputs at this adapter boundary; managed replacement fallback is not modeled.
std::u16string decode_culture_utf8(std::string_view bytes) {
    std::u16string result;
    for(std::size_t i=0;i<bytes.size();) {
        const auto first=static_cast<unsigned char>(bytes[i++]);
        std::uint32_t scalar=0,minimum=0;
        unsigned trailing=0;
        if(first<0x80)scalar=first;
        else if(first>=0xc2 && first<=0xdf){scalar=first&0x1fU;minimum=0x80;trailing=1;}
        else if(first>=0xe0 && first<=0xef){scalar=first&0x0fU;minimum=0x800;trailing=2;}
        else if(first>=0xf0 && first<=0xf4){scalar=first&0x07U;minimum=0x10000;trailing=3;}
        else throw std::invalid_argument("invalid extracted sign UTF-8");
        if(trailing>bytes.size()-i)throw std::invalid_argument("truncated extracted sign UTF-8");
        while(trailing--) {
            const auto next=static_cast<unsigned char>(bytes[i++]);
            if((next&0xc0U)!=0x80U)throw std::invalid_argument("invalid extracted sign continuation");
            scalar=(scalar<<6)|(next&0x3fU);
        }
        if(scalar<minimum || scalar>0x10ffff || (scalar>=0xd800 && scalar<=0xdfff))
            throw std::invalid_argument("invalid extracted sign scalar");
        if(scalar<0x10000)result.push_back(static_cast<char16_t>(scalar));
        else {
            scalar-=0x10000;
            result.push_back(static_cast<char16_t>(0xd800+(scalar>>10)));
            result.push_back(static_cast<char16_t>(0xdc00+(scalar&0x3ff)));
        }
    }
    return result;
}
ApplyNumberSigns read_apply_number_signs(std::string_view records,std::string_view pool,std::size_t index) {
    const auto text=assets::read_culture_signs(records,pool,index);
    auto negative=decode_culture_utf8(text.negative);
    auto positive=decode_culture_utf8(text.positive);
    return {std::move(positive),std::move(negative)};
}
}
