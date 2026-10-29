#include "core/gameplay/mesh_builder.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>

using namespace saga::core;
namespace {
void check_at(bool value,int line){if(!value){std::cerr<<"mesh builder line "<<line<<'\n';std::abort();}}
#define check(x) check_at((x),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Factory:MeshBuilderFactory {
    MeshDepthState depth;
    std::vector<std::unique_ptr<MeshBuilderState>> builders;
    std::vector<std::unique_ptr<SubmissionLayer>> layers;
    std::vector<std::unique_ptr<SubmissionGroup>> groups;
    std::function<void(MeshBuilderCache&)> builder_hook;
    std::function<void(MeshBuilderState&)> layer_hook;
    MeshBuilderState* create_builder(MeshBuilderCache& c,bool color,SubmissionMaterial* material)override{
        if(builder_hook)builder_hook(c);
        auto b=std::make_unique<MeshBuilderState>();b->depth.manager=&depth;b->vertex_color=color;b->material=material;
        b->layers=std::make_shared<MeshBuilderLayers>();auto* value=b.get();builders.push_back(std::move(b));return value;
    }
    SubmissionLayer* create_layer(MeshBuilderState& b,std::int32_t index)override{
        if(layer_hook)layer_hook(b);
        auto g=std::make_unique<SubmissionGroup>();auto l=std::make_unique<SubmissionLayer>();l->index=index;
        l->groups=std::make_shared<SubmissionGroupList>();l->groups->values={g.get()};groups.push_back(std::move(g));
        auto* value=l.get();layers.push_back(std::move(l));return value;
    }
};
struct Records:MeshRecordHost {
    std::vector<std::unique_ptr<SubmissionRecord>> owned;
    std::vector<SubmissionGroup*> prepared;
    bool surface=true;std::int32_t surface_calls=0;
    std::function<void()> surface_hook;
    std::function<void(SubmissionGroup&)> prepare_hook;
    SubmissionRecord* create_record()override{auto r=std::make_unique<SubmissionRecord>();auto* p=r.get();owned.push_back(std::move(r));return p;}
    bool has_surface()override{++surface_calls;if(surface_hook)surface_hook();return surface;}
    void prepare_group(MeshBuilderState&,SubmissionGroup& group)override{prepared.push_back(&group);if(prepare_hook)prepare_hook(group);}
};
MeshBuilderCache cache(){return {std::make_shared<std::map<SubmissionMaterial*,MeshBuilderState*>>(),std::make_shared<MeshBuilderOrder>()};}
}
int main()try{
    // Reuse is by material identity, including equal queues and a cached null value.
    {Factory f;auto c=cache();SubmissionMaterial a,b;a.queue.absolute=b.queue.absolute=2000;b.uniform_color=true;
        auto* first=get_mesh_builder(c,&a,f);auto* second=get_mesh_builder(c,&b,f);
        check(first!=second&&first->vertex_color&&!second->vertex_color&&first->material==&a);
        check(get_mesh_builder(c,&a,f)==first&&f.builders.size()==2);
        check(c.sorted->values.at(0).first==&b&&c.sorted->values.at(1).first==&a); // Equal queue inserts BEFORE the older material.
        c.builders->at(&a)=nullptr;check(get_mesh_builder(c,&a,f)==nullptr&&f.builders.size()==2);
        fails([&]{get_mesh_builder(c,nullptr,f);});}
    // Signed queue ordering has no implicit clamping or pointer tie-breaker.
    {Factory f;auto c=cache();SubmissionMaterial a,b,d;a.queue.absolute=3000;b.queue.absolute=-9;d.queue.absolute=std::numeric_limits<std::int32_t>::max();
        get_mesh_builder(c,&a,f);get_mesh_builder(c,&d,f);get_mesh_builder(c,&b,f);
        check(c.sorted->values.at(0).first==&b&&c.sorted->values.at(1).first==&a&&c.sorted->values.at(2).first==&d);}
    // Publish the dictionary before sorted-list validation. Failure does not silently retry insertion.
    {Factory f;auto c=cache();SubmissionMaterial a;c.sorted.reset();fails([&]{get_mesh_builder(c,&a,f);});
        check(c.builders->count(&a)==1&&get_mesh_builder(c,&a,f)==f.builders.front().get());}
    // Allocation failure precedes publication. Constructor mutations expose captured/fresh reads.
    {Factory f;auto c=cache();SubmissionMaterial a;f.builder_hook=[](auto&){throw std::bad_alloc();};
        fails([&]{get_mesh_builder(c,&a,f);});check(c.builders->empty());
        const auto old=c.builders;auto new_list=std::make_shared<MeshBuilderOrder>();
        f.builder_hook=[&](auto& state){state.builders=cache().builders;state.sorted=new_list;a.queue.absolute=91;a.uniform_color=true;};
        auto* value=get_mesh_builder(c,&a,f);check(old->at(&a)==value&&c.builders->empty()&&new_list->values.at(0).second==value);
        check(value->vertex_color); // Inverse uniform-color flag was captured BEFORE the constructor.
        c=cache();c.builders.reset();fails([&]{get_mesh_builder(c,&a,f);});}
    // GetLayer uses the full signed manager layer, creates only on a miss, and preserves stored nulls.
    {Factory f;auto c=cache();SubmissionMaterial material;auto* b=get_mesh_builder(c,&material,f);f.depth.layer=-4;
        auto* layer=get_mesh_layer(*b,f);check(layer->index==-4&&get_mesh_layer(*b,f)==layer&&f.layers.size()==1);
        b->layers->set(12,nullptr);f.depth.layer=12;check(get_mesh_layer(*b,f)==nullptr&&f.layers.size()==1);
        b->depth.manager=nullptr;fails([&]{get_mesh_layer(*b,f);});}
    // Layer constructor index is captured; insertion uses FRESH manager/directory fields afterward.
    {Factory f;auto c=cache();SubmissionMaterial material;auto* b=get_mesh_builder(c,&material,f);f.depth.layer=7;
        const auto old=b->layers;MeshDepthState replacement;replacement.layer=9;
        f.layer_hook=[&](auto& value){value.depth.manager=&replacement;value.layers=std::make_shared<MeshBuilderLayers>();};
        auto* layer=get_mesh_layer(*b,f);check(layer->index==7&&old->entries.empty()&&b->layers->entries.at(9)==layer);
        f.layer_hook=[](auto&){throw std::bad_alloc();};replacement.layer=10;fails([&]{get_mesh_layer(*b,f);});check(b->layers->entries.count(10)==0);}
    // Reset clears only vertex/index/head counts and total depth, retaining every allocation and selector.
    {Factory f;auto c=cache();SubmissionMaterial material;auto* b=get_mesh_builder(c,&material,f);auto* layer=get_mesh_layer(*b,f);
        auto* group=layer->groups->values.at(0);group->vertex_count=4;group->index_count=6;group->head=3;group->change=true;
        layer->current_group=5;b->current_layer=layer;b->empty_frames=19;b->depth.total=27;const auto storage=layer->groups;
        reset_mesh_builder(*b);check(group->vertex_count==0&&group->index_count==0&&group->head==0&&group->change);
        check(b->depth.total==0&&b->current_layer==layer&&b->empty_frames==19&&layer->current_group==5&&layer->groups==storage);}
    // Failure preserves the cleared prefix and the old depth total. Duplicate layer values are valid.
    {Factory f;auto c=cache();SubmissionMaterial material;auto* b=get_mesh_builder(c,&material,f);auto* first=get_mesh_layer(*b,f);
        b->layers->set(3,first);f.depth.layer=4;auto* second=get_mesh_layer(*b,f);b->layers->set(3,second);
        first->groups->values.at(0)->vertex_count=4;second->groups->values={nullptr};b->depth.total=99;
        fails([&]{reset_mesh_builder(*b);});check(first->groups->values.at(0)->vertex_count==0&&b->depth.total==99);
        b->layers.reset();fails([&]{reset_mesh_builder(*b);});}
    // Real cache order -> records; same-layer keys may wrap without triggering a sort.
    {Factory f;Records h;auto c=cache();MeshRecordList s{std::make_shared<std::vector<SubmissionRecord*>>()};SubmissionMaterial a,b;
        a.queue.absolute=std::numeric_limits<std::int32_t>::min();b.queue.absolute=std::numeric_limits<std::int32_t>::max();f.depth.layer=1;
        for(auto* material:{&b,&a}){auto* builder=get_mesh_builder(c,material,f);get_mesh_layer(*builder,f)->groups->values.at(0)->index_count=6;}
        update_mesh_records(c,s,h);check(s.active_count==2&&s.rows->at(0)->material==&a&&s.rows->at(1)->material==&b);
        check(s.rows->at(0)->key>s.rows->at(1)->key&&h.owned.size()==2&&h.prepared.size()==2);
        const auto retained=*s.rows;for(auto& builder:f.builders)reset_mesh_builder(*builder);
        update_mesh_records(c,s,h);check(s.active_count==0&&*s.rows==retained&&h.owned.size()==2);}
    // Values enumeration, signed totals, native multi-layer sort and retained suffix reuse.
    {Factory f;Records h;auto c=cache();SubmissionMaterial m;auto* builder=get_mesh_builder(c,&m,f);
        f.depth.layer=2;auto* high=get_mesh_layer(*builder,f);high->groups->values.at(0)->index_count=6;
        f.depth.layer=-1;auto* low=get_mesh_layer(*builder,f);low->groups->values.at(0)->index_count=6;
        f.depth.layer=3;auto* skip=get_mesh_layer(*builder,f);SubmissionGroup negative{-1};
        skip->groups->values.at(0)->index_count=std::numeric_limits<std::int32_t>::max();skip->groups->values.push_back(&negative);
        negative.index_count=1; // Wrapped signed sum is INT32_MIN, therefore skipped.
        MeshRecordList s{std::make_shared<std::vector<SubmissionRecord*>>()};update_mesh_records(c,s,h);
        check(h.prepared.size()==2&&h.prepared.at(0)==high->groups->values.at(0)&&h.prepared.at(1)==low->groups->values.at(0));
        check(s.active_count==2&&s.rows->at(0)->layer==low&&s.rows->at(1)->layer==high);
        high->groups->values.at(0)->index_count=0;auto* old_first=s.rows->at(0);update_mesh_records(c,s,h);
        check(s.active_count==1&&s.rows->size()==2&&s.rows->at(0)==old_first&&h.owned.size()==2);}
    // No live surface still produces records; zero-count groups retain their previous host mesh.
    {Factory f;Records h;auto c=cache();SubmissionMaterial m;auto* b=get_mesh_builder(c,&m,f);auto* l=get_mesh_layer(*b,f);
        SubmissionGroup empty{0},positive{6};l->groups->values={&empty,&positive};MeshRecordList s{std::make_shared<std::vector<SubmissionRecord*>>()};
        h.surface=false;update_mesh_records(c,s,h);check(s.active_count==1&&h.surface_calls==1&&h.prepared.empty());
        h.surface=true;update_mesh_records(c,s,h);check(h.prepared==std::vector<SubmissionGroup*>{&positive}&&h.owned.size()==1);}
    // A failed upload leaves the allocated record but does not initialize/publish its active count.
    {Factory f;Records h;auto c=cache();SubmissionMaterial a,b;
        for(auto* material:{&a,&b}){auto* builder=get_mesh_builder(c,material,f);get_mesh_layer(*builder,f)->groups->values.at(0)->index_count=6;}
        MeshRecordList s{std::make_shared<std::vector<SubmissionRecord*>>()};
        h.prepare_hook=[&](auto&){if(h.prepared.size()==2)throw std::runtime_error("upload failed");};
        fails([&]{update_mesh_records(c,s,h);});check(s.active_count==1&&s.rows->size()==2&&s.rows->at(0)->material==&b&&s.rows->at(1)->material==nullptr);
        h.prepare_hook={};h.prepared.clear();update_mesh_records(c,s,h);check(s.active_count==2&&h.owned.size()==2);}
    // Group enumeration invalidation follows the callback prefix; no stale vector iterator is retained.
    {Factory f;Records h;auto c=cache();SubmissionMaterial m;auto* b=get_mesh_builder(c,&m,f);auto* l=get_mesh_layer(*b,f);l->groups->values.at(0)->index_count=6;
        MeshRecordList s{std::make_shared<std::vector<SubmissionRecord*>>()};h.prepare_hook=[&](auto&){++l->groups->revision;};
        fails([&]{update_mesh_records(c,s,h);});check(s.active_count==0&&s.rows->size()==1);
        h.prepare_hook=[&](auto&){b->layers->set(7,l);};fails([&]{update_mesh_records(c,s,h);});check(s.active_count==1);}
    // Fresh record-list field after graphics callback; captured material/layer still initialize that record.
    {Factory f;Records h;auto c=cache();SubmissionMaterial m;auto* b=get_mesh_builder(c,&m,f);get_mesh_layer(*b,f)->groups->values.at(0)->index_count=6;
        MeshRecordList s{std::make_shared<std::vector<SubmissionRecord*>>()};auto old=s.rows;SubmissionRecord replacement;
        h.surface_hook=[&]{s.rows=std::make_shared<std::vector<SubmissionRecord*>>(1,&replacement);};update_mesh_records(c,s,h);
        check(s.active_count==1&&replacement.material==&m&&old->at(0)->material==nullptr);
        h.surface_hook={};s.rows->at(0)=nullptr;fails([&]{update_mesh_records(c,s,h);});check(s.active_count==0);
        c.sorted.reset();s.active_count=8;fails([&]{update_mesh_records(c,s,h);});check(s.active_count==0);}
    std::cout<<"mesh builder ownership contract ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
