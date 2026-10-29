#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace saga::core {
// Value representation at the public graphics boundary, column-major.
struct OffscreenMatrix {std::array<float,16> values{};};
using OffscreenMatrices=std::shared_ptr<std::vector<OffscreenMatrix>>;
struct OffscreenTransforms {
    std::int32_t effective_density=0; // Offscreen.useDensity_, +0x12c; not requested density_ +0x128.
    OffscreenMatrices statics; // +0x48. Constructor/lifetime belongs to the surface owner.
    OffscreenMatrix static_base,dynamic; // +0x50/+0x90.
};
class OffscreenTransformHost {
public:
    virtual ~OffscreenTransformHost()=default;
    // Public TRS with identity rotation. Do not reconstruct Unity's math internals.
    virtual OffscreenMatrix translation_scale(float x,float y,float z,float scale_x,float scale_y,float scale_z)=0;
    virtual OffscreenMatrices allocate_statics(std::int32_t count)=0; // Exact count, or throw; no zero/negative repair.
};
// The five-argument overloads accept binary32 coordinates AND depth.
void set_static_offscreen_transform(OffscreenTransforms&,float x,float y,float depth,float scale_x,float scale_y,OffscreenTransformHost&); // 0x1ca9b08.
void set_static_offscreen_transform(OffscreenTransforms&,OffscreenMatrix); // 0x1ca9bd8.
// This overload uses int32 coordinates/depth: multiply/negate with wrapping BEFORE float conversion.
void set_multi_static_offscreen_transform(OffscreenTransforms&,std::int32_t id,std::int32_t x,std::int32_t y,std::int32_t depth,float scale_x,float scale_y,OffscreenTransformHost&); // 0x1ca9c5c.
void set_multi_static_offscreen_transform(OffscreenTransforms&,std::int32_t id,OffscreenMatrix,OffscreenTransformHost&); // 0x1ca9d38.
void set_dynamic_offscreen_transform(OffscreenTransforms&,float x,float y,float depth,float scale_x,float scale_y,OffscreenTransformHost&); // 0x1ca9e6c.
void set_dynamic_offscreen_transform(OffscreenTransforms&,OffscreenMatrix); // 0x1ca9f38.

struct OffscreenCameraPlacement {float half_height,x,y,z;};
// _resize's camera arithmetic only (0x1ca99d4..0x1ca9a68), after image creation.
// Target allocation, camera queries/setters and the rest of _resize remain unbound.
OffscreenCameraPlacement offscreen_camera_placement(std::int32_t image_width,std::int32_t image_height,float previous_z);
} // namespace saga::core
