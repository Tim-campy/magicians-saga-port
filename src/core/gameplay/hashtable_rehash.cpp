#include "hashtable_rehash.hpp"
#include <cmath>
#include <limits>
namespace saga::core {
// Hashtable.InitHash 0x1e3c61c: multiplication wraps before unsigned remainder.
HashtableHash hashtable_hash(std::uint32_t raw_hash,std::uint32_t length) {
    if(length<2) throw std::invalid_argument("Hashtable hash requires length > 1");
    const auto seed=raw_hash&0x7fffffffU;
    return {seed,1U+(seed*101U)%(length-1U)};
}
namespace {
std::int32_t load_size(float factor,std::int32_t size) {
    const float product=factor*static_cast<float>(size);
    if(std::isnan(product))return 0;
    if(product==std::numeric_limits<float>::infinity()||product<=-2147483648.0F)
        return std::numeric_limits<std::int32_t>::min();
    if(product>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();
    return static_cast<std::int32_t>(product);
}
// Hashtable.putEntry 0x1e3d520, restricted to the fresh array used by rehash.
void place(HashtableState& table,std::vector<HashtableBucket>& target,const HashtableBucket& old) {
    const auto size=static_cast<std::uint32_t>(target.size());
    const auto hash=hashtable_hash(old.hash_collision,size);
    auto index=hash.seed%size;
    while(target[index].key) {
        auto& collision=target[index].hash_collision;
        if((collision&0x80000000U)==0) {collision|=0x80000000U;++table.occupancy;}
        index=static_cast<std::uint32_t>((static_cast<std::uint64_t>(index)+hash.step)%size);
    }
    auto& slot=target[index];
    slot.value=old.value;slot.key=old.key;slot.hash_collision|=hash.seed;
}
}
// Hashtable.rehash(int32) 0x1e3d3e4. No atomic/concurrent reader model is claimed.
void rehash_hashtable(HashtableState& table,std::int32_t new_size) {
    if(new_size<2)throw std::invalid_argument("Hashtable rehash requires prime capacity > 1");
    table.occupancy=0; // This store precedes allocation and remains on failure.
    auto target=std::make_shared<std::vector<HashtableBucket>>(static_cast<std::size_t>(new_size));
    auto old=table.buckets;
    if(!old)throw CultureManagedException("null Hashtable buckets");
    for(const auto& slot:*old) if(slot.key&&!slot.tombstone)place(table,*target,slot);
    const auto threshold=load_size(table.load_factor,new_size);
    const auto version=table.version;
    table.writer_in_progress=true;
    table.buckets=target;table.load_size=threshold;
    table.version=version+1U;
    table.writer_in_progress=false;
}
}
