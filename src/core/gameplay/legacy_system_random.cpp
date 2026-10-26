// Adapted from .NET CoreCLR v3.1.0 System.Private.CoreLib/shared/System/Random.cs.
// Copyright (c) .NET Foundation and Contributors. MIT license:
// third_party/licenses/dotnet-coreclr.txt. Only explicit-seed construction and Next are used.
#include "game_random.hpp"
#include <limits>

namespace saga::core {
namespace {
constexpr auto maximum=std::numeric_limits<std::int32_t>::max();
std::int32_t wrap(std::int64_t value){
    const auto bits=static_cast<std::uint32_t>(value);
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):
        static_cast<std::int32_t>(static_cast<std::int64_t>(bits)-0x100000000LL);
}
}
LegacySystemRandom::LegacySystemRandom(std::int32_t seed){
    const auto magnitude=seed==std::numeric_limits<std::int32_t>::min()?maximum:(seed<0?-seed:seed);
    auto previous=wrap(161803398LL-magnitude);
    seeds_[55]=previous;
    std::int32_t current=1,slot=0;
    for(int i=1;i<55;++i){
        slot+=21;if(slot>=55)slot-=55;
        seeds_[static_cast<std::size_t>(slot)]=current;
        current=wrap(static_cast<std::int64_t>(previous)-current);
        if(current<0)current=wrap(static_cast<std::int64_t>(current)+maximum);
        previous=seeds_[static_cast<std::size_t>(slot)];
    }
    for(int pass=0;pass<4;++pass)for(int i=1;i<56;++i){
        auto other=i+30;if(other>=55)other-=55;
        auto& value=seeds_[static_cast<std::size_t>(i)];
        value=wrap(static_cast<std::int64_t>(value)-seeds_[static_cast<std::size_t>(1+other)]);
        if(value<0)value=wrap(static_cast<std::int64_t>(value)+maximum);
    }
}
std::int32_t LegacySystemRandom::next(){
    if(++index_>=56)index_=1;
    if(++paired_index_>=56)paired_index_=1;
    auto value=wrap(static_cast<std::int64_t>(seeds_[static_cast<std::size_t>(index_)])-seeds_[static_cast<std::size_t>(paired_index_)]);
    if(value==maximum)--value;
    if(value<0)value=wrap(static_cast<std::int64_t>(value)+maximum);
    seeds_[static_cast<std::size_t>(index_)]=value;
    return value;
}
}
