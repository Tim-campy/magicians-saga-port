#pragma once
#include "animation_transitions.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace saga::core {
// Fixed payloads only, not the save envelope or StreamUtil's incremental I/O.
// Exact-sized arrays make truncated input unrepresentable at this boundary.
using PlaybackPayload = std::array<std::uint8_t, 3>;
using DirectionPayload = std::array<std::uint8_t, 2>;
struct DirectionState { std::int32_t direction; bool locked; };
PlaybackPayload encode_playback(const AnimationPlayback& value);
AnimationPlayback decode_playback(const PlaybackPayload& bytes);
DirectionPayload encode_direction(const DirectionState& value);
DirectionState decode_direction(const DirectionPayload& bytes);

struct SpritePayloadState {
    SpriteAnimation sprite;
    std::optional<std::vector<std::int32_t>> layers;
};
struct DecodedSpritePayload {
    SpritePayloadState value;
    std::size_t consumed;
};
std::vector<std::uint8_t> encode_sprite_payload(const SpritePayloadState& value);
// Finite-buffer prefix decoder. Reports bytes consumed and leaves trailing data
// untouched. Throws on truncation/invalid negative length; does not emulate I/O.
DecodedSpritePayload decode_sprite_payload(const std::vector<std::uint8_t>& bytes);
}
