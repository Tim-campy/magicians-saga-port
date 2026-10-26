#pragma once
#include "event_flow.hpp"
namespace saga::core {
using ImageAnimationGroup = std::vector<std::shared_ptr<EventIntegers>>;
using ImageAnimationValues = std::vector<std::shared_ptr<ImageAnimationGroup>>;
// Ordered dictionary values, supplied by the host; membership is stable during each pass.
void reset_image_animations(const ImageAnimationValues&);
void advance_image_animations(const ImageAnimationValues&);
struct SelectPreludeState {
    std::int32_t error_function;
    std::int32_t error_line;
    std::int32_t update_counter; // MyFormBase.update_, separate from loopCounter_.
    std::int32_t camera_x;
};
struct SelectPreludeSnapshot {std::int32_t camera_x;std::int32_t view_x;};
class SelectPreludeHost {
public:
    virtual ~SelectPreludeHost() = default;
    virtual std::int32_t saved_orientation() = 0;
    // May change the orientation-unlock save flag; not a pure geometry query.
    virtual bool enable_landscape() = 0;
    virtual void set_orientation(std::int32_t value) = 0;
    virtual std::int32_t game_width() = 0;
    virtual std::int32_t view_x() = 0;
    // Return values in original dictionary traversal order; preserve row aliases.
    virtual std::shared_ptr<ImageAnimationValues> image_animation_values() = 0;
};
bool update_my_form_base(std::int32_t& update_counter,SelectPreludeHost&);
SelectPreludeSnapshot update_select_prelude(SelectPreludeState&,const std::int32_t& loop_counter,SelectPreludeHost&);
}
