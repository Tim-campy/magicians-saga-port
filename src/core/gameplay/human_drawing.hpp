#pragma once
#include "combat_tables.hpp"
#include "render_draw.hpp"

namespace saga::core {
struct HumanDrawingRequest {
    std::int32_t x=0,y=0,clip=0,frame=0,character=0,scale=100,mask=0;
};
class ScaledHumanDrawingHost {
public:
    virtual ~ScaledHumanDrawingHost()=default;
    virtual std::shared_ptr<CharacterProgressionTable> human_characters()=0;
    virtual std::shared_ptr<EquipmentCombatTable> human_weapons()=0;
    virtual std::int32_t hero_cloth_image()=0;
    virtual std::int32_t hero_hat_image()=0;
    virtual std::int32_t hero_weapon_image()=0;
    // Scratch matrix must not alias an already-pushed transform.
    virtual RenderMatrix& human_scale_matrix()=0;
    virtual RenderDrawResource* human_character_resource()=0; // Captured AppData.resChara, not global AppData.
    virtual RenderDrawResource* human_resource(std::int32_t resource)=0;
};
// AppData.DrawScaledHuman 0x110de5c: five image IDs, with live main-hero overrides.
EventInstruction resolve_human_images(std::int32_t character,ScaledHumanDrawingHost&);
// AppData.DrawScaledHuman 0x110e298: matrix scale, instance SEB rows and image overrides.
// Mask value1 hides layers3/4; value2 hides layer0.
// Like the original, failures retain the pushed matrix and any active reverse flags.
void draw_human_images(const HumanDrawingRequest&,EventInstruction images,RenderGraphics*,ScaledHumanDrawingHost&,SebStaticWork&);
// The 0x110de38 wrapper supplies mask0; unused native scalar arguments are omitted.
void draw_scaled_human(const HumanDrawingRequest&,RenderGraphics*,ScaledHumanDrawingHost&,SebStaticWork&);
} // namespace saga::core
