#pragma once
#include <cstddef>
#include <string_view>
namespace saga::assets {
struct CultureSignText {
    std::string_view positive,negative; // UTF-8 views into caller-owned pool; no decoding.
};
// Native 0x10367cc record expansion and GetNFIValues sign selection.
CultureSignText read_culture_signs(std::string_view records,std::string_view pool,std::size_t index);
}
