#pragma once
#include "render_queue.hpp"
#include "quest_camera.hpp"
#include "animation_resources.hpp"
#include "enemy_defeat.hpp"

namespace saga::core {
struct RenderSystemState {MapComponent* map=nullptr;EventInstruction layers;}; // +0x28/+0x30.
class RenderInitializationWorld {
public:
    virtual ~RenderInitializationWorld()=default;
    virtual MapComponent* render_map()=0;
};
void initialize_render(RenderSystemState&,RenderInitializationWorld*); // 0x1160ca8.
class RenderCursor {
public:
    virtual ~RenderCursor()=default;
    virtual std::optional<ComponentSlots*> next()=0;
};
class RenderFrameHost {
public:
    virtual ~RenderFrameHost()=default;
    // Missing current world/visible HashSet faults; cursor preserves native slot/version order.
    virtual std::unique_ptr<RenderCursor> visible_render_cursor()=0;
    virtual AnimationResource* render_resource(RenderQueue& captured_application,std::int32_t)=0;
    // Native Seb.GetSprite's shared twelve-cell result; actual decoding is required, not guessed.
    virtual EventInstruction render_sprite(SharedAnimationClip&,std::int32_t frame,std::int32_t layer)=0;
    virtual std::int32_t render_layer_count(SharedAnimationClip&)=0;
    virtual std::int32_t render_extra_depth_rate()=0; // RenderSystem static starts at 100.
    virtual RenderQueue* current_render_queue()=0; // Fresh BaseSystem.apdat_; buffers may still be null.
    virtual void render_error(ComponentSlots&,const std::exception&)=0;
};
// RenderSystem.Update 0x1160d30. No automatic queue clear. Pass gate belongs to scheduler.
// Catches standard equivalents of managed loop faults, logs the current entity, sorts the prefix.
void update_render(RenderSystemState&,QuestCameraHost&,RenderFrameHost&);
float render_isometric_x(float x,float z); // AppData.TransormIsometricViewX 0x111580c.
float render_isometric_y(float x,float z); // AppData.TransormIsometricViewY 0x1115814.
std::int32_t pack_render_color(std::int32_t r,std::int32_t g,std::int32_t b); // Graphics.GetColorOfRGB 0x1b62c34.
}
