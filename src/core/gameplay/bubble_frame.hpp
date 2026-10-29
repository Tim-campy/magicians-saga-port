#pragma once
#include "cell_system.hpp"
#include <array>

namespace saga::core {
struct EntityBubble final:Component {
    std::int32_t data_id=0,type=0,frame=0,frame_max=0,offset_x=0,offset_y=0,offset_z=0;
    ComponentSlots* parent=nullptr; // FukidashiComponent +0x10..+0x30.
};
using BubbleOffsets=std::shared_ptr<std::vector<EventInstruction>>;
BubbleOffsets initial_bubble_offsets(); // FukidashiComponent..cctor: human/map-chip XYZ arrays.
class BubblePool {
public:
    virtual ~BubblePool()=default;
    virtual EntityBubble& acquire_bubble()=0;
    virtual BubbleOffsets bubble_offsets()=0;
};
ComponentSlots& add_bubble(ComponentSlots&,std::int32_t data_id,std::int32_t type,
                          ComponentSlots* parent,std::int32_t frame_max,BubblePool&); // 0x112e9f0.
struct BubbleFrameState {
    EntitySubset* subset=nullptr;
    CellEntities seen=std::make_shared<std::vector<ComponentSlots*>>(); // +0x28.
    CellEntities pending=std::make_shared<std::vector<ComponentSlots*>>(); // +0x30; preserve duplicates.
};
class BubbleInitializationWorld {
public:
    virtual ~BubbleInitializationWorld()=default;
    virtual EntitySubset* bubble_subset(const EventIntegers&)=0;
};
class BubbleFrameHost {
public:
    virtual ~BubbleFrameHost()=default;
    virtual bool bubble_parent_destroyed(ComponentSlots&)=0;
    virtual void destroy_bubble(ComponentSlots&,bool remove_registration)=0;
};
void initialize_bubbles(BubbleFrameState&,BubbleInitializationWorld*); // 0x11523d4.
void update_bubbles(BubbleFrameState&,BubbleFrameHost&); // 0x1152470.
void update_bubble_display(ComponentSlots*); // 0x1152c54: world XYZ and Cell, not font output.
}
