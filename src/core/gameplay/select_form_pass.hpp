#pragma once
#include "form_push.hpp"
#include "select_clouds.hpp"
#include "select_markers.hpp"
#include "select_event_pass.hpp"
#include "select_prelude.hpp"
#include "select_touch_move.hpp"
#include "select_triggers.hpp"
#include "town_bubble.hpp"
#include "render_draw.hpp"
#include <array>
#include <optional>
namespace saga::core {
struct SelectBackgroundState {
    const std::int32_t& camera_x;
    const std::int32_t& camera_y;
    const std::int32_t& left_extent;
    const std::int32_t& right_extent;
    const std::int32_t& view_y;
    const std::int32_t* view_x=nullptr; // Null selects the earlier zero-X profile.
};
using SelectBackgroundResource=void*; // Borrowed host drawing owner, captured after the fill.
class SelectBackgroundHost {
public:
    virtual ~SelectBackgroundHost()=default;
    virtual std::array<float,3> background_pixel()=0; // resSelect.images[RESSELECT_IMG_BG].GetPixel(0,0).
    virtual void background_color(std::int32_t r,std::int32_t g,std::int32_t b,std::int32_t a)=0;
    virtual std::int32_t background_width()=0; // SelectForm.GetGameWidth.
    virtual std::int32_t background_height()=0; // SelectForm.GetGameHeight.
    virtual void fill_background(std::int32_t width,std::int32_t height)=0; // At (0,0).
    virtual SelectBackgroundResource background_resource()=0;
    virtual std::int32_t background_view_height()=0; // Raw GameView height, queried per tile.
    virtual std::int32_t background_clip()=0; // Live RESSELECT_SEB_BG.
    virtual void draw_background(SelectBackgroundResource,std::int32_t clip,std::int32_t frame,float x,float y)=0;
};
// SelectForm._Draw 0x11f0850..0x11f0a6c; stops before DrawDecoObject(0).
// ResetTouchList/IsTopForm and all later town/resident/UI layers belong to the caller.
struct SelectBackgroundOrigin {std::int32_t x,y;}; // Captured after the color fill, before tile callbacks.
SelectBackgroundOrigin draw_select_background(const SelectBackgroundState&,SelectBackgroundHost&);
struct SelectSceneryState {
    EventScript clouds=std::make_shared<std::vector<EventInstruction>>(); // +0x108, eight-int rows.
    EventScript birds,leaves; // +0x198/+0x1a0, six-int rows.
    const std::int32_t& left_extent;
    const std::int32_t& right_extent;
};
class SelectSceneryHost:public SelectViewportHost {
public:
    virtual std::int32_t raw_view_height()=0;
    virtual std::int32_t cloud_right_edge()=0; // Live +0x160 + AppData.VIEW_W.
    virtual std::int32_t scenery_clip(std::int32_t slot)=0; // AppData static field offset.
    virtual std::int32_t app_random(std::int32_t bound)=0;
    virtual std::int32_t lib_random(std::int32_t bound)=0;
    virtual std::int32_t lib_random(std::int32_t minimum,std::int32_t maximum)=0;
    virtual SelectBackgroundResource scenery_resource()=0;
    virtual std::array<std::int32_t,4> scenery_bounds(std::int32_t clip,std::int32_t frame)=0;
    virtual void scenery_reverse(std::int32_t)=0;
    virtual void scenery_draw(SelectBackgroundResource,std::int32_t clip,std::int32_t frame,float x,float y,std::int32_t anchor)=0;
    virtual void scenery_draw_indexed(SelectBackgroundResource,std::int32_t clip,std::int32_t frame,float x,float y,std::int32_t layer,std::int32_t anchor)=0;
};
// SelectForm.Init cloud subsequence, before ChangeState(0)/InitDecoObject.
void initialize_select_scenery_clouds(SelectSceneryState&,SelectSceneryHost&);
EventInstruction generate_select_bird(SelectSceneryState&,bool initial,SelectSceneryHost&);
EventInstruction generate_select_leaf(SelectSceneryState&,bool initial,SelectSceneryHost&);
void initialize_select_decorations(SelectSceneryState&,SelectSceneryHost&);
// DrawDecoObject: types0 (birds) and2 (leaves); other types do not enumerate.
void draw_select_decorations(SelectSceneryState&,std::int32_t type,SelectBackgroundOrigin,SelectSceneryHost&);
// _Draw 0x11f0a80..0x11f0e10: two ground passes, then backward cloud drawing.
// Caller runs DrawDecoObject(0) first; floor/buildings/residents are later layers.
void draw_select_terrain(SelectSceneryState&,SelectBackgroundOrigin,SelectSceneryHost&);
class SelectFloorHost:public SelectViewportHost {
public:
    virtual std::int32_t floor_view_height()=0;
    virtual SebSpriteState* floor_sprite()=0; // Fresh resSelect.seb[RESSELECT_SEB_FLOOR].
    virtual RenderImage* floor_image()=0; // Fresh resSelect.images[RESSELECT_IMG_SELECT_FLOOR].
    virtual void floor_draw(SebSpriteState*,RenderImage*,std::int32_t frame,std::int32_t x,std::int32_t y)=0;
    virtual void floor_push_font(std::int32_t)=0;
    virtual SelectBackgroundResource gate_resource()=0;
    virtual std::int32_t gate_clip()=0;
    virtual void gate_draw(SelectBackgroundResource,std::int32_t clip,std::int32_t frame,std::int32_t layer,float x,float y)=0;
};
// _Draw 0x11f0e10..0x11f0fe0: floor ends, middle strips, font10 and gate0/layer0.
void draw_select_floor(const SelectSceneryState&,SelectBackgroundOrigin,SelectFloorHost&);
struct TownBuildingView {std::int32_t image=0,type=0;FormText name;};
struct SelectTownDrawingState {
    const std::int32_t& frame;
    const std::int32_t& selected;
    bool top_form;
    EventScript& notices;
};
class SelectTownDrawingHost:public SelectViewportHost {
public:
    virtual std::int32_t town_count()=0;
    virtual EventScript town_buildings()=0;
    virtual std::shared_ptr<std::vector<EventObject>> town_occupants()=0;
    virtual bool town_enabled(std::int32_t)=0;
    virtual std::shared_ptr<TownBuildingView> town_building(std::int32_t)=0;
    virtual std::int32_t town_view_height()=0;
    virtual SelectBackgroundResource town_resource()=0;
    virtual std::int32_t town_clip(std::int32_t field)=0;
    virtual std::int32_t town_clip_frames(std::int32_t clip)=0;
    virtual void town_draw_chip(SelectBackgroundResource,std::int32_t clip,std::int32_t frame,std::int32_t image,std::int32_t x,std::int32_t y)=0;
    virtual void town_push_font(std::int32_t)=0;
    virtual void town_pop_font()=0;
    virtual void town_push_dim()=0;
    virtual void town_pop_dim()=0;
    virtual void town_label(FormText,std::int32_t x,std::int32_t y)=0;
    virtual bool town_king_reward_available()=0;
    virtual std::int32_t town_share_count()=0;
    virtual bool town_share_active()=0;
    virtual std::int32_t town_form_state()=0;
    virtual void town_share_label(std::int32_t x,std::int32_t y)=0;
    virtual void town_touch(std::int32_t slot,bool selected,std::int32_t x,std::int32_t y)=0;
};
// DrawSebStack 0x11f3714: seven-int request; -99999 selects current list count.
void queue_select_notice(EventScript&,std::int32_t resource,std::int32_t x,std::int32_t y,std::int32_t clip,std::int32_t frame,std::int32_t layer,std::int32_t depth=-99999);
// _Draw 0x11f0fe4..0x11f1d98: building underlays, font scopes, sprites and notice requests.
// Does not flush the notice stack or draw the later area/resident/foreground passes.
void draw_select_town_buildings(const SelectTownDrawingState&,SelectBackgroundOrigin,SelectTownDrawingHost&);
struct SelectAreaDrawingRecord {std::int32_t image=0,discovery=0;FormText name;};
using SelectAreaDrawingRows=std::vector<std::shared_ptr<SelectAreaDrawingRecord>>;
struct SelectAreaDrawingState {
    const std::int32_t& frame;
    const std::int32_t& selected;
    const std::int32_t& form_state;
    bool top_form;
    EventInstruction& marks;
    EventScript& notices;
};
class SelectAreaDrawingHost:public SelectViewportHost {
public:
    virtual std::shared_ptr<SelectAreaDrawingRows> drawing_areas()=0;
    virtual std::int32_t area_view_height()=0;
    virtual void area_push_font(std::int32_t)=0;
    virtual void area_draw(std::int32_t resource,std::int32_t clip,std::optional<std::int32_t> frame,std::int32_t x,std::int32_t y)=0;
    virtual std::int32_t area_clip(std::int32_t field)=0;
    virtual std::int32_t area_clip_frames(std::int32_t resource,std::int32_t clip)=0;
    virtual std::int32_t area_number()=0; // AppData.GetAreaNum: count(discovery==1)-1, not highest found ID.
    virtual void area_label_color()=0; // AppData static+0x3b0.
    virtual void area_label(FormText,std::int32_t x,std::int32_t y)=0;
    virtual void area_touch(std::int32_t index,bool selected,bool locked,std::int32_t x,std::int32_t y)=0;
};
// GetAreaPos 0x11eb990; caller reads raw GameView height before coordinate calculation.
std::array<std::int32_t,2> select_area_position(std::int32_t index,std::int32_t raw_height);
// _Draw 0x11f1d9c..0x11f2324. Leaves font10 active and notices queued.
// Inherited PopClip, resident drawing and the later stack flush belong to the caller.
void draw_select_town_areas(const SelectAreaDrawingState&,SelectBackgroundOrigin,SelectAreaDrawingHost&);
struct SelectResidentDrawingState {
    const std::int32_t& camera_x;
    const std::int32_t& camera_y;
    const std::int32_t& view_x;
    const std::int32_t& view_y;
    const std::int32_t& frame;
    EventScript& notices;
};
class SelectResidentDrawingHost:public SelectViewportHost {
public:
    virtual bool resident_top_form()=0;
    virtual EventScript resident_rows()=0;
    virtual std::int32_t resident_view_height()=0;
    virtual void resident_draw_human(std::int32_t character,std::int32_t clip,std::int32_t frame,std::int32_t x,std::int32_t y)=0;
    virtual std::int32_t resident_character_flags(std::int32_t character)=0;
    virtual std::int32_t resident_arrow_clip()=0;
    virtual std::int32_t resident_arrow_frames(std::int32_t clip)=0;
    virtual void resident_push_font(std::int32_t)=0;
    virtual void resident_pop_font()=0;
    virtual void resident_touch(std::int32_t kind,std::int32_t x,std::int32_t y)=0;
    virtual void resident_label_color()=0; // AppData static+0x34.
    virtual void resident_special_label(std::int32_t x,std::int32_t y)=0;
    virtual void resident_level_label(std::int32_t character,std::int32_t x,std::int32_t y)=0;
    virtual void resident_attribute_label(std::int32_t character,std::int32_t x,std::int32_t y)=0;
};
// DrawTownHuman 0x11f38a8: materialize lane-filtered row aliases before drawing.
// Labels and input registration remain narrow presentation services.
void draw_select_resident_lane(const SelectResidentDrawingState&,std::int32_t lane,SelectResidentDrawingHost&);
// SelectForm.Init stores at +0x124/+0x128/+0x164; animation counter is shared
// with the other town passes, rather than a separate hero clock.
struct SelectHeroDrawingState {float current_x=40.0F;std::int32_t direction=1;float target_x=40.0F;};
class SelectHeroDrawingHost {
public:
    virtual ~SelectHeroDrawingHost()=default;
    virtual SelectBackgroundResource hero_marker_resource()=0;
    virtual std::int32_t hero_view_height()=0;
    virtual std::int32_t hero_marker_clip()=0;
    virtual void hero_target_marker(SelectBackgroundResource,std::int32_t clip,std::int32_t frame,std::int32_t x,std::int32_t y)=0;
    virtual void hero_target_effect(std::int32_t x,std::int32_t y,std::int32_t frame)=0;
    virtual ComponentSlots* town_hero()=0;
    virtual void hero_draw(ComponentSlots*,std::int32_t x,std::int32_t y,std::int32_t clip,std::int32_t direction,std::int32_t frame)=0;
};
// _Draw 0x11f2348..0x11f2528; call between resident lane0 and lanes1/2.
void draw_select_town_hero(const SelectHeroDrawingState&,const std::int32_t& frame,SelectBackgroundOrigin,SelectHeroDrawingHost&);
struct SelectFormPassState {
    std::int32_t state;
    bool captured;
    bool auto_save;
    FormPushState form;
    SelectMarkerState markers;
    SelectEventPassState events;
};
class SelectFormPassHost {
public:
    virtual ~SelectFormPassHost() = default;
    virtual bool capture_input(std::uint32_t mask) = 0;
    virtual void change_state(std::int32_t value) = 0;
    virtual bool soft_label(std::int32_t index) = 0;
    virtual void finish_capture() = 0;
    virtual void capture() = 0;
    virtual void play_bgm() = 0;
    virtual void update_notifications() = 0;
    virtual bool acquire_balance() = 0;
    virtual void set_active_x() = 0;
    virtual void update_decorations() = 0;
    virtual void update_town_humans() = 0;
    // Fresh SecureInt conversion of normal save_ integer slot 19, each loop test (AppData+0x198).
    virtual std::int32_t town_enable_count() = 0;
    virtual bool update_town_enable(std::int32_t index) = 0;
    virtual void check_guide_arrow(bool force) = 0;
};
enum class SelectPassResult { ReturnTrue, ContinueInput };
// Entry after common MyFormBase/image-animation updates (ELF 0x11ebde8).
// ContinueInput resumes original Update at 0x11ec7ec; it is not a completed frame.
SelectPassResult process_select_form_pass(SelectFormPassState&,SelectFormPassHost&,
    SelectCloudHost&,SelectMarkerHost&,EventDelayQueue&,EventExecutionHost&,EventConditionHost&);
struct SelectUpdatePrefixResult {SelectPreludeSnapshot snapshot;SelectPassResult pass;};
// Includes touch movement and triggers on ContinueInput; next instruction is 0x11eca28.
SelectUpdatePrefixResult update_select_form_prefix(SelectPreludeState&,SelectPreludeHost&,
    SelectFormPassState&,SelectFormPassHost&,SelectCloudHost&,SelectMarkerHost&,
    EventDelayQueue&,EventExecutionHost&,EventConditionHost&,SelectTouchMoveState&,SelectTouchMoveHost&,SelectTriggerHost&);
}
