#include "filter_identity.hpp"

namespace saga::core {
namespace {
std::int32_t signed_bits(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
bool equal(const std::vector<std::int32_t>* a,const std::vector<std::int32_t>* b){return a==b||(a&&b&&*a==*b);}
}
std::int32_t component_types_hash(const std::vector<std::int32_t>* list,const std::function<void()>& diagnostic){
    if(!list||list->empty())return 0;
    if(list->size()>=33)diagnostic();
    std::uint32_t value=0;
    for(const auto index:*list)value+=1U<<(static_cast<std::uint32_t>(index)&31U);
    return signed_bits(value);
}
// Filter.GetHashCode 0x113a6fc: publish partial cache after each successful list.
std::int32_t FilterIdentity::hash(const std::function<void()>& diagnostic){
    if(cached_!=-1)return cached_;
    const auto all=component_types_hash(lists.all,diagnostic);cached_=all;
    const auto any=component_types_hash(lists.any,diagnostic);
    const auto partial=signed_bits(static_cast<std::uint32_t>(all)^static_cast<std::uint32_t>(any));cached_=partial;
    const auto none=component_types_hash(lists.none,diagnostic);
    cached_=signed_bits(static_cast<std::uint32_t>(partial)^static_cast<std::uint32_t>(none));
    return cached_;
}
// Filter.Equals 0x113a5a8; exact concrete type is enforced by the C++ parameter.
bool FilterIdentity::equals(FilterIdentity* other,const std::function<void()>& diagnostic){
    if(!other)return false;
    const auto other_hash=other->hash(diagnostic);
    if(other_hash!=hash(diagnostic))return false;
    return equal(lists.all,other->lists.all)&&equal(lists.any,other->lists.any)&&equal(lists.none,other->lists.none);
}
}
