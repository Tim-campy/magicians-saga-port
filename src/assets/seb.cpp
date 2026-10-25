#include "assets/seb.hpp"

namespace saga::assets {
namespace {
class Reader {
public:
    Reader(const std::uint8_t* data, std::size_t size) : data_(data), size_(size) {}
    std::uint8_t byte() {
        if (pos_ >= size_) throw SebError("Truncated SEB field");
        return data_[pos_++];
    }
    std::uint32_t word() {
        std::uint32_t result = 0;
        for (unsigned i = 0; i < 4; ++i) result = (result << 8U) | byte();
        return result;
    }
    std::int16_t short_value() {
        const std::uint32_t high = byte();
        const std::uint32_t raw = (high << 8U) | byte();
        const auto value = raw <= 32767U ? static_cast<std::int32_t>(raw)
                                        : static_cast<std::int32_t>(raw) - 65536;
        return static_cast<std::int16_t>(value);
    }
    std::array<std::int16_t, 2> pair() { return {short_value(), short_value()}; }
    std::size_t remaining() const { return size_ - pos_; }
private:
    const std::uint8_t* data_;
    std::size_t size_;
    std::size_t pos_ = 0;
};

struct Scan { std::uint8_t mode; std::int16_t header; std::uint32_t groups; std::uint32_t records; };
Scan walk(const std::uint8_t* data, std::size_t size, SebLimits limits, Seb* output) {
    Reader reader(data, size);
    const auto first = reader.byte();
    const unsigned mode = first >= 128U ? 256U - first : 0U;
    if (mode > 1U) throw SebError("Unsupported SEB mode");
    std::int32_t group_count = 0;
    if (mode == 1U) group_count = reader.short_value();
    else group_count = (std::int32_t{first} << 8U) | reader.byte();
    const auto header = reader.short_value();
    if (group_count < 0 || header < 0) throw SebError("Negative SEB header allocation count");
    const auto groups = static_cast<std::uint32_t>(group_count);
    if (groups > limits.max_groups) throw SebError("SEB group limit exceeded");
    if (groups > reader.remaining() / (mode == 0U ? 4U : 2U)) throw SebError("Truncated SEB groups");
    std::uint32_t total_records = 0;
    for (std::uint32_t i = 0; i < groups; ++i) {
        const auto count = reader.short_value();
        if (count < 0) throw SebError("Negative SEB record count");
        std::optional<std::int16_t> legacy;
        if (mode == 0U) legacy = reader.short_value();
        const auto n = static_cast<std::uint16_t>(count);
        if (n > limits.max_records - total_records) throw SebError("SEB record limit exceeded");
        if (n > reader.remaining() / (mode == 0U ? 20U : 13U)) throw SebError("Truncated SEB records");
        if (output) output->groups.push_back({total_records, n, legacy});
        total_records += n;
        for (std::uint16_t j = 0; j < n; ++j) {
            SebRecord record;
            record.value_count = mode == 0U ? 10U : 4U;
            for (std::uint8_t k = 0; k < record.value_count; ++k) record.values[k] = reader.short_value();
            if (mode == 1U) {
                record.control = reader.byte();
                const auto d = record.values[1];
                if ((d >= -6 && d <= -2) || (d >= 0 && *record.control != 0))
                    record.extra_word = reader.word();
                if (d >= 0 || d == -8) record.pair = reader.pair();
                record.tail_pair = reader.pair();
                if (d >= 0) record.flags = reader.byte();
            }
            if (output) output->records.push_back(record);
        }
    }
    if (reader.remaining() != 0U) throw SebError("Unconsumed SEB bytes");
    return {static_cast<std::uint8_t>(mode), header, groups, total_records};
}
} // namespace

Seb parse_seb(const std::uint8_t* data, std::size_t size, SebLimits limits) {
    if (size > limits.max_bytes) throw SebError("SEB input exceeds byte limit");
    if (!data || size == 0U) throw SebError("Missing SEB header");
    const auto scan = walk(data, size, limits, nullptr);
    Seb result{scan.mode, scan.header, {}, {}};
    result.groups.reserve(scan.groups);
    result.records.reserve(scan.records);
    (void)walk(data, size, limits, &result);
    return result;
}
} // namespace saga::assets
