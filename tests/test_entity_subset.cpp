#include "core/ecs/entity_subset.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
std::vector<ComponentSlots*> members(EntitySubset& group){std::vector<ComponentSlots*> out;auto c=group.cursor();while(const auto e=c.next())out.push_back(*e);return out;}
struct Observer final:ComponentObserver {
    EntitySubset& group;int additions=0,removals=0,changes=0;bool fail_remove=false;
    explicit Observer(EntitySubset& g):group(g){}
    void added(ComponentSlots&,std::int32_t,Component*)override{check(group.cached_entities==nullptr);++additions;}
    void removed(ComponentSlots&,std::int32_t,Component*)override{++removals;if(fail_remove)throw std::runtime_error("observer");}
    void changed(ComponentSlots&,std::int32_t,Component*,Component*)override{++changes;}
};
int main(){
    SubsetFilter any;const auto identity=[](const ComponentSlots&){return 7;};
    {EntitySubset group(any,identity);ComponentSlots a(35),b(35),c(35),d(35),e(35);Observer observer(group);group.notifications=&observer;
        check(!group.add(nullptr));group.add(&a);group.add(&b);group.add(&c);check(observer.additions==0&&group.size()==3);
        auto cursor=group.cursor();group.add(&a,0,nullptr);group.remove(&d,0,nullptr);check(cursor.next()==&a&&observer.additions==1);
        group.remove(&b,0,nullptr);group.remove(&a,0,nullptr);group.add(&d);group.add(&e);
        check(members(group)==std::vector<ComponentSlots*>{&d,&e,&c}); // LIFO reuse, reference equality despite identical IDs.
        bool failed=false;try{cursor.next();}catch(const std::runtime_error&){failed=true;}check(failed);
        group.remove(&d,0,nullptr);group.remove(&e,0,nullptr);group.remove(&c,0,nullptr);group.add(&b);group.add(&a);
        check(members(group)==std::vector<ComponentSlots*>{&b,&a});}
    {std::vector<std::int32_t> all{0,32};SubsetFilter filter{&all};EntitySubset group(filter,identity);Observer observer(group);group.notifications=&observer;
        SubsetMembership membership({&group});ComponentSlots e(35,&membership);Component p,c,other;
        e.add(0,&p);check(group.size()==0);e.add(32,&c);check(group.size()==1&&observer.additions==1);
        auto cursor=group.cursor();e.add(5,&other);check(observer.additions==2&&cursor.next()==&e); // Matching duplicate still notifies.
        e.change(32,nullptr);check(group.size()==1&&observer.changes==1); // No membership recomputation on Change.
        e.remove(5);check(group.size()==0&&observer.removals==1);
        e.add(32,&c);check(group.size()==1);e.remove_all();check(group.size()==0);
        all={5};check(!group.add(&e));e.add(5,&other);check(group.size()==1);} // Live filter references.
    {EntitySubset group(any,identity);Observer observer(group);group.notifications=&observer;ComponentSlots e(35);std::vector<ComponentSlots*> cache;
        group.add(&e);group.cached_entities=&cache;observer.fail_remove=true;bool failed=false;
        try{group.remove(&e,0,nullptr);}catch(const std::runtime_error&){failed=true;}
        check(failed&&group.size()==0&&group.cached_entities==&cache);group.remove(&e,0,nullptr);check(group.cached_entities==nullptr);}
    {int id=1;EntitySubset group(any,[&](const ComponentSlots&){return id;});ComponentSlots e(35);group.add(&e);id=2;group.remove(&e,0,nullptr);check(group.size()==1);
        group.add(&e);check(group.size()==2&&members(group)==std::vector<ComponentSlots*>{&e,&e});id=1;group.remove(&e,0,nullptr);check(group.size()==1);} // Stored hash is not recomputed.
    {EntitySubset group(any,identity);ComponentSlots a(35),b(35);group.add(&a);auto cursor=group.cursor();check(cursor.next()==&a&&!cursor.next());group.add(&b);
        bool failed=false;try{cursor.next();}catch(const std::runtime_error&){failed=true;}check(failed);} // Even an exhausted cursor checks its version.
}
