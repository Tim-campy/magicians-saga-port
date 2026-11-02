#pragma once
#include "pass_initialization.hpp"
#include "gauge_drawing.hpp"
#include "enemy_rewards.hpp"
#include "stage_placement.hpp"
#include <memory>
#include <string>

namespace saga::core {
using DebugText=std::shared_ptr<const std::u16string>;
using DebugNames=std::shared_ptr<std::vector<DebugText>>;
using DebugSystemContext=void*;
enum class DebugPalette {white,collision}; // AppData static +0x34 / +0x48.
enum class DebugTextKey {level_separator,parameter_separator,value_separator,attack_format,
                         percent_suffix,hp_prefix,experience_prefix};
class DebugDrawingHost {
public:
    virtual ~DebugDrawingHost()=default;
    virtual bool debug_controls()=0; // Config.DEBUG +0x10, not MyConfig.DEBUG +0x4.
    virtual StageGeometry* debug_map()=0;
    virtual std::int32_t debug_game_width()=0;
    virtual std::int32_t debug_game_height()=0;
    virtual void debug_validate_quest_form()=0;
    virtual EntitySubset* debug_subset(const EventIntegers& all)=0; // Current global World.
    virtual GaugePoint debug_project(const ContactPosition*)=0; // ToScreenPos(position,true), null -> {0,0}.
    virtual GaugePoint debug_project(float x,float y,float z)=0; // ToScreenPos(x,y,z,true).
    virtual StageQuest* debug_quest()=0; // Current global World's StageSystem.quest.
    virtual DebugSystemContext debug_member_system()=0;
    virtual ComponentSlots* debug_hero(DebugSystemContext)=0;
    virtual std::int32_t debug_total_parameter(DebugSystemContext,ComponentSlots&,std::int32_t)=0;
    virtual DebugSystemContext debug_human_system()=0;
    virtual std::int32_t debug_attack_interval(DebugSystemContext,ComponentSlots&)=0;
    virtual std::int32_t debug_burst_interval(DebugSystemContext,ComponentSlots&)=0;
    virtual std::int32_t debug_burst_count(DebugSystemContext,ComponentSlots&)=0;
    virtual std::int32_t debug_distance(DebugSystemContext,ComponentSlots&)=0;
    virtual DebugText debug_equipment_name(ComponentSlots&,EquipmentSlot)=0;
    virtual std::int32_t debug_equipment_level(ComponentSlots&,EquipmentSlot)=0;
    virtual DebugText debug_character_name(ComponentSlots&)=0; // Current mutable CharaData.name.
    virtual const EnemyRecord* debug_enemy_record(ComponentSlots&)=0;
    virtual EventInstruction debug_parameter_ids()=0; // CharaData.PARAM_IDS, re-read each loop condition/index.
    virtual DebugNames debug_parameter_names()=0; // Param.Names.
    virtual DebugNames debug_human_state_names()=0;
    virtual DebugNames debug_enemy_state_names()=0;
    // Strings come from the original data/provider, never replacement text in this kernel.
    virtual DebugText debug_text(DebugTextKey)=0;
    virtual std::int32_t debug_palette(DebugPalette)=0;
    virtual void debug_set_color(std::int32_t packed)=0;
    virtual void debug_push_font(std::int32_t size)=0;
    virtual void debug_pop_font()=0;
    virtual void debug_render_mode(std::int32_t mode,std::int32_t source,std::int32_t destination)=0;
    virtual void debug_reset_render_mode()=0;
    virtual void debug_draw_text(DebugText,float x,float y,std::int32_t anchor)=0;
    virtual void debug_draw_line(float x1,float y1,float x2,float y2,float width)=0;
    virtual void debug_fill_rect(float x,float y,float width,float height)=0;
    virtual void debug_draw_arc(float x,float y,float width,float height,float start,float sweep,float line_width)=0;
};
void draw_debug_humans(DebugVisualizationState&,DebugDrawingHost&); // DrawHumanInfo 0x11482a8.
void draw_debug_quest(DebugDrawingHost&); // DrawQuestInfo 0x1148cac.
void draw_debug_collisions(DebugDrawingHost&); // DrawCollisionInfo 0x1149224.
void draw_debug_enemies(DebugDrawingHost&); // DrawEnemyInfo 0x11495f4.
// DrawDynamic 0x1148094. Scheduler owns the enabled-only NeedsDrawDynamic gate.
void draw_debug_dynamic(DebugVisualizationState&,DebugDrawingHost&);
void draw_debug_visualize_system(DebugDrawingHost&) noexcept; // 0x114829c, native RET.
void draw_debug_node_cost(DebugDrawingHost&) noexcept; // 0x11482a0, native RET.
void draw_debug_node_connect(DebugDrawingHost&) noexcept; // 0x11482a4, native RET.
void draw_debug_map_chip_info(DebugDrawingHost&) noexcept; // 0x1148ca4, native RET.
void draw_debug_town_area_info(DebugDrawingHost&) noexcept; // 0x1148ca8, native RET.
void draw_debug_node_info(DebugDrawingHost&,std::int32_t mode) noexcept; // 0x1149b10, native RET.
}
