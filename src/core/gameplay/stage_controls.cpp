#include "stage_controls.hpp"
#include "combat_rules.hpp"
#include "ecs_combat.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing stage control value");return *p;}
std::int32_t word(std::uint32_t value){return static_cast<std::int32_t>(value<=0x7fffffffU?static_cast<std::int64_t>(value):static_cast<std::int64_t>(value)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
}
void draw_stage_attack_controls(StageStartState& state,StageControlsHost& host){
    const auto width=host.controls_width(),height=host.controls_height();
    const auto base_x=add(width,-44);auto y=add(height,-180);
    auto buttons=state.attack_buttons;required(buttons.get());
    std::int32_t index=0,frame=0;
    while(static_cast<std::size_t>(index)<buttons->size()){
        auto& button=required(buttons->at(static_cast<std::size_t>(index)));
        std::int32_t offset=0,alpha=0;
        if(button.state==0){
            offset=easing(-16,0,4,button.frame,80);
            alpha=easing(255,0,4,button.frame,80);
        }else if(button.state==1){
            offset=easing(0,-16,4,button.frame,80);
            alpha=easing(0,255,4,button.frame,80);
        }
        const auto x=add(base_x,offset);
        if(alpha<=254)host.controls_draw_seb(7,0,frame,x,y);
        if(alpha>=1){
            host.controls_set_render_mode(1,alpha);
            host.controls_draw_seb(7,0,add(frame,1),x,y);
            host.controls_set_render_mode(0,255);
        }
        host.controls_add_attack(x,y,60,60,word(0x230000U|static_cast<std::uint32_t>(index)),{10,0,0,0});
        // Native code reloads the array even after registering its last control.
        buttons=state.attack_buttons;required(buttons.get());
        index=add(index,1);y=add(y,55);frame=add(frame,2);
    }
}
void draw_stage_move_controls(StageStartState& state,StageControlsHost& host){
    const auto height=host.controls_height();
    host.controls_move_button(35,2,add(height,-75),36,0x20000,17,{0,10,15,5});
    if(!state.move_front_pushed)host.controls_draw_seb(2,4,std::nullopt,75,add(height,-53));
    if(!state.collection.boss_defeated)host.controls_move_button(35,2,add(height,-38),37,0x80000,17,{0,10,5,15});
}
void draw_stage_player_mp_gauge(StageMpDrawingHost& host){
    const auto width=host.mp_width(),height=host.mp_height();
    auto& player=required(host.mp_player());
    auto& parameter=required(resolve_combat_parameter(player,1));
    const auto context=host.capture_mp_numbers();
    const auto value=add(parameter.value,parameter.extra),maximum=add(parameter.maximum,parameter.extra_maximum);
    const auto text=host.format_mp(value,maximum);
    const auto y=add(height,-2);
    const auto text_width=host.draw_mp_numbers(context,text,add(width,-83),y,0,68);
    const auto x=word(static_cast<std::uint32_t>(width)-static_cast<std::uint32_t>(text_width)-95U);
    host.draw_mp_icon(static_cast<float>(x),static_cast<float>(y),68);
    auto* fresh_player=host.mp_player();
    host.mp_parameter_gauge({fresh_player,width,height,4,1,68,true});
}
}
