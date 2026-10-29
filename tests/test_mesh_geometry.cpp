#include "core/gameplay/mesh_geometry.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>

using namespace saga::core;
namespace {
void check_at(bool v,int line){if(!v){std::cerr<<"mesh geometry line "<<line<<'\n';std::abort();}}
#define check(x) check_at((x),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Host:MeshQuadHost {
    MeshDepthState depth;MeshBuilderState b;
    std::vector<std::unique_ptr<SubmissionGroup>> groups;std::vector<std::unique_ptr<SubmissionLayer>> layers;
    std::function<void(const char*)> allocation;
    std::function<void(SubmissionGroup&)> record;
    bool step=true;
    Host(){b.depth.manager=&depth;b.layers=std::make_shared<MeshBuilderLayers>();b.vertex_color=true;b.layers->set(0,create_layer(b,0));
        b.current_layer=b.layers->entries.at(0);depth.current=100;configure_mesh_builder_depth(b.depth,24,false);}
    MeshBuilderState* create_builder(MeshBuilderCache&,bool,SubmissionMaterial*)override{check(false);return nullptr;}
    SubmissionGroup* create_group(MeshBuilderState&)override{
        auto g=std::make_unique<SubmissionGroup>();g->geometry=std::make_shared<MeshGeometry>();auto& s=*g->geometry;
        s.positions=MeshArrays::positions(0);s.uv=MeshArrays::uv(0);s.colors=MeshArrays::colors(0);s.indices=MeshArrays::indices(0);
        auto* p=g.get();groups.push_back(std::move(g));return p;
    }
    SubmissionLayer* create_layer(MeshBuilderState& builder,std::int32_t index)override{
        auto l=std::make_unique<SubmissionLayer>();l->index=index;l->groups=std::make_shared<SubmissionGroupList>();l->groups->values={create_group(builder)};
        auto* p=l.get();layers.push_back(std::move(l));return p;
    }
    bool depth_step_enabled()override{return step;}
    void record_indices(MeshDepthState& d,SubmissionGroup& g,std::int32_t start,std::int32_t count)override{check(&d==&depth&&start==g.index_count&&count==6);if(record)record(g);}
    MeshArray<MeshVertex> positions(std::int32_t n)override{if(allocation)allocation("positions");return MeshArrays::positions(n);}
    MeshArray<MeshUv> uv(std::int32_t n)override{if(allocation)allocation("uv");return MeshArrays::uv(n);}
    MeshArray<MeshColor> colors(std::int32_t n)override{if(allocation)allocation("colors");return MeshArrays::colors(n);}
    MeshArray<std::int32_t> indices(std::int32_t n)override{if(allocation)allocation("indices");return MeshArrays::indices(n);}
    SubmissionGroup& group(){return *current_mesh_group(*b.current_layer);}
    MeshArray<MeshVertex> points(std::size_t n=4){return std::make_shared<std::vector<MeshVertex>>(n,MeshVertex{1,2,3});}
    SubmissionGroup* quad(MeshArray<MeshVertex> p){return add_mesh_quad(b,p,0.1F,0.2F,0.3F,0.4F,{1,0.5F,0.25F,0.75F},*this);}
};
}
int main()try{
    {Host h;h.step=false;auto p=h.points();auto* g=h.quad(p);auto& s=*g->geometry;
        check(g->vertex_count==4&&g->index_count==6&&g->head==0&&g->change&&h.depth.current==100);
        check(s.positions->size()==4&&s.uv->size()==4&&s.colors->size()==4&&s.indices->size()==12);
        check(s.positions->at(0).z==104.5F&&h.b.depth.total==418);
        check((s.uv->at(0)==MeshUv{0.1F,0.2F+0.4F})&&(s.uv->at(2)==MeshUv{0.1F+0.3F,0.2F}));
        check((s.colors->at(0)==MeshColor{1,0.5F,0.25F,0.75F}));
        check(std::vector<std::int32_t>(s.indices->begin(),s.indices->begin()+6)==std::vector<std::int32_t>({0,1,2,0,2,3}));
        h.quad(p);check(g->head==4&&s.positions->size()==10&&s.indices->size()==12&&g->index_count==12);
        h.quad(p);check(s.positions->size()==16&&s.indices->size()==30&&g->vertex_count==12&&g->index_count==18);
        const auto storage=s.positions;reset_mesh_builder(h.b);h.quad(p);check(s.positions==storage&&g->head==0&&g->index_count==6);}
    // Expansion depends on used count, copies full old storage, and leaves color arrays alone when disabled.
    {Host h;auto& g=h.group();auto& s=*g.geometry;g.vertex_count=1;s.positions=h.positions(5);s.uv=h.uv(5);s.positions->at(4).x=91;
        h.b.vertex_color=false;auto colors=s.colors;expand_mesh_vertices(h.b,5,h);
        check(s.positions->size()==6&&s.positions->at(4).x==91&&s.colors==colors);}
    // Exact signed group thresholds; reset keeps the last group selected instead of recycling earlier groups.
    {Host h;auto& first=h.group();first.vertex_count=59999;first.index_count=54999;check_mesh_group(h.b,h);check(h.groups.size()==1&&first.change);
        first.vertex_count=60000;check_mesh_group(h.b,h);check(h.groups.size()==2&&h.b.current_layer->current_group==1);
        reset_mesh_builder(h.b);check_mesh_group(h.b,h);check(h.groups.size()==2&&&h.group()!=&first);
        h.group().index_count=55000;check_mesh_group(h.b,h);check(h.groups.size()==3&&h.b.current_layer->current_group==2);
        h.b.current_layer->groups->values.at(2)=nullptr;check_mesh_group(h.b,h);check(h.groups.size()==3);}
    // Vertex/UV arrays publish together; colors publish later and can fail after that prefix.
    {Host h;auto& s=*h.group().geometry;auto old=s.positions;auto old_uv=s.uv;
        h.allocation=[](const char* kind){if(std::string(kind)=="uv")throw std::bad_alloc();};fails([&]{expand_mesh_vertices(h.b,4,h);});
        check(s.positions==old&&s.uv==old_uv);
        h.allocation=[](const char* kind){if(std::string(kind)=="colors")throw std::bad_alloc();};fails([&]{expand_mesh_vertices(h.b,4,h);});
        check(s.positions!=old&&s.positions->size()==4&&s.uv->size()==4&&s.colors->empty());}
    {Host h;auto& s=*h.group().geometry;s.uv=h.uv(5);auto old=s.positions;auto old_uv=s.uv;
        fails([&]{expand_mesh_vertices(h.b,4,h);});check(s.positions==old&&s.uv==old_uv);}
    // Null source faults after group marking, depth step, capacity expansion and head assignment.
    {Host h;const auto initial=h.depth.current;fails([&]{h.quad(nullptr);});auto& g=h.group();
        check(g.change&&h.depth.current==initial-h.b.depth.step&&g.head==0&&g.vertex_count==0&&g.geometry->positions->size()==4&&h.b.depth.total==0);}
    // Five points are not silently truncated to a quad: four vertices are retained, then bounds failure.
    {Host h;fails([&]{h.quad(h.points(5));});check(h.group().vertex_count==4&&h.group().index_count==0&&h.b.depth.total>0);}
    // Index allocation failure occurs after every vertex/UV/color and depth-total update.
    {Host h;h.allocation=[](const char* kind){if(std::string(kind)=="indices")throw std::bad_alloc();};fails([&]{h.quad(h.points());});
        check(h.group().vertex_count==4&&h.group().index_count==0&&h.group().geometry->indices->empty()&&h.b.depth.total>0);}
    // Existing short color storage faults after the first position and UV stores, before count/total advance.
    {Host h;auto& s=*h.group().geometry;s.positions=h.positions(4);s.uv=h.uv(4);fails([&]{h.quad(h.points());});
        check(s.positions->at(0).x==1&&s.positions->at(0).z>0&&s.uv->at(0)[1]>0&&h.group().vertex_count==0&&h.b.depth.total==0);}
    // Optional recording receives the completed vertex prefix; failures precede index writes/count advance.
    {Host h;h.quad(h.points());h.record=[](auto& g){check(g.head==4&&g.vertex_count==8&&g.index_count==6);throw std::runtime_error("capture");};
        fails([&]{h.quad(h.points());});check(h.group().vertex_count==8&&h.group().index_count==6);}
    {Host h;h.record=[&](auto& g){g.geometry->indices=h.indices(3);};fails([&]{h.quad(h.points());});
        check(h.group().index_count==0&&h.group().vertex_count==4&&*h.group().geometry->indices==std::vector<std::int32_t>({0,1,2}));}
    // Empty sources still reserve/write the six native indices; do not invent an exact-four precondition.
    {Host h;h.quad(h.points(0));check(h.group().vertex_count==0&&h.group().index_count==6&&h.b.depth.total==0);}
    std::cout<<"mesh geometry contract ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
