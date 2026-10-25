#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace saga::assets {

struct JarLimits {
    std::size_t max_bytes = 128U * 1024U * 1024U;
    std::uint32_t max_members = 100000U;
    std::uint32_t max_name_bytes = 65536U;
};

struct JarMember {
    std::string_view name; // UTF-8; references the caller's immutable input.
    std::uint32_t offset; // Relative to data section; points at u32 length prefix.
    std::uint32_t size;
    std::size_t payload_offset; // Absolute input offset, after length prefix.
};

struct JarArchive {
    std::size_t data_offset;
    std::vector<JarMember> members;
};

class JarError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Layout evidenced by JarInflater._load, ARM64 VA 0x1bfb5d0; see docs/12.
// Input must remain alive and unchanged while any returned name view is used.
// Supported profile: decoded, contiguous members, flags == 0. No filesystem I/O.
JarArchive parse_jar(const std::uint8_t* data, std::size_t size,
                     JarLimits limits = {});

} // namespace saga::assets
