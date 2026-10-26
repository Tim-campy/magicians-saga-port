#pragma once
#include "call_context.hpp"
#include <variant>
namespace saga::core {
struct HashtableBucket {
    AsyncValue key, value;
    // Projection of key == the containing native bucket-array object.
    bool tombstone = false;
    std::uint32_t hash_collision = 0; // native record +0x10
};
struct HashtableState {
    std::shared_ptr<std::vector<HashtableBucket>> buckets;
    std::uint32_t version = 0;
    std::uint32_t occupancy = 0;
    std::int32_t count = 0, load_size = 0;
    float load_factor = 0.0F; // Caller supplies the recovered constructor state.
    bool writer_in_progress = false;
};
struct HashtableEntry { AsyncValue key, value; };
class HashtableEnumerator : public CallDataEnumerator {
public:
    explicit HashtableEnumerator(std::shared_ptr<HashtableState>, std::int32_t mode=3);
    bool next() override;
    AsyncValue key() override;
    AsyncValue value() override;
    HashtableEntry entry() const;
    std::variant<AsyncValue,HashtableEntry> current() const;
    void reset();
    // Original Clone is MemberwiseClone: cursor/cache copied, table shared.
    std::shared_ptr<HashtableEnumerator> clone() const;
private:
    void check_version() const;
    void check_current() const;
    std::shared_ptr<HashtableState> table_;
    std::int32_t bucket_=0,mode_=3;
    std::uint32_t version_=0;
    bool valid_=false;
    HashtableEntry cached_;
};
}
