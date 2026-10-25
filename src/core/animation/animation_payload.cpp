#include "animation_payload.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
std::int32_t signed_byte(std::uint8_t value) {
    const auto word = static_cast<std::int32_t>(value);
    return word < 128 ? word : word - 256;
}
std::int32_t signed_short(std::uint8_t high, std::uint8_t low) {
    const auto word = static_cast<std::int32_t>(high) * 256 + low;
    return word < 32768 ? word : word - 65536;
}
}
// Serialize37261 / Deserialize37262, confirmed big-endian short (report43).
PlaybackPayload encode_playback(const AnimationPlayback& value) {
    const auto clip = static_cast<std::uint32_t>(value.return_clip);
    return {static_cast<std::uint8_t>(value.rate),
            static_cast<std::uint8_t>(clip >> 8), static_cast<std::uint8_t>(clip)};
}
AnimationPlayback decode_playback(const PlaybackPayload& bytes) {
    return {signed_byte(bytes[0]), signed_short(bytes[1], bytes[2])};
}
// Serialize37296 / Deserialize37297. Only byte 1 represents true on read.
DirectionPayload encode_direction(const DirectionState& value) {
    return {static_cast<std::uint8_t>(value.direction),
            static_cast<std::uint8_t>(value.locked ? 1 : 0)};
}
DirectionState decode_direction(const DirectionPayload& bytes) {
    return {signed_byte(bytes[0]), bytes[1] == 1};
}

std::vector<std::uint8_t> encode_sprite_payload(const SpritePayloadState& value) {
    if (value.layers && value.layers->size() >= 127)
        throw std::length_error("sprite layer count must be less than 127");
    const auto clip = static_cast<std::uint32_t>(value.sprite.clip);
    std::vector<std::uint8_t> bytes;
    bytes.reserve(5 + (value.layers ? value.layers->size() : 0));
    bytes.push_back(static_cast<std::uint8_t>(value.sprite.resource));
    bytes.push_back(static_cast<std::uint8_t>(clip >> 8));
    bytes.push_back(static_cast<std::uint8_t>(clip));
    bytes.push_back(static_cast<std::uint8_t>(value.sprite.frame));
    bytes.push_back(value.layers ? static_cast<std::uint8_t>(value.layers->size()) : std::uint8_t{255});
    if (value.layers)
        for (const auto layer : *value.layers) bytes.push_back(static_cast<std::uint8_t>(layer));
    return bytes;
}

DecodedSpritePayload decode_sprite_payload(const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < 5) throw std::out_of_range("truncated sprite payload header");
    DecodedSpritePayload result{{{signed_byte(bytes[0]), signed_short(bytes[1], bytes[2]),
                                  signed_byte(bytes[3])}, std::nullopt}, 5};
    const auto count = signed_byte(bytes[4]);
    if (count == -1) return result;
    if (count < 0) throw std::length_error("negative sprite layer count");
    const auto size = static_cast<std::size_t>(count);
    if (bytes.size() - 5 < size) throw std::out_of_range("truncated sprite layers");
    result.value.layers.emplace();
    result.value.layers->reserve(size);
    for (std::size_t index = 0; index < size; ++index)
        result.value.layers->push_back(signed_byte(bytes[5 + index]));
    result.consumed += size;
    return result;
}
}
