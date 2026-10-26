#pragma once
#include "event_flow.hpp"
namespace saga::core {
using TownHumanRows = std::vector<std::shared_ptr<EventIntegers>>;
struct SelectTouchMoveState {
    bool enabled;
    float witch_x;
    float drag_distance;
    std::int32_t direction;
    std::int32_t collision_id;
};
class SelectTouchMoveHost {
public:
    virtual ~SelectTouchMoveHost() = default;
    virtual std::shared_ptr<TownHumanRows> town_humans() = 0;
    virtual void set_talk_bubble(std::int32_t index,bool option) = 0;
};
std::int32_t check_town_human_collision(const float& witch_x,SelectTouchMoveHost&);
// SelectForm.Update 0x11ec7ec..0x11ec944. Does not consume raw touch input.
void update_select_touch_move(SelectTouchMoveState&,SelectTouchMoveHost&);
}
