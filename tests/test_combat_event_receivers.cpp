#include "core/gameplay/combat_event_receivers.hpp"
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Host final:CombatEventHost {
    bool dead=false;int destroys=0,searches=0;
    std::function<ComponentSlots*(ComponentSlots&)> search;
    bool destroyed(ComponentSlots&)override{return dead;}
    void destroy(ComponentSlots& e,bool remove)override{check(remove);++destroys;e.remove_all();dead=true;}
    ComponentSlots* search_opponent(ComponentSlots& e)override{++searches;return search?search(e):nullptr;}
};
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
int main(){
    {Host h;ComponentSlots e(35);Component emitter,cell,projectile;
        fails([&]{projectile_impact_cleanup(nullptr,h);});
        projectile_impact_cleanup(&e,h);e.add(5,&cell);
        fails([&]{projectile_impact_cleanup(&e,h);});e.add(27,&projectile);
        projectile_impact_cleanup(&e,h);check(e.contains(27));
        e.add(31,&emitter);projectile_impact_cleanup(&e,h);
        check(h.destroys==1&&h.dead&&!e.contains(27));}
    {Host h;h.dead=true;ComponentSlots e(35);Component emitter;e.add(31,&emitter);
        projectile_impact_cleanup(&e,h);check(h.destroys==1);} // Destroy precedes the destroyed guard.
    {Host h;std::vector<std::int32_t> all{24};SubsetFilter filter{&all};
        EntitySubset group(filter,[](const ComponentSlots&){return 1;});
        ComponentSlots a(35),b(35),dead(35),target(35);EnemyRecordLink first,second,replaced;
        a.add(24,&first);b.add(24,&second);group.add(&a);group.add(&b);
        first.opponent=&dead;second.opponent=&target;
        h.search=[&](ComponentSlots& e){check(&e==&a);e.change(24,&replaced);return &target;};
        retarget_after_human_death(group,&dead,h);
        check(h.searches==1&&first.opponent==&target&&replaced.opponent==nullptr&&second.opponent==&target);
        h.dead=true;retarget_after_human_death(group,nullptr,h);check(h.searches==1);
        h.dead=false;h.search=[&](ComponentSlots& e)->ComponentSlots*{group.remove(&e,24,e.get(24));return &dead;};
        fails([&]{retarget_after_human_death(group,nullptr,h);});
        check(replaced.opponent==&dead&&group.size()==1); // Store commits before the next cursor invalidation.
        second.opponent=&dead;h.search=[](ComponentSlots&)->ComponentSlots*{throw std::runtime_error("search");};
        fails([&]{retarget_after_human_death(group,&dead,h);});check(second.opponent==&dead);
        b.change(24,nullptr);fails([&]{retarget_after_human_death(group,&dead,h);});}
}
