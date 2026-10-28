#include "cell_component.hpp"
namespace saga::core {
ComponentSlots& add_cell_component(ComponentSlots& entity,std::int32_t xi,std::int32_t yi,ComponentSlots* parent,CellPool& pool){
    auto& cell=pool.acquire_cell();cell.xi=xi;cell.yi=yi;cell.parent=parent;
    cell.children=std::make_shared<std::vector<ComponentSlots*>>();
    return entity.add(5,&cell);
}
}
