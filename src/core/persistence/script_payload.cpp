#include "script_payload.hpp"
#include <limits>
#include <stdexcept>
#include <utility>

namespace saga::core {
namespace {
void append(std::vector<std::uint8_t>& out, std::int32_t value) {
    const auto bits = static_cast<std::uint32_t>(value);
    for (unsigned shift : {24U, 16U, 8U, 0U})
        out.push_back(static_cast<std::uint8_t>((bits >> shift) & 0xffU));
}
std::int32_t count(std::size_t size) {
    if (size > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("script count exceeds signed 32-bit range");
    return static_cast<std::int32_t>(size);
}
std::int32_t read(const std::vector<std::uint8_t>& bytes, std::size_t& pos) {
    if (bytes.size() - pos < 4) throw std::out_of_range("truncated script integer");
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i) value = (value << 8U) | bytes[pos++];
    // Avoid implementation-defined conversion from unsigned values above INT_MAX.
    const auto signed_value = value < 0x80000000U ? static_cast<std::int64_t>(value)
        : static_cast<std::int64_t>(value) - 0x100000000LL;
    return static_cast<std::int32_t>(signed_value);
}
}
std::vector<std::uint8_t> encode_script_payload(const ScriptRows& rows) {
    std::vector<std::uint8_t> out;
    append(out, count(rows.size()));
    for (const auto& row : rows) {
        append(out, count(row.size()));
        for (const auto value : row) append(out, value);
    }
    return out;
}
DecodedScriptPayload decode_script_payload(const std::vector<std::uint8_t>& bytes) {
    DecodedScriptPayload out{{}, 0};
    const auto total = read(bytes, out.consumed);
    if (total <= 0) return out;
    // Bound allocations by the supplied buffer, including at least each header.
    if (static_cast<std::size_t>(total) > (bytes.size() - out.consumed) / 4)
        throw std::out_of_range("truncated script row headers");
    out.rows.reserve(static_cast<std::size_t>(total));
    for (std::int32_t i = 0; i < total; ++i) {
        const auto length = read(bytes, out.consumed);
        if (length < 0) throw std::length_error("negative script row length");
        if (static_cast<std::size_t>(length) > (bytes.size() - out.consumed) / 4)
            throw std::out_of_range("truncated script row");
        std::vector<std::int32_t> row;
        row.reserve(static_cast<std::size_t>(length));
        for (std::int32_t j = 0; j < length; ++j) row.push_back(read(bytes, out.consumed));
        out.rows.push_back(std::move(row));
    }
    return out;
}
}
