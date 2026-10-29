#pragma once
#include "gauge_drawing.hpp"
#include "cell_system.hpp"

namespace saga::core {
struct EntityGauge final:Component {
    std::int32_t type=0,from=0,to=0,maximum=0; // GaugeComponent +0x10..+0x1c.
    std::int32_t frame=0,frame_max=0,offset_x=0,offset_y=0,offset_z=0;
    std::int32_t flags=0,option=0,option2=0; // +0x34/+0x38/+0x3c.
    ComponentSlots* parent=nullptr; // +0x40.
};
using GaugeOffsets=std::shared_ptr<std::vector<EventInstruction>>;
GaugeOffsets initial_gauge_offsets(); // GaugeComponent..cctor 0x113fd34.
struct GaugeCreation {
    ComponentSlots* parent=nullptr;
    std::int32_t type=0,from=0,to=0,maximum=0,image=0,frame_max=0,flags=0,option=0,option2=0;
};
class GaugePool {
public:
    virtual ~GaugePool()=default;
    virtual EntityGauge& acquire_gauge()=0;
    virtual GaugeOffsets gauge_offsets()=0;
};
class GaugeFactoryHost:public EntityVisualHost,public GaugePool {
public:
    virtual StageGeometry* gauge_map()=0;
};
struct GaugePlacement {
    std::int32_t x=0,z=0,xi=0,yi=0,resource=23,clip=0,step_x=0,step_z=0;
};
ComponentSlots& add_gauge(ComponentSlots&,GaugeCreation,GaugePool&); // Entity.AddGauge 0x112ef1c.
ComponentSlots& create_gauge_at(GaugeCreation,GaugePlacement,EntityIdState&,GaugeFactoryHost&,GaugeDrawState&,GaugeDrawingHost&); // World._CreateGauge 0x113480c.
ComponentSlots& create_gauge(GaugeCreation,EntityIdState&,GaugeFactoryHost&,GaugeDrawState&,GaugeDrawingHost&); // World.CreateGauge 0x113460c.
struct GaugeFrameState {
    EntitySubset* subset=nullptr;
    CellEntities pending; // Init replaces +0x28; constructor leaves it null.
};
class GaugeInitializationWorld {
public:
    virtual ~GaugeInitializationWorld()=default;
    virtual EntitySubset* gauge_subset(const EventIntegers&)=0;
};
class GaugeFrameHost {
public:
    virtual ~GaugeFrameHost()=default;
    virtual bool gauge_parent_destroyed(ComponentSlots&)=0;
    virtual void destroy_gauge(ComponentSlots&,bool remove_registration)=0;
};
void initialize_gauges(GaugeFrameState&,GaugeInitializationWorld*); // GaugeSystem.Init 0x11535ac.
void update_gauge_display(ComponentSlots*,GaugeDrawState&,GaugeDrawingHost&); // 0x1153a64.
void update_gauges(GaugeFrameState&,GaugeDrawState&,GaugeFrameHost&,GaugeDrawingHost&); // 0x1153698.
} // namespace saga::core
