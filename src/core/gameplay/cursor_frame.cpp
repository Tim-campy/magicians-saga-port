#include "cursor_frame.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing cursor input");return *p;}
std::int32_t word(std::uint32_t bits){return static_cast<std::int32_t>(bits<0x80000000U?
    static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
std::int32_t sub(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b));}
std::int32_t squared(std::int32_t a){return word(static_cast<std::uint32_t>(a)*static_cast<std::uint32_t>(a));}
ContactPosition& position(ComponentSlots& e){return required(dynamic_cast<ContactPosition*>(e.get(0)));}
std::int32_t saved(CursorHost& host,std::size_t slot){auto values=host.cursor_save();return required(values.get()).at(slot);}
}
CursorPoint cursor_cell(std::int32_t step_x,std::int32_t step_z,float x,float y){
    const auto half_x=factory_cell_coordinate(x,1)/2,iy=factory_cell_coordinate(y,1);
    const auto first=add(add(iy,half_x),step_x/2),second=add(sub(iy,half_x),step_z/2);
    return {factory_cell_coordinate(static_cast<float>(first),step_x),
            factory_cell_coordinate(static_cast<float>(second),step_z)};
}
void initialize_cursor(CursorState& state,CursorInitializationWorld* world,CursorHost& host){
    const auto x=saved(host,17),y=saved(host,18);
    state.last_position=std::make_shared<CursorPoint>(CursorPoint{x,y});
    auto& target=required(world);
    target.subscribe_cursor_point(2,[&state,&host](CursorPointRef p){cursor_touch_down(state,std::move(p),host);});
    target.subscribe_cursor_point(4,[](CursorPointRef p){cursor_touch_click(std::move(p));});
}
void update_cursor(CursorState& state,CursorHost& host){
    if(state.tracking&&(!state.focus||host.cursor_destroyed(*state.focus)))state.tracking=false;
    const auto x=saved(host,17),y=saved(host,18);
    auto& last=required(state.last_position.get());
    if(last.x==x&&last.y==y)return;
    auto* map_pointer=host.cursor_map();
    const auto previous=required(state.last_position.get()); // GetMap may replace the saved point.
    auto& map=required(map_pointer); // The fresh point is validated before the map result.
    const auto old=std::make_shared<CursorPoint>(cursor_cell(map.step_x,map.step_z,static_cast<float>(previous.x),static_cast<float>(previous.y)));
    const auto next=std::make_shared<CursorPoint>(cursor_cell(map.step_x,map.step_z,static_cast<float>(x),static_cast<float>(y)));
    if(old->x!=next->x||old->y!=next->y)host.cursor_cell_changed(old,next);
    auto& current=required(state.last_position.get());current.x=x;current.y=y;
}
ComponentSlots* cursor_human_at(std::int32_t x,std::int32_t y,CursorHost& host){
    auto cursor=required(host.cursor_humans()).cursor();
    auto best=std::numeric_limits<std::int32_t>::max();ComponentSlots* selected=nullptr;
    while(const auto entry=cursor.next()){
        auto& e=required(*entry);
        const auto xx=factory_cell_coordinate(position(e).x,1),xz=factory_cell_coordinate(position(e).z,1);
        const auto iso_x=sub(xx,xz);
        const auto yx=factory_cell_coordinate(position(e).x,1),yz=factory_cell_coordinate(position(e).z,1);
        const auto iso_y=add(add(yx,yz)/2,factory_cell_coordinate(position(e).y,1));
        const auto left=sub(iso_x,20),top=sub(iso_y,35);
        if(left>=x||top>=y||add(left,40)<=x||add(top,40)<=y)continue;
        const auto distance=add(squared(sub(iso_x,x)),squared(sub(sub(iso_y,y),15)));
        if(distance<best){best=distance;selected=&e;}
    }
    return selected;
}
void cursor_touch_down(CursorState& state,CursorPointRef point,CursorHost& host){
    const auto p=required(point.get());auto* hit=cursor_human_at(p.x,p.y,host);
    if(hit==state.focus)return;
    if(!hit){state.focus=nullptr;return;}
    state.focus=hit;state.tracking=false;state.focus_wait=6;
    auto* effect=host.create_cursor_focus_effect();required(effect).entity_arg=state.focus;
}
void start_cursor_catch(CursorState& state,ComponentSlots* entity){state.focus=entity;state.catch_human=entity;state.tracking=true;}
void cursor_touch_click(CursorPointRef){}
}
