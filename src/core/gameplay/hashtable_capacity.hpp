#pragma once
#include "hashtable_enumerator.hpp"
namespace saga::core {
bool hashtable_is_prime(std::int32_t candidate);
// Supply the original ordered prime table as local data; never synthesize its entries.
std::int32_t hashtable_get_prime(std::int32_t minimum,const std::vector<std::int32_t>& primes);
std::int32_t hashtable_expand_prime(std::int32_t old_size,const std::vector<std::int32_t>& primes);
void expand_hashtable(HashtableState&,const std::vector<std::int32_t>& primes);
std::shared_ptr<HashtableState> make_hashtable(std::int32_t capacity,float load_factor,
                                             const std::vector<std::int32_t>& primes);
}
