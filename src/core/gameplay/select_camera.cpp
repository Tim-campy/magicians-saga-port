#include "select_camera.hpp"
#include <cmath>
#include <limits>
namespace saga::core {
namespace {
std::int32_t wrap(std::int64_t value) {
    const auto bits=static_cast<std::uint32_t>(value);
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):
        static_cast<std::int32_t>(static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::int32_t convert(float value) {
    if(std::isnan(value))return 0;
    if(value==std::numeric_limits<float>::infinity() || value<=-2147483648.0F)
        return std::numeric_limits<std::int32_t>::min();
    if(value>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();
    return static_cast<std::int32_t>(value);
}
void decay(float& value) {
    if(value>0.0F) {value=value*0.9F;if(value<=0.25F)value=0.0F;}
    else if(value<0.0F) {value=value*0.9F;if(value>=-0.25F)value=0.0F;}
}
}
// AppData.Clamp 0x110180c: upper limit first, then lower, even for inverted bounds.
std::int32_t camera_clamp(std::int32_t value,std::int32_t minimum,std::int32_t maximum) {
    if(value>maximum)value=maximum;
    if(value<minimum)value=minimum;
    return value;
}
std::int32_t scaled_view_dimension(std::int32_t pixels,float scale) {
    const auto numerator=static_cast<float>(wrap(static_cast<std::int64_t>(pixels)*100));
    const float quotient=numerator/scale;
    return convert(quotient+0.01F); // ELF literal 0x62798c, not conventional half-up rounding.
}
std::int32_t scaled_select_dimension(std::int32_t dimension,std::int32_t scale) {
    const auto numerator=wrap(static_cast<std::int64_t>(dimension)*100);
    if(scale==0)return 0;
    if(numerator==std::numeric_limits<std::int32_t>::min() && scale==-1)return numerator;
    return numerator/scale;
}
// SelectForm.SetCameraPosition 0x11f32b4. Keep repeated dimension queries and partial writes.
void set_select_camera_position(SelectCameraState& state,std::int32_t x,std::int32_t y,SelectCameraHost& host) {
    const auto right=state.right_extent;
    const auto width=host.game_width();
    const auto first_view_width=host.view_width();
    const auto second_view_width=host.view_width();
    const auto minimum_x=wrap(static_cast<std::int64_t>(width)-right-first_view_width+second_view_width);
    const auto maximum_x=wrap(-static_cast<std::int64_t>(state.left_extent));
    state.x=camera_clamp(x,minimum_x,maximum_x);
    const auto first_height=host.game_height();
    const auto first_view_height=host.view_height();
    const auto second_height=host.game_height();
    const auto second_view_height=host.view_height();
    const auto minimum_y=wrap(static_cast<std::int64_t>(first_height)-first_view_height-80);
    const auto maximum_y=wrap(static_cast<std::int64_t>(second_height)-second_view_height+50);
    state.y=camera_clamp(y,minimum_y,maximum_y);
}
// SelectForm.MoveCursor 0x11ef0dc. Sub-unit velocities may remain indefinitely.
void move_select_camera(SelectCameraState& state,SelectCameraHost& host) {
    if(state.dragging)return;
    if(convert(state.velocity_x)==0 && convert(state.velocity_y)==0)return;
    decay(state.velocity_x);
    decay(state.velocity_y);
    const auto x=wrap(static_cast<std::int64_t>(state.x)+convert(state.velocity_x));
    const auto y=wrap(static_cast<std::int64_t>(state.y)+convert(state.velocity_y));
    set_select_camera_position(state,x,y,host);
}
}
