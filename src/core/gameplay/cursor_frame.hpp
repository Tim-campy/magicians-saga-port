#pragma once
#include "cell_system.hpp"

namespace saga::core {
struct CursorPoint {std::int32_t x=0,y=0;};
using CursorPointRef=std::shared_ptr<CursorPoint>;
struct CursorFocusEffect {ComponentSlots* entity_arg=nullptr;};
struct CursorState {
    CursorPointRef last_position; // CursorRenderSystem +0x20.
    ComponentSlots* focus=nullptr;bool tracking=false;std::int32_t focus_wait=0;
    ComponentSlots* catch_human=nullptr; // +0x28/+0x30/+0x34/+0x38.
};
class CursorInitializationWorld {
public:
    using Receiver=std::function<void(CursorPointRef)>;
    virtual ~CursorInitializationWorld()=default;
    virtual void subscribe_cursor_point(std::int32_t event,Receiver)=0; // Events 2 and 4.
};
class CursorHost {
public:
    virtual ~CursorHost()=default;
    virtual EventInstruction cursor_save()=0; // Fresh decoded normal-save array per access.
    virtual MapComponent* cursor_map()=0;
    virtual EntitySubset* cursor_humans()=0; // Current global world's And(14), native order.
    virtual bool cursor_destroyed(ComponentSlots&)=0;
    virtual void cursor_cell_changed(CursorPointRef old_cell,CursorPointRef new_cell)=0; // Event 13.
    virtual CursorFocusEffect* create_cursor_focus_effect()=0; // GameView output; result.arg is set after return.
};
// Coordinate.IsometricToCellLocation 0x112849c -> TopViewToCellLocation 0x11285ac.
// Return a value copy of the native shared scratch point, without exposing global storage.
CursorPoint cursor_cell(std::int32_t step_x,std::int32_t step_z,float x,float y);
void initialize_cursor(CursorState&,CursorInitializationWorld*,CursorHost&); // 0x1147294.
void update_cursor(CursorState&,CursorHost&); // 0x114747c; enabled-only scheduler gate.
ComponentSlots* cursor_human_at(std::int32_t x,std::int32_t y,CursorHost&); // 0x114782c.
void cursor_touch_down(CursorState&,CursorPointRef,CursorHost&); // 0x114772c.
void start_cursor_catch(CursorState&,ComponentSlots*); // 0x1147c0c.
// OnCameraTouchClick 0x1147c08 has an inert native body.
void cursor_touch_click(CursorPointRef);
}
