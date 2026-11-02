#include "core/gameplay/select_form_pass.hpp"
#include <cstdlib>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
namespace {
void check(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::exit(1);}}
template<class Action>void fails(Action action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}check(failed,"expected failure");}
EventInstruction row(std::initializer_list<std::int32_t> values){return std::make_shared<EventIntegers>(values);}
EventScript rows(std::initializer_list<EventInstruction> values={}){return std::make_shared<std::vector<EventInstruction>>(values);}
struct RandomCall{std::string name;std::int32_t value;};
struct Draw{bool indexed;SelectBackgroundResource resource;std::int32_t clip,frame,layer,anchor,reverse;float x,y;};
struct Host final:SelectSceneryHost {
    std::vector<std::string> log;
    std::deque<RandomCall> random;
    std::deque<std::int32_t> heights;
    std::deque<std::array<std::int32_t,4>> bounds;
    std::vector<Draw> draws;
    std::function<void(const std::string&)> action;
    std::int32_t width=600,height=500,raw_height=500,edge=0,clip_bias=0,reverse=0;
    int token=0;
    SelectBackgroundResource resource=&token;
    void call(const std::string& name){log.push_back(name);if(action)action(name);}
    std::int32_t rng(const std::string& name){call(name);check(!random.empty(),"unexpected RNG query");const auto next=random.front();random.pop_front();check(next.name==name,"RNG stream/bound/order mismatch");return next.value;}
    std::int32_t app_random(std::int32_t n)override{return rng("A"+std::to_string(n));}
    std::int32_t lib_random(std::int32_t n)override{return rng("L"+std::to_string(n));}
    std::int32_t lib_random(std::int32_t a,std::int32_t b)override{return rng("L"+std::to_string(a)+":"+std::to_string(b));}
    std::int32_t raw_view_height()override{call("H");if(heights.empty())return raw_height;const auto n=heights.front();heights.pop_front();return n;}
    std::int32_t cloud_right_edge()override{call("E");return edge;}
    std::int32_t scenery_clip(std::int32_t slot)override{call("C"+std::to_string(slot));return slot+clip_bias;}
    std::int32_t game_width()override{call("W");return width;}
    std::int32_t game_height()override{call("G");return height;}
    SelectBackgroundResource scenery_resource()override{call("R");return resource;}
    std::array<std::int32_t,4> scenery_bounds(std::int32_t clip,std::int32_t frame)override{
        call("B"+std::to_string(clip)+":"+std::to_string(frame));check(!bounds.empty(),"unexpected bounds query");const auto value=bounds.front();bounds.pop_front();return value;
    }
    void scenery_reverse(std::int32_t value)override{call("V"+std::to_string(value));reverse=value;}
    void scenery_draw(SelectBackgroundResource r,std::int32_t clip,std::int32_t frame,float x,float y,std::int32_t anchor)override{
        draws.push_back({false,r,clip,frame,-1,anchor,reverse,x,y});call("D");
    }
    void scenery_draw_indexed(SelectBackgroundResource r,std::int32_t clip,std::int32_t frame,float x,float y,std::int32_t layer,std::int32_t anchor)override{
        draws.push_back({true,r,clip,frame,layer,anchor,reverse,x,y});call("I");
    }
};
struct Fixture {
    std::int32_t left=-100,right=100;
    SelectSceneryState state{rows(),rows(),rows(),left,right};
    Host host;
};
void cloud_initialization(){
    Fixture f;auto previous=f.state.clouds;previous->push_back(row({99}));f.host.edge=-14;f.host.heights={500,501};
    f.host.random={{"L0:330",12},{"A4",2},{"A20",0},{"L0:331",13},{"A4",3},{"A20",1}};
    int variants=0,spacings=0;
    f.host.action=[&](const auto& call){
        if(call=="A4"&&variants++==0){f.state.clouds=rows();f.host.clip_bias=100;}
        if(call=="A20")check(f.state.clouds->size()==static_cast<std::size_t>(++spacings),"append precedes spacing RNG");
    };
    initialize_select_scenery_clouds(f.state,f.host);
    check(previous->empty(),"clear old cloud owner before callbacks");check(f.state.clouds->size()==2,"zero spacing must produce another cloud");
    check(*f.state.clouds->at(0)==EventIntegers({-15,12,0x19c,0,1,2,0,20}),"first cloud row and captured clip");
    check(*f.state.clouds->at(1)==EventIntegers({-15,13,0x19c,0,1,3,0,20}),"second cloud and live height");
    check(f.state.clouds->at(0)!=f.state.clouds->at(1),"cloud rows have distinct owners");
    check(f.host.log==std::vector<std::string>({"C412","E","H","L0:330","A4","A20","E","H","L0:331","A4","A20","E"}),"cloud native query order");
    Fixture failure;failure.host.edge=-14;failure.host.random={{"L0:330",4},{"A4",1}};
    failure.host.action=[](const auto& call){if(call=="A20")throw std::runtime_error("spacing failed");};
    fails([&]{initialize_select_scenery_clouds(failure.state,failure.host);});
    check(failure.state.clouds->size()==1,"spacing failure retains appended cloud");
    Fixture empty;empty.host.edge=-15;initialize_select_scenery_clouds(empty.state,empty.host);
    check(empty.host.log==std::vector<std::string>({"C412","E"}),"empty cloud loop still captures clip");
}
void birds(){
    Fixture f;f.host.random={{"A2",1},{"A3",2},{"A60",7},{"A3",0},{"L20:330",44},{"A10",9},{"A3",0}};
    f.host.action=[&](const auto& call){if(call=="A60")f.left=-900;};
    auto bird=generate_select_bird(f.state,false,f.host);
    check(*bird==EventIntegers({2,-107,44,9,1,-1}),"bird left edge captured before offset callback");
    check(f.host.log==std::vector<std::string>({"A2","A3","A60","A3","H","L20:330","A10","A3"}),"bird RNG streams and height order");
    Fixture reverse;reverse.host.random={{"A2",0},{"A3",1},{"A60",8},{"A3",2},{"L20:330",30},{"A10",4},{"A3",2}};
    reverse.host.action=[&](const auto& call){if(call=="A60")reverse.right=999;};
    check(*generate_select_bird(reverse.state,false,reverse.host)==EventIntegers({1,108,30,4,-3,1}),"bird reverse direction and captured right edge");
    Fixture initial;initial.host.random={{"A2",1},{"A3",1},{"A60",4},{"A3",2},{"L-150:150",-30},{"L20:330",45},{"A10",3},{"A3",2}};
    initial.host.action=[&](const auto& call){if(call=="A60"){initial.left=-200;initial.right=200;}};
    check(*generate_select_bird(initial.state,true,initial.host)==EventIntegers({1,-30,45,3,3,1}),"initial bird overwrites X after consuming edge draws");
    check(initial.host.random.empty(),"initial bird range call retained");
}
void leaves(){
    Fixture negative;negative.host.heights={1,0};negative.host.random={{"A3",2},{"L-100:100",7},{"L60",3},{"A10",6}};
    check(*generate_select_leaf(negative.state,false,negative.host)==EventIntegers({2,7,-4,6,0,0}),"leaf signed division truncates toward zero");
    check(negative.host.log==std::vector<std::string>({"A3","L-100:100","H","H","L60","A10"}),"leaf two height reads before offset draw");
    Fixture initial;initial.host.heights={640,641,642};initial.host.random={{"A3",0},{"L-100:100",10},{"L60",59},{"L-159:642",200},{"A10",9}};
    check(*generate_select_leaf(initial.state,true,initial.host)==EventIntegers({0,10,200,9,0,0}),"initial leaf consumes unused offset then overwrites Y");
    check(initial.host.log==std::vector<std::string>({"A3","L-100:100","H","H","L60","H","L-159:642","A10"}),"initial leaf fresh third height and streams");
    Fixture wrap;const auto max=std::numeric_limits<std::int32_t>::max();wrap.host.heights={max,max};
    wrap.host.random={{"A3",1},{"L-100:100",0},{"L60",1},{"A10",0}};
    check(generate_select_leaf(wrap.state,false,wrap.host)->at(2)==max,"leaf multiply/base/offset use int32 wrapping");
}
void decoration_initialization(){
    Fixture f;auto old_birds=f.state.birds,old_leaves=f.state.leaves;EventScript published;
    f.host.random={{"A4",1},{"A2",1},{"A3",0},{"A60",1},{"A3",0},{"L20:330",21},{"A10",0},{"A3",1}};
    f.host.action=[&](const auto& call){
        if(call=="A4"){
            check(f.state.birds!=old_birds&&f.state.leaves!=old_leaves,"both lists published before first RNG");
            check(f.state.birds!=f.state.leaves,"separate decoration owners");published=f.state.birds;
        }
        if(call=="A2")f.state.birds=rows();
        if(call=="A25")throw std::runtime_error("leaf count failed");
    };
    fails([&]{initialize_select_decorations(f.state,f.host);});
    check(published&&published->size()==1&&f.state.birds->empty(),"captured destination retains bird across owner replacement");
    check(f.state.leaves->empty(),"leaf count failure retains published empty list");
    Fixture leaves;leaves.host.random={{"A4",0},{"A25",0}};
    for(int i=0;i<5;++i){leaves.host.random.push_back({"A3",i%3});leaves.host.random.push_back({"L-100:100",i});leaves.host.random.push_back({"L60",i});leaves.host.random.push_back({"L-125:500",i});leaves.host.random.push_back({"A10",i});}
    initialize_select_decorations(leaves.state,leaves.host);
    check(leaves.state.birds->empty()&&leaves.state.leaves->size()==5,"native minimum five initial leaves");
    for(std::size_t i=1;i<5;++i)check(leaves.state.leaves->at(i)!=leaves.state.leaves->at(i-1),"fresh leaf owner per call");
    Fixture early;old_birds=early.state.birds;old_leaves=early.state.leaves;
    early.host.action=[](const auto& call){if(call=="A4")throw std::runtime_error("first draw failed");};
    fails([&]{initialize_select_decorations(early.state,early.host);});
    check(early.state.birds!=old_birds&&early.state.leaves!=old_leaves,"publication survives initial RNG failure");
}
void decoration_drawing(){
    Fixture f;auto first=row({2,10,20,9,0,-1});f.state.birds=rows({first,row({0,-999,0})});
    draw_select_decorations(f.state,0,{-39,3},f.host);
    check(f.host.draws.size()==1,"culled short row never reads draw-only cells");const auto d=f.host.draws.front();
    check(d.indexed&&d.clip==0x1bc&&d.frame==9&&d.layer==2&&d.anchor==2&&d.x==9&&d.y==23&&d.reverse==1,"bird signed parallax selected layer and anchor");
    check(f.host.log==std::vector<std::string>({"W","G","V1","R","C444","I","V0"}),"bird culling/draw/reverse query order");
    Fixture live;auto original=row({1,10,20,5,0,-1});live.state.birds=rows({original});
    live.host.action=[&](const auto& call){if(call=="I"&&live.host.draws.size()==1){original->at(5)=0;live.state.birds=rows({nullptr,row({7,30,40,-17,0,0})});}};
    draw_select_decorations(live.state,0,{-39,3},live.host);
    check(live.host.draws.size()==2&&live.host.draws[1].layer==7&&live.host.draws[1].x==29,"decoration loop rereads list and size");
    check(live.host.reverse==1,"reverse cleanup rereads live VY instead of restoring saved state");
    Fixture leaf;leaf.state.leaves=rows({row({2,10,20,-17,0,-1})});
    draw_select_decorations(leaf.state,2,{-39,3},leaf.host);
    check(leaf.host.draws.size()==1&&leaf.host.draws[0].clip==0x1c0&&leaf.host.draws[0].frame==-1&&leaf.host.draws[0].x==-29,"leaf full camera offset and frame division");
    Fixture failure;failure.state.birds=rows({row({0,1,2,3,0,-1})});failure.host.action=[](const auto& call){if(call=="I")throw std::runtime_error("draw failed");};
    fails([&]{draw_select_decorations(failure.state,0,{0,0},failure.host);});check(failure.host.reverse==1,"draw failure retains reverse prefix");
    Fixture unsupported;unsupported.state.birds.reset();unsupported.state.leaves.reset();
    draw_select_decorations(unsupported.state,1,{0,0},unsupported.host);check(unsupported.host.log.empty(),"unsupported type does not enumerate");
}
void terrain(){
    Fixture f;f.left=0;f.right=0;f.host.height=200;f.host.heights={500,510};f.host.bounds={{{0,0,100,20}},{{0,0,80,20}}};
    f.state.clouds=rows({row({10,20,800,0,1,2}),row({40,80,900,0,1,8})});
    draw_select_terrain(f.state,{-3,5},f.host);
    check(f.host.draws.size()==4,"two ground passes precede clouds");
    const auto& a=f.host.draws[0];const auto& b=f.host.draws[1];const auto& c=f.host.draws[2];const auto& d=f.host.draws[3];
    check(!a.indexed&&a.clip==0x164&&a.frame==3&&a.anchor==0&&a.x==-1&&a.y==380,"ground pass uses half camera and fresh raw height");
    check(!b.indexed&&b.clip==0x174&&b.frame==1&&b.x==-3&&b.y==495,"second ground uses fresh bounds and fixed Y");
    check(c.clip==900&&c.frame==8&&c.x==40&&c.y==85&&d.clip==800&&d.frame==2&&d.x==10&&d.y==25,"clouds reverse insertion order and parallax truncate");
    check(f.host.log==std::vector<std::string>({"C372","B372:0","G","W","G","R","H","C356","D","C372","B372:0","H","G","W","G","R","C372","D","G","W","G","R","D","G","W","G","R","D"}),"terrain culling and resource query order");
    Fixture empty;empty.left=100;empty.right=-100;empty.host.bounds={{{0,0,50,10}},{{0,0,50,10}}};
    draw_select_terrain(empty.state,{0,0},empty.host);
    check(empty.host.log==std::vector<std::string>({"C372","B372:0","C372","B372:0","H"}),"empty ground loops still query both bounds and second height");
    Fixture culled;culled.left=0;culled.right=0;culled.host.bounds={{{0,0,50,10}},{{0,0,50,10}}};
    draw_select_terrain(culled.state,{-1000,0},culled.host);
    check(culled.host.draws.empty(),"both terrain passes culled");
    check(culled.host.log==std::vector<std::string>({"C372","B372:0","G","C372","B372:0","H","G"}),"left culling skips width, resource and draw-only height");
}
}
int main(){cloud_initialization();birds();leaves();decoration_initialization();decoration_drawing();terrain();return 0;}
