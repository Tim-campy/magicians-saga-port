#pragma once
#include "stage_start.hpp"
#include <array>
#include <optional>

namespace saga::core {
class StageControlsHost {
public:
    virtual ~StageControlsHost()=default;
    virtual std::int32_t controls_width()=0;
    virtual std::int32_t controls_height()=0;
    // A missing frame selects ResourceManager.DrawSeb's current animation frame.
    virtual void controls_draw_seb(std::int32_t resource,std::int32_t clip,std::optional<std::int32_t> frame,std::int32_t x,std::int32_t y)=0;
    virtual void controls_set_render_mode(std::int32_t mode,std::int32_t alpha)=0;
    // Group10, TouchOption.Create(8); margins are left/right/top/bottom.
    virtual void controls_add_attack(std::int32_t x,std::int32_t y,std::int32_t width,std::int32_t height,std::int32_t value,std::array<std::int32_t,4> margins)=0;
    // Stage.DrawButton owns resource7, key-derived value, hover frame and Keys option.
    virtual void controls_move_button(std::int32_t group,std::int32_t x,std::int32_t y,std::int32_t clip,std::int32_t key,std::int32_t anchor,std::array<std::int32_t,4> margins)=0;
};
// StageSystem.DrawAttackButton 0x1167604 / DrawMoveButton 0x1167a00.
// The enclosing Stage.Draw state1 gate belongs to the caller.
void draw_stage_attack_controls(StageStartState&,StageControlsHost&);
void draw_stage_move_controls(StageStartState&,StageControlsHost&);
}
