#include "stage_menu.hpp"
#include "select_camera.hpp"
#include "combat_rules.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(const std::shared_ptr<T>& value){
    if(!value)throw std::invalid_argument("missing stage-menu data");return *value;
}
void require_object(const EventObject& value){if(!value)throw std::invalid_argument("missing stage-menu object");}
template<class T>T& record(std::vector<std::optional<T>>& rows,std::int32_t index){
    auto& value=rows.at(static_cast<std::size_t>(index));
    if(!value)throw std::invalid_argument("missing stage-menu record");return *value;
}
std::int32_t increment(std::int32_t value){
    return value==std::numeric_limits<std::int32_t>::max()?std::numeric_limits<std::int32_t>::min():value+1;
}
std::int32_t size32(std::size_t size){
    if(size>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("stage-menu list exceeds int32");
    return static_cast<std::int32_t>(size);
}
constexpr std::array<std::array<std::int32_t,2>,2> labels{{{{18,3}},{{0,0}}}};
}
void change_stage_menu_state(StageMenuState& state,std::int32_t value,StageMenuHost& host){
    const auto row=labels.at(static_cast<std::size_t>(value));
    host.set_soft_labels(row[0],row[1]);state.state=value;
}
bool initialize_stage_menu(StageMenuState& state,StageMenuHost& host){
    host.base_initialize();host.set_background_white();host.prepare_resources();
    state.frame=0;state.selected=-1;state.loop_counter=0;
    const auto save=host.save_integers();state.area=required(save).at(8);state.production_frame=0;
    change_stage_menu_state(state,0,host);host.system_flag_on(0x100U);
    state.entries=std::make_shared<EventIntegers>();
    const auto areas=host.areas();auto& area=record(required(areas),state.area);
    auto index=size32(required(area.stages).size())-1;
    for(;index>=0;--index){
        const auto stages=area.stages;const auto id=required(stages).at(static_cast<std::size_t>(index));
        if(id==-1)continue;
        if(!host.debug()){
            const auto quests=host.quests();const auto& quest=record(required(quests),id);
            if((static_cast<std::uint32_t>(quest.stage.flags)&2U)!=0)continue;
        }
        const auto quests=host.quests();const auto& quest=record(required(quests),id);
        if(quest.discovery_state==1)required(state.entries).push_back(id);
    }
    const auto count=size32(required(state.entries).size());
    const StageMenuScrollParameters parameters{{1,3,count,count,40,5,0x20000,0},true,{}};
    const auto scroll=host.make_scroll(parameters);require_object(scroll);
    host.scroll_element_size(scroll,210,40);host.scroll_touch_flags(scroll,17);
    state.scroll=scroll;state.renderer=host.make_renderer();return true;
}
bool update_stage_menu(StageMenuState& state,StageMenuHost& host,std::array<std::int32_t,2> diagnostic){
    state.error_function=diagnostic[0];state.error_line=diagnostic[1];host.base_update();
    if(state.state==1){
        state.production_frame=increment(state.production_frame);
        if(state.production_frame<41)return true;
        const auto quests=host.quests();
        const auto entries=state.entries;const auto id=required(entries).at(static_cast<std::size_t>(state.selected));
        // Native production validates the catalog index but does not use/require its row.
        static_cast<void>(required(quests).at(static_cast<std::size_t>(id)));
        try{host.save_game(0,true);host.save_system();}
        catch(...){if(!host.recover_save_exception(std::current_exception()))throw;}
        host.reset_town_camera();host.change_to_quest();return true;
    }
    if(state.loop_counter==0)host.reset_image_clocks();
    host.advance_image_clocks();host.update_main_blur();
    state.frame=increment(state.frame);
    state.loop_counter=increment(state.loop_counter)%std::numeric_limits<std::int32_t>::max();
    require_object(state.scroll);host.update_scroll(state.scroll,true);
    if(host.pressed(0x100000U)){
        const auto quests=host.quests();
        const auto entries=state.entries;const auto id=required(entries).at(static_cast<std::size_t>(state.selected));
        auto& quest=record(required(quests),id);
        host.play_button_sound();
        const auto save=host.save_integers();required(save).at(9)=quest.id;
        const auto cost=quest.need_stamina;
        if(host.stamina()<cost)host.push_form(host.make_stamina_dialog());
        else{host.use_stamina(cost);change_stage_menu_state(state,1,host);}
    }
    if(host.soft_label_pressed(18)){
        host.play_button_sound();host.push_form(host.make_formation());
    }
    if(host.soft_label_pressed(3)){host.play_button_sound();return host.pop_form();}
    host.play_bgm();return true;
}
// SelectStageForm.SetTouchValue/GetTouchValue0x120960c/0x1209638.
void set_stage_menu_touch_value(StageMenuState& state,std::uint32_t flags,std::int32_t value,StageMenuHost& host){
    flags&=0xffff0000U;
    if(flags==0x20000U)state.selected=value;
    else if(flags==0x40000U)state.scroll_value=value;
    else host.base_set_touch_value(flags,value);
}
std::int32_t get_stage_menu_touch_value(const StageMenuState& state,std::uint32_t flags,std::int32_t fallback,StageMenuHost& host){
    flags&=0xffff0000U;
    if(flags==0x20000U)return state.selected;
    if(flags==0x40000U)return state.scroll_value;
    return host.base_get_touch_value(flags,fallback);
}
// SelectStageForm.OnTouchEvent0x1209664.
bool stage_menu_touch(StageMenuState& state,const StageMenuTouch* event,StageMenuHost& host){
    if(!event||!event->input.component)throw std::invalid_argument("missing stage-menu touch");
    const auto group=event->input.component->group;
    if(group==17){if(state.scroll)host.scroll_camera_touch(state.scroll,event->input.type,event->input.component->value,*event);return false;}
    if(group==10&&state.scroll)host.scroll_list_touch(state.scroll,event->input.type,event->input.component->value,*event);
    return true;
}
bool resume_stage_menu(StageMenuHost& host){host.base_resume();return true;}
bool finish_stage_menu(StageMenuHost& host){host.base_finish();host.end_main_blur();return true;}
void update_neutral_stage_scroll(StageNeutralScroll& state,bool repeat_enabled,bool surface_active,
    const std::function<bool(std::uint32_t,bool)>& key){
    const auto wrap=[](std::uint32_t bits){return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):-1-static_cast<std::int32_t>(0xffffffffU-bits);};
    if(!surface_active)state.damping=wrap(static_cast<std::uint32_t>(state.damping)*70U)/100;
    state.width=210;state.height=130;
    if(repeat_enabled)for(auto mask:{0x80000U,0x20000U,0x40000U,0x10000U})
        if(key(mask,true))throw std::logic_error("stage scroll keyboard navigation unbound");
    const auto absolute=state.offset_y<0?wrap(0U-static_cast<std::uint32_t>(state.offset_y)):state.offset_y;
    state.first_visible=camera_clamp(absolute/45,0,wrap(static_cast<std::uint32_t>(state.count)-3U));
    const auto minimum=state.count<3?0:wrap(45U*(3U-static_cast<std::uint32_t>(state.count)));
    state.offset_y=camera_clamp(state.offset_y,minimum,0);
    if(key(0x80000U,false)||key(0x20000U,false))throw std::logic_error("stage scroll held navigation unbound");
    state.fade_progress=std::min(increment(state.fade_progress),10);
    state.color=easing(255,128,10,state.fade_progress,-80);
}
} // namespace saga::core
