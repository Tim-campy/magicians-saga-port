#include "mission_profile.hpp"
#include <functional>

namespace saga::desktop {
namespace {
struct AttackControls final:core::StageControlsHost {
    std::int32_t width,height;BitmapBackend* backend;
    std::function<void(core::InputRectangle,std::int32_t)> add;
    AttackControls(std::int32_t w,std::int32_t h,BitmapBackend* b,decltype(add) a):width(w),height(h),backend(b),add(std::move(a)){}
    std::int32_t controls_width()override{return width;}
    std::int32_t controls_height()override{return height;}
    void controls_draw_seb(std::int32_t resource,std::int32_t clip,std::optional<std::int32_t> frame,std::int32_t x,std::int32_t y)override{
        if(backend)backend->draw_seb(resource,clip,frame,x,y);
    }
    void controls_set_render_mode(std::int32_t mode,std::int32_t alpha)override{if(backend)backend->set_render_mode(mode,alpha);}
    void controls_add_attack(std::int32_t x,std::int32_t y,std::int32_t w,std::int32_t h,std::int32_t value,std::array<std::int32_t,4> margins)override{
        add(core::input_screen_rectangle({x,y,w,h},margins,width,height),value);
    }
    void controls_move_button(std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::array<std::int32_t,4>)override{
        throw std::logic_error("movement SEB widgets are unbound");
    }
};
}
void MissionProfile::draw_attack_controls(BitmapBackend* backend){
    attack_regions_.clear();
    if(!attack_widgets_enabled()||!system_states_[3].enabled)return;
    // Stage.Draw state1, physical GameView coordinates, outside DrawOffscreen.
    // SAGA_STUB(mission-stage-hud): remaining Stage.Draw overlays/controls are unbound.
    AttackControls host(view.width,view.height,backend,[this](core::InputRectangle rectangle,std::int32_t value){
        // TOUCHPARAMS[10].COLLISION=1; all three controls have priority8.
        for(auto& previous:attack_regions_)core::partition_input_rectangles(previous.bounds,rectangle);
        attack_regions_.push_back({rectangle,{10,value}});
    });
    humans->draw_attack_controls(host);
}
bool MissionProfile::attack_point(std::int32_t x,std::int32_t y){
    draw_attack_controls();
    if(talk_pending()||!humans||humans->stage_modal_pending()||humans->transition_pending()){
        ++attack_point_rejected_;return false;
    }
    for(auto& region:attack_regions_)if(core::input_hit_rectangle(region.bounds,x,y)){
        core::QuestFormTouch event{{2,&region.control},0,0,0};
        spatial->form_touch(quest_frame_state_,event);++attack_point_events_;return true;
    }
    ++attack_point_rejected_;return false;
} // Desktop normalized click; native Surface down/start/drag/cancel queues remain unbound.
}
