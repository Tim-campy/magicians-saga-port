#include "core/gameplay/quest_frame.hpp"
#include "core/gameplay/select_prelude.hpp"
#include <cstdlib>
#include <functional>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool value){if(!value)std::abort();}
struct Host final:QuestFrameHost {
    QuestFrameState state;std::int32_t fade=0;std::vector<std::string> calls;std::vector<int> passes,engine_queries;
    ImageAnimationValues clocks{{std::make_shared<ImageAnimationGroup>(1,std::make_shared<EventIntegers>(EventIntegers{4,7}))}};
    std::uint32_t flags=0x100,held=0,pulses=0;bool debug=false,is_active=true,top=true;std::set<int> keys;
    int resets=0,advances=0,config_reads=0,active_reads=0,top_reads=0;
    std::function<void(int)> on_world;std::function<void()> on_advance,on_result,on_change;
    bool run(){return update_quest_frame(state,fade,*this);}
    void base_update()override{calls.push_back("base");}
    bool flag_check(std::uint32_t mask)override{check(mask==0x100);calls.push_back("flag");return (flags&mask)!=0;}
    void flag_off(std::uint32_t mask)override{calls.push_back("off");flags&=~mask;}
    void reset_image_clocks()override{calls.push_back("reset");++resets;reset_image_animations(clocks);}
    void advance_image_clocks()override{calls.push_back("advance");++advances;advance_image_animations(clocks);if(on_advance)on_advance();}
    bool config_debug_controls()override{++config_reads;return debug;}
    bool key_state(std::uint32_t mask)override{check(mask==0x10);return (held&mask)!=0;}
    bool key_pulse(std::uint32_t mask)override{return (pulses&mask)!=0;}
    bool engine_key_down(std::int32_t value)override{engine_queries.push_back(value);return keys.count(value)!=0;}
    void debug_show_result()override{calls.push_back("result");if(on_result)on_result();}
    void debug_complete_quest()override{calls.push_back("complete");}
    void queue_debug_return()override{calls.push_back("queue");}
    void push_debug_defeat(std::int32_t reason)override{check(reason==1);calls.push_back("defeat");}
    void push_debug_victory(std::int32_t enemy,bool complete)override{check(enemy==65&&complete);calls.push_back("victory");}
    bool active()override{++active_reads;return is_active;}
    bool is_top_form()override{++top_reads;return top;}
    void update_world(std::int32_t pass)override{check(!state.dialog);passes.push_back(pass);calls.push_back("world");if(on_world)on_world(pass);}
    void change_state(std::int32_t value)override{check(value==7&&state.fade_type==2&&fade==0);calls.push_back("change");if(on_change)on_change();}
};
int main(){
    {Host h;h.state.dialog=std::make_shared<int>(9);check(h.run());
        check(h.calls==std::vector<std::string>{"base","flag","off","reset","advance","world"});
        check(h.state.loop_counter==1&&h.state.frame==1&&h.fade==1&&h.state.pinch_frame==1);
        check(h.passes==std::vector<int>{0}&&h.config_reads==5&&h.active_reads==0&&h.engine_queries.empty());
        check(h.clocks.at(0)->at(0)->at(1)==1);check(h.run()&&h.resets==1&&h.advances==2);}
    {Host h;h.debug=true;h.held=0x10;check(h.run());check(h.passes==std::vector<int>{0,1,2});
        check(h.fade==1&&h.state.frame==1&&h.active_reads==2&&h.top_reads==2&&h.advances==1);}
    {Host h;h.debug=true;h.held=0x10;h.is_active=false;check(h.run());
        check(h.passes==std::vector<int>{0}&&h.active_reads==2&&h.top_reads==0&&h.fade==1);}
    {Host h;h.debug=true;h.held=0x10;h.on_world=[&](int){h.top=false;};check(!h.run());
        check(h.passes==std::vector<int>{0}&&h.state.frame==1&&h.fade==0&&h.state.pinch_frame==0);}
    {Host h;h.debug=true;h.pulses=0x1e0;check(h.run());
        check(h.calls==std::vector<std::string>{"base","flag","off","reset","advance","result","complete","queue","defeat","victory","world"});
        check(h.engine_queries.empty());}
    {Host h;h.debug=true;h.held=0x10;h.keys={0x35};h.on_result=[&]{h.debug=false;};check(h.run());
        check(h.passes==std::vector<int>{0,1,2}&&h.engine_queries==std::vector<int>{0x35}&&h.config_reads==5);}
    {Host h;h.fade=20;check(!h.run());check(h.fade==0&&h.state.fade_type==-1);check(h.run());check(h.fade==0);}
    {Host h;h.fade=20;h.state.fade_type=1;h.state.fade_state=7;h.on_change=[&]{h.state.fade_state=100;};
        check(h.run());check(h.state.fade_type==2&&h.state.fade_state==-1&&h.fade==0);h.fade=20;check(!h.run()&&h.state.fade_type==-1);}
    {Host h;h.fade=20;h.state.fade_type=1;h.state.fade_state=7;h.on_change=[] {throw std::runtime_error("layout");};bool failed=false;
        try{h.run();}catch(const std::runtime_error&){failed=true;}check(failed&&h.state.fade_type==2&&h.state.fade_state==7&&h.fade==0);}
    {Host h;h.state.loop_counter=std::numeric_limits<int>::max();h.fade=std::numeric_limits<int>::max();check(h.run());
        check(h.state.loop_counter==-1&&h.state.frame==-1&&h.fade==std::numeric_limits<int>::min());
        h.state.pinching=true;h.state.pinch_frame=39;check(h.run()&&h.state.pinch_frame==39);h.state.pinching=false;
        check(h.run()&&h.state.pinch_frame==40);check(h.run()&&h.state.pinch_frame==40);}
    {Host h;h.state.dialog=std::make_shared<int>(1);h.on_advance=[] {throw std::runtime_error("clock");};bool failed=false;
        try{h.run();}catch(const std::runtime_error&){failed=true;}
        check(failed&&h.flags==0&&h.state.dialog&&h.state.loop_counter==0&&h.clocks.at(0)->at(0)->at(1)==1);}
}
