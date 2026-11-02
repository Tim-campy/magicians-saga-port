#include "select_form_pass.hpp"
#include "entity_visual.hpp"
#include "select_building.hpp"
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
void draw_select_floor(const SelectSceneryState& state,SelectBackgroundOrigin origin,SelectFloorHost& host){
    const auto start=background_add(state.left_extent,40);const auto end=background_add(state.right_extent,-72);
    for(auto tile=start;tile<end;){
        const auto x=background_add(origin.x,tile);const auto height=host.game_height();
        const auto visible=check_select_view_area(x,background_add(height,-1),74,0,host);
        const auto next=background_add(tile,32);
        if(visible){
            auto frame=tile>start?1:0;if(end<next)frame=2;
            const auto raw_height=host.floor_view_height();
            auto* sprite=host.floor_sprite();auto* image=host.floor_image();
            host.floor_draw(sprite,image,frame,x,background_add(background_add(origin.y,-100),raw_height));
        }
        tile=next;
    }
    host.floor_push_font(10);
    const auto height=host.floor_view_height();const auto resource=host.gate_resource();
    if(!resource)throw std::invalid_argument("missing SelectForm gate resource");
    const auto clip=host.gate_clip();
    host.gate_draw(resource,clip,0,0,static_cast<float>(background_add(origin.x,40)),static_cast<float>(background_add(background_add(origin.y,-70),height)));
}
void queue_select_notice(EventScript& stack,std::int32_t resource,std::int32_t x,std::int32_t y,std::int32_t clip,std::int32_t frame,std::int32_t layer,std::int32_t depth){
    if(depth==-99999)depth=scenery_count(stack);
    const auto target=stack;auto row=std::make_shared<EventIntegers>(EventIntegers{resource,x,y,clip,frame,layer,depth});
    scenery_required(target).push_back(std::move(row));
}
namespace {
std::int32_t town_remainder(std::int32_t frame,std::int32_t maximum){
    if(!maximum)return frame;if(maximum==-1)return 0;return frame%maximum;
}
}
void draw_select_town_buildings(const SelectTownDrawingState& state,SelectBackgroundOrigin origin,SelectTownDrawingHost& host){
    const auto count=host.town_count();const auto buildings=host.town_buildings();
    auto tile=background_add(origin.x,85);
    for(std::int32_t i=0;i<count;++i,tile=background_add(tile,90)){
        const auto height=host.town_view_height();const auto game_height=host.game_height();
        if(!check_select_view_area(background_add(tile,45),background_add(game_height,-1),60,0,host))continue;
        if(!scenery_required(buildings).at(static_cast<std::size_t>(i)))continue;
        const auto resource=host.town_resource();const auto clip=host.town_clip(0x168);
        host.town_draw_chip(resource,clip,state.frame,i%3==0?17:18,tile,background_add(background_add(origin.y,-100),height));
    }
    host.town_pop_font();host.town_push_font(7);
    const auto occupants=host.town_occupants();
    for(std::int32_t i=0;i<count;++i){
        // GetBuildingPos 0x11eb8f4 queries raw GameView height before returning.
        const auto height=host.town_view_height();
        const auto position=static_cast<std::uint32_t>(i)*90U+130U;
        const auto px=position<=0x7fffffffU?static_cast<std::int32_t>(position):-1-static_cast<std::int32_t>(0xffffffffU-position);
        const auto x=background_add(origin.x,px);const auto game_height=host.game_height();
        if(!check_select_view_area(x,background_add(game_height,-1),60,0,host))continue;
        const auto row=scenery_required(buildings).at(static_cast<std::size_t>(i));if(!row)continue;
        static_cast<void>(row->at(2));const auto id=row->at(1);const auto frame=state.frame;
        const auto enabled=host.town_enabled(i);const auto resource=host.town_resource();const auto clip=host.town_clip(0x168);
        const auto y=background_add(background_add(height,-100),origin.y);
        const auto notice=[&](std::int32_t field,std::int32_t nx,std::int32_t ny,std::int32_t nf){
            const auto nc=host.town_clip(field);queue_select_notice(state.notices,11,nx,ny,nc,nf,0);
        };
        const auto animated_notice=[&](std::int32_t field,std::int32_t nx,std::int32_t ny,std::int32_t nf){
            const auto nc=host.town_clip(field);const auto maximum=host.town_clip_frames(nc);
            queue_select_notice(state.notices,11,nx,ny,nc,town_remainder(nf,maximum),0);
        };
        if(id==-1){
            host.town_draw_chip(resource,clip,state.frame,enabled?33:32,x,y);
            if(enabled)animated_notice(0x1a8,x,background_add(y,-50),state.frame);
        }else{
            const auto image=scenery_required(host.town_building(id)).image;
            host.town_draw_chip(resource,clip,background_add(frame,id),image,x,y);
            const auto type=scenery_required(host.town_building(id)).type;
            if(!building_is_decoration(type)){
                const bool dim=i!=state.selected;if(dim)host.town_push_dim();
                notice(0x190,x,background_add(y,-80),1);
                if(state.top_form)host.town_label(scenery_required(host.town_building(id)).name,x,background_add(y,-79));
                if(i!=state.selected)host.town_pop_dim();
            }
            if(scenery_required(host.town_building(id)).type==7&&host.town_king_reward_available())animated_notice(0x1a8,x,background_add(y,-45),state.frame);
            if(scenery_required(host.town_building(id)).type==6&&!scenery_required(occupants).at(static_cast<std::size_t>(i)))animated_notice(0x1a8,x,background_add(y,-45),state.frame);
            if(scenery_required(host.town_building(id)).type==12&&host.town_share_count()==0&&host.town_form_state()==0&&host.town_share_active()){
                if(state.top_form)host.town_share_label(x,background_add(y,-45));
                animated_notice(0x1a8,x,background_add(y,-45),state.frame);
            }
        }
        const bool share=host.town_share_active();
        if(id!=-1){
            const auto type=scenery_required(host.town_building(id)).type;
            if(type==8||(!share&&type==12))continue;
        }
        if(i==state.selected){
            if(!enabled)continue;
            animated_notice(0x188,background_add(x,-5),background_add(y,14),state.frame/6);
            host.town_touch(i,true,background_add(x,-40),background_add(y,-90));
        }else host.town_touch(i,false,background_add(x,-40),background_add(y,-70));
    }
    host.town_pop_font();
}
std::array<std::int32_t,2> select_area_position(std::int32_t index,std::int32_t height){
    const auto product=static_cast<std::uint32_t>(index)*129U;
    const auto scaled=product<=0x7fffffffU?static_cast<std::int32_t>(product):-1-static_cast<std::int32_t>(0xffffffffU-product);
    return {scenery_sub(-160,scaled),background_add(height,-157)};
}
void draw_select_town_areas(const SelectAreaDrawingState& state,SelectBackgroundOrigin origin,SelectAreaDrawingHost& host){
    static_cast<void>(host.area_view_height());host.area_push_font(10);
    for(std::int32_t index=0;;++index){
        const auto rows=host.drawing_areas();
        if(static_cast<std::size_t>(index)>=scenery_required(rows).size())break;
        const auto record=rows->at(static_cast<std::size_t>(index));
        const auto position=select_area_position(index,host.area_view_height());
        const auto x=background_add(origin.x,position[0]);const auto height=host.game_height();
        if(!check_select_view_area(x,background_add(height,-1),134,0,host))continue;
        const bool found=scenery_required(scenery_required(host.drawing_areas()).at(static_cast<std::size_t>(index))).discovery==1;
        const auto y=background_add(origin.y,position[1]);
        const auto notice=[&](std::int32_t resource,std::int32_t clip,std::int32_t nx,std::int32_t ny,std::int32_t frame){
            queue_select_notice(state.notices,resource,nx,ny,clip,frame,0);
        };
        const auto animated=[&](std::int32_t resource,std::int32_t clip,std::int32_t nx,std::int32_t ny,std::int32_t divisor){
            const auto maximum=host.area_clip_frames(resource,clip);const auto frame=state.frame/divisor;
            notice(resource,clip,nx,ny,town_remainder(frame,maximum));
        };
        if(!found){
            if(background_add(host.area_number(),1)>static_cast<std::int64_t>(scenery_required(host.drawing_areas()).size()))continue;
            if(index!=background_add(host.area_number(),1))continue;
            host.area_draw(11,host.area_clip(0x194),std::nullopt,x,y);
            if(state.form_state==0)host.area_touch(index,false,true,x,y);
            continue;
        }
        host.area_draw(0,3,scenery_required(record).image,background_add(x,18),background_add(y,-23));
        const auto label_x=background_add(x,64);const auto selected=state.selected;
        const auto name_clip=host.area_clip(0x18c);
        notice(11,name_clip,label_x,background_add(y,-5),index==selected?1:0);
        if(index==selected){
            const auto go_clip=host.area_clip(0x188);animated(11,go_clip,label_x,background_add(y,72),6);
            host.area_touch(index,true,false,background_add(x,50),background_add(y,45));
        }else host.area_touch(index,false,false,x,y);
        host.area_label_color();
        if(state.top_form)host.area_label(scenery_required(record).name,label_x,background_add(y,1));
        if(scenery_required(state.marks).at(static_cast<std::size_t>(index))!=1)continue;
        animated(2,15,background_add(x,50),background_add(y,-24),4);
        const auto arrow_clip=host.area_clip(0x1a8);animated(11,arrow_clip,label_x,background_add(y,-28),1);
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
