#include "culture_signs.hpp"
#include <stdexcept>
namespace saga::assets {
// Native 0x1036a00/0x1037294; bsearch ordering from AOSP android-15.0.0_r1.
std::optional<std::size_t> find_culture_record_by_lcid(std::int32_t lcid,std::string_view cultures) {
    if(cultures.size()%56)throw std::invalid_argument("truncated culture records");
    const auto low=static_cast<std::uint32_t>(lcid)&0xffffU;
    const auto key=static_cast<std::int32_t>(low)-(low>=0x8000U?65536:0);
    std::size_t first=0,last=cultures.size()/56;
    while(first<last) {
        const auto mid=first+(last-first)/2,offset=mid*56;
        const auto raw=static_cast<std::int32_t>(static_cast<unsigned char>(cultures[offset]))
            +256*static_cast<unsigned char>(cultures[offset+1]);
        const auto value=raw-(raw>=0x8000?65536:0);
        if(key==value)return mid;
        if(key<value)last=mid;
        else first=mid+1;
    }
    return std::nullopt;
}
// Native bsearch/comparator/population: 0x1036c08, 0x1036cc0, 0x1036a60.
std::optional<std::size_t> find_culture_number_index(std::string_view key,std::string_view names,
    std::string_view cultures,std::string_view pool) {
    if(names.size()%4 || cultures.size()%56)throw std::invalid_argument("truncated culture lookup tables");
    key=key.substr(0,key.find('\0'));
    const auto u16=[](std::string_view data,std::size_t offset){
        return static_cast<std::size_t>(static_cast<unsigned char>(data[offset]))
            +256U*static_cast<unsigned char>(data[offset+1]);
    };
    std::size_t first=0,last=names.size()/4;
    while(first<last) {
        const auto mid=first+(last-first)/2;
        const auto offset=u16(names,mid*4);
        if(offset>=pool.size())throw std::invalid_argument("culture name outside pool");
        const auto end=pool.find('\0',offset);
        if(end==pool.npos)throw std::invalid_argument("unterminated culture name");
        const auto comparison=key.compare(pool.substr(offset,end-offset));
        if(comparison<0)last=mid;
        else if(comparison>0)first=mid+1;
        else {
            const auto index=u16(names,mid*4+2);
            if(index>=0x8000 || index>=cultures.size()/56)throw std::invalid_argument("invalid signed culture index");
            const auto number=u16(cultures,index*56+0x20);
            if(number>=0x8000)throw std::invalid_argument("negative number-format index");
            return number;
        }
    }
    return std::nullopt;
}
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
