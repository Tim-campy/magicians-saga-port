#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace saga::core {
struct RenderColor {float r=0,g=0,b=0,a=0;};
using RenderColors=std::shared_ptr<std::vector<RenderColor>>;
using ColorModeRow=std::shared_ptr<std::vector<std::int32_t>>;
struct RenderColorState {
    std::int32_t mode=0,source=255,destination=0;bool replace=true;
    std::int32_t blend_mode=0,blend_color=0;RenderColor blend{};
    std::int32_t render_count=0,blend_count=0;
    std::shared_ptr<std::vector<ColorModeRow>> renders=std::make_shared<std::vector<ColorModeRow>>();
    std::shared_ptr<std::vector<ColorModeRow>> blends=std::make_shared<std::vector<ColorModeRow>>();
    bool* cache_dirty=nullptr; // Required original RenderCache.Reset, after field writes.
    bool is_gui=false,gl_render=false;std::uint8_t figure_clip=0;
    const void* forced_material=nullptr; // Typed material backend remains external.
};
void set_render_mode(RenderColorState&,std::int32_t mode,std::int32_t source,std::int32_t destination);
void save_render_mode(RenderColorState&);
void push_render_mode(RenderColorState&,std::int32_t mode,std::int32_t source,std::int32_t destination);
void pop_render_mode(RenderColorState&);
void set_blend_mode(RenderColorState&,std::int32_t mode,std::int32_t packed);
void save_blend_mode(RenderColorState&);
void push_blend_mode(RenderColorState&,std::int32_t mode,std::int32_t packed);
void pop_blend_mode(RenderColorState&);

struct SpriteRenderPass {
    RenderColorState* owner=nullptr;
    RenderColors local=std::make_shared<std::vector<RenderColor>>(1); // Private singleton work.
    std::int32_t mode=0,source=0,destination=0;
    RenderColors colors;const void* material=nullptr;
    std::int32_t alpha_test=0;bool restore_blend=false;
    std::int32_t rgb_factor=0,alpha_factor=0;RenderColors output;
};
using SpriteRenderPasses=std::shared_ptr<std::vector<std::shared_ptr<SpriteRenderPass>>>;
struct RenderColorWork {
    RenderColors white=std::make_shared<std::vector<RenderColor>>(1,RenderColor{1,1,1,1});
    RenderColors empty_fallback=std::make_shared<std::vector<RenderColor>>(1,RenderColor{1,1,1,1});
    std::map<std::size_t,RenderColors> outputs; // Retained shared output per color-array length.
    std::shared_ptr<std::vector<SpriteRenderPasses>> passes; // Lazy cache of arrays with 0..4 passes.
};
void initialize_render_pass(SpriteRenderPass&,RenderColorState*,std::int32_t mode,std::int32_t source,
                            std::int32_t destination,std::int32_t rgb_factor,std::int32_t alpha_factor,
                            RenderColors,RenderColorWork&);
void initialize_material_pass(SpriteRenderPass&,RenderColorState*,const void* material,RenderColors,RenderColorWork&);
void copy_render_pass(SpriteRenderPass&,const SpriteRenderPass*);
class RenderColorHost {
public:
    virtual ~RenderColorHost()=default;
    virtual std::int32_t image_alpha_type()=0; // Optional image: caller invokes only when present.
    virtual bool render_gui_mode(RenderColorState&)=0;
    virtual void push_gui_matrix()=0;
    virtual void load_gui_pixel_matrix()=0;
    virtual std::int32_t screen_width()=0;
    virtual std::int32_t screen_height()=0;
    virtual void gui_viewport(float x,float y,float width,float height)=0;
    virtual void pop_gui_matrix()=0;
};
SpriteRenderPasses get_render_passes(RenderColorState&,bool has_image,RenderColors,RenderColorWork&,RenderColorHost&);
void begin_render_pass_state(SpriteRenderPass&,RenderColorHost&);
RenderColors begin_render_pass(SpriteRenderPass&,RenderColorHost&);
void end_render_pass(SpriteRenderPass&,RenderColorHost&);

// Image._getAlphaType's scalar classification from sampled alpha values. The
// native texture tile/readback/liveness services are outside this pure kernel.
class ImageAlphaClassifier {
public:
    void add(float alpha);
    std::int32_t finish(std::int32_t pixel_count,std::int32_t previous=0)const;
private:
    std::int32_t transparent_=0,partial_=0;
};
struct SpriteShaderState {
    bool shape=false,stencil=false,invert_stencil=false,has_image=false,color_blend=false,gui=false,replace=false;
    bool stencil_write=false,color_mask=false,depth_only=false;
    std::int32_t mode=0,source=255,destination=0,alpha_type=-1,alpha_test=-1;
};
// ShaderBuilder.Init's reached textured-sprite contract; private shader source
// generation and Unity material caches are deliberately not reconstructed.
SpriteShaderState sprite_shader_state(const RenderColorState&,bool has_image,std::int32_t image_alpha,
                                      float first_alpha,RenderColors,std::int32_t alpha_test,bool gui);
std::uint32_t sprite_shader_key(const SpriteShaderState&); // ShaderBuilder.GetNameId.
enum class SpriteBlend {replace,alpha,add,subtract_alpha,subtract_add,attenuate};
enum class SpriteAlphaTest {none,at_least_001,greater_099,at_most_099};
struct SpritePixelPipeline {SpriteBlend blend=SpriteBlend::replace;SpriteAlphaTest alpha_test=SpriteAlphaTest::none;bool modulate=true;};
SpritePixelPipeline sprite_pixel_pipeline(std::uint32_t shader_key);
bool accepts_sprite_alpha(SpriteAlphaTest,float alpha);
} // namespace saga::core
