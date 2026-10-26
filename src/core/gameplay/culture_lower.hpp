#pragma once
#include <array>
#include <string>
#include <string_view>
namespace saga::core {
// TextInfoToLowerData static fields +0..+0x40, in field order.
// Caller-owned, exact-length arrays of little-endian uint16 from the user's APK.
using CultureLowerTables=std::array<std::string_view,9>;
char16_t lower_invariant_unit(char16_t,const CultureLowerTables&);
std::u16string lower_invariant_text(std::u16string_view,const CultureLowerTables&);
}
