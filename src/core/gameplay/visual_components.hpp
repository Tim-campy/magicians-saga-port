#pragma once
#include "../ecs/component_slots.hpp"
#include "../animation/animation_transitions.hpp"
#include <array>
#include <memory>

namespace saga::core {
// Shared identity and mutable elements model managed int[] references. Callers
// must retain array lengths; resizing is outside the original array contract.
using ImageArray = std::shared_ptr<std::vector<std::int32_t>>;
struct ImageArrays {
    ImageArray textures; // ImageComponent +0x18.
    ImageArray resources; // +0x10; argument order is texture, then resource.
    ImageArray u, v, width, height; // +0x20/+0x28/+0x30/+0x38.
};
struct EntityImage final : Component { ImageArrays arrays; };
struct EntityPlayback final : Component { AnimationPlayback state{}; };
class ImagePool {
public:
    virtual ~ImagePool() = default;
    virtual EntityImage& acquire_image() = 0;
};
class PlaybackPool {
public:
    virtual ~PlaybackPool() = default;
    virtual EntityPlayback& acquire_playback() = 0;
};
// Entity.AddImage 0x112f720: acquire first; normalize/store each reference; add slot 7.
ComponentSlots& add_image_arrays(ComponentSlots&, ImageArrays, ImagePool&);
// Scalar overload 0x112f5d0 creates six distinct singleton arrays before acquisition.
ComponentSlots& add_image_component(ComponentSlots&, std::array<std::int32_t, 6>, ImagePool&);
// Two-array overload 0x112f9a0 forwards null U/V/W/H references.
ComponentSlots& add_image_component(ComponentSlots&, ImageArray textures, ImageArray resources, ImagePool&);
// Entity.AddAnimation 0x112c488: write both int32 fields before slot 10 insertion.
ComponentSlots& add_animation_component(ComponentSlots&, std::int32_t rate, std::int32_t backup, PlaybackPool&);
} // namespace saga::core
