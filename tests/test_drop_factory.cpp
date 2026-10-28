#include "core/gameplay/drop_factory.hpp"
#include "core/gameplay/enemy_defeat.hpp"
#include "core/gameplay/combat_rules.hpp"
#include "core/ecs/dynamic_subsets.hpp"
#include <cstdlib>
#include <deque>
#include <stdexcept>
using namespace saga::core;
void check(bool v){if(!v)std::abort();}
struct Node {
    int id;ComponentEvents events;ComponentSlots entity{35,&events};EntityRegistration registration{id,&entity,&events};
    DropComponent drop;ContactPosition position;ProjectileSpeed speed;EntityDepth depth;EntitySprite sprite;ProjectileModification modification;
    Component cell;std::array<std::int32_t,6> image{};int cell_x=0,cell_z=0;explicit Node(int n):id(n){}
};
struct Host final:DropFactoryHost,ComponentObserver {
    std::deque<Node> nodes;Component sync;EntityRegistry registry{*this};std::vector<int> calls;bool fail_image=false;
    DynamicSubsets groups{registry,[&](const ComponentSlots& e){for(auto& n:nodes)if(&n.entity==&e)return n.id;throw std::invalid_argument("id");}};
    bool registered_id(std::int32_t id)override{return registry.contains(id);}
    ComponentSlots* create(std::int32_t type,std::int32_t id)override{check(type==0);calls.push_back(-1);nodes.emplace_back(id);return &nodes.back().entity;}
    void register_entity(ComponentSlots& e,bool temporary)override{check(&e==&nodes.back().entity&&!temporary);registry.add(&nodes.back().registration,false);}
    void added(ComponentSlots& e,std::int32_t i,Component* c)override{calls.push_back(i);groups.added(e,i,c);}
    void removed(ComponentSlots& e,std::int32_t i,Component* c)override{groups.removed(e,i,c);}
    void changed(ComponentSlots& e,std::int32_t i,Component* a,Component* b)override{groups.changed(e,i,a,b);}
    DropComponent& acquire_drop()override{return nodes.back().drop;}
    ContactPosition& acquire_position()override{return nodes.back().position;}
    ProjectileSpeed& acquire_speed()override{return nodes.back().speed;}
    EntityDepth& acquire_depth()override{return nodes.back().depth;}
    EntitySprite& acquire_sprite()override{return nodes.back().sprite;}
    ProjectileModification& acquire_modification()override{return nodes.back().modification;}
    Component* sync_animation()override{return &sync;}
    void add_cell(ComponentSlots& e,std::int32_t x,std::int32_t z)override{nodes.back().cell_x=x;nodes.back().cell_z=z;e.add(5,&nodes.back().cell);}
    void add_image(ComponentSlots&,const std::array<std::int32_t,6>& args)override{calls.push_back(-2);nodes.back().image=args;if(fail_image)throw std::runtime_error("image");}
};
struct Rewards final:EnemyRewardHost {
    Host world;EntityIdState ids;TreasureTable treasures{TreasureRecord{}};EnemyRecord enemy;int draws=0;
    Rewards(){enemy.experience=EventIntegers{10,10};enemy.treasures=EnemyTreasureGroups{{{0,1000},{9,1000}}};treasures[0]->image=87;}
    const EnemyRecord& enemy_data()override{return enemy;}
    bool hits1000(std::int32_t threshold)override{check(threshold==1000);++draws;return true;}
    void spawn_experience_drop(std::int32_t value)override{
        auto* e=create_drop(0,value,{5,20,30},easing(25,50,500,value,-50),treasures,ids,world);
        dynamic_cast<DropDepth&>(*e->get(4)).extra=-10;
    }
    void spawn_treasure_drop(std::int32_t value)override{
        auto* e=create_drop(3,value,{10,0,40},25,treasures,ids,world);dynamic_cast<DropDepth&>(*e->get(4)).extra=-10;
    }
};
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
int main(){
    for(int type=0;type<4;++type){Host h;EntityIdState ids;TreasureTable treasures{TreasureRecord{}};treasures[0]->image=87;
        auto& group=h.groups.get(std::make_shared<ComponentGroupFilter>(std::vector<std::int32_t>{34}));
        auto* entity=create_drop(type,0,{-47.9F,3,48.9F},25,treasures,ids,h);auto& n=h.nodes.back();
        check(entity==&n.entity&&ids.counter==1&&group.size()==1&&h.registry.find(0)==entity);
        auto expected=std::vector<int>{-1,34,0,1,5,4,2,-2,15};if(type!=3)expected.push_back(13);check(h.calls==expected);
        check(n.drop.type==type&&n.drop.value==0&&n.drop.frame==-1&&n.drop.x0==-47.9F&&n.drop.z0==48.9F);
        check(n.cell_x==-1&&n.cell_z==2&&n.depth.extra==10&&!n.position.parent&&n.speed.z==0);
        check(n.sprite.animation.resource==7&&n.sprite.animation.clip==(type==2?43:type==3?41:42));
        check(n.image[0]==(type==0?98:type==1?105:type==2?-1:87)&&n.image[1]==7);
        check(n.modification.type==10&&n.modification.anchor==34&&n.modification.loop&&!n.modification.destroy&&n.modification.alpha==255);
        check(n.modification.scale_x==(type>=2?1.0F:0.25F)&&entity->contains(13)==(type!=3));}
    {Host h;EntityIdState ids;TreasureTable empty;check(!create_drop(-1,0,{},25,empty,ids,h)&&!create_drop(4,0,{},25,empty,ids,h));
        check(ids.counter==0&&h.nodes.empty());fails([&]{create_drop(3,0,{},25,empty,ids,h);});check(ids.counter==0);
        create_drop(1,1,{},-50,empty,ids,h);check(h.nodes.back().image[0]==99&&h.nodes.back().modification.scale_x==-0.5F);}
    {Host h;EntityIdState ids;h.fail_image=true;TreasureTable empty;fails([&]{create_drop(0,8,{},25,empty,ids,h);});
        check(ids.counter==1&&h.registry.contains(0)&&h.nodes.back().entity.contains(2)&&!h.nodes.back().entity.contains(15));}
    {Rewards r;spawn_enemy_rewards(1,r);check(r.draws==1&&r.ids.counter==2&&r.world.nodes.size()==2);
        check(r.world.nodes[0].drop.type==0&&r.world.nodes[0].drop.value==10&&r.world.nodes[0].depth.extra==-10);
        check(r.world.nodes[1].drop.type==3&&r.world.nodes[1].modification.scale_x==1&&r.world.nodes[1].depth.extra==-10);}
}
