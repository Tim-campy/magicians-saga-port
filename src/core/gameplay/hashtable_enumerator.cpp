#include "hashtable_enumerator.hpp"
#include <limits>
namespace saga::core {
namespace {
std::int32_t length(const std::shared_ptr<std::vector<HashtableBucket>>& buckets) {
    if (!buckets) throw CultureManagedException("null Hashtable buckets");
    if (buckets->size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("Hashtable bucket count exceeds int32");
    return static_cast<std::int32_t>(buckets->size());
}
}
// HashtableEnumerator..ctor 0x1e3d62c; Hashtable.GetEnumerator passes mode 3.
HashtableEnumerator::HashtableEnumerator(std::shared_ptr<HashtableState> table,std::int32_t mode):table_(std::move(table)),mode_(mode) {
    if (!table_) throw CultureManagedException("null Hashtable");
    bucket_=length(table_->buckets);
    version_=table_->version;
}
void HashtableEnumerator::check_version() const {
    if (version_!=table_->version) throw CultureManagedException("Hashtable enumeration version changed");
}
void HashtableEnumerator::check_current() const {
    if (!valid_) throw CultureManagedException("Hashtable enumeration has no current entry");
}
// HashtableEnumerator.MoveNext 0x1e3f98c.
bool HashtableEnumerator::next() {
    check_version();
    while (bucket_>0) {
        --bucket_;
        const auto buckets=table_->buckets;
        if (!buckets) throw CultureManagedException("null Hashtable buckets");
        if (static_cast<std::size_t>(bucket_)>=buckets->size())
            throw CultureManagedException("Hashtable bucket outside array");
        const auto& slot=(*buckets)[static_cast<std::size_t>(bucket_)];
        if (slot.key&&!slot.tombstone) {
            cached_={slot.key,slot.value};valid_=true;return true;
        }
    }
    valid_=false; // Exhaustion retains cached references, unlike Reset.
    return false;
}
// Key 0x1e3f92c, Value 0x1e3fbe0 and Entry 0x1e3faac do not check version.
AsyncValue HashtableEnumerator::key() {check_current();return cached_.key;}
AsyncValue HashtableEnumerator::value() {check_current();return cached_.value;}
HashtableEntry HashtableEnumerator::entry() const {check_current();return cached_;}
// Current 0x1e3fb10: modes 1/2 return key/value, all other modes return an entry.
std::variant<AsyncValue,HashtableEntry> HashtableEnumerator::current() const {
    check_current();
    if (mode_==1) return cached_.key;
    if (mode_==2) return cached_.value;
    return cached_;
}
// Reset 0x1e3fc40 checks version before changing state.
void HashtableEnumerator::reset() {
    check_version();valid_=false;
    const auto count=length(table_->buckets);
    cached_={};bucket_=count;
}
// Clone 0x1e3f924.
std::shared_ptr<HashtableEnumerator> HashtableEnumerator::clone() const {
    return std::make_shared<HashtableEnumerator>(*this);
}
}
