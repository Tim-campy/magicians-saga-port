#include "core/gameplay/stage_controls.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace saga::core;
void check_at(bool value,int line){if(!value){std::cerr<<"stage controls failed at "<<line<<'\n';std::abort();}}
#define check(...) check_at((__VA_ARGS__),__LINE__)
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);}
struct Call {
    std::string kind;std::vector<std::int32_t> args;std::optional<std::int32_t> frame;
};
struct Host final:StageControlsHost {
    std::int32_t width=600,height=640;
    std::vector<Call> calls;std::function<void(const Call&)> callback;
    void record(Call c){calls.push_back(c);if(callback)callback(c);}
    std::int32_t controls_width()override{record({"width",{},{}});return width;}
    std::int32_t controls_height()override{record({"height",{},{}});return height;}
    void controls_draw_seb(std::int32_t r,std::int32_t c,std::optional<std::int32_t> f,std::int32_t x,std::int32_t y)override{record({"draw",{r,c,x,y},f});}
    void controls_set_render_mode(std::int32_t m,std::int32_t a)override{record({"mode",{m,a},{}});}
    void controls_add_attack(std::int32_t x,std::int32_t y,std::int32_t w,std::int32_t h,std::int32_t v,std::array<std::int32_t,4> margins)override{
        record({"attack",{x,y,w,h,v,margins[0],margins[1],margins[2],margins[3]}, {}});
    }
    void controls_move_button(std::int32_t g,std::int32_t x,std::int32_t y,std::int32_t c,std::int32_t k,std::int32_t a,std::array<std::int32_t,4> margins)override{
        record({"move",{g,x,y,c,k,a,margins[0],margins[1],margins[2],margins[3]}, {}});
    }
};
void buttons(StageStartState& s,std::initializer_list<AttackButtonState*> values){s.attack_buttons=std::make_shared<std::vector<AttackButtonState*>>(values);}
int main()try{
    // Native easing values at each animation pass, including both endpoints.
    const std::array<std::int32_t,5> off0{-16,-9,-3,-1,0},off1{0,-6,-12,-14,-16};
    const std::array<std::int32_t,5> alpha0{255,145,62,17,0},alpha1{0,109,192,237,255};
    for(int state:{0,1})for(int frame=0;frame<=4;++frame){
        StageStartState s;AttackButtonState b{state,frame};buttons(s,{&b});Host h;
        draw_stage_attack_controls(s,h);
        check(h.calls[0].kind=="width"&&h.calls[1].kind=="height");
        const auto offset=(state==0?off0:off1)[frame],alpha=(state==0?alpha0:alpha1)[frame];
        std::size_t at=2;
        if(alpha<=254){check(h.calls[at].kind=="draw"&&h.calls[at].frame==0&&h.calls[at].args==std::vector<std::int32_t>({7,0,556+offset,460}));++at;}
        if(alpha>=1){check(h.calls[at].kind=="mode"&&h.calls[at].args==std::vector<std::int32_t>({1,alpha}));++at;
            check(h.calls[at].kind=="draw"&&h.calls[at].frame==1);++at;
            check(h.calls[at].kind=="mode"&&h.calls[at].args==std::vector<std::int32_t>({0,255}));++at;}
        check(h.calls[at].kind=="attack"&&h.calls[at].args==std::vector<std::int32_t>({556+offset,460,60,60,0x230000,10,0,0,0}));
        check(h.calls.size()==at+1&&b.state==state&&b.frame==frame);
    }
    {StageStartState s;AttackButtonState a{2,0},b{-1,99},c{7,4},d{2,-99};buttons(s,{&a,&b,&c,&d});Host h;
     draw_stage_attack_controls(s,h);check(h.calls.size()==10);
     for(int i=0;i<4;++i){check(h.calls[2+2*i].frame==2*i);check(h.calls[3+2*i].args[1]==460+55*i&&h.calls[3+2*i].args[4]==(0x230000|i));}}
    {StageStartState s;buttons(s,{});Host h;draw_stage_attack_controls(s,h);check(h.calls.size()==2);}
    {StageStartState s;Host h;fails([&]{draw_stage_attack_controls(s,h);});check(h.calls.size()==2);}
    {StageStartState s;AttackButtonState b{2,0};buttons(s,{&b,nullptr});Host h;
     fails([&]{draw_stage_attack_controls(s,h);});check(h.calls.size()==4&&h.calls.back().kind=="attack");}
    {StageStartState s;AttackButtonState a{2,0},b{2,0},c{2,0};buttons(s,{&a});Host h;
     h.callback=[&](const Call& call){if(call.kind=="attack"&&call.args[4]==0x230000)buttons(s,{nullptr,&b,&c});};
     draw_stage_attack_controls(s,h);check(h.calls.size()==8&&h.calls.back().args[4]==0x230002);}
    {StageStartState s;AttackButtonState a{2,0},b{2,0};buttons(s,{&a});Host h;
     h.callback=[&](const Call& call){if(call.kind=="draw"&&call.frame==0)buttons(s,{nullptr,&b});};
     draw_stage_attack_controls(s,h);check(h.calls.size()==6&&h.calls[3].args[4]==0x230000&&h.calls.back().args[4]==0x230001);}
    {StageStartState s;AttackButtonState a{2,0},b{2,0};buttons(s,{&a,&b});Host h;
     h.callback=[&](const Call& call){if(call.kind=="attack")buttons(s,{});};draw_stage_attack_controls(s,h);check(h.calls.size()==4);}
    {StageStartState s;AttackButtonState a{2,0};buttons(s,{&a});Host h;
     h.callback=[&](const Call& call){if(call.kind=="attack")s.attack_buttons.reset();};
     fails([&]{draw_stage_attack_controls(s,h);});check(h.calls.size()==4&&h.calls.back().kind=="attack");}
    {StageStartState s;AttackButtonState a{0,0};buttons(s,{&a});Host h;
     h.callback=[](const Call& call){if(call.kind=="draw")throw std::runtime_error("draw failed");};
     fails([&]{draw_stage_attack_controls(s,h);});check(h.calls.size()==4&&h.calls[2].kind=="mode"&&h.calls.back().kind=="draw");} // No invented reset/finally on failure.
    {StageStartState s;AttackButtonState a{2,0};buttons(s,{&a});Host h;
     h.callback=[&](const Call& call){if(call.kind=="height")buttons(s,{});};draw_stage_attack_controls(s,h);check(h.calls.size()==2);}
    {StageStartState s;Host h;h.callback=[](const Call& call){if(call.kind=="width")throw std::runtime_error("width failed");};
     fails([&]{draw_stage_attack_controls(s,h);});check(h.calls.size()==1);}
    {StageStartState s;AttackButtonState a{2,0};buttons(s,{&a});Host h;h.width=h.height=std::numeric_limits<std::int32_t>::min();
     draw_stage_attack_controls(s,h);check(h.calls.back().args[0]==2147483604&&h.calls.back().args[1]==2147483468);}
    for(bool front:{false,true})for(bool boss:{false,true}){
        StageStartState s;s.state=6;s.move_front_pushed=front;s.collection.boss_defeated=boss;Host h;
        draw_stage_move_controls(s,h);check(h.calls[0].kind=="height");
        check(h.calls[1].kind=="move"&&h.calls[1].args==std::vector<std::int32_t>({35,2,565,36,0x20000,17,0,10,15,5}));
        std::size_t at=2;
        if(!front){check(h.calls[at].kind=="draw"&&!h.calls[at].frame&&h.calls[at].args==std::vector<std::int32_t>({2,4,75,587}));++at;}
        if(!boss){check(h.calls[at].kind=="move"&&h.calls[at].args==std::vector<std::int32_t>({35,2,602,37,0x80000,17,0,10,5,15}));++at;}
        check(h.calls.size()==at);
    }
    {StageStartState s;Host h;h.callback=[&](const Call& call){if(call.kind=="move"){s.move_front_pushed=true;s.collection.boss_defeated=true;}};
     draw_stage_move_controls(s,h);check(h.calls.size()==2);}
    {StageStartState s;Host h;h.callback=[&](const Call& call){if(call.kind=="draw")s.collection.boss_defeated=true;};
     draw_stage_move_controls(s,h);check(h.calls.size()==3);}
    {StageStartState s;Host h;h.callback=[](const Call& call){if(call.kind=="move")throw std::runtime_error("move failed");};
     fails([&]{draw_stage_move_controls(s,h);});check(h.calls.size()==2);}
    {StageStartState s;s.move_front_pushed=true;s.collection.boss_defeated=true;Host h;h.height=std::numeric_limits<std::int32_t>::min();
     draw_stage_move_controls(s,h);check(h.calls.back().args[2]==2147483573);}
    std::cout<<"stage controls ok\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
