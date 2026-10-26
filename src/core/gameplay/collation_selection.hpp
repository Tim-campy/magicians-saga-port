#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
namespace saga::core {
struct CollationSelection {bool initialized=false,use_managed=false;};
class CollationHost {
public:
    virtual ~CollationHost() = default;
    virtual std::optional<std::u16string> disable_managed_environment() = 0;
    // MSCompatUnicodeTable readiness after resource loading, not just resource presence.
    virtual bool managed_tables_ready() = 0;
    virtual bool managed_prefix(std::u16string_view,std::u16string_view,std::int32_t options) = 0;
    virtual std::int32_t compare_slices(std::u16string_view,std::u16string_view,std::int32_t options) = 0;
};
bool use_managed_collation(CollationSelection&,CollationHost&);
// Internal CompareInfo prefix route; public null/options/empty checks precede it.
bool cultural_prefix(CollationSelection&,CollationHost&,std::u16string_view source,
                     std::u16string_view prefix,std::int32_t options);
}
