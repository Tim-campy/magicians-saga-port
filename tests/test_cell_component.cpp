#include "core/gameplay/cell_component.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Pool final:CellPool {
    CellComponent cell;int calls=0;bool fail=false;
    CellComponent& acquire_cell()override{++calls;if(fail)throw std::runtime_error("pool");return cell;}
};
struct Observer final:ComponentObserver {
    int calls=0;bool fail=false;
    void added(ComponentSlots& e,std::int32_t i,Component* c)override{
        check(i==5&&e.get(5)==c);auto& cell=dynamic_cast<CellComponent&>(*c);
        check(cell.children&&cell.children->empty());++calls;if(fail)throw std::runtime_error("added");
    }
};
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
int main(){
    Pool pool;Observer observer;ComponentSlots entity(35,&observer),parent(35),child(35);
    pool.cell.children=std::make_shared<std::vector<ComponentSlots*>>(1,&child);auto previous=pool.cell.children;
    check(&add_cell_component(entity,-7,99,&parent,pool)==&entity);
    check(pool.calls==1&&observer.calls==1&&pool.cell.xi==-7&&pool.cell.yi==99&&pool.cell.parent==&parent);
    check(previous!=pool.cell.children&&previous->size()==1&&previous->at(0)==&child);
    auto first_list=pool.cell.children;first_list->push_back(&child);
    add_cell_component(entity,1,2,nullptr,pool); // Same pooled object is updated, but occupied slot does not notify.
    check(pool.calls==2&&observer.calls==1&&pool.cell.xi==1&&!pool.cell.parent&&first_list->size()==1&&pool.cell.children->empty());
    Pool other;add_cell_component(entity,5,6,&entity,other);
    check(entity.get(5)==&pool.cell&&observer.calls==1&&other.cell.parent==&entity&&other.cell.children);
    entity.remove(5);observer.fail=true;fails([&]{add_cell_component(entity,8,9,nullptr,pool);});
    check(entity.get(5)==&pool.cell&&pool.cell.xi==8&&pool.cell.children);
    pool.fail=true;fails([&]{add_cell_component(entity,10,11,nullptr,pool);});check(pool.cell.xi==8);
}
