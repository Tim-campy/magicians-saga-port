#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>

namespace saga::core {

// Field meanings recovered in docs/archive/reports/30-animation-transitions.md.
// Aggregates intentionally have no game-default initializers: spawn/load decides.
struct SpriteAnimation {
    std::int32_t resource;  // SebComponent +0x10
    std::int32_t clip;      // +0x14
    std::int32_t frame;     // +0x18
};

struct AnimationPlayback {
    std::int32_t rate;         // AnimationComponent +0x10
    std::int32_t return_clip;  // +0x14; -1 means no pending restoration
};

struct EntityAnimation {
    std::optional<SpriteAnimation> sprite;
    std::optional<AnimationPlayback> playback;
    std::optional<std::int32_t> direction;
    bool human;
    bool monster;
    bool enemy;
};

// Invalid ECS state is explicit. Mutations preceding the missing component are
// retained, as in the observed native control flow. This is not an IL2CPP ABI.
class MissingAnimationComponent : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

void change_animation(EntityAnimation& entity, std::int32_t clip, bool reset_frame);
void cut_in_animation(EntityAnimation& entity, std::int32_t clip, std::int32_t return_clip);
void restore_animation(EntityAnimation& entity);

} // namespace saga::core
