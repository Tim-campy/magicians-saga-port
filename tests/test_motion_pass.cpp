#include "core/gameplay/pass_initialization.hpp"
#include "core/ecs/update_scheduler.hpp"
#include "core/gameplay/motion_pass.hpp"
#include <array>
#include <cstdlib>
#include <functional>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct World final:ProjectileMotionHost,ProjectileLaunchHost,ComponentObserver,PassInitializationWorld {
    const std::vector<std::int32_t> projectile_slots{27,0,1},movement_slots{0,1};
    SubsetFilter projectile_filter{&projectile_slots},movement_filter{&movement_slots};
    EntitySubset projectiles{projectile_filter,[this](const ComponentSlots& e){return id(e);}};
    EntitySubset movers{movement_filter,[this](const ComponentSlots& e){return id(e);}};
    SubsetMembership membership{{&projectiles,&movers}};
    ComponentSlots a{35,this},b{35,this},c{35,this};std::array<ComponentSlots*,3> entities{{&a,&b,&c}};
    std::array<ContactPosition,3> positions;std::array<ProjectileSpeed,3> speeds;
    std::array<ProjectileLink,3> links;std::array<ProjectileModification,3> modifications;
    std::vector<int> impacts,returned;int allocation=0;std::function<void(ComponentSlots&)> on_impact;
    std::function<void()> on_pool;EntityEvents events;
    EntitySubset* subset(const std::vector<std::int32_t>& all)override{
        if(all==projectile_slots)return &projectiles;if(all==movement_slots)return &movers;
        throw std::invalid_argument("unexpected subset");
    }
    void subscribe_impact(EntityEvents::Single receiver,std::optional<EventHashes> hashes)override{events.subscribe(23,std::move(receiver),hashes);}
    int id(const ComponentSlots& e)const{for(int i=0;i<3;++i)if(entities[static_cast<std::size_t>(i)]==&e)return i;throw std::invalid_argument("unknown entity");}
    World(){for(std::size_t i=0;i<3;++i)entities[i]->add(0,&positions[i]).add(1,&speeds[i]);}
    void added(ComponentSlots& e,std::int32_t s,Component* c)override{membership.added(e,s,c);}
    void removed(ComponentSlots& e,std::int32_t s,Component* c)override{membership.removed(e,s,c);}
    void changed(ComponentSlots& e,std::int32_t s,Component* a,Component* b)override{membership.changed(e,s,a,b);}
    ProjectileLink& acquire_projectile()override{return links.at(static_cast<std::size_t>(allocation++));}
    ProjectileModification& acquire_modification()override{return modifications.at(static_cast<std::size_t>(allocation-1));}
    void add_animation(ComponentSlots&,std::int32_t,std::int32_t)override{check(false);}
    void return_component(std::int32_t slot,Component*)override{returned.push_back(slot);if(on_pool)on_pool();}
    void send_impact(ComponentSlots& e)override{check(e.contains(27));impacts.push_back(id(e));if(on_impact)on_impact(e);events.send(23,&e);}
    void launch(int i,bool finish_now=false){
        auto& e=*entities.at(static_cast<std::size_t>(i));
        launch_projectile(e,{},{0,0,40},4,nullptr,false,true,200,*this);
        if(finish_now){auto& p=*dynamic_cast<ProjectileLink*>(e.get(27));p.frame=1;p.max_height=0;p.max_frame=0;}
    }
};
struct Cleanup final:CombatEventHost {
    bool destroyed(ComponentSlots&)override{return false;}
    void destroy(ComponentSlots&,bool)override{check(false);}
    ComponentSlots* search_opponent(ComponentSlots&)override{check(false);return nullptr;}
};
int main(){
    {World w;Cleanup cleanup;ProjectileSystemState state;initialize_projectiles(state,&w,cleanup);w.launch(0,true);
        w.on_impact=[](ComponentSlots&){throw std::runtime_error("retain completion");};bool failed=false;
        try{state.pass->update(*state.subset,w);}catch(const std::runtime_error&){failed=true;}
        check(failed&&w.impacts==std::vector<int>{0});w.a.remove(1);w.on_impact=nullptr;
        initialize_projectiles(state,&w,cleanup);state.pass->update(*state.subset,w);
        check(w.impacts==std::vector<int>{0}&&w.a.contains(27)); // Init discards the retained queue.
    }

    {World w;Cleanup cleanup;ProjectileSystemState state;initialize_projectiles(state,&w,cleanup);
        EntitySubset* movers=nullptr;initialize_movement(movers,&w);auto& pass=*state.pass;w.launch(0);check(w.projectiles.size()==1&&w.movers.size()==3);
        bool enabled=true;
        GameplayUpdate projectile(10,enabled,[&](std::int32_t){pass.update(*state.subset,w);});
        GameplayUpdate movement(11,enabled,[&](std::int32_t){move_subset(*movers);});
        // This projection binds two real bodies; it is not the complete 26-system frame.
        std::vector<UpdateSystem*> systems{&projectile,&movement};UpdateSchedule schedule{&systems,nullptr};
        enabled=false;update_systems(schedule,1);check(w.links[0].frame==0&&w.positions[0].z==0);
        enabled=true;for(int frame=0;frame<11;++frame)update_systems(schedule,1);
        check(w.impacts==std::vector<int>{0}&&w.projectiles.size()==0&&w.movers.size()==3);
        check(w.positions[0].z==40&&w.positions[0].y==0&&w.speeds[0].z==0);
        check(w.returned==std::vector<int>{15,27});}
    {World w;ProjectilePass pass;w.launch(0,true);w.launch(1,true);
        w.on_impact=[&](ComponentSlots& e){if(&e==&w.a){check(w.links[1].frame==2&&w.positions[1].z==40&&w.b.contains(27));w.launch(2);}};
        pass.update(w.projectiles,w);check(w.impacts==std::vector<int>{0,1}&&w.projectiles.size()==1&&w.links[2].frame==0);
        move_subset(w.movers);check(w.positions[2].z==0);pass.update(w.projectiles,w);check(w.links[2].frame==1);}
    {World w;w.positions[1].parent=&w.a;w.positions[1].offset_x=2;w.speeds[0].x=3;
        move_subset(w.movers);check(w.positions[0].x==3&&w.positions[1].x==5);
        // Remove/rejoin changes parent/child update ordering through real group slots.
        w.a.remove(1);w.b.remove(1);w.a.add(1,&w.speeds[0]);w.b.add(1,&w.speeds[1]);
        move_subset(w.movers);check(w.positions[0].x==6&&w.positions[1].x==5);}
    {World w;ProjectilePass pass;w.launch(0,true);w.launch(1,true);w.on_impact=[](ComponentSlots&){throw std::runtime_error("impact");};
        bool failed=false;try{pass.update(w.projectiles,w);}catch(const std::runtime_error&){failed=true;}
        check(failed&&w.impacts==std::vector<int>{0}&&w.a.contains(27)&&w.b.contains(27));
        // Remove from the live subset while retaining Projectile for the saved completion queue.
        w.a.remove(1);w.b.remove(1);w.on_impact=nullptr;pass.update(w.projectiles,w);
        check(w.impacts==std::vector<int>{0,0,1}&&!w.a.contains(27)&&!w.b.contains(27));}
    {World w;ProjectilePass pass;w.launch(0,true);w.launch(1,true);bool once=false;
        w.on_pool=[&]{if(!once){once=true;w.b.remove(1);}};
        bool failed=false;try{pass.update(w.projectiles,w);}catch(const std::runtime_error&){failed=true;}
        check(failed&&w.impacts.empty()&&w.links[0].frame==2&&w.links[1].frame==1);
        w.a.remove(1);w.on_pool=nullptr;pass.update(w.projectiles,w);check(w.impacts==std::vector<int>{0});}
    {World w;ProjectilePass pass;w.launch(0,true);w.on_impact=[](ComponentSlots& e){e.remove(27);};
        pass.update(w.projectiles,w);
        check(w.projectiles.size()==0&&w.impacts==std::vector<int>{0});
        pass.update(w.projectiles,w);check(w.impacts==std::vector<int>{0});} // Successful drain cleared its queue.
}
