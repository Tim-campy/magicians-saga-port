#pragma once
#include "cursor_frame.hpp"
#include "input_frame.hpp"
#include "stage_update.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
// Managed List identity and version are observed by controller callbacks. Use these
// mutations rather than exposing a vector that could silently invalidate enumeration.
template<class T>class InfoBars {
public:
    using Ref=std::shared_ptr<T>;
    std::int32_t size()const{return static_cast<std::int32_t>(values_.size());}
    Ref at(std::int32_t index)const{return values_.at(static_cast<std::size_t>(index));}
    void append(Ref value){insert(size(),std::move(value));}
    void insert(std::int32_t index,Ref value){
        if(index<0||index>size())throw std::out_of_range("info bar insertion index");
        if(size()==std::numeric_limits<std::int32_t>::max())throw std::length_error("info bar list limit");
        values_.insert(values_.begin()+index,std::move(value));++version_;
    }
    void erase(std::int32_t index){at(index);values_.erase(values_.begin()+index);++version_;}
    void replace(std::int32_t index,Ref value){values_.at(static_cast<std::size_t>(index))=std::move(value);++version_;}
    std::uint32_t version()const{return version_;}
private:
    std::vector<Ref> values_;std::uint32_t version_=0;
};
struct PopupBar {
    explicit PopupBar(std::int32_t id_value):id(id_value){}
    virtual ~PopupBar()=default;
    std::int32_t id,x=0,y=0,bottom=0,frame=0,state_frame=0,state=0;
    virtual std::int32_t category()=0;virtual std::int32_t type()=0;virtual std::int32_t priority()=0;
    virtual bool single()=0;virtual std::int32_t height()=0;virtual std::int32_t duration()=0;
    virtual void wait(); // Native base virtual Wait; concrete types may override it.
    virtual void dispose()=0;virtual bool same_popup(PopupBar*)=0;
};
struct SlideBar {
    explicit SlideBar(std::int32_t id_value):id(id_value){}
    virtual ~SlideBar()=default;
    std::int32_t id,x=0,y=0,frame=0,state_frame=0,state=0;
    virtual std::int32_t width()=0;virtual std::int32_t height()=0;virtual std::int32_t duration()=0;
    virtual void wait();
};
bool update_popup(PopupBar&); // Base PopupBar.Update 0x11cf1e0, NOT a virtual Update call.
bool update_slide(SlideBar&); // Base SlideBar.Update 0x11ce438.
void popup_in(PopupBar&);void popup_out(PopupBar&);void slide_in(SlideBar&);void slide_out(SlideBar&);
void change_popup_state(PopupBar&,std::int32_t);void change_slide_state(SlideBar&,std::int32_t);
void drop_popup(PopupBar*); // Nullable Info.DropPopupBar -> PopupBar.Drop.
std::int32_t popup_start_frame(PopupBar&);std::int32_t popup_max_frame(PopupBar&);
std::int32_t slide_start_frame(SlideBar&);std::int32_t slide_max_frame(SlideBar&);
struct InfoState {
    std::shared_ptr<InfoBars<SlideBar>> slides=std::make_shared<InfoBars<SlideBar>>();
    std::shared_ptr<InfoBars<PopupBar>> popups=std::make_shared<InfoBars<PopupBar>>();
    std::int32_t emphasize_frame=0;std::shared_ptr<PopupBar> emphasized;
    std::int32_t offset_bottom=0;std::shared_ptr<StageDialog> dialog;
};
class InfoHost {
public:
    virtual ~InfoHost()=default;
    virtual std::int32_t info_view_height()=0;
    virtual bool info_world_paused()=0; // Require current World/SystemManager even for an empty click group.
    virtual EntitySubset* info_monsters()=0; // Live And16, distinct from Enemy24.
    virtual std::shared_ptr<PopupBar> make_monster_info(ComponentSlots*)=0;
    virtual std::shared_ptr<SlideBar> make_attack_notice(EventObject)=0;
};
class InfoInitializationWorld {
public:
    using Click=std::function<void(CursorPointRef)>;
    using Button=std::function<void(const InputTouch*,ComponentSlots*)>;
    using Town=std::function<void(EventObject)>;
    virtual ~InfoInitializationWorld()=default;
    virtual void subscribe_info_click(Click)=0;virtual void subscribe_info_button(Button)=0;virtual void subscribe_info_town(Town)=0;
};
void initialize_info(InfoState&,InfoInitializationWorld*,InfoHost&); // 0x115942c; lists/dialog retained.
void update_info(InfoState&,InfoHost&);void update_popup_bars(InfoState&,InfoHost&);void update_slide_bars(InfoState&);
std::shared_ptr<PopupBar> search_popup_type(InfoState&,std::int32_t);
std::shared_ptr<PopupBar> search_same_popup(InfoState&,PopupBar*);
std::int32_t popup_index(InfoState&,PopupBar*);
void add_popup(InfoState&,std::shared_ptr<PopupBar>,bool force);
void add_slide(InfoState&,std::shared_ptr<SlideBar>);
float info_emphasis_scale(const InfoState&);float info_emphasis_offset(InfoState&);
void pause_info(InfoState&);void info_camera_click(InfoState&,CursorPointRef,InfoHost&);
void info_button_touch(const InputTouch*,ComponentSlots*);void info_town_attack(InfoState&,EventObject,InfoHost&);
}
