#include "cell_fixture.hpp"
#include "core/gameplay/motion_pass.hpp"
#include "core/ecs/update_scheduler.hpp"
#include <cstdlib>
#include <cmath>
#include <limits>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check_at(bool v,int line){if(!v){std::cerr<<"cell contract failed at line "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Node {
    ComponentSlots entity;ContactPosition position;ProjectileSpeed speed;CellComponent cell;
    explicit Node(ComponentObserver* o):entity(35,o){cell.children=std::make_shared<std::vector<ComponentSlots*>>();}
};
struct Clock final:UpdateStopwatch {
    std::vector<int> calls;
    void reset()override{calls.push_back(0);}void start()override{calls.push_back(1);}void stop()override{calls.push_back(2);}
};
struct Fixture {
    std::vector<std::int32_t> moving_ids{5,0,1},cell_ids{5};SubsetFilter moving_filter{&moving_ids},cell_filter{&cell_ids};
    std::unordered_map<const ComponentSlots*,std::int32_t> ids;
    EntitySubset moving{moving_filter,[this](const ComponentSlots& e){return ids.at(&e);}};
    EntitySubset cells{cell_filter,[this](const ComponentSlots& e){return ids.at(&e);}};
    SubsetMembership membership{{&moving,&cells}};
    CellServices services{[this](const auto& all){check(all==moving_ids||all==cell_ids);return all==moving_ids?&moving:&cells;}};
    void add(Node& n){ids.emplace(&n.entity,static_cast<std::int32_t>(ids.size()+1));
        n.entity.add(0,&n.position);n.entity.add(1,&n.speed);n.entity.add(5,&n.cell);}
};
int main()try{
    const auto hi=std::numeric_limits<std::int32_t>::max(),lo=std::numeric_limits<std::int32_t>::min();
    {Fixture f;Node n(&f.membership);f.add(n);auto& h=f.services;h.init();
     check(h.calls==std::vector<std::string>{"moving-subset","map","map","cell-subset","subscribe-added","subscribe-removed","global-world","map","event-world","subscribe14"});
     check(h.frame.subset==&f.moving&&h.frame.map==&h.map&&h.index.map==&h.map);
     check(cell_entities(h.index,0)->at(0)==&n.entity&&h.frame.changed->empty());
     n.position.x=11.9F;update_cells(h.frame,h);check(h.changes.empty()&&n.cell.xi==0);
     n.position.x=12;update_cells(h.frame,h);check(n.cell.xi==1&&n.cell.yi==0&&h.changes.back().second==0);
     check(cell_entities(h.index,0)->empty()&&cell_entities(h.index,1)->at(0)==&n.entity&&h.frame.changed->empty());
     n.position.z=36;update_cells(h.frame,h);check(n.cell.yi==2&&h.changes.back().second==1);
     check(cell_entities(h.index,1,2,h)->at(0)==&n.entity&&cell_entities(h.index,1)->empty());
     n.entity.remove(5);check(f.moving.size()==0&&f.cells.size()==0&&cell_entities(h.index,13)->empty());}
    // Initial coordinates use the factory's uncentered conversion. The first pass may migrate them.
    {Fixture f;Node n(&f.membership);n.position.x=23;n.cell.xi=factory_cell_coordinate(n.position.x,24);f.add(n);f.services.init();
     update_cells(f.services.frame,f.services);check(n.cell.xi==1&&f.services.changes.size()==1);}
    for(const auto test:std::vector<std::pair<float,std::int32_t>>{{-11.9F,0},{-35.9F,0},{-36.0F,-1},{-60.0F,-2},{36.9F,2},
        {std::numeric_limits<float>::infinity(),-89478484},{-std::numeric_limits<float>::infinity(),-89478484},
        {std::numeric_limits<float>::quiet_NaN(),0},{2147483648.0F,-89478484}}){
        Fixture f;Node n(&f.membership);n.position.x=test.first;n.cell.xi=99;f.add(n);f.services.init();update_cells(f.services.frame,f.services);
        check(n.cell.xi==test.second);}
    {Fixture f;Node n(&f.membership);f.add(n);f.services.map.step_x=0;f.services.map.step_z=-1;
     n.position.x=123;n.position.z=-2147483648.0F;f.services.init();update_cells(f.services.frame,f.services);
     check(n.cell.xi==0&&n.cell.yi==lo);}
    {Fixture f;Node n(&f.membership);f.add(n);f.services.map.step_x=-25;n.position.x=38;f.services.init();update_cells(f.services.frame,f.services);check(n.cell.xi==-1);}
    {Fixture f;auto& h=f.services;h.map.width=hi;check(cell_key(&h.map,hi,hi)==-2147483647);check(cell_key(&h.map,lo,lo)==lo);
     fails([&]{cell_key(nullptr,0,0);});h.global_world=nullptr;fails([&]{cell_key(0,0,h);});
     CellComponent c;fails([&]{cell_key(&c,h);});}
    {Fixture f;auto& h=f.services;h.init();auto old=h.frame.changed;old->push_back(nullptr);
     initialize_cells(h.frame,&h);check(h.frame.changed!=old&&old->size()==1&&h.frame.changed->empty());
     auto* group=h.frame.subset;auto* map=h.frame.map;auto list=h.frame.changed;
     h.hook=[](const std::string& s){if(s=="moving-subset")throw std::runtime_error(s);};
     fails([&]{initialize_cells(h.frame,&h);});check(h.frame.subset==group&&h.frame.map==map&&h.frame.changed==list);
     h.hook=[](const std::string& s){if(s=="map")throw std::runtime_error(s);};h.frame.subset=nullptr;
     fails([&]{initialize_cells(h.frame,&h);});check(h.frame.subset==group&&h.frame.changed==list);
     fails([&]{initialize_cells(h.frame,nullptr);});}
    {Fixture f;auto& h=f.services;h.init();h.frame.map=nullptr;h.frame.changed->push_back(nullptr);update_cells(h.frame,h);check(h.frame.changed->empty());
     h.frame.subset=nullptr;fails([&]{update_cells(h.frame,h);});h.frame.subset=&f.moving;h.frame.changed.reset();fails([&]{update_cells(h.frame,h);});}
    // A failed event leaves coordinates and the pending entry written; retry does not resend an unchanged cell.
    {Fixture f;Node n(&f.membership);f.add(n);auto& h=f.services;h.init();n.position.x=36;h.event_world=nullptr;
     fails([&]{update_cells(h.frame,h);});check(n.cell.xi==2&&h.frame.changed->size()==1&&cell_entities(h.index,0)->size()==1);
     h.event_world=&h;update_cells(h.frame,h);check(h.changes.empty()&&h.frame.changed->empty()&&cell_entities(h.index,0)->size()==1);}
    {Fixture f;Node n(&f.membership);f.add(n);auto& h=f.services;h.init();n.position.x=36;h.frame.changed.reset();
     fails([&]{update_cells(h.frame,h);});check(n.cell.xi==2&&h.changes.empty());}
    {Fixture f;Node n(&f.membership);f.add(n);auto& h=f.services;h.init();n.position.x=36;auto old=h.frame.changed;
     h.hook=[&](const std::string& s){if(s=="send14")h.frame.changed=std::make_shared<std::vector<ComponentSlots*>>(1,nullptr);};
     update_cells(h.frame,h);check(old->size()==1&&h.frame.changed!=old&&h.frame.changed->empty());}
    {Fixture f;Node a(&f.membership),b(&f.membership);f.add(a);f.add(b);auto& h=f.services;h.init();a.position.x=36;b.position.x=60;
     h.hook=[&](const std::string& s){if(s=="send14")b.entity.remove(1);};
     fails([&]{update_cells(h.frame,h);});check(a.cell.xi==2&&b.cell.xi==0&&h.frame.changed->size()==1);}
    {Fixture f;Node n(&f.membership);f.add(n);auto& h=f.services;h.init();auto* saved=h.map_result;h.map_result=nullptr;
     fails([&]{on_cell_change(h.index,&n.entity,0,h);});check(cell_entities(h.index,0)->empty());h.map_result=saved;
     on_cell_change(h.index,&n.entity,0,h);check(cell_entities(h.index,0)->size()==1);
     fails([&]{on_cell_change(h.index,nullptr,0,h);});check(cell_entities(h.index,0)->size()==1);}
    {Fixture f;Node n(&f.membership);f.add(n);auto& h=f.services;h.init();add_indexed_cell(h.index,&n.entity,5,&n.cell,h);
     check(cell_entities(h.index,0)->size()==2);remove_indexed_cell(h.index,&n.entity,5,&n.cell,h);check(cell_entities(h.index,0)->size()==1);
     n.cell.xi=1;on_cell_change(h.index,&n.entity,0,h);check(cell_entities(h.index,0)->empty()&&cell_entities(h.index,1)->size()==1);
     remove_indexed_cell(h.index,&n.entity,0,nullptr,h);check(cell_entities(h.index,1)->size()==1);
     Component wrong;fails([&]{add_indexed_cell(h.index,&n.entity,5,&wrong,h);});fails([&]{add_indexed_cell(h.index,&n.entity,5,nullptr,h);});
     check(!cell_entities(h.index,123));h.index.buckets.reset();fails([&]{cell_entities(h.index,0);});}
    {Fixture f;Node n(&f.membership);f.add(n);auto& h=f.services;h.init();h.index.buckets->at(0).reset();
     add_indexed_cell(h.index,nullptr,5,&n.cell,h);check(cell_entities(h.index,0)->at(0)==nullptr);
     fails([&]{change_indexed_cell(h.index,nullptr,0,h);});check(cell_entities(h.index,0)->empty());}
    // Repeated Init retains older delegates, which all address the current index dictionary.
    {Fixture f;auto& h=f.services;h.init();auto old=h.index.buckets;initialize_cell_index(h.index,&h,h,EventHashes{14,114});
     check(h.index.buckets!=old&&h.receivers.size()==2&&h.hashes.back()->wrapper==114);
     Node n(&f.membership);f.add(n);check(cell_entities(h.index,0)->size()==2&&old->empty());
     n.position.x=36;update_cells(h.frame,h);check(cell_entities(h.index,0)->empty()&&cell_entities(h.index,2)->size()==2);}
    // Movement executes at factory index 11, cells at 17. The same event updates the actual index.
    {Fixture f;Node n(&f.membership);f.add(n);auto& h=f.services;h.init();n.speed.x=13;bool enabled=true;
     GameplayUpdate motion(11,enabled,[&](std::int32_t){move_subset(f.moving);});
     GameplayUpdate cells(17,enabled,[&](std::int32_t){update_cells(h.frame,h);});
     std::vector<UpdateSystem*> systems{&motion,&cells};UpdateSchedule schedule{&systems,nullptr};
     update_systems(schedule,1);check(n.position.x==13&&n.cell.xi==1&&cell_entities(h.index,1)->at(0)==&n.entity);
     enabled=false;update_systems(schedule,1);check(n.position.x==13&&h.changes.size()==1);}
    {auto values=std::make_shared<EventIntegers>(EventIntegers{0,0,777});
     check(project_cell_location(values,0,0,312,156)==values&&*values==EventIntegers({0,0,777}));
     project_cell_location(values,936,0,312,156);check(*values==EventIntegers({1,-1,777}));
     project_cell_location(values,0,312,312,156);check(*values==EventIntegers({0,1,777}));
     project_cell_location(values,1,2,0,0);check(*values==EventIntegers({-50,-50,777}));
     project_cell_location(values,hi,lo,hi,-1);check(*values==EventIntegers({-50,-1073741824,777}));
     auto one=std::make_shared<EventIntegers>(1,99);fails([&]{project_cell_location(one,0,0,312,156);});check(one->at(0)==30888);
     fails([&]{project_cell_location(nullptr,0,0,1,1);});fails([&]{project_cell_location(std::make_shared<EventIntegers>(),0,0,1,1);});}
    {Fixture f;Node a(&f.membership),b(&f.membership),c(&f.membership),d(&f.membership);Component invisible;
     b.cell.xi=1;b.cell.yi=-1;c.cell.xi=1;d.cell.xi=2;d.cell.yi=-1;
     f.add(a);f.add(b);f.add(c);f.add(d);d.entity.add(21,&invisible);auto& h=f.services;h.init();add_indexed_cell(h.index,&a.entity,5,&a.cell,h);
     update_cell_visibility(h.index,h);check(h.visibility_calls==std::vector<ComponentSlots*>{&a.entity,&a.entity,&b.entity,&c.entity});
     check(h.visible.size()==3&&h.visible_count==3&&h.visible.count(&d.entity)==0);
     bool enabled=true;GameplayUpdate culling(20,enabled,[&](std::int32_t){update_cell_visibility(h.index,h);});
     std::vector<UpdateSystem*> systems{&culling};Clock clock;UpdateSchedule schedule{&systems,&clock};h.visibility_calls.clear();
     update_systems(schedule,1);check(h.visibility_calls.empty()&&clock.calls.empty());update_systems(schedule,0);
     check(h.visibility_calls.size()==4&&clock.calls==std::vector<int>{0,1,2});}
    {Fixture f;Node n(&f.membership);f.add(n);auto& h=f.services;h.init();h.visible.insert(&n.entity);h.game_height=-48;
     update_cell_visibility(h.index,h);check(h.visible.empty()&&h.visible_count==0&&h.visibility_calls.empty());
     h.game_height=0;h.game_width=-144;update_cell_visibility(h.index,h);check(h.visible_count==0&&h.visibility_calls.empty());}
    {Fixture f;Node n(&f.membership);f.add(n);auto& h=f.services;h.init();h.visible.insert(&n.entity);h.index.map=nullptr;
     fails([&]{update_cell_visibility(h.index,h);});check(h.visible.empty()&&h.visible_count==-1);
     h.index.map=&h.map;h.origin=std::make_shared<EventIntegers>(1,0);
     fails([&]{update_cell_visibility(h.index,h);});check(h.visible_count==-1&&h.origin->at(0)==2376);}
    {Fixture f;Node a(&f.membership),b(&f.membership);f.add(a);f.add(b);auto& h=f.services;h.init();
     auto list=cell_entities(h.index,0);list->clear();list->push_back(&a.entity);bool appended=false;
     h.hook=[&](const std::string& s){if(s=="add-visible"&&!appended){appended=true;list->push_back(&b.entity);}};
     update_cell_visibility(h.index,h);check(h.visible_count==2&&h.visibility_calls==std::vector<ComponentSlots*>{&a.entity,&b.entity});}
    {Fixture f;Node n(&f.membership);f.add(n);auto& h=f.services;h.init();cell_entities(h.index,0)->push_back(nullptr);
     fails([&]{update_cell_visibility(h.index,h);});check(h.visible.count(&n.entity)==1&&h.visible_count==-1);}
    {Fixture f;auto& h=f.services;h.init();h.map_result=nullptr;h.calls.clear();
     add_indexed_cell(h.index,nullptr,0,nullptr,h);remove_indexed_cell(h.index,nullptr,0,nullptr,h);check(h.calls.empty());
     fails([&]{initialize_cell_index(h.index,nullptr,h);});}
    {Fixture f;Node n(&f.membership);f.add(n);auto& h=f.services;h.init();auto old=h.index.buckets;
     h.hook=[](const std::string& s){if(s=="subscribe14")throw std::runtime_error(s);};
     fails([&]{initialize_cell_index(h.index,&h,h);});check(h.index.buckets!=old&&cell_entities(h.index,0)->at(0)==&n.entity&&h.receivers.size()==1);}
}catch(const std::exception& e){std::cerr<<"unhandled cell contract exception: "<<e.what()<<'\n';return 1;}
