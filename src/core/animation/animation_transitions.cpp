#include "core/animation/animation_transitions.hpp"

#include <limits>

namespace saga::core {
namespace {
// Preserve ARM64 W-register wrap without signed overflow or implementation-
// defined uint32->int32 conversion. No dependency on host endianness.
std::int32_t signed_word(std::uint32_t bits) noexcept {
    constexpr auto max = std::numeric_limits<std::int32_t>::max();
    if (bits <= static_cast<std::uint32_t>(max)) {
        return static_cast<std::int32_t>(bits);
    }
    return -1 - static_cast<std::int32_t>(~bits);
}

std::int32_t add_word(std::int32_t a, std::int32_t b) noexcept {
    return signed_word(static_cast<std::uint32_t>(a) + static_cast<std::uint32_t>(b));
}

template<class T>
T& required(T* value, const char* name) {
    if (!value) throw MissingAnimationComponent(name);
    return *value;
}

SpriteAnimation& sprite(EntityAnimationView& e) {
    return required(e.sprite, "seb");
}
AnimationPlayback& playback(EntityAnimationView& e) {
    return required(e.playback, "animation");
}
EntityAnimationView view(EntityAnimation& e) {
    return {e.sprite ? &*e.sprite : nullptr, e.playback ? &*e.playback : nullptr,
            e.direction ? &*e.direction : nullptr, e.human, e.monster, e.enemy};
}
} // namespace

// ELF 0x11420e0, AnimationSystem.ChangeAnimation37436.
void change_animation(EntityAnimationView e, std::int32_t clip, bool reset_frame) {
    if (e.human || e.monster) {
        clip = add_word(required(e.direction, "direction"), clip);
    }
    sprite(e).clip = clip;
    if (reset_frame) sprite(e).frame = 0;
}

// ELF 0x1142168, AnimationSystem.CutInAnimation37437.
void cut_in_animation(EntityAnimationView e, std::int32_t clip, std::int32_t return_clip) {
    if (!e.playback) return;
    playback(e).return_clip = return_clip;
    // Deliberately enemy, not monster: the native predicates differ.
    if (e.direction && (e.human || e.enemy)) {
        clip = add_word(*e.direction, clip);
    }
    sprite(e).clip = clip;
    sprite(e).frame = 0;
}

// ELF 0x1141f40, AnimationSystem.RestoreAnimation37438.
void restore_animation(EntityAnimationView e) {
    const auto restore = playback(e).return_clip;
    if (restore == -1) return;
    sprite(e).clip = restore;
    sprite(e).frame = 0;
    if (e.human || e.monster) {
        const auto clip = sprite(e).clip;
        // Native ADD(+3)/CSEL(signed<0)/AND(~3), then directional ADD.
        const auto adjusted = clip < 0 ? add_word(clip, 3) : clip;
        const auto group = signed_word(static_cast<std::uint32_t>(adjusted) & ~std::uint32_t{3});
        sprite(e).clip = add_word(required(e.direction, "direction"), group);
    }
    playback(e).return_clip = -1;
}

void change_animation(EntityAnimation& e, std::int32_t clip, bool reset_frame) {
    change_animation(view(e), clip, reset_frame);
}
void cut_in_animation(EntityAnimation& e, std::int32_t clip, std::int32_t return_clip) {
    cut_in_animation(view(e), clip, return_clip);
}
void restore_animation(EntityAnimation& e) { restore_animation(view(e)); }

} // namespace saga::core
