#pragma once
#include "quest_catalog.hpp"
#include "input_frame.hpp"
#include <array>
#include <exception>
#include <memory>
#include <vector>

namespace saga::core {
// SelectStageForm fields; constructor leaves CLR-zero state before Init.
struct StageMenuState {
    std::int32_t error_function=0,error_line=0,base_update_counter=0,loop_counter=0;
    std::int32_t state=0,frame=0,selected=0,area=0,production_frame=0,scroll_value=0;
    EventInstruction entries;
    EventObject scroll,renderer;
};
struct StageMenuScrollParameters {
    std::array<std::int32_t,8> integers;
    bool option;
    EventObject listener;
};
struct StageMenuTouch {
    InputTouch input;
    std::int32_t x=0,y=0,dx=0,dy=0,pinch_delta=0;
};
class StageMenuHost {
public:
    virtual ~StageMenuHost()=default;
    virtual void base_initialize()=0;
    virtual void set_background_white()=0;
    // Retain AppData's cached resources and original conditional load order.
    virtual void prepare_resources()=0;
    virtual EventInstruction save_integers()=0;
    virtual void system_flag_on(std::uint32_t)=0;
    // Catalogs/record identities stay alive across callbacks; getters are live.
    virtual std::shared_ptr<AreaTable> areas()=0;
    virtual std::shared_ptr<QuestTable> quests()=0;
    virtual bool debug()=0;
    virtual void set_soft_labels(std::int32_t left,std::int32_t right)=0;
    virtual EventObject make_scroll(const StageMenuScrollParameters&)=0;
    virtual void scroll_element_size(const EventObject&,std::int32_t width,std::int32_t height)=0;
    virtual void scroll_touch_flags(const EventObject&,std::int32_t)=0;
    virtual EventObject make_renderer()=0;
    virtual void base_update()=0;
    virtual void reset_image_clocks()=0;
    virtual void advance_image_clocks()=0;
    virtual void update_main_blur()=0;
    virtual void update_scroll(const EventObject&,bool)=0;
    virtual bool pressed(std::uint32_t)=0;
    virtual void play_button_sound()=0;
    virtual std::int32_t stamina()=0;
    virtual void use_stamina(std::int32_t)=0;
    virtual EventObject make_stamina_dialog()=0;
    virtual void push_form(const EventObject&)=0;
    virtual bool soft_label_pressed(std::int32_t)=0;
    virtual EventObject make_formation()=0;
    virtual bool pop_form()=0;
    virtual void play_bgm()=0;
    virtual void save_game(std::int32_t,bool)=0;
    virtual void save_system()=0;
    // Native typed save-exception filter and Config-gated logging; false rethrows.
    virtual bool recover_save_exception(std::exception_ptr)=0;
    virtual void reset_town_camera()=0;
    virtual void change_to_quest()=0;
    virtual void base_set_touch_value(std::uint32_t,std::int32_t)=0;
    virtual std::int32_t base_get_touch_value(std::uint32_t,std::int32_t)=0;
    virtual void scroll_camera_touch(const EventObject&,std::int32_t,std::int32_t,const StageMenuTouch&)=0;
    virtual void scroll_list_touch(const EventObject&,std::int32_t,std::int32_t,const StageMenuTouch&)=0;
    virtual void base_resume()=0;
    virtual void base_finish()=0;
    virtual void end_main_blur()=0;
};
// SelectStageForm.Init0x1207608, ChangeState0x1207b44, Update0x1207d5c.
// Diagnostic values are explicit inputs from the APK constant at0x627350.
bool initialize_stage_menu(StageMenuState&,StageMenuHost&);
void change_stage_menu_state(StageMenuState&,std::int32_t,StageMenuHost&);
bool update_stage_menu(StageMenuState&,StageMenuHost&,std::array<std::int32_t,2> diagnostic);
void set_stage_menu_touch_value(StageMenuState&,std::uint32_t,std::int32_t,StageMenuHost&);
std::int32_t get_stage_menu_touch_value(const StageMenuState&,std::uint32_t,std::int32_t,StageMenuHost&);
bool stage_menu_touch(StageMenuState&,const StageMenuTouch*,StageMenuHost&);
bool resume_stage_menu(StageMenuHost&);
bool finish_stage_menu(StageMenuHost&);
// Decoded sysSave_.longArray storage; slots0/1/5 retain original int64 widths.
using StageStaminaValues=std::vector<std::int64_t>;
class StageStaminaHost {
public:
    virtual ~StageStaminaHost()=default;
    virtual std::shared_ptr<StageStaminaValues> stamina_values()=0;
    virtual std::int32_t stamina_limit()=0;
    virtual std::int64_t server_time()=0;
};
// AppData.UseStamina0x1111e14; services supply billing-derived limit/server time.
void use_stage_stamina(std::int32_t cost,StageStaminaHost&);
// Selected neutral ScrollList.Update(1) path after SelectStageForm.Init.
// Gestures/keyboard navigation remain an explicit unavailable branch.
struct StageNeutralScroll {
    std::int32_t count=0,first_visible=0,offset_y=0,damping=0;
    std::int32_t color=255,fade_progress=-10,width=210,height=130;
    std::int32_t maximum_height=0x7fffffff,last_touched=-1;
};
void update_neutral_stage_scroll(StageNeutralScroll&,bool repeat_enabled,bool surface_active,
    const std::function<bool(std::uint32_t,bool repeat)>& key);
// Selected ScrollList.OnTouchList0x1124d58; other event types do not dereference the event.
void touch_stage_scroll_list(StageNeutralScroll&,std::int32_t type,const StageMenuTouch*);
struct StageMenuRowRegion {
    InputRectangle bounds;
    InputControl control;
    std::int32_t option_flags=17,priority=1;
    std::array<std::int32_t,4> margins{};
};
class StageMenuDrawHost {
public:
    virtual ~StageMenuDrawHost()=default;
    virtual std::int32_t raw_width()=0;
    virtual std::int32_t raw_height()=0;
    virtual std::int32_t center_x()=0;
    virtual std::shared_ptr<AreaTable> areas()=0;
    virtual std::shared_ptr<QuestTable> quests()=0;
    // Native ScrollList.DrawScrollBar -> AppData.DrawScrollBar, width6/visible3.
    virtual void draw_scroll_bar(InputRectangle,std::int32_t total,std::int32_t visible,
                                 std::int32_t first,std::int32_t color)=0;
    virtual bool first_camera_touch()=0;
    virtual void add_camera_touch(const CameraTouchRegion&)=0;
    virtual void push_clip(InputRectangle)=0;
    virtual void pop_clip()=0;
    // Validate select_stage clip0/frame0 GetBoundingRect before center/catalog queries.
    virtual void validate_row_bounds()=0;
    // Resource13 clip0/current frame, scale100/anchor17.
    virtual void draw_row_background(std::int32_t x,std::int32_t y)=0;
    // Color AppData+0x34; LF's APK TextFormat description applies only to the name.
    virtual void set_row_color()=0;
    virtual void draw_quest_name(const std::string&,std::int32_t x,std::int32_t y)=0;
    // APK level prefix + invariant int32 string, anchor1, current font.
    virtual void draw_level(std::int32_t level,std::int32_t x,std::int32_t y)=0;
    // Resource13 clip1/frame1, or resource2 clip15/explicit frame; direct SEB draws.
    virtual void draw_completed(std::int32_t x,std::int32_t y)=0;
    virtual std::int32_t pending_max_frame()=0;
    virtual void draw_pending(std::int32_t frame,std::int32_t x,std::int32_t y)=0;
    virtual void add_row_touch(const StageMenuRowRegion&)=0;
    // Draw caller presentation boundaries retain original ordering and failure effects.
    virtual void begin_stage_draw()=0; // native graphics/base/blur prefix
    virtual std::array<std::int32_t,2> draw_origin()=0;
    virtual void set_draw_origin(std::int32_t x,std::int32_t y)=0;
    virtual void draw_heading(std::int32_t center)=0; // common66/title/color34
    // Required selected branch: panel clip2/soft-key Y, selected QuestRecord's six
    // original label/value groups, font scope and widths. Never an optional no-op.
    virtual void draw_selected_details(const StageMenuState&)=0;
    virtual void end_stage_draw(const StageMenuState&)=0; // graphics suffix/production effect
};
// SelectStageForm.DrawScrollList0x1208f58 / DrawStageContents0x1209270.
// Selected configuration is vertical, one column, element210x40/gap5/three rows.
void draw_stage_menu_contents(const StageMenuState&,std::int32_t index,std::int32_t x,
                              std::int32_t y,StageMenuDrawHost&);
void draw_stage_menu_list(const StageMenuState&,StageNeutralScroll&,std::int32_t x,
                          std::int32_t y,StageMenuDrawHost&);
// SelectStageForm.Draw0x12083a0 orchestration; presentation remains explicit above.
void draw_stage_menu(const StageMenuState&,StageNeutralScroll&,StageMenuDrawHost&);
} // namespace saga::core
