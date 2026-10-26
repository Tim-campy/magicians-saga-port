#pragma once
#include "hashtable_capacity.hpp"
#include <string_view>
namespace saga::core {
std::uint32_t hashtable_string_hash(std::u16string_view);
class HashtableKeyHost {
public:
    virtual ~HashtableKeyHost() = default;
    virtual std::uint32_t hash(AsyncValue key) = 0;
    virtual bool equals(AsyncValue stored,AsyncValue key) = 0;
    // Original duplicate error formats both keys; preserve any resulting side effects.
    virtual void duplicate(AsyncValue stored,AsyncValue key) = 0;
};
// Base Hashtable semantics (no virtual overrides of KeyEquals/Insert).
void insert_hashtable(HashtableState&,AsyncValue key,AsyncValue value,bool add,
                      const std::vector<std::int32_t>& primes,HashtableKeyHost&);
// All keys must be shared_ptr<u16string>; other managed key types need a checked bridge.
class StringCallDataStore : public CallDataStore {
public:
    StringCallDataStore(std::shared_ptr<HashtableState>,std::shared_ptr<const std::vector<std::int32_t>>);
    std::int32_t count() override;
    std::shared_ptr<CallDataEnumerator> enumerate() override;
    void set(AsyncValue,AsyncValue) override;
private:
    std::shared_ptr<HashtableState> state_;
    std::shared_ptr<const std::vector<std::int32_t>> primes_;
};
}
