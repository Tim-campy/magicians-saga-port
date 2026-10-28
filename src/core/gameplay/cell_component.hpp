#pragma once
#include "../ecs/component_slots.hpp"
#include <memory>
#include <vector>
namespace saga::core {
struct CellComponent final:Component {
    std::int32_t xi=0,yi=0; // Native +0x10/+0x14; yi is the grid's Z coordinate here.
    ComponentSlots* parent=nullptr; // +0x18.
    std::shared_ptr<std::vector<ComponentSlots*>> children; // +0x20; list identity survives replacement.
};
class CellPool {
public:
    virtual ~CellPool()=default;
    virtual CellComponent& acquire_cell()=0;
};
// Entity.AddCell 0x112cfe4, with out-of-line load at 0x25fe1d8 returning to 0x112d004.
ComponentSlots& add_cell_component(ComponentSlots&,std::int32_t xi,std::int32_t yi,ComponentSlots* parent,CellPool&);
}
