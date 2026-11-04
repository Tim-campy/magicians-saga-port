#include "select_camera.hpp"
#include "input_frame.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace saga::core {
namespace {
template<class T> T& required(const std::shared_ptr<T>& value) {
    if(!value)throw std::invalid_argument("missing active-area data");
    return *value;
}
std::int32_t area_count(const std::shared_ptr<CameraAreas>& areas) {
    const auto size=required(areas).size();
    if(size>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("area count exceeds int32");
    return static_cast<std::int32_t>(size);
}
std::int32_t wrap(std::int64_t value) {
    const auto bits=static_cast<std::uint32_t>(value);
    return bits<=0x7fffffffU?static_cast<std::int32_t>(bits):
        static_cast<std::int32_t>(static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::int32_t convert(float value) {
    if(std::isnan(value))return 0;
    if(value==std::numeric_limits<float>::infinity() || value<=-2147483648.0F)
        return std::numeric_limits<std::int32_t>::min();
    if(value>=2147483648.0F)return std::numeric_limits<std::int32_t>::max();
    return static_cast<std::int32_t>(value);
}
void decay(float& value) {
    if(value>0.0F) {value=value*0.9F;if(value<=0.25F)value=0.0F;}
    else if(value<0.0F) {value=value*0.9F;if(value>=-0.25F)value=0.0F;}
}
}
// AppData.Clamp 0x110180c: upper limit first, then lower, even for inverted bounds.
std::int32_t camera_clamp(std::int32_t value,std::int32_t minimum,std::int32_t maximum) {
    if(value>maximum)value=maximum;
    if(value<minimum)value=minimum;
    return value;
}
// AppData.GetAreaNum 0x111511c: count found rows rather than the greatest found ID.
std::int32_t select_area_number(SelectActiveAreaHost& host){
    std::int32_t count=-1;
    for(std::int32_t index=0;index<area_count(host.camera_areas());++index){
        const auto row=required(host.camera_areas()).at(static_cast<std::size_t>(index));
        if(required(row).state==1)++count;
    }
    return count;
}
// SelectForm.SetActiveX 0x11eb234. The first extent write survives later failures.
void set_select_active_x(SelectCameraState& state,SelectActiveAreaHost& host) {
    const auto capacity=required(host.normal_save_integers()).at(19);
    const auto first_width=host.view_width();
    state.right_extent=wrap(static_cast<std::int64_t>(capacity)*90+first_width);
    auto remaining=area_count(host.camera_areas());
    std::int32_t left_base=-129;
    while(remaining>0) {
        const auto index=static_cast<std::size_t>(remaining-1);
        const auto candidate=required(host.camera_areas()).at(index);
        if(candidate) {
            const auto fresh=required(host.camera_areas()).at(index);
            if(required(fresh).state==1) {
                const auto maximum=wrap(static_cast<std::int64_t>(area_count(host.camera_areas()))-1);
                const auto selected=camera_clamp(remaining,1,maximum);
                left_base=wrap(-static_cast<std::int64_t>(selected)*129);
                break;
            }
        }
        --remaining;
    }
    const auto final_width=host.view_width();
    state.left_extent=wrap(static_cast<std::int64_t>(left_base)-final_width);
}
std::int32_t scaled_view_dimension(std::int32_t pixels,float scale) {
    const auto numerator=static_cast<float>(wrap(static_cast<std::int64_t>(pixels)*100));
    const float quotient=numerator/scale;
    return convert(quotient+0.01F); // ELF literal 0x62798c, not conventional half-up rounding.
}
std::int32_t scaled_select_dimension(std::int32_t dimension,std::int32_t scale) {
    const auto numerator=wrap(static_cast<std::int64_t>(dimension)*100);
    if(scale==0)return 0;
    if(numerator==std::numeric_limits<std::int32_t>::min() && scale==-1)return numerator;
    return numerator/scale;
}
// SelectForm.SetCameraPosition 0x11f32b4. Keep repeated dimension queries and partial writes.
void set_select_camera_position(SelectCameraState& state,std::int32_t x,std::int32_t y,SelectCameraHost& host) {
    const auto right=state.right_extent;
    const auto width=host.game_width();
    const auto first_view_width=host.view_width();
    const auto second_view_width=host.view_width();
    const auto minimum_x=wrap(static_cast<std::int64_t>(width)-right-first_view_width+second_view_width);
    const auto maximum_x=wrap(-static_cast<std::int64_t>(state.left_extent));
    state.x=camera_clamp(x,minimum_x,maximum_x);
    const auto first_height=host.game_height();
    const auto first_view_height=host.view_height();
    const auto second_height=host.game_height();
    const auto second_view_height=host.view_height();
    const auto minimum_y=wrap(static_cast<std::int64_t>(first_height)-first_view_height-80);
    const auto maximum_y=wrap(static_cast<std::int64_t>(second_height)-second_view_height+50);
    state.y=camera_clamp(y,minimum_y,maximum_y);
}
// SelectForm.MoveCursor 0x11ef0dc. Sub-unit velocities may remain indefinitely.
void move_select_camera(SelectCameraState& state,SelectCameraHost& host) {
    if(state.dragging)return;
    if(convert(state.velocity_x)==0 && convert(state.velocity_y)==0)return;
    decay(state.velocity_x);
    decay(state.velocity_y);
    const auto x=wrap(static_cast<std::int64_t>(state.x)+convert(state.velocity_x));
    const auto y=wrap(static_cast<std::int64_t>(state.y)+convert(state.velocity_y));
    set_select_camera_position(state,x,y,host);
}
void set_select_touch_value(SelectTouchState& state,std::uint32_t flags,std::int32_t value,SelectTouchHost& host){
    // SelectForm.SetTouchValue 0x11f4474: the masked word also reaches the base.
    flags&=0xffff0000U;
    if(flags==0x220000U){state.selected_chip=value;state.selected_area=-1;}
    else if(flags==0xc0000U){state.selected_area=value;state.selected_chip=-1;}
    else host.base_set_touch_value(flags,value);
}
std::int32_t get_select_touch_value(SelectTouchState& state,std::uint32_t flags,std::int32_t fallback,SelectTouchHost& host){
    flags&=0xffff0000U;
    if(flags==0x220000U)return state.selected_chip;
    if(flags==0xc0000U)return state.selected_area;
    return host.base_get_touch_value(flags,fallback);
}
void on_select_camera_touch(SelectTouchState& state,std::int32_t type,const SelectFormTouch* input,SelectTouchHost& host){
    if(!input)throw std::invalid_argument("missing SelectForm touch event");
    const auto scale=state.scale;
    const auto dx=scaled_select_dimension(input->dx,scale),dy=scaled_select_dimension(input->dy,scale);
    const auto x=static_cast<float>(dx),y=static_cast<float>(dy);
    const auto count=[&](){
        const auto size=required(state.history).size();
        if(size>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))throw std::length_error("town touch history exceeds int32");
        return size;
    };
    switch(type){
    case 0:{
        required(state.history).clear();
        state.camera.dragging=true;state.build_drag=false;state.movement.enabled=true;
        if(state.form_state!=0)return;
        state.movement.drag_distance=static_cast<float>(wrap(static_cast<std::int64_t>(scaled_select_dimension(input->x,scale))-state.camera.x));
        const auto range=host.active_range();
        const auto left=static_cast<float>(required(range).at(0));
        const auto target=state.movement.drag_distance;
        if(target<=left){state.movement.drag_distance=left;return;}
        const auto right=static_cast<float>(range->at(1));
        // Native B.LT accepts unordered too; an active-range callback can have
        // changed the live target to NaN after its earlier finite assignment.
        if(!(target>=right))return;
        state.movement.drag_distance=right;return;
    }
    case 1:
        if(count()>=1 && state.camera.dragging){
            const auto sample=state.history->front();
            static_cast<void>(required(sample).at(1)); // Both array-length guards precede the vector null guard.
            auto& speed=required(state.camera_speed);
            const auto speed_x=wrap(static_cast<std::int64_t>(convert(x))-sample->at(0));
            const auto speed_y=wrap(static_cast<std::int64_t>(convert(y))-sample->at(1));
            speed={static_cast<float>(speed_x),0.0F,static_cast<float>(speed_y)};
            required(state.camera_decimal)={0.0F,0.0F,0.0F};
        }
        state.camera.dragging=false;return;
    case 5:
        if(count()==0)state.build_drag=true;
        return;
    case 8:{
        if(host.touch_count()>1)return;
        if(state.camera.dragging){
            const auto camera_x=wrap(static_cast<std::int64_t>(state.camera.x)+convert(x));
            const auto camera_y=wrap(static_cast<std::int64_t>(state.camera.y)+convert(y));
            host.set_camera_position(camera_x,camera_y);
        }
        EventInstruction sample;
        if(count()>4){
            sample=state.history->back();required(sample).at(0)=convert(x);sample->at(1)=convert(y);
            auto& current=required(state.history);
            if(current.empty())throw std::out_of_range("empty town touch history removal");
            current.pop_back();
        }else sample=std::make_shared<EventIntegers>(EventIntegers{convert(x),convert(y)});
        auto& current=required(state.history);current.insert(current.begin(),std::move(sample));
        state.camera.velocity_x=x;state.camera.velocity_y=y;
        // Signed comparisons deliberately ignore large negative drags.
        if(dx>5 || dy>=6){state.selected_chip=-1;state.selected_area=-1;}
        return;
    }
    case 12:{
        state.build_drag=false;
        auto value=camera_clamp(wrap(static_cast<std::int64_t>(state.pinch_value)+input->pinch_delta),40,190);
        state.pinch_value=value;
        if(value<=49)value=std::min(wrap(static_cast<std::int64_t>(value)+40),50);
        else if(value>=101)value=std::max(wrap(static_cast<std::int64_t>(value)-40),100);
        host.set_scale(value);state.pinching=true;state.pinch_frame=0;return;
    }
    default:return;
    }
}
bool on_select_form_touch(SelectTouchState& state,const SelectFormTouch* input,SelectTouchHost& host){
    if(!input||!input->control)throw std::invalid_argument("missing SelectForm touch/control");
    const auto group=input->control->group;
    if(group==17)on_select_camera_touch(state,input->type,input,host);
    return group!=17;
}
}
