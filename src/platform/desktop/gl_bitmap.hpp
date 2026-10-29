#pragma once
#include "bitmap_backend.hpp"
#include "decoded_image.hpp"
#include "core/gameplay/mesh_submission.hpp"
#include <SDL3/SDL.h>
#include <functional>
#include <string>

namespace saga::desktop {
enum class GlBitmapWrap {clamp,repeat};
enum class GlBitmapClear {color_and_depth,depth,preserve};
struct GlBitmapTexture {const DecodedImage* pixels;GlBitmapWrap wrap_x,wrap_y;};
// Set the explicit desktop GL3.3 core profile before creating an SDL_WINDOW_OPENGL window.
void configure_gl_bitmap_context();
class GlBitmapRenderer {
public:
    using Textures=std::function<GlBitmapTexture(std::size_t)>;
    // Window and immutable source images outlive this renderer. It owns its GL context,
    // RGBA8/D24 offscreen target and uploaded nearest-sampled textures.
    GlBitmapRenderer(SDL_Window&,std::int32_t width,std::int32_t height,Textures);
    ~GlBitmapRenderer();
    GlBitmapRenderer(const GlBitmapRenderer&)=delete;
    GlBitmapRenderer& operator=(const GlBitmapRenderer&)=delete;
    // Column-major world -> OpenGL clip matrix is an explicit host input; no camera is inferred.
    void begin_frame(GlBitmapClear,core::RenderColor background,const core::OffscreenMatrix& view_projection);
    void set_viewport(std::int32_t x,std::int32_t y,std::int32_t width,std::int32_t height); // Bottom-left pixels.
    void clear_depth(); // Active viewport only, preserving color, regardless of the preceding ZWrite state.
    void set_material(std::uint32_t shader_key);
    void draw(const BitmapQuad&,const core::OffscreenMatrix& model);
    DecodedImage read_pixels(); // Top-left RGBA8, explicit synchronous capture only.
    void present();
    std::string driver()const;
private:
    struct Impl;std::unique_ptr<Impl> impl_;
};
// Ready builder/material identity is supplied by the mission owner, not inferred from a texture sort.
struct GlMeshQuads {const BitmapQuad* data=nullptr;std::size_t count=0;};
struct GlMeshBindings {
    std::map<const core::SubmissionMaterial*,std::uint32_t> materials;
    std::map<const core::SubmissionGroup*,GlMeshQuads> groups;
};
class GlMeshSubmissionHost:public core::MeshSubmissionHost {
public:
    using Diagnostics=std::function<void(core::SubmissionMaterial&,core::SubmissionGroup&)>;
    GlMeshSubmissionHost(GlBitmapRenderer&,const GlMeshBindings&,Diagnostics,std::function<std::int32_t()> check_count);
    void clear_depth()override;
    void set_material_pass(core::SubmissionMaterial&)override;
    void draw_mesh(core::SubmissionGroup&,core::OffscreenMatrix)override;
    void after_mesh_draw(core::SubmissionMaterial&,core::SubmissionGroup&)override;
    std::int32_t check_count()override;
private:
    GlBitmapRenderer& renderer_;const GlMeshBindings& bindings_;Diagnostics diagnostics_;std::function<std::int32_t()> check_count_;
};
} // namespace saga::desktop
