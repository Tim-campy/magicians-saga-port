#pragma once
#include <cstdint>
#include <vector>

namespace saga::desktop {
struct DecodedImage {
    std::int32_t width=0,height=0;
    std::vector<std::uint8_t> rgba;
};
// Public stb_image PNG decoder, compiled by the optional viewer target.
// Explicit desktop profile: RGBA8, <=8192/axis, <=16 Mi pixels, <=64 MiB encoded.
DecodedImage decode_png(const std::vector<std::uint8_t>&);
} // namespace saga::desktop
