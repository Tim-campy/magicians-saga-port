#pragma once
#include "hashtable_enumerator.hpp"
namespace saga::core {
struct HashtableHash { std::uint32_t seed, step; };
// GetHash remains the caller's virtual provider; length must be greater than 1.
HashtableHash hashtable_hash(std::uint32_t raw_hash, std::uint32_t length);
// Original callers supply a prime capacity with room for every live entry.
// Hashes are reused; this does not invoke key hash/equality callbacks.
void rehash_hashtable(HashtableState&, std::int32_t new_size);
}
