#include "core/gameplay/subset_cursors.hpp"
#include <array>
#include <cstdlib>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct World final:CollisionPassHost,GarbageUpdateHost {
    const std::vector<std::int32_t> contact_slots{32,0},garbage_slots{22};
    SubsetFilter contact_filter{&contact_slots},garbage_filter{&garbage_slots};
    EntitySubset contacts{contact_filter,[this](const ComponentSlots& e){return static_cast<std::int32_t>(id(e));}};
    EntitySubset garbage{garbage_filter,[this](const ComponentSlots& e){return static_cast<std::int32_t>(id(e));}};
    SubsetMembership membership{{&contacts,&garbage}};
    ComponentSlots a{35,&membership},b{35,&membership},c{35,&membership};
    std::array<ComponentSlots*,3> entities{{&a,&b,&c}};
    std::array<ContactPosition,3> positions;std::array<ProjectileCollision,3> collisions;
    std::array<GarbageLifetime,3> timers;std::array<QuestEntityLifetime,3> lifetimes;
    std::vector<std::pair<QuestMemberId,QuestMemberId>> events;
    QuestMemberId id(const ComponentSlots& e)const{for(std::size_t i=0;i<3;++i)if(entities[i]==&e)return i;throw std::invalid_argument("unknown entity");}
    World(){for(std::size_t i=0;i<3;++i){collisions[i].radius=10;timers[i].frames=i==0?1:2;entities[i]->add(0,&positions[i]).add(32,&collisions[i]).add(22,&timers[i]);}}
    std::unique_ptr<CollisionCursor> open_collision_cursor()override{return open_collision_subset(contacts);}
    std::unique_ptr<GarbageCursor> open_garbage_cursor()override{return open_garbage_subset(garbage,[this](const ComponentSlots& e){return id(e);});}
    void send_collision(std::int32_t event,ComponentSlots& x,ComponentSlots& y)override{check(event==29);events.emplace_back(id(x),id(y));}
    QuestEntityLifetime& entity_lifetime(QuestMemberId i)override{return lifetimes.at(static_cast<std::size_t>(i));}
    void delete_from_manager(QuestMemberId i,bool remove)override{check(remove);entities.at(static_cast<std::size_t>(i))->remove_all();}
};
int main(){
    {World w;CollisionPass collisions;GarbagePass garbage;check(w.contacts.size()==3&&w.garbage.size()==3);
        // Rejoining entities reuse free slots, changing collision delivery order.
        w.a.remove(32);w.b.remove(32);w.a.add(32,&w.collisions[0]);w.b.add(32,&w.collisions[1]);
        collisions.update(w);check(w.events==std::vector<std::pair<QuestMemberId,QuestMemberId>>{{1,0},{1,2},{0,2}});
        garbage.update(w);check(w.contacts.size()==2&&w.garbage.size()==2&&w.lifetimes[0].destroyed);
        w.events.clear();collisions.update(w);check(w.events==std::vector<std::pair<QuestMemberId,QuestMemberId>>{{1,2}});
        garbage.update(w);check(w.contacts.size()==0&&w.garbage.size()==0&&w.lifetimes[1].destroyed&&w.lifetimes[2].destroyed);
        w.events.clear();collisions.update(w);check(w.events.empty());}
    {World w;auto cursor=w.open_collision_cursor();w.a.remove(32);bool failed=false;
        try{cursor->next();}catch(const std::runtime_error&){failed=true;}check(failed);}
    {World w;w.a.change(32,nullptr);check(w.contacts.size()==3);CollisionPass pass;bool failed=false;
        try{pass.update(w);}catch(const std::invalid_argument&){failed=true;}check(failed);} // Preserve original stale-group quirk.
}
