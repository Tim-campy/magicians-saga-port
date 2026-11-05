#pragma once
#include <cstdint>
#include "event_flow.hpp"
#include "select_touch_move.hpp"
#include <array>
namespace saga::core {
struct SelectCameraState {
    bool dragging; // SelectForm+0x148.
    float velocity_x,velocity_y; // +0x14c/+0x150, binary32.
    std::int32_t& x; // Bind to SelectPreludeState.camera_x / SelectBuildingState.camera_x.
    std::int32_t& y; // Bind to SelectBuildingState.camera_y.
    std::int32_t left_extent,right_extent; // +0x168/+0x16c; retain original signed values.
};
class SelectCameraHost {
public:
    virtual ~SelectCameraHost() = default;
    virtual std::int32_t game_width() = 0; // SelectForm.GetGameWidth.
    virtual std::int32_t game_height() = 0;
    virtual std::int32_t view_width() = 0; // Requires the current GameView, each call.
    virtual std::int32_t view_height() = 0;
};
struct CameraAreaRecord {std::int32_t state;};
using CameraAreas=std::vector<std::shared_ptr<CameraAreaRecord>>;
class SelectActiveAreaHost {
public:
    virtual ~SelectActiveAreaHost() = default;
    virtual std::shared_ptr<EventIntegers> normal_save_integers() = 0; // AppData.save_ +0x198.
    virtual std::int32_t view_width() = 0;
    virtual std::shared_ptr<CameraAreas> camera_areas() = 0; // Fresh Data.areaData each call.
};
void set_select_active_x(SelectCameraState&,SelectActiveAreaHost&);
// AppData.GetAreaNum 0x111511c: count exactly-found rows, starting at -1.
std::int32_t select_area_number(SelectActiveAreaHost&);
std::int32_t camera_clamp(std::int32_t value,std::int32_t minimum,std::int32_t maximum);
// GameView.GetGameWidth/Height; scaling uses wrapped int32 multiplication then binary32.
std::int32_t scaled_view_dimension(std::int32_t pixels,float scale);
// SelectForm.GetGameWidth/Height; ARM64 SDIV defines zero and overflow outcomes.
std::int32_t scaled_select_dimension(std::int32_t view_dimension,std::int32_t scale);
void set_select_camera_position(SelectCameraState&,std::int32_t x,std::int32_t y,SelectCameraHost&);
void move_select_camera(SelectCameraState&,SelectCameraHost&);
class SelectReturnPositionHost {
public:
    virtual ~SelectReturnPositionHost()=default;
    virtual EventInstruction return_saved_integers()=0;
    virtual EventInstruction return_area_position(std::int32_t)=0;
    virtual void return_scale(std::int32_t)=0;
    virtual std::int32_t return_game_width()=0;
    virtual std::int32_t return_game_height()=0;
    virtual std::int32_t return_surface_game_height()=0;
    virtual void return_camera_position(std::int32_t x,std::int32_t y)=0;
};
// SelectForm.SetReturnPos 0x11eba2c; SetScale and camera bounds belong to host.
void set_select_return_position(SelectTouchMoveState&,SelectReturnPositionHost&);
struct InputControl;
// TouchEvent type/x/deltaX/deltaY/pinch and component identity. The host owns
// the pointed control; non-camera events do not read coordinates or deltas.
struct SelectFormTouch {
    std::int32_t type=0,x=0,dx=0,dy=0,pinch_delta=0;
    InputControl* control=nullptr;
};
struct SelectTouchState {
    SelectCameraState& camera;
    SelectTouchMoveState& movement;
    std::int32_t& selected_chip;
    std::int32_t& selected_area;
    std::int32_t& form_state;
    std::int32_t& scale;
    // SelectForm..ctor 0x11e9b18..0x11e9b68 allocates distinct fresh Vector3D
    // owners and an empty point history; their scalar fields start at zero.
    EventScript history=std::make_shared<std::vector<EventInstruction>>();
    std::shared_ptr<std::array<float,3>> camera_speed=std::make_shared<std::array<float,3>>();
    std::shared_ptr<std::array<float,3>> camera_decimal=std::make_shared<std::array<float,3>>();
    bool build_drag=false,pinching=false;
    std::int32_t pinch_value=0,pinch_frame=0;
};
class SelectTouchHost {
public:
    virtual ~SelectTouchHost()=default;
    virtual EventInstruction active_range()=0; // GetActiveX; preserve returned row alias.
    virtual std::int32_t touch_count()=0;
    virtual void set_camera_position(std::int32_t x,std::int32_t y)=0;
    virtual void set_scale(std::int32_t value)=0; // Actual SelectForm.SetScale or explicit unavailable path.
    virtual void base_set_touch_value(std::uint32_t masked_flags,std::int32_t value)=0;
    virtual std::int32_t base_get_touch_value(std::uint32_t masked_flags,std::int32_t fallback)=0;
};
void set_select_touch_value(SelectTouchState&,std::uint32_t flags,std::int32_t value,SelectTouchHost&); // 0x11f4474.
std::int32_t get_select_touch_value(SelectTouchState&,std::uint32_t flags,std::int32_t fallback,SelectTouchHost&); // 0x11f44b0.
// OnTouchCamera 0x11f4518. The native second int parameter is unused.
void on_select_camera_touch(SelectTouchState&,std::int32_t type,const SelectFormTouch*,SelectTouchHost&);
bool on_select_form_touch(SelectTouchState&,const SelectFormTouch*,SelectTouchHost&); // 0x11f44dc.
}
