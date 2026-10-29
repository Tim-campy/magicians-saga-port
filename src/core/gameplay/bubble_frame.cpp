#include "bubble_frame.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing speech-bubble data");return *p;}
EntityBubble& bubble(ComponentSlots& e){return required(dynamic_cast<EntityBubble*>(e.get(18)));}
ContactPosition* position(ComponentSlots& e){auto* p=e.get(0);auto* result=dynamic_cast<ContactPosition*>(p);if(p&&!result)throw std::invalid_argument("incorrect bubble Position");return result;}
CellComponent* cell(ComponentSlots& e){auto* p=e.get(5);auto* result=dynamic_cast<CellComponent*>(p);if(p&&!result)throw std::invalid_argument("incorrect bubble Cell");return result;}
std::int32_t add(std::int32_t a,std::int32_t b){const auto v=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);
    return static_cast<std::int32_t>(v<0x80000000U?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}
}
BubbleOffsets initial_bubble_offsets(){
    auto result=std::make_shared<std::vector<EventInstruction>>();
    result->push_back(std::make_shared<EventIntegers>(EventIntegers{-36,0,-30}));
    result->push_back(std::make_shared<EventIntegers>(EventIntegers{-36,0,-20}));return result;
}
ComponentSlots& add_bubble(ComponentSlots& e,std::int32_t id,std::int32_t type,ComponentSlots* parent,
                          std::int32_t maximum,BubblePool& pool){
    auto& b=pool.acquire_bubble();b.frame=0;b.frame_max=maximum;b.data_id=id;b.type=type;
    const auto offsets=pool.bubble_offsets();const auto xyz=required(offsets.get()).at(static_cast<std::size_t>(type));
    auto& values=required(xyz.get());b.offset_x=values.at(0);b.offset_y=values.at(1);b.offset_z=values.at(2);
    b.parent=parent;return e.add(18,&b);
}
void initialize_bubbles(BubbleFrameState& state,BubbleInitializationWorld* world){state.subset=required(world).bubble_subset({18});}
void update_bubble_display(ComponentSlots* value){
    auto& e=required(value);auto* target=position(e);
    auto& source=required(position(required(bubble(e).parent)));const auto x=source.x;
    required(target).x=static_cast<float>(add(factory_cell_coordinate(x,1),bubble(e).offset_x));
    target=position(e);const auto y=required(position(required(bubble(e).parent))).y;
    required(target).y=static_cast<float>(add(factory_cell_coordinate(y,1),bubble(e).offset_y));
    target=position(e);const auto z=required(position(required(bubble(e).parent))).z;
    required(target).z=static_cast<float>(add(factory_cell_coordinate(z,1),bubble(e).offset_z));
    auto* destination=cell(e);const auto xi=required(cell(required(bubble(e).parent))).xi;
    required(destination).xi=xi;
    destination=cell(e);const auto yi=required(cell(required(bubble(e).parent))).yi;required(destination).yi=yi;
}
void update_bubbles(BubbleFrameState& state,BubbleFrameHost& host){
    {
        auto cursor=required(state.subset).cursor();
        while(const auto entry=cursor.next()){
            auto& e=required(*entry);auto* parent=bubble(e).parent;
            if(!parent||host.bubble_parent_destroyed(*parent)){required(state.pending.get()).push_back(&e);continue;}
            auto& frame=bubble(e).frame;frame=add(frame,1);
            if(bubble(e).frame_max!=-1&&bubble(e).frame>bubble(e).frame_max)required(state.pending.get()).push_back(&e);
            update_bubble_display(&e); // Expiry-queued entities still update their geometry.
        }
    }
    // Public Enumerable.Reverse buffers the fresh source, then returns it backwards.
    std::vector<ComponentSlots*> reversed;
    {auto cursor=required(state.subset).cursor();while(const auto entry=cursor.next())reversed.push_back(*entry);}
    for(auto at=reversed.rbegin();at!=reversed.rend();++at){
        auto& e=required(*at);const auto seen=state.seen;auto* parent=bubble(e).parent;auto& values=required(seen.get());
        if(std::find(values.begin(),values.end(),parent)!=values.end())required(state.pending.get()).push_back(&e);
        else{const auto target=state.seen;parent=bubble(e).parent;required(target.get()).push_back(parent);}
    }
    required(state.seen.get()).clear();
    for(;;){
        auto& pending=required(state.pending.get());if(pending.empty())return;auto* e=pending.at(0);
        host.destroy_bubble(required(e),true);auto& current=required(state.pending.get());
        if(current.empty())throw std::out_of_range("bubble removal queue was cleared by callback");
        current.erase(current.begin());
    }
}
}
