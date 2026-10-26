#include "culture_lower.hpp"
#include <cstdint>
#include <stdexcept>
namespace saga::core {
namespace {
struct Range {std::uint16_t first,last;};
constexpr std::array<Range,9> ranges{{{0x00c0,0x0556},{0x10a0,0x10c5},{0x1e00,0x1ffc},
    {0x2160,0x216f},{0x24b6,0x24cf},{0x2c00,0x2c2e},{0x2c60,0x2ce2},
    {0xa640,0xa696},{0xa722,0xa78b}}};
}
// TextInfo.ToLower(char) 0x1e18e94 and invariant branch of 0x1e1901c.
char16_t lower_invariant_unit(char16_t unit,const CultureLowerTables& tables) {
    const auto value=static_cast<std::uint16_t>(unit);
    if(value>=0x41 && value<=0x5a)return static_cast<char16_t>(value|0x20);
    for(std::size_t i=0;i<ranges.size();++i) {
        const auto range=ranges[i];
        if(value<range.first || value>range.last)continue;
        const auto count=static_cast<std::size_t>(range.last-range.first)+1;
        if(tables[i].size()!=2*count)throw std::invalid_argument("invalid invariant lowercase table extent");
        const auto offset=2*static_cast<std::size_t>(value-range.first);
        return static_cast<char16_t>(static_cast<unsigned char>(tables[i][offset])
            +256U*static_cast<unsigned char>(tables[i][offset+1]));
    }
    if(value>=0xff21 && value<=0xff3a)return static_cast<char16_t>(value+0x20);
    if(value==0x2132)return static_cast<char16_t>(0x214e);
    if(value==0x2183)return static_cast<char16_t>(0x2184);
    return unit;
}
// TextInfo.ToLowerInternal(string) 0x1e194f4 visits each code unit independently.
std::u16string lower_invariant_text(std::u16string_view value,const CultureLowerTables& tables) {
    std::u16string result;result.reserve(value.size());
    for(auto unit:value)result.push_back(lower_invariant_unit(unit,tables));
    return result;
}
}
