#include "select_form_pass.hpp"
#include "entity_visual.hpp"
#include <stdexcept>
#include <limits>
namespace saga::core {
namespace {
std::int32_t background_add(std::int32_t a,std::int32_t b){
    const auto bits=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):-1-static_cast<std::int32_t>(0xffffffffU-bits);
}
std::int32_t scenery_sub(std::int32_t a,std::int32_t b){return background_add(background_add(a,~b),1);}
template<class T>T& scenery_required(const std::shared_ptr<T>& value){
    if(!value)throw std::invalid_argument("missing SelectForm scenery data");return *value;
}
std::int32_t scenery_count(const EventScript& rows){
    const auto count=scenery_required(rows).size();
    if(count>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("scenery list exceeds int32");
    return static_cast<std::int32_t>(count);
}
}
SelectBackgroundOrigin draw_select_background(const SelectBackgroundState& state,SelectBackgroundHost& host){
    const auto pixel=host.background_pixel();
    host.background_color(factory_cell_coordinate(pixel[0]*255.0F,1),factory_cell_coordinate(pixel[1]*255.0F,1),factory_cell_coordinate(pixel[2]*255.0F,1),255);
    const auto width=host.background_width();const auto height=host.background_height();
    host.fill_background(width,height);
    const auto base_x=background_add(state.camera_x,state.view_x?*state.view_x:0);
    const auto base_y=background_add(state.camera_y,state.view_y);
    auto x=state.left_extent;
    SelectBackgroundResource resource=nullptr;
    for(;;){
        const auto limit=state.right_extent;
        resource=host.background_resource();
        if(x>=limit)break;
        const auto view_height=host.background_view_height();
        if(!resource)throw std::invalid_argument("missing select background resource");
        const auto clip=host.background_clip();
        const auto y=background_add(background_add(base_y,-125),view_height);
        host.draw_background(resource,clip,0,static_cast<float>(x),static_cast<float>(y));
        x=background_add(x,240);
    }
    const auto parallax_x=background_add(state.camera_x/30,30);
    if(!resource)throw std::invalid_argument("missing select background resource");
    const auto clip=host.background_clip();
    host.draw_background(resource,clip,1,static_cast<float>(parallax_x),static_cast<float>(background_add(base_y,20)));
    return {base_x,base_y};
}
void initialize_select_scenery_clouds(SelectSceneryState& state,SelectSceneryHost& host){
    scenery_required(state.clouds).clear();
    const auto clip=host.scenery_clip(0x19c);
    for(std::int32_t x=-15;x<host.cloud_right_edge();){
        const auto height=host.raw_view_height();
        const auto y=host.lib_random(0,background_add(height,-170));
        const auto variant=host.app_random(4);
        const auto destination=state.clouds;
        auto row=std::make_shared<EventIntegers>(EventIntegers{x,y,clip,0,1,variant,0,20});
        scenery_required(destination).push_back(std::move(row));
        x=background_add(x,host.app_random(20));
    }
}
EventInstruction generate_select_bird(SelectSceneryState& state,bool initial,SelectSceneryHost& host){
    const auto direction=host.app_random(2);
    auto row=std::make_shared<EventIntegers>(6,0);
    row->at(0)=host.app_random(3);
    const auto edge=direction?state.left_extent:state.right_extent;
    const auto offset=host.app_random(60);
    row->at(1)=direction?scenery_sub(edge,offset):background_add(edge,offset);
    const auto speed=host.app_random(3);
    row->at(4)=direction?background_add(speed,1):~speed;
    if(initial){const auto left=background_add(state.left_extent,50);const auto right=background_add(state.right_extent,-50);
        row->at(1)=host.lib_random(left,right);}
    const auto height=host.raw_view_height();row->at(2)=host.lib_random(20,background_add(height,-170));
    row->at(3)=host.app_random(10);row->at(5)=background_add(host.app_random(3),-1);
    return row;
}
EventInstruction generate_select_leaf(SelectSceneryState& state,bool initial,SelectSceneryHost& host){
    auto row=std::make_shared<EventIntegers>(6,0);row->at(0)=host.app_random(3);
    const auto left=state.left_extent;const auto right=state.right_extent;
    row->at(1)=host.lib_random(left,right);
    const auto first_height=host.raw_view_height();const auto second_height=host.raw_view_height();
    const auto offset=host.lib_random(60);
    const auto scaled=static_cast<std::uint32_t>(first_height)*100U;
    const auto numerator=scaled<=0x7fffffffU?static_cast<std::int32_t>(scaled):-1-static_cast<std::int32_t>(0xffffffffU-scaled);
    const auto base_y=background_add(numerator/-80,second_height);
    row->at(2)=scenery_sub(base_y,offset);
    if(initial){const auto height=host.raw_view_height();row->at(2)=host.lib_random(base_y,height);}
    row->at(3)=host.app_random(10);row->at(4)=0;row->at(5)=0;return row;
}
void initialize_select_decorations(SelectSceneryState& state,SelectSceneryHost& host){
    state.birds=std::make_shared<std::vector<EventInstruction>>();
    state.leaves=std::make_shared<std::vector<EventInstruction>>();
    const auto birds=host.app_random(4);
    for(std::int32_t i=0;i<birds;++i){const auto destination=state.birds;
        auto row=generate_select_bird(state,false,host);scenery_required(destination).push_back(std::move(row));}
    const auto leaves=background_add(host.app_random(25),5);
    for(std::int32_t i=0;i<leaves;++i){const auto destination=state.leaves;
        auto row=generate_select_leaf(state,true,host);scenery_required(destination).push_back(std::move(row));}
}
void draw_select_decorations(SelectSceneryState& state,std::int32_t type,SelectBackgroundOrigin origin,SelectSceneryHost& host){
    if(type!=0&&type!=2)return;
    if(type==0)origin.x/=20;
    const auto list=[&](){return type==0?state.birds:state.leaves;};
    for(std::int32_t i=0;i<scenery_count(list());++i){
        const auto row=scenery_required(list()).at(static_cast<std::size_t>(i));
        const auto x=background_add(scenery_required(row).at(1),origin.x);
        const auto y=background_add(row->at(2),origin.y);
        if(!check_select_view_area(x,y,30,30,host))continue;
        if(row->at(5)<0)host.scenery_reverse(1);
        const auto resource=host.scenery_resource();
        const auto rx=row->at(1);const auto ry=row->at(2);
        if(!resource)throw std::invalid_argument("missing SelectForm scenery resource");
        const auto clip=host.scenery_clip(type==0?0x1bc:0x1c0);
        const auto id=row->at(0);const auto frame=type==0?row->at(3):row->at(3)/16;
        host.scenery_draw_indexed(resource,clip,frame,static_cast<float>(background_add(rx,origin.x)),static_cast<float>(background_add(ry,origin.y)),id,2);
        if(row->at(5)<0)host.scenery_reverse(0);
    }
}
void draw_select_terrain(SelectSceneryState& state,SelectBackgroundOrigin origin,SelectSceneryHost& host){
    const auto first_bounds=host.scenery_bounds(host.scenery_clip(0x174),0);
    const auto width=first_bounds[2];const auto limit=background_add(state.right_extent,width);
    for(auto tile=state.left_extent;tile<limit;tile=background_add(tile,width)){
        const auto x=background_add(origin.x/2,tile);const auto height=host.game_height();
        if(!check_select_view_area(x,background_add(height,-1),background_add(width,5),0,host))continue;
        const auto resource=host.scenery_resource();const auto raw_height=host.raw_view_height();
        if(!resource)throw std::invalid_argument("missing SelectForm ground resource");
        const auto clip=host.scenery_clip(0x164);
        host.scenery_draw(resource,clip,3,static_cast<float>(x),static_cast<float>(background_add(background_add(origin.y,-125),raw_height)),0);
    }
    const auto second_bounds=host.scenery_bounds(host.scenery_clip(0x174),0);
    const auto height=host.raw_view_height();const auto floor_y=scenery_sub(height,second_bounds[3]);
    const auto second_width=second_bounds[2];const auto second_limit=background_add(state.right_extent,second_width);
    const auto y=background_add(floor_y,origin.y);
    for(auto tile=state.left_extent;tile<second_limit;tile=background_add(tile,second_width)){
        const auto x=background_add(origin.x,tile);const auto game_height=host.game_height();
        if(!check_select_view_area(x,background_add(game_height,-1),background_add(second_width,5),0,host))continue;
        const auto resource=host.scenery_resource();if(!resource)throw std::invalid_argument("missing SelectForm ground resource");
        const auto clip=host.scenery_clip(0x174);
        host.scenery_draw(resource,clip,1,static_cast<float>(x),static_cast<float>(y),0);
    }
    for(auto i=scenery_count(state.clouds);i>0;--i){
        const auto row=scenery_required(state.clouds).at(static_cast<std::size_t>(i-1));
        const auto x=background_add(scenery_required(row).at(0),origin.x/20);
        const auto game_height=host.game_height();
        if(!check_select_view_area(x,background_add(game_height,-1),80,0,host))continue;
        const auto resource=host.scenery_resource();
        const auto rx=row->at(0);const auto ry=row->at(1);const auto clip=row->at(2);const auto frame=row->at(5);
        if(!resource)throw std::invalid_argument("missing SelectForm cloud resource");
        host.scenery_draw(resource,clip,frame,static_cast<float>(background_add(rx,origin.x/20)),static_cast<float>(background_add(ry,origin.y)),0);
    }
}
SelectUpdatePrefixResult update_select_form_prefix(SelectPreludeState& prelude,SelectPreludeHost& prelude_host,
    SelectFormPassState& state,SelectFormPassHost& host,SelectCloudHost& clouds,SelectMarkerHost& markers,
    EventDelayQueue& queue,EventExecutionHost& execution,EventConditionHost& conditions,
    SelectTouchMoveState& movement,SelectTouchMoveHost& movement_host,SelectTriggerHost& triggers) {
    const auto snapshot=update_select_prelude(prelude,state.events.loop_counter,prelude_host);
    const auto pass=process_select_form_pass(state,host,clouds,markers,queue,execution,conditions);
    if(pass==SelectPassResult::ContinueInput) {
        update_select_touch_move(movement,movement_host);
        process_select_triggers(triggers,queue,execution);
    }
    return {snapshot,pass};
}
// SelectForm.Update, ELF 0x11ebde8..0x11ec7e8, with returned-form Push at 0x11ecb30.
SelectPassResult process_select_form_pass(SelectFormPassState& state,SelectFormPassHost& host,
    SelectCloudHost& clouds,SelectMarkerHost& markers,EventDelayQueue& queue,
    EventExecutionHost& execution,EventConditionHost& conditions) {
    if (consume_dialog_update_gate(state.form)) return SelectPassResult::ReturnTrue;
    if (state.state==4) {
        host.capture();
        return SelectPassResult::ReturnTrue;
    }
    if (state.state==3) {
        if (state.captured) return SelectPassResult::ReturnTrue;
        if (host.capture_input(0x100000U)) {
            host.change_state(4); // Host owns ChangeState's ordered labels and final state write.
            return SelectPassResult::ReturnTrue;
        }
        if (host.soft_label(3)) host.finish_capture();
        host.play_bgm();
        return SelectPassResult::ReturnTrue;
    }
    if (state.state!=0) {
        host.play_bgm();
        return SelectPassResult::ReturnTrue;
    }
    state.events.loop_counter=advance_select_loop_counter(state.events.loop_counter);
    update_select_clouds(state.events.loop_counter,clouds);
    refresh_select_markers(state.markers,markers);
    host.update_notifications();
    if (host.acquire_balance()) return SelectPassResult::ReturnTrue;
    host.set_active_x();
    host.update_decorations();
    host.update_town_humans();
    if (state.events.loop_counter<=2) {
        for (std::int32_t index=0;index<host.town_enable_count();++index) {
            if (host.update_town_enable(index)) {
                // ExecEvent(int) defaults both contexts; its result is discarded here.
                execute_indexed_event(1,{},queue,execution);
            }
        }
        host.check_guide_arrow(true);
    }
    const auto form=process_select_event_pass(state.events,queue,execution,conditions);
    if (form) {
        push_select_form(state.form,form,state.auto_save,state.events.event_check_requested);
        return SelectPassResult::ReturnTrue;
    }
    return SelectPassResult::ContinueInput;
}
}
