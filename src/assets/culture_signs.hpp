#pragma once
#include <cstddef>
#include <string_view>
#include <optional>
#include <cstdint>
namespace saga::assets {
struct CultureSignText {
    std::string_view positive,negative; // UTF-8 views into caller-owned pool; no decoding.
};
// Native 0x10367cc record expansion and GetNFIValues sign selection.
CultureSignText read_culture_signs(std::string_view records,std::string_view pool,std::size_t index);
// Exact UTF-8 key, sorted 4-byte name index, 56-byte culture records and shared pool.
std::optional<std::size_t> find_culture_number_index(std::string_view key,std::string_view names,
    std::string_view cultures,std::string_view pool);
std::optional<std::size_t> find_culture_record_by_name(std::string_view key,std::string_view names,
    std::string_view cultures,std::string_view pool);
// Native low-16-bit signed comparison; AOSP bsearch midpoint tie selection.
// The managed constructor's negative-ID rejection is a separate caller gate.
std::optional<std::size_t> find_culture_record_by_lcid(std::int32_t lcid,std::string_view cultures);
}
