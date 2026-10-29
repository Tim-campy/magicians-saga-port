#include "core/gameplay/bubble_frame.hpp"
#include "core/gameplay/pass_initialization.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <cstdlib>
#include <deque>
#include <iostream>
#include <limits>
#include <map>
#include <set>
using namespace saga::core;
void check_at(bool v,int line){if(!v){std::cerr<<"bubble contract failed at "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Host final:BubbleFrameHost,BubbleInitializationWorld,BubblePool,DebugInitializationWorld {
    std::map<const ComponentSlots*,int> ids;std::set<ComponentSlots*> dead;
    EntitySubset group{[](auto* e){return e->contains(18);},[this](const auto& e){return ids.at(&e);}};
    SubsetMembership membership{{&group}};
    std::deque<ComponentSlots> entities;std::deque<ContactPosition> positions;std::deque<CellComponent> cells;std::deque<EntityBubble> payloads;
    BubbleOffsets offsets=initial_bubble_offsets();BubbleFrameState frame;EntityBubble* pooled=nullptr;
    std::vector<ComponentSlots*> destroyed;std::function<void(ComponentSlots&)> on_destroy;
    bool null_group=false,throw_query=false;
    Host(){initialize_bubbles(frame,this);}
    EntitySubset* bubble_subset(const EventIntegers& slots)override{check(slots==EventIntegers{18});if(throw_query)throw std::runtime_error("subset");return null_group?nullptr:&group;}
    EntitySubset* debug_subset(const EventIntegers& all,const EventIntegers& none)override{
        check(all==EventIntegers{14,0,1}&&none==EventIntegers{9});if(throw_query)throw std::runtime_error("debug subset");return null_group?nullptr:&group;
    }
    bool bubble_parent_destroyed(ComponentSlots& e)override{return dead.count(&e)!=0;}
    void destroy_bubble(ComponentSlots& e,bool remove)override{
        check(remove);destroyed.push_back(&e);if(on_destroy)on_destroy(e);if(dead.count(&e))return;
        for(int i=0;i<35;++i)if(e.contains(i))e.remove(i);dead.insert(&e);
    }
    EntityBubble& acquire_bubble()override{if(pooled)return *pooled;payloads.emplace_back();return payloads.back();}
    BubbleOffsets bubble_offsets()override{return offsets;}
    ComponentSlots& node(float x=100.9F,float y=-10.7F,float z=50.4F){
        entities.emplace_back(35,&membership);auto& e=entities.back();ids[&e]=static_cast<int>(ids.size());
        positions.emplace_back();auto& p=positions.back();p.x=x;p.y=y;p.z=z;e.add(0,&p);
        cells.emplace_back();cells.back().xi=7;cells.back().yi=-8;e.add(5,&cells.back());return e;
    }
    ComponentSlots& make(ComponentSlots* parent,int maximum=-1,int type=0){auto& e=node(900,800,700);add_bubble(e,123,type,parent,maximum,*this);return e;}
    EntityBubble& data(ComponentSlots& e){return *dynamic_cast<EntityBubble*>(e.get(18));}
    ContactPosition& pos(ComponentSlots& e){return *dynamic_cast<ContactPosition*>(e.get(0));}
};
int main()try{
    {Host h;auto& parent=h.node();auto& e=h.make(&parent);update_bubbles(h.frame,h);
     check(h.data(e).frame==1&&h.pos(e).x==64&&h.pos(e).y==-10&&h.pos(e).z==20);
     auto& c=*dynamic_cast<CellComponent*>(e.get(5));check(c.xi==7&&c.yi==-8&&h.destroyed.empty());
     check(h.frame.seen->empty()&&h.frame.pending->empty());}
    {Host h;auto& parent=h.node();auto& e=h.make(&parent,0,1);auto& payload=h.data(e);update_bubbles(h.frame,h);
     check(h.destroyed==std::vector<ComponentSlots*>{&e}&&payload.frame==1&&h.positions.back().z==30);}
    {Host h;auto& parent=h.node();auto& a=h.make(&parent);auto& b=h.make(&parent);update_bubbles(h.frame,h);
     check(h.destroyed==std::vector<ComponentSlots*>{&a}&&h.group.size()==1&&h.group.first_entity()==&b);}
    {Host h;auto& parent=h.node();auto& a=h.make(&parent);auto& b=h.make(&parent,0);update_bubbles(h.frame,h);
     check(h.destroyed==std::vector<ComponentSlots*>{&b,&a}&&h.group.size()==0); // An expiry-queued last occurrence still wins dedup.
     check(h.frame.pending->empty()&&h.frame.seen->empty());}
    {Host h;auto& a=h.make(nullptr);auto& b=h.make(nullptr);auto& first=h.data(a);auto& second=h.data(b);
     update_bubbles(h.frame,h);check(h.destroyed==std::vector<ComponentSlots*>{&a,&b,&a});
     check(first.frame==0&&second.frame==0&&h.positions.front().x==900);}
    {Host h;auto& parent=h.node();h.dead.insert(&parent);auto& e=h.make(&parent);auto& payload=h.data(e);
     update_bubbles(h.frame,h);check(payload.frame==0&&h.destroyed==std::vector<ComponentSlots*>{&e});}
    {Host h;auto& parent=h.node();auto& e=h.make(&parent);h.data(e).frame=std::numeric_limits<std::int32_t>::max();
     update_bubbles(h.frame,h);check(h.data(e).frame==std::numeric_limits<std::int32_t>::min());}
    {Host h;auto& parent=h.node(std::numeric_limits<float>::infinity(),0,-50.9F);auto& e=h.make(&parent);
     update_bubble_display(&e);check(h.pos(e).x==2147483648.0F&&h.pos(e).z==-80);}
    {Host h;auto& parent=h.node();auto& e=h.make(&parent,0);parent.remove(5);
     fails([&]{update_bubbles(h.frame,h);});check(h.data(e).frame==1&&h.pos(e).x==64&&h.pos(e).z==20);
     check(*h.frame.pending==std::vector<ComponentSlots*>{&e}&&h.destroyed.empty());}
    {Host h;auto& e=h.make(nullptr);h.on_destroy=[](auto&){throw std::runtime_error("destroy");};
     fails([&]{update_bubbles(h.frame,h);});check(h.frame.seen->empty()&&*h.frame.pending==std::vector<ComponentSlots*>{&e});}
    {Host h;auto& e=h.make(nullptr);auto replacement=std::make_shared<std::vector<ComponentSlots*>>();replacement->push_back(&e);
     h.on_destroy=[&](auto&){h.frame.pending=replacement;};update_bubbles(h.frame,h);check(replacement->empty());}
    {Host h;h.make(nullptr);h.on_destroy=[&](auto&){h.frame.pending->clear();};fails([&]{update_bubbles(h.frame,h);});check(h.frame.pending->empty());}
    {Host h;auto& parent=h.node();auto& e=h.make(&parent);h.frame.seen->push_back(&parent);update_bubbles(h.frame,h);
     check(h.destroyed==std::vector<ComponentSlots*>{&e});}
    {Host h;auto pending=h.frame.pending,seen=h.frame.seen;h.frame.pending->push_back(nullptr);h.frame.seen->push_back(nullptr);
     initialize_bubbles(h.frame,&h);check(h.frame.pending==pending&&h.frame.seen==seen&&pending->size()==1&&seen->size()==1);
     h.null_group=true;initialize_bubbles(h.frame,&h);check(!h.frame.subset);fails([&]{update_bubbles(h.frame,h);});fails([&]{initialize_bubbles(h.frame,nullptr);});}
    {Host h;auto& e=h.node();EntityBubble reused;reused.parent=&e;reused.offset_y=88;reused.offset_z=99;h.pooled=&reused;
     h.offsets->at(0)=std::make_shared<EventIntegers>(EventIntegers{42});
     fails([&]{add_bubble(e,9,0,nullptr,-1,h);});check(reused.data_id==9&&reused.frame==0&&reused.frame_max==-1&&reused.offset_x==42);
     check(reused.offset_y==88&&reused.offset_z==99&&reused.parent==&e&&!e.contains(18));}
    {Host h;DebugVisualizationState d;d.flags=7;initialize_debug_visualization(d,&h);check(d.subset==&h.group&&d.flags==0);
     d.flags=5;h.throw_query=true;fails([&]{initialize_debug_visualization(d,&h);});check(d.flags==5&&d.subset==&h.group);
     fails([&]{initialize_debug_visualization(d,nullptr);});check(d.flags==5);
     h.throw_query=false;h.null_group=true;initialize_debug_visualization(d,&h);check(!d.subset&&d.flags==0);
     enable_debug_flag(d,1);check(check_debug_flag(d,3)&&!check_debug_flag(d,0));switch_debug_flag(d,3);check(d.flags==0);
     switch_debug_flag(d,3);check(d.flags==3);disable_debug_flag(d,2);check(d.flags==1);
     enable_debug_flag(d,std::numeric_limits<int>::min());check(d.flags==std::numeric_limits<int>::min()+1);
     const auto flags=d.flags;update_debug_visualization(d,false);update_debug_visualization(d,true);check(d.flags==flags);
     reset_debug_flags(d);check(d.flags==0);
     DebugPathVisualization path{false,999999999999LL};debug_visualization_enabled(path);check(path.visualize&&path.wait_milliseconds==0);
     path.wait_milliseconds=-4;debug_visualization_disabled(path);check(!path.visualize&&path.wait_milliseconds==0);}
    check(gameplay_needs_update(16,true,7)&&gameplay_needs_update(30,true,7)&&!gameplay_needs_update(16,false,0));
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
