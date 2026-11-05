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
void use_stage_stamina(std::int32_t cost,StageStaminaHost& host){
    const auto read=[&](std::size_t slot){return required(host.stamina_values()).at(slot);};
    const auto write=[&](std::size_t slot,std::int64_t value){required(host.stamina_values()).at(slot)=value;};
    const auto limit=[](std::int32_t value){return static_cast<std::int64_t>(static_cast<std::uint32_t>(value));};
    const auto before=read(1);
    if(before>=limit(host.stamina_limit())){
        // Native captures the destination before GetServerTime, then validates it.
        const auto values=host.stamina_values();
        const auto time=host.server_time();
        required(values).at(5)=time;
    }
    const auto values=host.stamina_values();
    auto& balance=required(values).at(1);
    const auto bits=static_cast<std::uint64_t>(balance)-static_cast<std::uint64_t>(static_cast<std::int64_t>(cost));
    balance=bits<=0x7fffffffffffffffULL?static_cast<std::int64_t>(bits)
        :-1-static_cast<std::int64_t>(0xffffffffffffffffULL-bits);
    if(read(1)<0)write(1,0);
    const auto upper=limit(host.stamina_limit());
    if(read(1)>upper){
        // The third limit query can replace the current save array; store to the captured one.
        const auto destination=host.stamina_values();
        const auto replacement=host.stamina_limit();
        required(destination).at(1)=replacement;
    }
    if(read(0)>=901)write(0,900);
    if(cost>=1&&read(0)<=0)write(0,900);
    if(read(0)<0)write(0,0);
}
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
namespace {
std::int32_t draw_wrap(std::uint32_t value){
    return value<=0x7fffffffU?static_cast<std::int32_t>(value):-1-static_cast<std::int32_t>(0xffffffffU-value);
}
std::int32_t draw_add(std::int32_t a,std::int32_t b){return draw_wrap(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t draw_sub(std::int32_t a,std::int32_t b){return draw_wrap(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t draw_multiply(std::int32_t a,std::int32_t b){return draw_wrap(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
}
void touch_stage_scroll_list(StageNeutralScroll& scroll,std::int32_t type,const StageMenuTouch* event){
    if(type!=0&&type!=2)return;
    if(!event||!event->input.component)throw std::invalid_argument("missing stage-list touch");
    scroll.last_touched=static_cast<std::uint32_t>(event->input.component->value)&0xffffU;
}
void draw_stage_menu_contents(const StageMenuState& state,std::int32_t index,std::int32_t,
                              std::int32_t y,StageMenuDrawHost& host){
    const auto areas=host.areas();
    // Native only validates the area index here; it does not dereference the row.
    static_cast<void>(required(areas).at(static_cast<std::size_t>(state.area)));
    host.validate_row_bounds();
    const auto center=host.center_x();
    const auto quests=host.quests();
    const auto entries=state.entries;
    const auto id=required(entries).at(static_cast<std::size_t>(index));
    // Native captures the nullable quest before drawing the row background/color.
    auto& quest=required(quests).at(static_cast<std::size_t>(id));
    host.draw_row_background(draw_sub(center,105),y);
    host.set_row_color();
    if(!quest)throw std::invalid_argument("missing stage-menu record");
    host.draw_quest_name(quest->name,draw_sub(center,80),draw_add(y,14));
    host.draw_level(quest->stage.base_level,draw_add(center,5),draw_add(y,14));
    if(quest->stage.completed!=0)host.draw_completed(draw_add(center,61),draw_add(y,21));
    else{
        const auto maximum=host.pending_max_frame();
        // ARM64 SDIV by zero produces zero; MSUB then retains the dividend.
        const auto clock=state.frame/4;
        const auto frame=maximum==0?clock:clock==std::numeric_limits<std::int32_t>::min()&&maximum==-1?0:clock%maximum;
        host.draw_pending(frame,draw_add(center,64),draw_add(y,13));
    }
}
void draw_stage_menu_list(const StageMenuState& state,StageNeutralScroll& scroll,std::int32_t x,
                          std::int32_t y,StageMenuDrawHost& host){
    const auto height=host.raw_height();require_object(state.scroll);
    if(draw_sub(height,42)<draw_add(scroll.height,y))
        scroll.maximum_height=draw_sub(draw_sub(host.raw_height(),y),42);
    const auto element_width=scroll.width;
    const auto area_height=scroll.height;
    const auto offset=scroll.offset_y;
    host.draw_scroll_bar({draw_add(draw_add(x,element_width),3),y,6,std::min(area_height,scroll.maximum_height)},
                         scroll.count,3,scroll.first_visible,scroll.color);
    const auto first=host.first_camera_touch();
    host.add_camera_touch({first?InputRectangle{-1000,-1000,2000,2000}:
        InputRectangle{x,y,element_width,std::min(area_height,scroll.maximum_height)},
        {17,0},1.0F,1.0F,first?32:1});
    host.push_clip({x,y,element_width,std::min(draw_add(area_height,1),scroll.maximum_height)});
    for(std::int32_t index=0;index<scroll.count;index=increment(index)){
        const auto row_y=draw_add(draw_add(y,offset),draw_multiply(index,45));
        if(row_y>draw_add(y,area_height)||draw_add(row_y,40)<y)continue;
        require_object(state.renderer);
        draw_stage_menu_contents(state,index,x,row_y,host);
        const auto top=draw_sub(y,2);
        const auto relative_top=draw_sub(row_y,top);
        const auto overflow=std::max(draw_sub(draw_sub(draw_add(row_y,40),draw_add(y,
            std::min(area_height,scroll.maximum_height))),2),0);
        const auto hit_height=relative_top>=0?draw_sub(40,overflow):draw_add(40,relative_top);
        if(hit_height>=1)host.add_row_touch({{x,relative_top>=0?row_y:top,element_width,hit_height},
            {10,draw_wrap(0x20000U|static_cast<std::uint32_t>(index))},17,1,{0,0,0,0}});
    }
    host.pop_clip();
}
void draw_stage_menu(const StageMenuState& state,StageNeutralScroll& scroll,StageMenuDrawHost& host){
    host.begin_stage_draw();
    const auto origin=host.draw_origin();
    const auto width=host.raw_width();const auto height=host.raw_height();
    host.set_draw_origin(draw_sub(width,240)/2,draw_sub(height,240)/2);
    host.draw_heading(host.center_x());
    draw_stage_menu_list(state,scroll,15,32,host);
    if(state.selected!=-1){
        host.draw_selected_details(state);
        // The native unselected branch does not restore Graphics origin.
        host.set_draw_origin(origin[0],origin[1]);
    }
    host.end_stage_draw(state);
}
} // namespace saga::core
