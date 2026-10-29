#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace saga::test {
using Bytes=std::vector<std::uint8_t>;
inline void append32(Bytes& out,std::uint32_t n) {
    for (unsigned shift:{24U,16U,8U,0U}) out.push_back(static_cast<std::uint8_t>(n>>shift));
}
inline Bytes jar_fixture(const std::vector<std::pair<std::string,Bytes>>& members) {
    Bytes index,body;
    append32(index,static_cast<std::uint32_t>(members.size()));
    for(const auto& member:members){
        append32(index,static_cast<std::uint32_t>(member.first.size()));
        index.insert(index.end(),member.first.begin(),member.first.end());
    }
    std::uint32_t cursor=0;
    for(const auto& member:members){append32(index,cursor);cursor+=4U+static_cast<std::uint32_t>(member.second.size());}
    for(const auto& member:members)append32(index,static_cast<std::uint32_t>(member.second.size()));
    index.insert(index.end(),members.size(),0);
    for(const auto& member:members){append32(body,static_cast<std::uint32_t>(member.second.size()));body.insert(body.end(),member.second.begin(),member.second.end());}
    Bytes out;append32(out,static_cast<std::uint32_t>(index.size()+4U));append32(out,static_cast<std::uint32_t>(body.size()));
    out.insert(out.end(),index.begin(),index.end());out.insert(out.end(),body.begin(),body.end());return out;
}
} // namespace saga::test
