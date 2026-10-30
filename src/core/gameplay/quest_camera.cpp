#include "quest_camera.hpp"
#include <stdexcept>
#include <algorithm>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing quest camera input");return *p;}
std::int32_t word(std::uint32_t bits){return static_cast<std::int32_t>(bits<0x80000000U?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t subtract(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
QuestCameraComponent* camera(ComponentSlots& e){
    auto* component=e.get(3);auto* result=dynamic_cast<QuestCameraComponent*>(component);
    if(component&&!result)throw std::invalid_argument("incorrect camera component type");return result;
}
ContactPosition& position(ComponentSlots& e){return required(dynamic_cast<ContactPosition*>(e.get(0)));}
void check_maps(QuestCameraForm& form,int count,bool ignore_first=false){
    for(int i=0;i<count;++i){auto* map=required(form.world).camera_map();if(i||!ignore_first)required(map);}
}
}
ComponentSlots& add_quest_camera(ComponentSlots& e,std::int32_t angle,std::int32_t scale,QuestCameraFactoryHost& host){
    auto& c=host.allocate_camera_component();c.angle=angle;c.scale=scale;return e.add(3,&c);
}
ComponentSlots& create_quest_camera(EntityIdState& ids,QuestCameraFactoryHost& host){
    auto& e=create_entity(ids,0,false,host);auto& p=host.acquire_camera_position();
    p.x=0;p.y=0;p.z=0;p.offset_x=0;p.offset_y=0;p.offset_z=0;p.parent=nullptr;e.add(0,&p);
    return add_quest_camera(e,0,100,host);
}
void initialize_quest_camera(QuestCameraSystem& state,QuestCameraInitializationWorld* world){
    state.camera=required(world).create_camera();required(camera(required(state.camera))).angle=45;
    state.dirty=false;state.dragging=false;state.cursor_revision=false;state.pinch_scale=120;
    state.history=std::make_shared<std::vector<EventInstruction>>();
}
std::int32_t quest_game_width(QuestCameraForm* input,QuestFormCameraHost& host){
    auto& f=required(input);const auto size=required(f.view).game_width();const auto margin=host.quest_view_x();
    return scaled_select_dimension(subtract(size,margin),f.state.scale);
}
std::int32_t quest_game_height(QuestCameraForm* input,QuestFormCameraHost& host){
    auto& f=required(input);const auto size=required(f.view).game_height();const auto margin=host.quest_view_y();
    return scaled_select_dimension(subtract(size,margin),f.state.scale);
}
void set_quest_cursor(QuestCameraForm* input,std::int32_t x,std::int32_t y,QuestFormCameraHost& host){
    auto& f=required(input);check_maps(f,5,true);quest_game_width(&f,host);quest_game_height(&f,host);
    auto first=host.camera_saved_integers();required(first.get()).at(17)=x;
    auto second=host.camera_saved_integers();required(second.get()).at(18)=y;
    auto read_x=host.camera_saved_integers();const auto center_x=required(read_x.get()).at(17);
    const auto width=quest_game_width(&f,host);f.x=subtract(center_x,width/2);
    auto read_y=host.camera_saved_integers();const auto center_y=required(read_y.get()).at(18);
    const auto height=quest_game_height(&f,host);f.y=subtract(center_y,height/2);
}
void set_quest_camera_position(QuestCameraForm* input,std::int32_t x,std::int32_t y,QuestFormCameraHost& host){
    auto& f=required(input);check_maps(f,4);quest_game_width(&f,host);quest_game_height(&f,host);
    f.x=x;f.y=y;
    auto first=host.camera_saved_integers();const auto width=quest_game_width(&f,host);
    required(first.get()).at(17)=add(x,width/2);
    auto second=host.camera_saved_integers();const auto current_y=f.y,height=quest_game_height(&f,host);
    required(second.get()).at(18)=add(current_y,height/2);f.update_static_render=true;
}
void set_camera_cursor(float x,float y,QuestCameraHost& host){
    auto& form=required(host.current_camera_form());set_quest_cursor(&form,factory_cell_coordinate(x,1),factory_cell_coordinate(y,1),host);
}
EventInstruction quest_form_scale_range(QuestCameraForm* input){
    auto& form=required(input);const auto values=form.form_scale_range;
    required(values.get()).at(0)=form.minimum_scale;values->at(1)=200;return values;
}
void set_quest_form_scale(QuestCameraForm* input,std::int32_t requested,QuestFormCameraHost& host){
    auto& form=required(input);const auto range=quest_form_scale_range(&form);
    const auto scale=camera_clamp(requested,range->at(0),range->at(1));if(scale==form.state.scale)return;
    const auto old_width=quest_game_width(&form,host),old_height=quest_game_height(&form,host),old_x=form.x;
    form.state.scale=scale;
    const auto width=quest_game_width(&form,host),old_y=form.y,height=quest_game_height(&form,host);
    set_quest_camera_position(&form,subtract(old_x,subtract(width,old_width)/2),subtract(old_y,subtract(height,old_height)/2),host);
    form.update_static_render=true;
}
void on_quest_form_camera_touch(QuestCameraForm* input,QuestFrameState& frame,std::int32_t type,const QuestFormTouch* touch,QuestFormTouchHost& host){
    const auto& event=required(touch);
    if(type==1){if(frame.pinching)frame.pinching=false;return;}
    if(type==8){
        auto& form=required(input);const auto scale=form.state.scale;
        const auto dx=factory_cell_coordinate(static_cast<float>(scaled_select_dimension(event.dx,scale)),1);
        const auto dy=factory_cell_coordinate(static_cast<float>(scaled_select_dimension(event.dy,scale)),1);
        set_quest_camera_position(&form,subtract(form.x,dx),subtract(form.y,dy),host.form_camera_host());return;
    }
    if(type!=12)return;
    auto& form=required(input);
    auto scale=camera_clamp(add(form.pinch_value,event.pinch_delta),subtract(form.minimum_scale,40),240);
    form.pinch_value=scale;
    if(scale<=49)scale=std::min(add(scale,40),50);
    else if(scale>100)scale=std::max(subtract(scale,40),100);
    if(scale>100)form.minimum_scale=95;
    else if(scale<100){host.read_quest_priority();form.minimum_scale=50;}
    else scale=100;
    set_quest_form_scale(&form,scale,host.form_camera_host());
    frame.pinching=true;frame.pinch_frame=0;host.notify_quest_scale(scale);
}
bool on_quest_form_touch(QuestCameraForm* form,QuestFrameState& frame,const QuestFormTouch* input,QuestFormTouchHost& host){
    const auto& event=required(input);auto* control=&required(event.input.component);
    const auto type=event.input.type,group=control->group;
    host.forward_quest_touch(&event.input);
    if(group==17){on_quest_form_camera_touch(form,frame,type,input,host);return false;}
    if(host.quest_touch_debug()&&type==5&&control->group==37)host.debug_clear_quest();
    return true;
}
void set_camera_position(float x,float y,QuestCameraHost& host){
    auto& form=required(host.current_camera_form());set_quest_camera_position(&form,factory_cell_coordinate(x,1),factory_cell_coordinate(y,1),host);
}
void update_quest_camera(QuestCameraSystem& state,QuestCameraHost& host){
    camera(required(state.camera)); // The nullable getter result is intentionally discarded.
    auto& members=required(host.current_camera_members());const auto x=members.x(0),z=members.z(0);
    const auto xi=add(factory_cell_coordinate(x,1),72),zi=add(factory_cell_coordinate(z,1),72);
    const auto iso_x=subtract(xi,zi),iso_y=add(xi,zi)/2;
    set_camera_cursor(static_cast<float>(add(iso_x,20)),static_cast<float>(subtract(iso_y,60)),host);
}
std::int32_t quest_camera_scale_min(QuestCameraHost& host){return host.camera_view_width()<=240?10:75;}
std::int32_t quest_camera_scale_max(){return 200;}
EventInstruction quest_camera_scale_range(QuestCameraSystem& state,QuestCameraHost& host){
    auto first=state.scale_range;const auto minimum=quest_camera_scale_min(host);required(first.get()).at(0)=minimum;
    auto second=state.scale_range;required(second.get()).at(1)=200;return second;
}
void set_quest_camera_scale(QuestCameraSystem& state,std::int32_t requested,QuestCameraHost& host){
    const auto minimum=quest_camera_scale_min(host),scale=camera_clamp(requested,minimum,200);
    if(required(camera(required(state.camera))).scale==scale)return;
    required(camera(required(state.camera))).scale=scale;
    const auto x=position(required(state.camera)).x,y=position(required(state.camera)).y;
    set_camera_position(x,y,host); // CameraComponent.scale and QuestForm.scale_ are distinct fields.
}
void on_quest_camera_touch(QuestCameraSystem& state,const QuestCameraTouch* input,QuestCameraHost& host){
    auto& e=required(state.camera);position(e);auto& event=required(input);required(camera(e));position(e);required(camera(e));
    const auto dx=event.dx,scale=required(camera(e)).scale,dy=event.dy;auto& current=required(camera(e));
    switch(event.type){
    case 0:required(state.history.get()).clear();state.dragging=true;break;
    case 1:
        if(required(state.history.get()).size()>=1&&state.dragging)static_cast<void>(required(state.history.get()).at(0));
        state.dragging=false;break;
    case 5:if(!required(state.history.get()).empty())state.cursor_revision=true;break;
    case 12:state.dirty=true;break;
    case 8:{
        const auto second_scale=current.scale;if(host.camera_touch_count()>1)return;
        state.dirty=true;if(state.dragging&&state.cursor_revision)state.cursor_revision=false;
        const float x=static_cast<float>(scaled_select_dimension(dx,scale));
        const float y=static_cast<float>(scaled_select_dimension(dy,second_scale));
        EventInstruction sample;
        if(required(state.history.get()).size()>4){
            sample=required(state.history.get()).back();required(sample.get()).at(0)=factory_cell_coordinate(x,1);
            required(sample.get()).at(1)=factory_cell_coordinate(y,1);required(state.history.get()).pop_back();
        }else{sample=std::make_shared<EventIntegers>(EventIntegers{factory_cell_coordinate(x,1),factory_cell_coordinate(y,1)});}
        auto& history=required(state.history.get());history.insert(history.begin(),std::move(sample));break;
    }
    default:break;
    }
}
std::int32_t QuestCameraCellView::cell_game_width(){return quest_game_width(host_.current_camera_form(),host_);}
std::int32_t QuestCameraCellView::cell_game_height(){return quest_game_height(host_.current_camera_form(),host_);}
EventInstruction QuestCameraCellView::cell_view_origin(std::int32_t step_x,std::int32_t half_step_z){
    auto& form=required(host_.current_camera_form());
    return project_cell_location(scratch_,form.x,form.y,step_x,half_step_z);
}
}
