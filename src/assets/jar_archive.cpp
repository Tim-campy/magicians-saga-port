#include "assets/jar_archive.hpp"

namespace saga::assets {
namespace {

std::uint32_t be32(const std::uint8_t* data, std::size_t pos, std::size_t end) {
    if (pos > end || end - pos < 4U) throw JarError("Truncated u32 field");
    return (std::uint32_t{data[pos]} << 24U) |
           (std::uint32_t{data[pos + 1U]} << 16U) |
           (std::uint32_t{data[pos + 2U]} << 8U) | data[pos + 3U];
}

bool valid_utf8(const std::uint8_t* text, std::size_t length) {
    std::size_t i = 0;
    while (i < length) {
        const auto c = text[i++];
        if (c < 0x80U) continue;
        unsigned continuation = 0;
        std::uint32_t code = 0;
        std::uint32_t minimum = 0;
        if (c >= 0xc2U && c <= 0xdfU) { continuation = 1; code = c & 0x1fU; minimum = 0x80U; }
        else if (c >= 0xe0U && c <= 0xefU) { continuation = 2; code = c & 0x0fU; minimum = 0x800U; }
        else if (c >= 0xf0U && c <= 0xf4U) { continuation = 3; code = c & 7U; minimum = 0x10000U; }
        else return false;
        if (length - i < continuation) return false;
        for (unsigned j = 0; j < continuation; ++j) {
            const auto next = text[i++];
            if ((next & 0xc0U) != 0x80U) return false;
            code = (code << 6U) | (next & 0x3fU);
        }
        if (code < minimum || code > 0x10ffffU || (code >= 0xd800U && code <= 0xdfffU)) return false;
    }
    return true;
}
} // namespace

JarArchive parse_jar(const std::uint8_t* data, std::size_t size, JarLimits limits) {
    if (size > limits.max_bytes) throw JarError("Input exceeds byte limit");
    if (!data || size < 12U) throw JarError("Missing or truncated container header");
    const auto index_size = be32(data, 0, size);
    const auto body_size = be32(data, 4, size);
    const auto count = be32(data, 8, size);
    if (std::uint64_t{index_size} + body_size + 4U != size) throw JarError("Container size mismatch");
    if (index_size < 8U || index_size > size - 4U) throw JarError("Invalid index size");
    const std::size_t index_end = static_cast<std::size_t>(index_size) + 4U;
    if (count > limits.max_members) throw JarError("Member count exceeds limit");
    // Each member needs at least name length (4), offset (4), size (4), flag (1).
    if (count > (index_end - 12U) / 13U) throw JarError("Member count cannot fit index");

    JarArchive result{index_end, {}};
    result.members.reserve(count);
    std::size_t pos = 12U;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto length = be32(data, pos, index_end);
        pos += 4U;
        if (length > limits.max_name_bytes || length > index_end - pos) throw JarError("Name exceeds bounds/limit");
        if (!valid_utf8(data + pos, length)) throw JarError("Member name is not valid UTF-8");
        result.members.push_back({std::string_view{reinterpret_cast<const char*>(data + pos), length}, 0, 0, 0});
        pos += length;
    }
    if (std::uint64_t{count} * 9U != index_end - pos) throw JarError("Index table size mismatch");
    for (auto& member : result.members) { member.offset = be32(data, pos, index_end); pos += 4U; }
    for (auto& member : result.members) { member.size = be32(data, pos, index_end); pos += 4U; }
    for (std::uint32_t i = 0; i < count; ++i) {
        if (data[pos++] != 0U) throw JarError("Unsupported compressed member flag");
    }
    std::size_t cursor = 0;
    for (auto& member : result.members) {
        if (member.offset != cursor) throw JarError("Noncontiguous member offset");
        if (cursor > body_size || body_size - cursor < 4U) throw JarError("Member prefix exceeds data");
        const auto stored_size = be32(data, index_end + cursor, size);
        if (stored_size != member.size) throw JarError("Member length prefix mismatch");
        if (member.size > body_size - cursor - 4U) throw JarError("Member payload exceeds data");
        member.payload_offset = index_end + cursor + 4U;
        cursor += 4U + static_cast<std::size_t>(member.size);
    }
    if (cursor != body_size) throw JarError("Unconsumed data bytes");
    return result;
}
} // namespace saga::assets
