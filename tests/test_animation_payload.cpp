#include "core/animation/animation_payload.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace saga::core;
void check(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
template<class Error, class F> void rejects(F action) {
    bool failed = false;
    try { action(); } catch (const Error&) { failed = true; }
    check(failed, "expected payload rejection");
}
int main() {
    check(encode_playback({1, -1}) == PlaybackPayload{0x01, 0xff, 0xff}, "producer 1/-1 bytes");
    check(encode_playback({-128, 0x1234}) == PlaybackPayload{0x80, 0x12, 0x34}, "byte sign and short order");
    check(encode_playback({256, 0x12345678}) == PlaybackPayload{0x00, 0x56, 0x78}, "serialization truncates widths");
    const auto low = decode_playback({0x80, 0x80, 0x00});
    check(low.rate == -128 && low.return_clip == -32768, "minimum signed values");
    const auto high = decode_playback({0x7f, 0x7f, 0xff});
    check(high.rate == 127 && high.return_clip == 32767, "maximum signed values");
    const auto wrap = decode_playback(encode_playback({std::numeric_limits<std::int32_t>::min(),
                                                      std::numeric_limits<std::int32_t>::max()}));
    check(wrap.rate == 0 && wrap.return_clip == -1, "full-width integers narrow deterministically");
    for (int i = 0; i < 256; ++i) {
        const auto byte = static_cast<std::uint8_t>(i);
        const auto d = decode_direction({byte, byte});
        check(d.direction == (i < 128 ? i : i - 256), "all signed directions");
        check(d.locked == (i == 1), "boolean accepts exactly one");
    }
    for (int i = 0; i < 65536; ++i) {
        const PlaybackPayload p{0xff, static_cast<std::uint8_t>(i >> 8), static_cast<std::uint8_t>(i)};
        const auto value = decode_playback(p);
        check(value.return_clip == (i < 32768 ? i : i - 65536), "all signed short patterns");
        check(encode_playback(value) == p, "all short byte patterns preserved");
    }
    check(encode_direction({-1, true}) == DirectionPayload{0xff, 1}, "direction encoding");
    const SpriteAnimation sprite{1, 0x1234, -128};
    check(encode_sprite_payload({sprite, std::nullopt}) ==
          std::vector<std::uint8_t>({1, 0x12, 0x34, 0x80, 0xff}), "null layers fixture");
    check(encode_sprite_payload({sprite, std::vector<std::int32_t>{}}) ==
          std::vector<std::uint8_t>({1, 0x12, 0x34, 0x80, 0}), "empty layers fixture");
    const std::vector<std::uint8_t> payload{1, 0x12, 0x34, 0x80, 3, 0xff, 0x80, 0x7f};
    check(encode_sprite_payload({sprite, std::vector<std::int32_t>{-1, -128, 127}}) == payload,
          "signed layers fixture");
    for (std::size_t size = 0; size < payload.size(); ++size) {
        const std::vector<std::uint8_t> prefix(payload.begin(), payload.begin() + static_cast<std::ptrdiff_t>(size));
        rejects<std::out_of_range>([&] { decode_sprite_payload(prefix); });
    }
    auto trailing = payload; trailing.push_back(0x42);
    const auto decoded = decode_sprite_payload(trailing);
    check(decoded.consumed == payload.size() && decoded.value.sprite.clip == 0x1234 &&
          decoded.value.sprite.frame == -128 && decoded.value.layers->at(0) == -1,
          "prefix consumption and signed fields");
    for (int count = 0; count <= 127; ++count) {
        std::vector<std::uint8_t> bytes(5 + static_cast<std::size_t>(count), 0xff);
        bytes[4] = static_cast<std::uint8_t>(count);
        const auto read = decode_sprite_payload(bytes);
        check(read.value.layers && read.value.layers->size() == static_cast<std::size_t>(count), "reader count range");
        check(read.consumed == bytes.size(), "reader exact count");
        if (count < 127) check(encode_sprite_payload(read.value) == bytes, "writable layer counts");
        else rejects<std::length_error>([&] { encode_sprite_payload(read.value); });
    }
    for (int count = 128; count < 255; ++count) {
        const std::vector<std::uint8_t> bytes{0, 0, 0, 0, static_cast<std::uint8_t>(count)};
        rejects<std::length_error>([&] { decode_sprite_payload(bytes); });
    }
    check(!decode_sprite_payload({0, 0, 0, 0, 255, 42}).value.layers, "null sentinel ignores trailing bytes");
    std::cout << "animation payload contracts passed\n";
}
