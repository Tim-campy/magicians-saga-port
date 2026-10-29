#pragma once
#include "image_draw.hpp"
#include "combat_records.hpp"
#include "contact_collision.hpp"
#include "../ecs/entity_subset.hpp"

namespace saga::core {
struct GaugePoint {std::int32_t x=0,y=0;};
struct ParameterGaugeRequest {
    ComponentSlots* entity=nullptr;
    std::int32_t x=0,y=0,image=0,parameter=0,anchor=0;
    bool force=false;
};
struct GaugeDrawState {
    // GaugeSystem..cctor 0x1153544: 128 zero Color entries; only alpha is observed.
    std::shared_ptr<std::vector<float>> alpha=std::make_shared<std::vector<float>>(128,0.0F);
};
class GaugeDrawingHost {
public:
    virtual ~GaugeDrawingHost()=default;
    virtual PixelImage* gauge_image(std::int32_t)=0; // Fresh global AppData resource23 image.
    virtual std::int32_t gauge_view_width()=0;
    virtual std::int32_t gauge_view_height()=0;
    // Image.GetPixels top-down RGBA projection; overwrite the crop prefix only.
    // Reuse a nonnull fixed-size buffer, preserve its suffix, and fail on insufficient length.
    virtual std::shared_ptr<std::vector<float>> gauge_alpha(PixelImage&,std::int32_t x,std::int32_t y,
        std::int32_t width,std::int32_t height,std::shared_ptr<std::vector<float>>)=0;
};
GaugePoint anchor_gauge(std::int32_t x,std::int32_t y,std::int32_t width,std::int32_t height,std::int32_t anchor); // 0x1154d00.
std::int32_t gauge_image_offset(GaugeDrawState&,PixelImage*,GaugeDrawingHost&); // 0x1154210.
void draw_gauge(GaugeDrawState&,RenderGraphics*,std::int32_t x,std::int32_t y,std::int32_t image,
                std::int32_t rate,std::int32_t scale,GaugeDrawingHost&); // 0x1154350.
void draw_parameter_gauge(GaugeDrawState&,RenderGraphics*,const ParameterGaugeRequest&,GaugeDrawingHost&); // 0x1154a48.
class EnemyGaugeDrawHost {
public:
    virtual ~EnemyGaugeDrawHost()=default;
    virtual const EnemyRecord* gauge_enemy_record(ComponentSlots&)=0;
    virtual std::int32_t gauge_enemy_defeats(const EnemyRecord&)=0; // Saved EnemyData.defeatCount, not level.
    virtual GaugePoint gauge_screen_position(const ContactPosition*)=0; // AppData.ToScreenPos(position,true).
    virtual std::int32_t enemy_gauge_palette(std::int32_t attribute)=0;
    virtual void parameter_gauge(const ParameterGaugeRequest&)=0;
};
void draw_enemy_gauges(EntitySubset&,EnemyGaugeDrawHost&); // EnemySystem.DrawDynamic 0x114e1d4.
} // namespace saga::core
