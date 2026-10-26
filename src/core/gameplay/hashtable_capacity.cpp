#include "hashtable_capacity.hpp"
#include "hashtable_rehash.hpp"
#include <cmath>
#include <limits>
namespace saga::core {
// HashHelpers.IsPrime 0x1e2faa8. Preserve its treatment of 1 and negative odd inputs.
bool hashtable_is_prime(std::int32_t candidate) {
    if((static_cast<std::uint32_t>(candidate)&1U)==0)return candidate==2;
    if(candidate<0)return true; // sqrt -> NaN -> ARM64 FCVTZS 0; no divisor loop.
    const auto limit=static_cast<std::int32_t>(std::sqrt(static_cast<double>(candidate)));
    for(std::int32_t divisor=3;divisor<=limit;divisor+=2)
        if(candidate%divisor==0)return false;
    return true;
}
// HashHelpers.GetPrime 0x1e2fb60.
std::int32_t hashtable_get_prime(std::int32_t minimum,const std::vector<std::int32_t>& primes) {
    if(minimum<0)throw CultureManagedException("negative Hashtable prime minimum");
    for(auto prime:primes)if(prime>=minimum)return prime;
    auto candidate=static_cast<std::uint32_t>(minimum)|1U;
    while(candidate<0x7fffffffU) {
        // Exact unsigned strength-reduced filter in the shipped binary.
        if(hashtable_is_prime(static_cast<std::int32_t>(candidate)) &&
           candidate*0x7c32b16dU+0x8511be19U>0x0288df0cU)
            return static_cast<std::int32_t>(candidate);
        candidate+=2U;
    }
    return minimum;
}
// HashHelpers.ExpandPrime 0x1e2fce8: signed old-size comparison, unsigned doubled one.
std::int32_t hashtable_expand_prime(std::int32_t old_size,const std::vector<std::int32_t>& primes) {
    const auto doubled=static_cast<std::uint32_t>(old_size)*2U;
    if(old_size<=0x7feffffc && doubled>=0x7feffffeU)return 0x7feffffd;
    const auto signed_value=doubled<=0x7fffffffU?static_cast<std::int64_t>(doubled):
        static_cast<std::int64_t>(doubled)-0x100000000LL;
    return hashtable_get_prime(static_cast<std::int32_t>(signed_value),primes);
}
// Hashtable.expand 0x1e3d378.
void expand_hashtable(HashtableState& table,const std::vector<std::int32_t>& primes) {
    if(!table.buckets)throw CultureManagedException("null Hashtable buckets");
    if(table.buckets->size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("Hashtable capacity exceeds int32");
    const auto size=hashtable_expand_prime(static_cast<std::int32_t>(table.buckets->size()),primes);
    rehash_hashtable(table,size);
}
// Hashtable..ctor(int32,binary32) 0x1e3c294. Default ctor supplies 0 and 1.0F.
std::shared_ptr<HashtableState> make_hashtable(std::int32_t capacity,float load_factor,
                                             const std::vector<std::int32_t>& primes) {
    if(capacity<0)throw CultureManagedException("negative Hashtable capacity");
    if(!(load_factor>=0.1F&&load_factor<=1.0F))throw CultureManagedException("Hashtable load factor out of range");
    auto table=std::make_shared<HashtableState>();
    table->load_factor=load_factor*0.72F;
    const float requested=static_cast<float>(capacity)/table->load_factor;
    if(static_cast<double>(requested)>2147483647.0)throw CultureManagedException("Hashtable capacity overflow");
    const auto size=requested<=3.0F?3:hashtable_get_prime(static_cast<std::int32_t>(requested),primes);
    table->buckets=std::make_shared<std::vector<HashtableBucket>>(static_cast<std::size_t>(size));
    const float threshold=table->load_factor*static_cast<float>(size);
    // Valid constructor inputs keep threshold finite and inside int32.
    table->load_size=threshold>=2147483648.0F?std::numeric_limits<std::int32_t>::max():static_cast<std::int32_t>(threshold);
    return table;
}
}
