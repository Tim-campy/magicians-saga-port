#include "core/gameplay/stage_controls.hpp"
#include "core/gameplay/ecs_combat.hpp"
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
struct MpActor {
    ComponentSlots entity{35};DamageParameter mp{20,50,3,7};DamageParameterMap values{{1,&mp}};CombatParameterLinks links;
    MpActor(){links.values=&values;entity.add(23,&links);}
};
struct MpHost final:StageMpDrawingHost {
    std::int32_t width=600,height=640,text_width=27,player_queries=0,formatted_value=0,formatted_maximum=0;
    ComponentSlots* player=nullptr;MpNumberContext context=this,drawn_context=nullptr;
    std::u16string formatted=u"23/57",drawn_text;
    std::vector<std::string> calls;std::string fail;std::function<void(const std::string&)> callback;
    std::array<std::int32_t,4> number_args{};float icon_x=0,icon_y=0;std::int32_t icon_anchor=0;
    std::optional<ParameterGaugeRequest> gauge;
    void step(const std::string& name){calls.push_back(name);if(callback)callback(name);if(name==fail)throw std::runtime_error(name);}
    std::int32_t mp_width()override{step("width");return width;}
    std::int32_t mp_height()override{step("height");return height;}
    ComponentSlots* mp_player()override{step(++player_queries==1?"player1":"player2");return player;}
    MpNumberContext capture_mp_numbers()override{step("capture");return context;}
    std::u16string format_mp(std::int32_t value,std::int32_t maximum)override{
        formatted_value=value;formatted_maximum=maximum;step("format");return formatted;
    }
    std::int32_t draw_mp_numbers(MpNumberContext captured,const std::u16string& text,std::int32_t x,std::int32_t y,std::int32_t pad,std::int32_t anchor)override{
        drawn_context=captured;drawn_text=text;number_args={x,y,pad,anchor};step("numbers");return text_width;
    }
    void draw_mp_icon(float x,float y,std::int32_t anchor)override{icon_x=x;icon_y=y;icon_anchor=anchor;step("icon");}
    void mp_parameter_gauge(const ParameterGaugeRequest& request)override{gauge=request;step("gauge");}
};
void mp_contracts(){
    const std::vector<std::string> order{"width","height","player1","capture","format","numbers","icon","player2","gauge"};
    // No HP/Human/state dependency: only the MP component is needed before drawing.
    {MpActor a;MpHost h;h.player=&a.entity;draw_stage_player_mp_gauge(h);check(h.calls==order);
     check(h.formatted_value==23&&h.formatted_maximum==57&&h.drawn_text==u"23/57"&&h.drawn_context==&h);
     check(h.number_args==std::array<std::int32_t,4>{517,638,0,68}&&h.icon_x==478&&h.icon_y==638&&h.icon_anchor==68);
     check(h.gauge&&h.gauge->entity==&a.entity&&h.gauge->x==600&&h.gauge->y==640&&h.gauge->image==4&&h.gauge->parameter==1&&h.gauge->anchor==68&&h.gauge->force);
     check(a.mp.value==20&&a.mp.maximum==50&&a.mp.extra==3&&a.mp.extra_maximum==7&&h.player_queries==2);}
    for(const auto values:std::array<std::array<std::int32_t,6>,4>{{
        {{0,0,0,8,0,8}},{{-5,-10,2,3,-3,-7}},
        {{std::numeric_limits<std::int32_t>::max(),std::numeric_limits<std::int32_t>::min(),1,-1,std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max()}},
        {{100,1,50,-1,150,0}}}}){
     MpActor a;MpHost h;h.player=&a.entity;a.mp={values[0],values[1],values[2],values[3]};draw_stage_player_mp_gauge(h);
     check(h.formatted_value==values[4]&&h.formatted_maximum==values[5]&&h.calls==order);}
    // Capture precedes getter totals and the external formatter; preserve the borrowed identity.
    {MpActor a,b;MpHost h;h.player=&a.entity;int captured_identity=0,new_identity=0;h.context=&captured_identity;
     h.callback=[&](const std::string& name){
         if(name=="capture"){a.mp.value=40;a.mp.extra=2;a.mp.maximum=90;a.mp.extra_maximum=1;}
         if(name=="format"){h.context=&new_identity;a.mp.value=999;h.formatted=u"custom MP";}
         if(name=="numbers"){h.width=1;h.height=2;}
         if(name=="icon")h.player=&b.entity;
     };
     draw_stage_player_mp_gauge(h);check(h.drawn_context==&captured_identity&&h.drawn_text==u"custom MP"&&h.formatted_value==42&&h.formatted_maximum==91);
     check(h.number_args==std::array<std::int32_t,4>{517,638,0,68}&&h.gauge->entity==&b.entity&&h.gauge->x==600&&h.gauge->y==640);}
    {MpActor a;MpHost h;h.player=&a.entity;h.callback=[&](const std::string& name){if(name=="icon")h.player=nullptr;};
     draw_stage_player_mp_gauge(h);check(h.calls==order&&h.gauge&&!h.gauge->entity);} // Concrete GaugeSystem validates the fresh result.
    // The text width is a signed int32; subtraction wraps before the icon's binary32 conversion.
    {MpActor a;MpHost h;h.player=&a.entity;h.width=std::numeric_limits<std::int32_t>::max();h.height=std::numeric_limits<std::int32_t>::min();
     h.text_width=std::numeric_limits<std::int32_t>::min();draw_stage_player_mp_gauge(h);
     check(h.number_args[0]==2147483564&&h.number_args[1]==2147483646&&h.icon_x==-96.0F&&h.icon_y==2147483648.0F);
     check(h.gauge->x==std::numeric_limits<std::int32_t>::max()&&h.gauge->y==std::numeric_limits<std::int32_t>::min());}
    {MpActor a;MpHost h;h.player=&a.entity;h.width=std::numeric_limits<std::int32_t>::min();h.text_width=std::numeric_limits<std::int32_t>::min();
     draw_stage_player_mp_gauge(h);check(h.number_args[0]==2147483565&&h.icon_x==-95.0F);}
    {MpActor a;MpHost h;h.player=&a.entity;h.text_width=-7;draw_stage_player_mp_gauge(h);check(h.icon_x==512.0F);}
    // Every provider failure leaves the completed draw prefix intact, with no later player query.
    for(std::size_t i=0;i<order.size();++i){MpActor a;MpHost h;h.player=&a.entity;h.fail=order[i];fails([&]{draw_stage_player_mp_gauge(h);});
     check(h.calls==std::vector<std::string>(order.begin(),order.begin()+static_cast<std::ptrdiff_t>(i+1)));}
    {MpHost h;fails([&]{draw_stage_player_mp_gauge(h);});check(h.calls==std::vector<std::string>{"width","height","player1"});}
    for(int kind=0;kind<5;++kind){MpActor a;MpHost h;h.player=&a.entity;Component wrong;
     if(kind==0)a.entity.remove(23);if(kind==1)a.links.values=nullptr;if(kind==2)a.values.clear();if(kind==3)a.values[1]=nullptr;if(kind==4)a.entity.change(23,&wrong);
     fails([&]{draw_stage_player_mp_gauge(h);});check(h.calls==std::vector<std::string>{"width","height","player1"});}
    {MpActor a;MpHost h;h.player=&a.entity;h.context=nullptr;h.callback=[&](const std::string& name){if(name=="numbers"){check(!h.drawn_context);throw std::runtime_error("missing captured AppData");}};
     fails([&]{draw_stage_player_mp_gauge(h);});check(h.calls==std::vector<std::string>{"width","height","player1","capture","format","numbers"});}
}
int main()try{
    mp_contracts();
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
