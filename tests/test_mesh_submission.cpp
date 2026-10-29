#include "core/gameplay/mesh_submission.hpp"
#include <array>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <string>

using namespace saga::core;
namespace {
void check_at(bool value,int line){if(!value){std::cerr<<"mesh submission line "<<line<<'\n';std::abort();}}
#define check(x) check_at((x),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Node {
    SubmissionImage image;SubmissionMaterial material;SubmissionGroup group;
    SubmissionLayer layer;SubmissionRecord record;
    void init(int index,int queue,int count=6){
        material.queue.absolute=queue;material.unused_frames=91;material.image=&image;group.index_count=count;
        layer.index=index;layer.groups=std::make_shared<SubmissionGroupList>();layer.groups->values={&group};
        init_submission_record(record,&material,&layer);
    }
};
struct Host:MeshSubmissionHost {
    std::map<SubmissionMaterial*,int> materials;std::map<SubmissionGroup*,int> groups;
    std::vector<int> drawn,passed;std::vector<float> matrices;std::vector<std::string> calls;
    int clears=0,checks=0;std::function<void(const std::string&)> hook;
    void event(const std::string& s){calls.push_back(s);if(hook)hook(s);}
    void clear_depth()override{++clears;event("clear");}
    void set_material_pass(SubmissionMaterial& m)override{check(m.unused_frames==0);passed.push_back(materials.at(&m));event("pass");}
    void draw_mesh(SubmissionGroup& g,OffscreenMatrix m)override{drawn.push_back(groups.at(&g));matrices.push_back(m.values[12]);event("draw");}
    void after_mesh_draw(SubmissionMaterial&,SubmissionGroup&)override{event("log");}
    std::int32_t check_count()override{++checks;event("check");return -77;}
};
struct Scene {
    std::array<Node,8> nodes;Host host;MeshSubmissionState state;
    Scene(){for(std::size_t i=0;i<nodes.size();++i){nodes[i].init(1,2000);host.materials[&nodes[i].material]=static_cast<int>(i);host.groups[&nodes[i].group]=static_cast<int>(i);}}
    void streams(std::initializer_list<std::vector<SubmissionRecord*>> streams){
        state.cursors=std::make_shared<std::vector<std::int32_t>>(streams.size(),0);
        state.lengths=std::make_shared<std::vector<std::int32_t>>();
        state.lists=std::make_shared<std::vector<SubmissionRecords>>();state.matrices=std::make_shared<std::vector<OffscreenMatrix>>();
        state.used_materials=std::make_shared<std::vector<SubmissionMaterial*>>(8,&nodes[7].material);
        for(const auto& s:streams){state.lengths->push_back(static_cast<std::int32_t>(s.size()));state.lists->push_back(std::make_shared<std::vector<SubmissionRecord*>>(s));
            OffscreenMatrix m;m.values[12]=static_cast<float>(state.matrices->size()+10);state.matrices->push_back(m);}
    }
    void submit(){submit_mesh_streams(state,static_cast<std::int32_t>(state.lengths->size()),host);}
};
}
int main()try{
    // LayerMesh keys include layer*100000 and wrap before signed comparison.
    {Node n;n.init(2,3005);check(n.record.key==203005);
        n.init(-2,-9);check(n.record.key==-200009);n.init(42950,2000);check(n.record.key==34704);
        n.record.key=123;fails([&]{init_submission_record(n.record,nullptr,&n.layer);});
        check(!n.record.material&&n.record.layer==&n.layer&&n.record.key==123);
        fails([&]{init_submission_record(n.record,&n.material,nullptr);});check(n.record.material==&n.material&&!n.record.layer&&n.record.key==123);}
    // Count is the wrapping signed sum, not a clamp or count of nonempty groups.
    {Node n;n.init(0,2000);SubmissionGroup b{0};n.layer.groups->values.push_back(&b);check(submission_index_count(n.layer)==6);
        n.group.index_count=std::numeric_limits<std::int32_t>::max();b.index_count=1;
        check(submission_index_count(n.layer)==std::numeric_limits<std::int32_t>::min());
        n.layer.groups->values={nullptr};fails([&]{submission_index_count(n.layer);});n.layer.groups.reset();fails([&]{submission_index_count(n.layer);});}
    // The original quicksort's equal-key permutations are observable and shared with AppData.
    {Scene s;const std::array<int,5> keys{2,1,2,0,1};auto rows=std::make_shared<std::vector<SubmissionRecord*>>();
        for(std::size_t i=0;i<keys.size();++i){s.nodes[i].record.key=keys[i];rows->push_back(&s.nodes[i].record);}
        check(submission_pivot(rows,0,4)==0);sort_submissions(rows,0,4);
        const std::array<int,5> order{3,1,4,2,0};for(std::size_t i=0;i<order.size();++i)check(rows->at(i)==&s.nodes[static_cast<std::size_t>(order[i])].record);
        s.nodes[4].record.key=2;
        auto equal=std::make_shared<std::vector<SubmissionRecord*>>(std::initializer_list<SubmissionRecord*>{&s.nodes[0].record,&s.nodes[2].record,&s.nodes[4].record});const auto before=*equal;
        sort_submissions(equal,0,2);check(*equal==before);check(submission_pivot(nullptr,2,1)==-1);
        sort_submissions(nullptr,0,0);check(partition_submissions(nullptr,2,1,0)==2);
        rows->at(0)=nullptr;fails([&]{submission_pivot(rows,0,4);});}
    // Dynamic stream0 wins equal keys; consecutive equal heads drain from that stream first.
    {Scene s;s.nodes[1].init(1,3000);s.nodes[2].init(2,2000);s.nodes[3].init(0,3000);
        s.streams({{&s.nodes[0].record,&s.nodes[1].record,&s.nodes[2].record},{&s.nodes[3].record,&s.nodes[4].record,&s.nodes[5].record}});
        const auto retained=s.state.used_materials;s.state.used_count=123;s.submit();
        check((s.host.drawn==std::vector<int>{3,0,4,5,1,2}));check(s.host.drawn==s.host.passed);
        check((s.host.matrices==std::vector<float>{11,10,11,11,10,10}));check(s.host.clears==2&&s.host.checks==6);
        check(*s.state.cursors==std::vector<std::int32_t>({3,3}));check(s.state.used_materials==retained&&s.state.used_count==6);
        const std::array<int,6> used{3,0,4,5,1,2};for(std::size_t i=0;i<used.size();++i)check(retained->at(i)==&s.nodes[static_cast<std::size_t>(used[i])].material);
        check(retained->at(7)==&s.nodes[7].material);for(int i:used)check(s.nodes[static_cast<std::size_t>(i)].image.access_check==-77);}
    // Empty layers are consumed without clearing or changing the previous drawable layer.
    {Scene s;s.nodes[1].init(8,2000,0);s.nodes[2].init(1,2000);for(auto& n:s.nodes)n.record.key=10;
        s.streams({{&s.nodes[0].record,&s.nodes[1].record,&s.nodes[2].record}});s.submit();
        check((s.host.drawn==std::vector<int>{0,2}));check(s.host.clears==0&&s.state.cursors->at(0)==3&&s.nodes[1].material.unused_frames==91);}
    // A drawable layer submits ALL groups, even a zero-index group. No per-group gate is added.
    {Scene s;SubmissionGroup empty{0};s.host.groups[&empty]=8;s.nodes[0].layer.groups->values.push_back(&empty);
        s.streams({{&s.nodes[0].record}});s.submit();check((s.host.drawn==std::vector<int>{0,8}));check(s.host.passed.size()==1&&s.host.checks==1);}
    // The int32-min sentinel suppresses the next clear even after drawing that layer index.
    {Scene s;s.nodes[0].layer.index=7;s.nodes[1].layer.index=std::numeric_limits<std::int32_t>::min();s.nodes[2].layer.index=9;
        s.streams({{&s.nodes[0].record,&s.nodes[1].record,&s.nodes[2].record}});s.submit();check(s.host.clears==1);}
    // Draw failure retains successful material/image prefix, resets inactivity before the fault,
    // and leaves the failing record's cursor in place. The outer managed catch is a caller concern.
    {Scene s;s.streams({{&s.nodes[0].record,&s.nodes[1].record}});int calls=0;
        s.host.hook=[&](const auto& event){if(event=="draw"&&++calls==2)throw std::runtime_error("graphics");};
        fails([&]{s.submit();});check(s.state.cursors->at(0)==1&&s.state.used_count==1);
        check(s.nodes[0].image.access_check==-77&&s.nodes[1].image.access_check==0&&s.nodes[1].material.unused_frames==0);}
    // Live matrix rereads per mesh, captured group-list identity and public enumeration invalidation.
    {Scene s;SubmissionGroup second{6};s.host.groups[&second]=8;s.nodes[0].layer.groups->values.push_back(&second);
        s.streams({{&s.nodes[0].record}});s.host.hook=[&](const auto& event){if(event=="log")s.state.matrices->at(0).values[12]=27;};
        s.submit();check((s.host.matrices==std::vector<float>{10,27}));}
    {Scene s;auto old=s.nodes[0].layer.groups;s.streams({{&s.nodes[0].record}});
        s.host.hook=[&](const auto& event){if(event=="draw"){old->values.push_back(&s.nodes[1].group);++old->revision;}};
        fails([&]{s.submit();});check(s.state.cursors->at(0)==0&&s.state.used_count==0&&s.host.drawn.size()==1&&s.host.checks==0);}
    // An image is captured before the check counter read; unused-material storage faults follow the stamp.
    {Scene s;s.streams({{&s.nodes[0].record}});SubmissionImage replacement;
        s.host.hook=[&](const auto& event){if(event=="check")s.nodes[0].material.image=&replacement;};
        s.state.used_materials.reset();fails([&]{s.submit();});check(s.nodes[0].image.access_check==-77&&replacement.access_check==0&&s.state.cursors->at(0)==0);}
    {Scene s;s.streams({{&s.nodes[0].record}});s.state.used_materials->resize(1);
        s.host.hook=[&](const auto& event){if(event=="check")s.state.used_count=3;};
        fails([&]{s.submit();});check(s.state.used_materials->size()==2&&s.state.used_materials->at(1)==nullptr);
        check(s.state.used_count==3&&s.nodes[0].image.access_check==-77&&s.state.cursors->at(0)==0);}
    // Zero streams requires no arrays, and exhausted streams require no record list or matrix.
    {MeshSubmissionState s;s.used_count=17;Host h;submit_mesh_streams(s,0,h);check(s.used_count==0&&h.calls.empty());
        s.cursors=std::make_shared<std::vector<std::int32_t>>(1,2);s.lengths=std::make_shared<std::vector<std::int32_t>>(1,1);
        submit_mesh_streams(s,1,h);s.cursors->at(0)=-1;fails([&]{submit_mesh_streams(s,1,h);});}
    std::cout<<"mesh submission contract ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
