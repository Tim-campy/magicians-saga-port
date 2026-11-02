#pragma once
#include "emitter_frame.hpp"
#include "gauge_drawing.hpp"
#include <memory>
#include <string>

namespace saga::core {
struct EffectSystemState {
    EntitySubset* subset=nullptr; // EffectSystem +0x20: And26/Nor4.
    std::shared_ptr<EmitterExpiryQueue> expired; // +0x28; same List<Entity> contract.
};
class EffectInitializationWorld {
public:
    virtual ~EffectInitializationWorld()=default;
    virtual EntitySubset* effect_subset(const EventIntegers& all,const EventIntegers& none)=0;
};
class EffectFrameHost {
public:
    virtual ~EffectFrameHost()=default;
    virtual bool effect_parent_destroyed(ComponentSlots&)=0;
    virtual void destroy_effect_entity(ComponentSlots&)=0;
};
void initialize_effects(EffectSystemState&,EffectInitializationWorld*); // Init 0x1149cf0.
void update_normal_effect(ComponentSlots*); // 0x114a294.
void update_damage_number_effect(ComponentSlots*); // 0x114a318.
void update_time_effect(ComponentSlots*) noexcept; // 0x114a36c; not called by Update.
void update_level_up_effect(ComponentSlots*); // 0x114a370.
void update_jump_number_effect(ComponentSlots*); // 0x114a3a0.
void update_warehouse_balloon_effect(ComponentSlots*); // 0x114a3d0.
void update_string_background_effect(ComponentSlots*); // 0x114a400.
// EffectSystem.Update 0x1149e6c; scheduler owns enabled/pass/pause gates.
void update_effects(EffectSystemState&,EffectFrameHost&);
using EffectNumberContext=const void*; // Borrowed current AppData identity, captured before the fresh Seb lookup.
using EffectResourceContext=void*; // Borrowed resource manager resolved from a captured AppData.
using EffectText=std::shared_ptr<const std::u16string>; // Nullable immutable source identity, retained through font callbacks.
class EffectDrawingHost {
public:
    virtual ~EffectDrawingHost()=default;
    virtual void effect_draw_normal(ComponentSlots&)=0;
    virtual void effect_draw_damage(ComponentSlots&)=0;
    virtual void effect_draw_time(ComponentSlots&)=0;
    virtual void effect_draw_jump(ComponentSlots&)=0;
    virtual void effect_draw_string_background(ComponentSlots&)=0;
    virtual GaugePoint& effect_screen_position(ContactPosition*)=0; // Retain the shared point's identity.
    virtual EffectNumberContext capture_effect_numbers()=0;
    virtual void effect_draw_number(EffectNumberContext,std::int32_t clip,std::int64_t value,
        std::int32_t x,std::int32_t y,std::int32_t padding,std::int32_t anchor,std::int32_t scale,
        std::int32_t width,std::int32_t height)=0;
    virtual EffectResourceContext capture_effect_resource(EffectNumberContext,std::int32_t resource);
    virtual void effect_draw_sprite(EffectResourceContext,std::int32_t clip,std::int32_t frame,float x,float y);
    virtual void effect_render_mode(std::int32_t mode,std::int32_t alpha);
    virtual void effect_reset_render_mode(); // Default mode/source/destination (0,255,0).
    // Native duration formatting is discarded. Public formatting/culture services stay outside the core.
    virtual void effect_format_time(std::int32_t){}
    virtual std::int32_t effect_notice_frame(std::int32_t notice);
    virtual EffectText effect_notice_text(std::int32_t notice);
    virtual void effect_draw_anchored_sprite(EffectResourceContext,std::int32_t clip,std::int32_t frame,
        float x,float y,std::int32_t anchor);
    virtual void effect_push_font(std::int32_t size);
    virtual void effect_white_text_color();
    virtual void effect_draw_text(EffectText,std::int32_t x,std::int32_t y,std::int32_t anchor);
    virtual void effect_pop_font();
};
class EffectProjectionHost {
public:
    virtual ~EffectProjectionHost()=default;
    virtual GaugePoint* effect_screen_work()=0; // Fresh EffectSystem static point; nullable until its next use.
    virtual float effect_projection_x(float x,float z)=0; // Require current AppData, then TransormIsometricViewX.
    virtual float effect_projection_y(float x,float z)=0; // Require current AppData, then TransormIsometricViewY.
    virtual void validate_effect_draw_context()=0; // Current AppData/frmQuest; after both point writes.
    virtual GaugePoint effect_camera_offset()=0; // Current QuestForm cameraX/cameraY, int32.
};
GaugePoint& effect_to_screen_position(ContactPosition*,EffectProjectionHost&); // ToScreenPos 0x114b0fc.
void draw_number_effect(ComponentSlots*,EffectDrawingHost&); // DrawNumberEffect 0x114ac34.
void draw_normal_effect(ComponentSlots*,EffectDrawingHost&); // DrawNormalEffect 0x114a63c.
void draw_damage_number_effect(ComponentSlots*,EffectDrawingHost&); // DrawDamageNumEffect 0x114a79c.
void draw_time_effect(ComponentSlots*,EffectDrawingHost&); // DrawTimeEffect 0x114aad4; no pixel output.
void draw_jump_number_effect(ComponentSlots*,EffectDrawingHost&); // DrawJumpNumEffect 0x114ad64; fade/projection only.
void draw_string_background_effect(ComponentSlots*,EffectDrawingHost&); // DrawStrWithBgEffect 0x114ae64.
// DrawDynamic 0x114a438. Types4/6 are native RET handlers; no enabled/parent/context gate here.
void draw_effects(EffectSystemState&,EffectDrawingHost&);
}
