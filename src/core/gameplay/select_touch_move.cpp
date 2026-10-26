#include "select_touch_move.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
namespace saga::core {
namespace {
template<class T> T& required(const std::shared_ptr<T>& value) {
    if (!value) throw std::invalid_argument("null town human data");
    return *value;
}
float clamp_step(float value) {
    if(value>6.0F)value=6.0F;
    if(value<1.0F)value=1.0F;
    return value; // Unordered comparisons preserve NaN, matching FCSEL.
}
float integer_position(float value) {
    // FCVTZS Wn,Sn followed by SCVTF, with original +infinity override.
    if (std::isnan(value)) return 0.0F;
    if (value==std::numeric_limits<float>::infinity() || value<=-2147483648.0F) return -2147483648.0F;
    if (value>=2147483648.0F) return static_cast<float>(std::numeric_limits<std::int32_t>::max());
    return static_cast<float>(static_cast<std::int32_t>(value));
}
}
// SelectForm.CheckCollisionTownHuman, 0x11eecdc. First match, live list accesses.
std::int32_t check_town_human_collision(const float& x,SelectTouchMoveHost& host) {
    for (std::int32_t index=0;;++index) {
        const auto rows=host.town_humans();
        const auto size=required(rows).size();
        if(size>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
            throw std::length_error("town human count exceeds int32");
        if(static_cast<std::size_t>(index)>=size)return -1;
        const auto row=rows->at(static_cast<std::size_t>(index));
        if(required(row).at(8)==2)continue;
        const float distance=std::fabs(static_cast<float>(row->at(1))-x);
        if(distance<5.0F)return index;
    }
}
void update_select_touch_move(SelectTouchMoveState& state,SelectTouchMoveHost& host) {
    if(!state.enabled)return;
    const float x=state.witch_x,target=state.drag_distance;
    if(x<target) {
        state.direction=1;
        const float quotient=x/-3.0F;
        const float step=clamp_step(std::fabs(target)+quotient);
        float next=step+x;
        const float limit=std::fabs(target);
        if(next>limit)next=limit;
        state.witch_x=integer_position(next);
    } else if(x>target) {
        state.direction=2;
        const float quotient=x/3.0F;
        const float step=clamp_step(std::fabs(target)+quotient);
        float next=x-step;
        if(next<target)next=target;
        state.witch_x=integer_position(next);
    }
    const auto collision=check_town_human_collision(state.witch_x,host);
    if(collision!=-1 && collision!=state.collision_id)host.set_talk_bubble(collision,false);
    state.collision_id=collision; // Must remain unchanged when bubble creation fails.
}
}
