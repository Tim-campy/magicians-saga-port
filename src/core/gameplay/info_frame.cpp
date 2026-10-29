#include "info_frame.hpp"
#include "combat_rules.hpp"
#include "projectile_motion.hpp"

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing info-controller data");return *p;}
std::int32_t word(std::uint32_t v){return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t sub(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t mul(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(b));}
template<class Predicate>std::shared_ptr<PopupBar> find_popup(InfoState& state,Predicate test){
    const auto list=state.popups;auto& values=required(list.get());const auto version=values.version();std::int32_t index=0;
    for(;;){
        if(version!=values.version())throw std::logic_error("info popup list changed during enumeration");
        if(index>=values.size())return nullptr;
        auto bar=values.at(index++);if(test(required(bar.get())))return bar;
    }
}
ContactPosition& position(ComponentSlots& e){return required(dynamic_cast<ContactPosition*>(e.get(0)));}
}
void change_popup_state(PopupBar& b,std::int32_t state){b.state_frame=0;b.state=state;}
void change_slide_state(SlideBar& b,std::int32_t state){b.state_frame=0;b.state=state;}
std::int32_t popup_start_frame(PopupBar& b){return add(b.duration(),8);}
std::int32_t popup_max_frame(PopupBar& b){return add(b.duration(),16);}
std::int32_t slide_start_frame(SlideBar& b){return add(b.duration(),8);}
std::int32_t slide_max_frame(SlideBar& b){return add(b.duration(),16);}
void popup_in(PopupBar& b){const auto bottom=b.bottom;const auto height=b.height();b.y=easing(bottom,sub(bottom,height),8,b.state_frame,80);}
void popup_out(PopupBar& b){const auto bottom=b.bottom;const auto height=b.height();b.y=easing(sub(bottom,height),b.bottom,8,b.state_frame,80);}
void PopupBar::wait(){const auto captured=bottom;const auto h=height();y=sub(captured,h);}
void SlideBar::wait(){x=0;}
void slide_in(SlideBar& b){const auto width=b.width();b.x=easing(sub(0,width),0,8,b.state_frame,80);}
void slide_out(SlideBar& b){const auto width=b.width();b.x=easing(0,sub(0,width),8,b.state_frame,80);}
bool update_popup(PopupBar& b){
    const auto state=b.state;b.frame=add(b.frame,1);const auto elapsed=add(b.state_frame,1);b.state_frame=elapsed;
    if(state==0){popup_in(b);if(b.frame>=9)change_popup_state(b,1);}
    else if(state==1){b.wait();if(b.duration()!=-1){const auto frame=b.frame;const auto limit=add(b.duration(),8);if(frame>limit)change_popup_state(b,2);}}
    else if(state==2){if(elapsed>=8)return true;popup_out(b);}
    return false;
}
bool update_slide(SlideBar& b){
    const auto state=b.state;b.frame=add(b.frame,1);const auto elapsed=add(b.state_frame,1);b.state_frame=elapsed;
    if(state==0){slide_in(b);if(b.frame>=9)change_slide_state(b,1);}
    else if(state==1){b.wait();if(b.duration()!=-1){const auto frame=b.frame;const auto limit=add(b.duration(),8);if(frame>limit)change_slide_state(b,2);}}
    else if(state==2){if(elapsed>=8)return true;slide_out(b);}
    return false;
}
void drop_popup(PopupBar* bar){if(!bar)return;auto& b=*bar;if(b.state==2)return;const auto frame=add(b.duration(),8);b.frame=frame;change_popup_state(b,2);}
std::shared_ptr<PopupBar> search_popup_type(InfoState& s,std::int32_t type){return find_popup(s,[type](auto& bar){return bar.type()==type;});}
std::shared_ptr<PopupBar> search_same_popup(InfoState& s,PopupBar* other){return find_popup(s,[other](auto& bar){return bar.same_popup(other);});}
std::int32_t popup_index(InfoState& s,PopupBar* bar){
    for(std::int32_t i=0;;++i){auto list=s.popups;auto& values=required(list.get());if(i>=values.size())return -1;if(values.at(i).get()==bar)return i;}
}
float info_emphasis_scale(const InfoState& s){
    const auto frame=s.emphasize_frame;if(frame==-1)return 1.0F;
    if(frame<3)return easing_float(1.0F,1.100000023841858F,3.0F,static_cast<float>(frame),100.0F);
    return easing_float(1.100000023841858F,1.0F,3.0F,static_cast<float>(sub(frame,3)),100.0F);
}
float info_emphasis_offset(InfoState& s){
    auto bar=s.emphasized;if(!bar)return 0;
    const auto initial_height=bar->height();bar=s.emphasized;const auto height=required(bar.get()).height();
    const auto scale=info_emphasis_scale(s);const float product=scale*static_cast<float>(height);
    return static_cast<float>(initial_height)-product;
}
void add_popup(InfoState& s,std::shared_ptr<PopupBar> bar,bool force){
    if(!force){auto existing=search_same_popup(s,bar.get());if(existing){s.emphasize_frame=0;s.emphasized=std::move(existing);return;}}
    auto& incoming=required(bar.get());
    if(incoming.single())find_popup(s,[&incoming](auto& old){
        const auto category=old.category();const auto target=incoming.category();if(category==target)drop_popup(&old);return false;
    });
    for(std::int32_t i=0;;++i){
        auto list=s.popups;auto& values=required(list.get());
        if(i>=values.size()){values.append(std::move(bar));return;}
        auto old=values.at(i);const auto priority=required(old.get()).priority();const auto requested=incoming.priority();
        if(priority<requested){required(s.popups.get()).insert(i,std::move(bar));return;}
    }
}
void add_slide(InfoState& s,std::shared_ptr<SlideBar> bar){required(s.slides.get()).append(std::move(bar));}
void update_popup_bars(InfoState& s,InfoHost& host){
    if(s.emphasize_frame>=0){s.emphasize_frame=add(s.emphasize_frame,1);if(s.emphasize_frame>=6){s.emphasize_frame=-1;s.emphasized.reset();}}
    for(std::int32_t i=0;;++i){
        auto list=s.popups;auto& values=required(list.get());if(i>=values.size())return;
        auto bar=values.at(i);const auto height=host.info_view_height();auto& b=required(bar.get());b.bottom=sub(height,s.offset_bottom);
        if(bar!=s.emphasized){const auto index=popup_index(s,bar.get());const auto emphasized=s.emphasized;
            if(index>popup_index(s,emphasized.get())){const auto bottom=b.bottom;const auto offset=info_emphasis_offset(s);b.bottom=add(bottom,factory_cell_coordinate(offset,1));}}
        for(std::int32_t j=0;j<i;++j){
            auto old=required(s.popups.get()).at(j);bool skip=false;
            if(b.single()){const auto old_category=required(old.get()).category();const auto category=b.category();skip=old_category==category;}
            if(!skip){const auto bottom=b.bottom;auto& previous=required(old.get());b.bottom=add(bottom,sub(previous.y,previous.bottom));}
        }
        if(update_popup(b)){b.dispose();required(s.popups.get()).erase(i);--i;}
    }
}
void update_slide_bars(InfoState& s){
    for(std::int32_t i=0;;++i){
        auto list=s.slides;auto& values=required(list.get());if(i>=values.size())return;
        auto first=values.at(i);auto second=required(s.slides.get()).at(i);
        auto& b=required(first.get());const auto height=b.height();required(second.get()).y=add(mul(height,i),65);
        if(update_slide(b)){required(s.slides.get()).erase(i);--i;}
    }
}
void update_info(InfoState& s,InfoHost& host){
    auto dialog=s.dialog;if(dialog){if(dialog->answer==0){auto bar=search_popup_type(s,5);if(bar)drop_popup(bar.get());}s.dialog.reset();}
    update_popup_bars(s,host);update_slide_bars(s);
}
void pause_info(InfoState& s){find_popup(s,[](auto& bar){if(bar.category()==0)drop_popup(&bar);return false;});}
void info_button_touch(const InputTouch* input,ComponentSlots*){const auto& event=required(input);if(event.type==5)required(event.component);}
void info_town_attack(InfoState& s,EventObject town,InfoHost& host){auto bar=host.make_attack_notice(std::move(town));add_slide(s,std::move(bar));}
void info_camera_click(InfoState& s,CursorPointRef point,InfoHost& host){
    if(host.info_world_paused())return;
    auto cursor=required(host.info_monsters()).cursor();ComponentSlots* hit=nullptr;
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);const auto xx=factory_cell_coordinate(position(e).x,1),xz=factory_cell_coordinate(position(e).z,1);
        const auto x=sub(xx,xz);const auto yx=factory_cell_coordinate(position(e).x,1),yz=factory_cell_coordinate(position(e).z,1);
        const auto y=add(add(yx,yz)/2,factory_cell_coordinate(position(e).y,1));const auto& p=required(point.get());
        const auto left=sub(x,20),top=sub(y,35);
        if(left<p.x&&top<p.y&&add(left,40)>p.x&&add(top,40)>p.y){hit=&e;break;}
    }
    if(hit){auto bar=host.make_monster_info(hit);add_popup(s,std::move(bar),false);}
}
void initialize_info(InfoState& s,InfoInitializationWorld* world,InfoHost& host){
    s.emphasized.reset();s.emphasize_frame=-1;s.offset_bottom=0;auto& w=required(world);
    w.subscribe_info_click([&s,&host](CursorPointRef p){info_camera_click(s,std::move(p),host);});
    w.subscribe_info_button([](const InputTouch* e,ComponentSlots* target){info_button_touch(e,target);});
    w.subscribe_info_town([&s,&host](EventObject town){info_town_attack(s,std::move(town),host);});
}
}
