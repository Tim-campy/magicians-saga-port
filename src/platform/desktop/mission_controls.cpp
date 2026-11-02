#include "mission_profile.hpp"
#include <functional>

namespace saga::desktop {
namespace {
struct MissionControls final:core::StageControlsHost {
    MissionProfile& profile;std::int32_t width,height;BitmapBackend* backend;
    std::function<void(core::InputRectangle,std::int32_t)> add;
    std::function<bool(core::InputControl*)> pressed;
    std::function<void(core::InputRectangle,core::InputControl*,std::uint32_t,std::int32_t)> move;
    MissionControls(MissionProfile& p,BitmapBackend* b,decltype(add) a,decltype(pressed) down,decltype(move) m)
        :profile(p),width(p.view.width),height(p.view.height),backend(b),add(std::move(a)),pressed(std::move(down)),move(std::move(m)){}
    std::int32_t controls_width()override{return width;}
    std::int32_t controls_height()override{return height;}
    void controls_draw_seb(std::int32_t resource,std::int32_t clip,std::optional<std::int32_t> frame,std::int32_t x,std::int32_t y)override{
        if(backend)backend->draw_seb(resource,clip,frame,x,y);
    }
    void controls_set_render_mode(std::int32_t mode,std::int32_t alpha)override{if(backend)backend->set_render_mode(mode,alpha);}
    void controls_add_attack(std::int32_t x,std::int32_t y,std::int32_t w,std::int32_t h,std::int32_t value,std::array<std::int32_t,4> margins)override{
        add(core::input_screen_rectangle({x,y,w,h},margins,width,height),value);
    }
    void controls_move_button(std::int32_t group,std::int32_t x,std::int32_t y,std::int32_t clip,std::int32_t key,std::int32_t anchor,std::array<std::int32_t,4> margins)override{
        if(group!=35||anchor!=17||(key!=0x20000&&key!=0x80000))throw std::logic_error("unsupported movement widget contract");
        const auto value=0x10000|(key==0x20000?17:19);auto* control=profile.humans->widget_control(value);
        if(!control)throw std::logic_error("missing stable movement control");
        const auto frame=pressed(control)?1:0;
        auto* resource=core::get_application_resource(profile.application,7);
        if(!resource||!resource->animations||!resource->animations->clips)throw std::invalid_argument("missing movement resource7");
        auto* clock=resource->animations->clips->at(static_cast<std::size_t>(clip));
        if(!clock||!clock->sprites)throw std::invalid_argument("missing movement SEB");
        // ButtonSystem.DrawButton reads bounds before drawing, then GameView.AddTouch reads again.
        static_cast<void>(core::seb_bounding_rect(*clock->sprites,frame));
        if(backend){core::ImageAnimationGroup* clocks=nullptr;
            for(const auto& owner:profile.owners)if(&owner->manager.drawing()==resource){clocks=owner->image_clocks.get();break;}
            backend->draw_seb_button(7,clip,frame,x,y,clocks);
        }
        const auto bounds=core::seb_bounding_rect(*clock->sprites,frame);
        const auto add_word=[](std::int32_t a,std::int32_t b){const auto v=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);};
        move(core::input_screen_rectangle({add_word(x,bounds[0]),add_word(y,bounds[1]),bounds[2],bounds[3]},margins,width,height),control,static_cast<std::uint32_t>(key),frame);
    }
};
}
void MissionProfile::draw_stage_controls(BitmapBackend* backend){
    attack_regions_.clear();move_regions_.clear();
    if(movement_widgets_enabled())humans->bind_movement_widgets(false,false);
    if(!attack_widgets_enabled()||!system_states_[3].enabled)return;
    // Stage.Draw state1, physical GameView coordinates, outside DrawOffscreen.
    // SAGA_STUB(mission-stage-hud): remaining Stage.Draw overlays/controls are unbound.
    MissionControls host(*this,backend,[this](core::InputRectangle rectangle,std::int32_t value){
        // TOUCHPARAMS[10].COLLISION=1; all three controls have priority8.
        for(auto& previous:attack_regions_)core::partition_input_rectangles(previous.bounds,rectangle);
        attack_regions_.push_back({rectangle,{10,value}});
    },[this](core::InputControl* control){return held_move_control_==control;},
    [this](core::InputRectangle rectangle,core::InputControl* control,std::uint32_t key,std::int32_t frame){
        // Group35's own collision bit and priority0 partition its movement controls, not group10.
        for(auto& previous:move_regions_)core::partition_input_rectangles(previous.bounds,rectangle);
        move_regions_.push_back({rectangle,control,key,frame});
    });
    humans->draw_attack_controls(host);
    if(movement_widgets_enabled()){
        humans->draw_move_controls(host);bool front=false,back=false;
        for(const auto& region:move_regions_){if(region.control->value==0x10011)front=true;if(region.control->value==0x10013)back=true;}
        humans->bind_movement_widgets(front,back);
    }
}
bool MissionProfile::attack_point(std::int32_t x,std::int32_t y){
    draw_stage_controls();
    if(talk_pending()||!humans||humans->stage_modal_pending()||humans->transition_pending()){
        ++attack_point_rejected_;return false;
    }
    for(auto& region:attack_regions_)if(core::input_hit_rectangle(region.bounds,x,y)){
        core::QuestFormTouch event{{2,&region.control},0,0,0};
        spatial->form_touch(quest_frame_state_,event);++attack_point_events_;return true;
    }
    ++attack_point_rejected_;return false;
} // Desktop normalized click; native Surface down/start/drag/cancel queues remain unbound.
bool MissionProfile::control_pointer(std::int32_t type,std::int32_t x,std::int32_t y){
    if(type!=0&&type!=1)throw std::invalid_argument("control pointer requires down0 or release1");
    if(type==1){
        if(!held_move_control_)return false;
        // Desktop source arbitration: releasing one source must not release a control still held by the other.
        if(!humans->keyboard_holds_widget(held_move_key_)){
            core::QuestFormTouch event{{1,held_move_control_},0,0,0};spatial->form_touch(quest_frame_state_,event);}
        humans->widget_key(1,held_move_key_);held_move_control_=nullptr;held_move_key_=0;++move_pointer_events_;return true;
    }
    if(held_move_control_)return false;
    draw_stage_controls();
    if(!humans||talk_pending()||humans->stage_modal_pending()||humans->transition_pending())return false;
    for(const auto& region:move_regions_)if(core::input_hit_rectangle(region.bounds,x,y)){
        held_move_control_=region.control;held_move_key_=region.key;
        core::QuestFormTouch event{{2,held_move_control_},0,0,0};spatial->form_touch(quest_frame_state_,event);
        humans->widget_key(2,held_move_key_);++move_pointer_events_;return true;
    }
    return attack_point(x,y);
} // Desktop single-pointer capture retains the selected control until release, including outside the window.
void MissionProfile::cancel_control_pointer(){
    if(!held_move_control_)return;
    if(!humans->keyboard_holds_widget(held_move_key_)){
        core::QuestFormTouch event{{3,held_move_control_},0,0,0};spatial->form_touch(quest_frame_state_,event);}
    humans->widget_key(3,held_move_key_);held_move_control_=nullptr;held_move_key_=0;++move_pointer_events_;++move_pointer_cancellations_;
}
}
