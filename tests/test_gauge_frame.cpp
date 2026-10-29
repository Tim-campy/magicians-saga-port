#include "core/gameplay/gauge_frame.hpp"
#include "core/gameplay/ecs_combat.hpp"
#include <cstdlib>
#include <deque>
#include <iostream>
#include <limits>
#include <map>
#include <set>
using namespace saga::core;
void check_at(bool v,int line){if(!v){std::cerr<<"gauge frame failed at "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Host final:GaugeFrameHost,GaugeInitializationWorld,GaugeDrawingHost {
    std::map<const ComponentSlots*,int> ids;std::set<ComponentSlots*> dead;
    EntitySubset group{[](auto* e){return e->contains(19);},[this](const auto& e){return ids.at(&e);}};
    SubsetMembership membership{{&group}};
    std::deque<ComponentSlots> entities;std::deque<ContactPosition> positions;std::deque<CellComponent> cells;
    std::deque<EntityGauge> gauges;std::deque<EntityImage> images;Component sprite;
    GaugeFrameState state;GaugeDrawState scratch;PixelImage pixels;
    std::vector<ComponentSlots*> destroyed;std::function<void(ComponentSlots&)> on_destroy;std::function<void()> on_alpha;
    bool null_group=false,throw_query=false;int offset=3,alpha_calls=0;
    Host(){pixels.width=100;pixels.height=20;initialize_gauges(state,this);}
    EntitySubset* gauge_subset(const EventIntegers& slots)override{
        check(slots==EventIntegers{19,7,2,0,5});if(throw_query)throw std::runtime_error("query");return null_group?nullptr:&group;
    }
    bool gauge_parent_destroyed(ComponentSlots& e)override{return dead.count(&e)!=0;}
    void destroy_gauge(ComponentSlots& e,bool remove)override{
        check(remove);destroyed.push_back(&e);if(on_destroy)on_destroy(e);if(dead.count(&e))return;
        e.remove_all();dead.insert(&e);
    }
    PixelImage* gauge_image(std::int32_t id)override{check(id==8);return &pixels;}
    std::int32_t gauge_view_width()override{std::abort();}
    std::int32_t gauge_view_height()override{std::abort();}
    std::shared_ptr<std::vector<float>> gauge_alpha(PixelImage&,std::int32_t x,std::int32_t y,std::int32_t w,std::int32_t h,
            std::shared_ptr<std::vector<float>> a)override{
        check(x==0&&y==15&&w==30&&h==2);++alpha_calls;
        for(std::size_t i=0;i<60;++i)a->at(i)=static_cast<int>(i)==offset?1.0F:0.0F;
        if(on_alpha)on_alpha();return a;
    }
    ComponentSlots& node(){
        entities.emplace_back(35,&membership);auto& e=entities.back();ids[&e]=static_cast<int>(ids.size());
        positions.emplace_back();auto& p=positions.back();p.x=100.9F;p.y=-10.7F;p.z=50.4F;e.add(0,&p);
        cells.emplace_back();cells.back().xi=7;cells.back().yi=-8;e.add(5,&cells.back());return e;
    }
    ComponentSlots& make(ComponentSlots* parent,int max=-1){
        auto& e=node();gauges.emplace_back();auto& g=gauges.back();g.parent=parent;g.frame_max=max;g.from=0;g.to=100;g.maximum=100;
        g.offset_x=-2;g.offset_y=4;g.offset_z=-6;e.add(19,&g);e.add(2,&sprite);
        images.emplace_back();auto& a=images.back().arrays;
        a.textures=std::make_shared<EventIntegers>(1,8);a.resources=std::make_shared<EventIntegers>(1,999); // Alpha ignores this resource.
        a.u=std::make_shared<EventIntegers>(2,-1);a.v=std::make_shared<EventIntegers>(2,-1);
        a.width=std::make_shared<EventIntegers>(2,-1);a.height=std::make_shared<EventIntegers>(2,-1);e.add(7,&images.back());return e;
    }
    EntityGauge& data(ComponentSlots& e){return *dynamic_cast<EntityGauge*>(e.get(19));}
    ImageArrays& image(ComponentSlots& e){return dynamic_cast<EntityImage*>(e.get(7))->arrays;}
    ContactPosition& pos(ComponentSlots& e){return *dynamic_cast<ContactPosition*>(e.get(0));}
    void step(){update_gauges(state,scratch,*this,*this);}
};
struct Factory final:GaugeFactoryHost,ComponentObserver {
    ComponentSlots parent{35},entity{35,this};ContactPosition parent_position,position;
    CellComponent parent_cell,cell;EntityGauge gauge;EntitySprite sprite;EntityImage image;EntityDepth depth;StageGeometry map;
    GaugeOffsets offsets=initial_gauge_offsets();std::vector<int> added_slots;std::function<void(int)> after_add;
    int map_calls=0,null_map_call=0,id=-1;bool registered=false;
    Factory(){parent_position.x=100.9F;parent_position.y=-10.7F;parent_position.z=50.4F;parent_cell.xi=7;parent_cell.yi=-8;
        parent.add(0,&parent_position);parent.add(5,&parent_cell);}
    void added(ComponentSlots&,std::int32_t slot,Component*)override{added_slots.push_back(slot);if(after_add)after_add(slot);}
    void removed(ComponentSlots&,std::int32_t,Component*)override{}
    void changed(ComponentSlots&,std::int32_t,Component*,Component*)override{}
    bool registered_id(std::int32_t key)override{return registered&&key==id;}
    ComponentSlots* create(std::int32_t type,std::int32_t key)override{check(type==0);id=key;return &entity;}
    void register_entity(ComponentSlots& e,bool temporary)override{check(&e==&entity&&!temporary);registered=true;}
    EntityGauge& acquire_gauge()override{return gauge;}
    GaugeOffsets gauge_offsets()override{return offsets;}
    StageGeometry* gauge_map()override{return ++map_calls==null_map_call?nullptr:&map;}
    ContactPosition& acquire_position()override{return position;}
    ProjectileSpeed& acquire_speed()override{std::abort();}
    EntitySprite& acquire_sprite()override{return sprite;}
    EntityDepth& acquire_depth()override{return depth;}
    CellComponent& acquire_cell()override{return cell;}
    EntityImage& acquire_image()override{return image;}
};
int main()try{
    {Factory f;Host images;EntityIdState ids;GaugeCreation args;args.parent=&f.parent;args.image=8;args.frame_max=-1;args.flags=1;
     f.after_add=[&](int slot){if(slot==0)check(f.position.x==50&&f.position.y==0&&f.position.z==50);};
     auto& e=create_gauge(args,ids,f,images.scratch,images);
     check(&e==&f.entity&&ids.counter==1&&f.registered&&f.map_calls==2);
     check(f.added_slots==std::vector<int>{19,0,2,7,5,4}&&!e.contains(1)&&!e.contains(10));
     check(f.position.x==50&&f.position.y==-10&&f.position.z==30&&f.cell.xi==7&&f.cell.yi==-8);
     check(f.sprite.animation.resource==23&&f.sprite.animation.clip==0&&*f.sprite.layers==EventIntegers{-1});
     auto& a=f.image.arrays;check(a.u!=a.v&&a.v!=a.width&&a.width!=a.height&&a.u!=a.height&&a.u!=a.width&&a.v!=a.height);
     check(*a.resources==EventIntegers{23}&&*a.textures==EventIntegers{8}&&*a.width==EventIntegers{100,0});
     check(f.depth.value==0&&f.depth.extra==10000&&f.gauge.frame==0);}
    {Factory f;Host images;EntityIdState ids;GaugeCreation args;args.parent=&f.parent;args.image=8;args.flags=1;
     GaugePlacement at;at.resource=99;at.clip=7;at.x=123;at.z=456;at.xi=3;at.yi=4;at.step_x=0;at.step_z=-1;
     create_gauge_at(args,at,ids,f,images.scratch,images);check(f.sprite.animation.resource==99&&f.sprite.animation.clip==7&&f.map_calls==0);
     check(*f.image.arrays.resources==EventIntegers{99}&&f.position.x==50);}
    {Factory f;Host images;EntityIdState ids;GaugeCreation args;args.parent=&f.parent;args.image=8;f.null_map_call=2;
     fails([&]{create_gauge(args,ids,f,images.scratch,images);});check(f.map_calls==2&&ids.counter==0&&f.added_slots.empty());}
    {Factory f;Host images;EntityIdState ids;GaugeCreation args;args.parent=&f.parent;args.image=8;
     images.on_alpha=[](){throw std::runtime_error("alpha");};fails([&]{create_gauge(args,ids,f,images.scratch,images);});
     check(f.registered&&ids.counter==1&&f.entity.contains(4)&&f.image.arrays.u->at(0)==0&&images.scratch.alpha->at(3)==1);}
    {Factory f;GaugeCreation args;args.type=0;args.from=7;args.parent=&f.parent;f.gauge.parent=&f.entity;
     f.offsets->at(0)=std::make_shared<EventIntegers>(EventIntegers{-99});
     fails([&]{add_gauge(f.entity,args,f);});check(f.gauge.from==7&&f.gauge.offset_x==-99&&f.gauge.parent==&f.entity&&!f.entity.contains(19));}
    {Host h;auto& p=h.node();auto& e=h.make(&p,11);h.step();
     check(h.data(e).frame==1&&h.pos(e).x==98&&h.pos(e).y==-6&&h.pos(e).z==44);
     auto& a=h.image(e);check(*a.u==EventIntegers{0,3}&&*a.v==EventIntegers{0,10}&&*a.width==EventIntegers{100,9}&&*a.height==EventIntegers{10,10});
     check(dynamic_cast<CellComponent*>(e.get(5))->yi==-8&&h.destroyed.empty());}
    {Host h;auto& p=h.node();auto& e=h.make(&p);h.data(e).flags=2;h.data(e).option=9;
     DamageParameter param;param.value=150;param.maximum=100;param.extra=50;param.extra_maximum=0;
     DamageParameterMap map{{9,&param}};CombatParameterLinks links;links.values=&map;p.add(23,&links);
     h.step();check(h.data(e).frame==0&&h.image(e).width->at(1)==188); // Unclamped ratio2, no percentage conversion.
     h.data(e).type=4;param.experience=1;param.max_experience=4;h.step();check(h.image(e).width->at(1)==23);
     map.clear();h.step();check(h.image(e).width->at(1)==0);map[9]=nullptr;h.step();check(h.image(e).width->at(1)==0);
     p.remove(23);h.step();check(h.image(e).width->at(1)==0);
     param.experience=1;param.max_experience=0;map[9]=&param;p.add(23,&links);h.step();check(h.image(e).width->at(1)==std::numeric_limits<int>::min());
     param.experience=0;h.step();check(h.image(e).width->at(1)==0);}
    {Host h;auto& p=h.node();auto& e=h.make(&p,2);auto& g=h.data(e);g.frame=17;h.step();
     check(g.frame==18&&h.destroyed==std::vector<ComponentSlots*>{&e}&&h.alpha_calls==1&&h.state.pending->empty());}
    {Host h;auto& a=h.make(nullptr,0);auto& p=h.node();auto& b=h.make(&p,0);h.dead.insert(&p);h.step();
     check(h.destroyed==std::vector<ComponentSlots*>{&a,&b}&&h.gauges[0].frame==0&&h.gauges[1].frame==0&&h.alpha_calls==0);}
    {Host h;auto& p=h.node();auto& e=h.make(&p,std::numeric_limits<int>::max());auto& g=h.data(e);g.frame=std::numeric_limits<int>::max();
     h.step();check(g.frame==std::numeric_limits<int>::min()&&h.destroyed.empty());}
    {Host h;auto& p=h.node();auto& e=h.make(&p,3);h.data(e).from=80;h.data(e).to=20;h.step();check(h.image(e).width->at(1)==75); // Inverted clamp returns lower bound80.
     h.image(e).u=std::make_shared<EventIntegers>(1,-1);fails([&]{h.step();});check(h.image(e).u->at(0)==0&&h.image(e).v->at(0)==0&&h.alpha_calls==2);}
    {Host h;auto& p=h.node();auto& e=h.make(&p,1);h.image(e).height.reset();h.data(e).from=10;
     fails([&]{h.step();});check(h.image(e).u->at(1)==3&&h.image(e).width->at(0)==100&&h.image(e).width->at(1)==9&&h.pos(e).x==98);}
    {Host h;auto& p=h.node();auto& e=h.make(&p,1);h.offset=-1;h.scratch.alpha->at(80)=0.5F;
     h.data(e).flags=1;h.step();check(h.image(e).u->at(1)==80&&h.scratch.alpha->at(80)==0.5F);}
    {Host h;auto& e=h.make(nullptr);auto& f=h.make(nullptr);h.on_destroy=[](auto&){throw std::runtime_error("destroy");};
     fails([&]{h.step();});check(*h.state.pending==std::vector<ComponentSlots*>{&e,&f});h.on_destroy=nullptr;h.group.clear_entities();h.step();check(h.state.pending->empty());}
    {Host h;auto& e=h.make(nullptr);auto& f=h.make(nullptr);h.state.pending->push_back(&e);
     h.on_destroy=[&](auto&){h.state.pending=std::make_shared<std::vector<ComponentSlots*>>(1,&f);};h.group.clear_entities();h.step();check(h.destroyed==std::vector<ComponentSlots*>{&e}&&h.state.pending->empty());}
    {Host h;auto& e=h.make(nullptr);h.state.pending->push_back(&e);h.group.clear_entities();h.on_destroy=[&](auto&){h.state.pending->clear();};
     fails([&]{h.step();});check(h.state.pending->empty());}
    {Host h;auto old=h.state.pending;old->push_back(nullptr);h.null_group=true;initialize_gauges(h.state,&h);
     check(!h.state.subset&&h.state.pending!=old&&h.state.pending->empty()&&old->size()==1);
     auto current=h.state.pending;h.throw_query=true;fails([&]{initialize_gauges(h.state,&h);});check(h.state.pending==current);}
    std::cout<<"gauge frame contract ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
