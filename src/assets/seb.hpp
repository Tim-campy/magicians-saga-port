#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <vector>

namespace saga::assets {
struct SebLimits {
    std::size_t max_bytes = 32U * 1024U * 1024U;
    std::uint32_t max_groups = 32767U;
    std::uint32_t max_records = 1000000U;
};
struct SebRecord {
    std::array<std::int16_t, 10> values{};
    std::uint8_t value_count = 0; // 10 for mode 0; 4 for mode 1.
    std::optional<std::uint8_t> control;
    std::optional<std::uint32_t> extra_word;
    std::optional<std::array<std::int16_t, 2>> pair;
    std::optional<std::array<std::int16_t, 2>> tail_pair;
    std::optional<std::uint8_t> flags;
};
struct SebGroup {
    std::uint32_t first_record;
    std::uint16_t record_count;
    std::optional<std::int16_t> legacy_value;
};
struct Seb {
    std::uint8_t mode;
    std::int16_t header_value; // Runtime Seb.maxFrame +0x1c, in both supported disk modes.
    std::vector<SebGroup> groups;
    std::vector<SebRecord> records;
};
class SebError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
// Seb._load VA 0x1baf5a0, docs/13 and tools/inspect_seb.py.
// Disk modes 0/1 only; no compact runtime representation or animation semantics.
// Input is immutable for the call; output owns all scalar storage.
Seb parse_seb(const std::uint8_t* data, std::size_t size, SebLimits limits = {});
} // namespace saga::assets
