#include "select_prelude.hpp"
#include "select_event_pass.hpp"
#include <limits>
#include <stdexcept>
namespace saga::core {
namespace {
template<class T> T& required(const std::shared_ptr<T>& value) {
    if (!value) throw std::invalid_argument("null image animation data");
    return *value;
}
void visit_animations(const ImageAnimationValues& values,bool reset) {
    for (const auto& group:values) {
        for (const auto& row:required(group)) {
            auto& tick=required(row).at(1);
            tick=reset ? 0 : tick==std::numeric_limits<std::int32_t>::max()
                ? std::numeric_limits<std::int32_t>::min() : tick+1;
        }
    }
}
}
// AppData.ResetImageAnimation / UpdateImageAnimations, 0x10ffac0 / 0x10ffc6c.
void reset_image_animations(const ImageAnimationValues& values) {visit_animations(values,true);}
void advance_image_animations(const ImageAnimationValues& values) {visit_animations(values,false);}
// MyFormBase.Update 0x11d1684: increment survives later missing-save failures.
bool update_my_form_base(std::int32_t& counter,SelectPreludeHost& host) {
    counter=advance_select_loop_counter(counter);
    if (host.saved_orientation()==-1) {
        const auto enabled=host.enable_landscape();
        host.set_orientation(enabled ? -1 : 0); // ARM64 SBFX of boolean bit 0.
    }
    // get_IsLandspace_ reads GameView.GetGameWidth and compares >240.
    // Its result is discarded here, but the required view access must still happen.
    static_cast<void>(host.game_width());
    return true;
}
// SelectForm.Update 0x11ebd7c..0x11ebde4.
SelectPreludeSnapshot update_select_prelude(SelectPreludeState& state,const std::int32_t& loop_counter,SelectPreludeHost& host) {
    state.error_function=1;state.error_line=0;
    update_my_form_base(state.update_counter,host);
    const auto reset=loop_counter==0;
    const auto camera=state.camera_x;
    const auto view=host.view_x();
    if (reset) reset_image_animations(required(host.image_animation_values()));
    advance_image_animations(required(host.image_animation_values()));
    return {camera,view};
}
}
