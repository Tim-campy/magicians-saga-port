#pragma once
#include <cstdint>
#include "event_flow.hpp"
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
std::int32_t camera_clamp(std::int32_t value,std::int32_t minimum,std::int32_t maximum);
// GameView.GetGameWidth/Height; scaling uses wrapped int32 multiplication then binary32.
std::int32_t scaled_view_dimension(std::int32_t pixels,float scale);
// SelectForm.GetGameWidth/Height; ARM64 SDIV defines zero and overflow outcomes.
std::int32_t scaled_select_dimension(std::int32_t view_dimension,std::int32_t scale);
void set_select_camera_position(SelectCameraState&,std::int32_t x,std::int32_t y,SelectCameraHost&);
void move_select_camera(SelectCameraState&,SelectCameraHost&);
}
