#include "core/gameplay/member_selection.hpp"
#include "core/gameplay/contact_collision.hpp"
#include <cstdlib>
#include <limits>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct PlayerFixture final:ComponentObserver {
    ComponentEvents events_a,events_b;ComponentSlots a{35,&events_a},b{35,&events_b};Component tag;
    EntityRegistry registry{*this};DynamicSubsets groups{registry,[&](const ComponentSlots& e){return &e==&a?1:2;}};
    PlayerWorldState world{nullptr,&groups};
    PlayerFixture(){EntityRegistration one{1,&a,&events_a},two{2,&b,&events_b};registry.add(&one,false);registry.add(&two,false);}
    void added(ComponentSlots& e,std::int32_t i,Component* c)override{groups.added(e,i,c);}
    void removed(ComponentSlots& e,std::int32_t i,Component* c)override{groups.removed(e,i,c);}
    void changed(ComponentSlots& e,std::int32_t i,Component* a_,Component* b_)override{groups.changed(e,i,a_,b_);}
};
int main(){
    ComponentSlots a(35),b(35),c(35);ContactPosition pa,pb,pc;
    a.add(0,&pa);b.add(0,&pb);c.add(0,&pc);
    std::vector<ComponentSlots*> members{&a,&b,&c};
    pa.z=20;pb.z=10;pc.z=10;check(front_line_human(&members)==&b);
    pb.z=30;check(front_line_human(&members)==&c); // Read current coordinates every call.
    HumanRecordLink human;human.state=4;c.add(14,&human);check(front_line_human(&members)==&c); // No death or Human filter.
    members.erase(members.begin()+2);check(front_line_human(&members)==&a); // List membership, not dead-state filtering.
    pa.z=std::numeric_limits<float>::max();pb.z=std::numeric_limits<float>::infinity();check(!front_line_human(&members));
    pa.z=std::numeric_limits<float>::quiet_NaN();pb.z=-std::numeric_limits<float>::infinity();check(front_line_human(&members)==&b);
    members.push_back(nullptr);fails([&]{front_line_human(&members);}); // No early exit after finding -infinity.
    members.pop_back();pa.z=-0.0F;pb.z=0.0F;check(front_line_human(&members)==&a);
    b.remove(0);fails([&]{front_line_human(&members);});
    members.clear();check(!front_line_human(&members));fails([]{front_line_human(nullptr);});

    {PlayerFixture f;check(!member_hero(&f.world)&&!f.world.cached_player);
     f.b.add(30,&f.tag);check(member_hero(&f.world)==&f.b&&f.world.cached_player==&f.b);
     f.a.add(30,&f.tag);check(member_hero(&f.world)==&f.b); // Cached identity is not reselected.
     f.b.remove_all();f.registry.erase(2);check(member_hero(&f.world)==&f.b); // No destroyed/registration/component guard.
     f.world.groups=nullptr;check(world_player_entity(f.world)==&f.b); // Cache hit needs no manager.
     f.world.cached_player=nullptr;fails([&]{world_player_entity(f.world);});
    }
    {PlayerFixture f;f.a.add(30,&f.tag);f.b.add(30,&f.tag);
     auto& group=f.groups.get(std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{30}));
     std::vector<ComponentSlots*> diagnostic_cache{&f.b};group.cached_entities=&diagnostic_cache;
     check(group.first_entity()==&f.a&&group.cached_entities==&diagnostic_cache);
     check(world_player_entity(f.world)==&f.a); // First member in HashSet order, not a Human/data-main lookup.
     f.world.cached_player=nullptr;f.a.remove(30);check(member_hero(&f.world)==&f.b);
     f.b.remove(30);f.world.cached_player=nullptr;check(!member_hero(&f.world));
     f.a.add(30,&f.tag);check(member_hero(&f.world)==&f.a); // Null misses are retried.
    }
    fails([]{member_hero(nullptr);});
}
