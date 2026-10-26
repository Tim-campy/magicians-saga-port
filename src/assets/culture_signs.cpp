#include "culture_signs.hpp"
#include <stdexcept>
namespace saga::assets {
CultureSignText read_culture_signs(std::string_view records,std::string_view pool,std::size_t index) {
    constexpr std::size_t stride=48;
    if(records.size()%stride!=0 || index>=records.size()/stride)
        throw std::invalid_argument("invalid culture number-format record");
    const auto record=records.substr(index*stride,stride);
    const auto text=[&](std::size_t field){
        const auto offset=static_cast<std::size_t>(static_cast<unsigned char>(record[field]))
            +256U*static_cast<unsigned char>(record[field+1]);
        if(offset>=pool.size())throw std::invalid_argument("culture sign offset outside pool");
        const auto end=pool.find('\0',offset);
        if(end==pool.npos)throw std::invalid_argument("unterminated culture sign");
        return pool.substr(offset,end-offset);
    };
    // Negative sign is +0x14; +0x10 belongs to negative infinity.
    const auto negative=text(0x14);
    const auto positive=text(0x16);
    return {positive,negative};
}
}
