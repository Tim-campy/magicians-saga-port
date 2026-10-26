#include "core/gameplay/select_event_pass.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool v,const char* m) {if(!v) {std::cerr<<m;std::exit(1);}}
struct Host : EventExecutionHost {
    std::shared_ptr<EventCatalog> data;
    DecodedEventConditionState decoded;
    EventObject form=std::make_shared<int>(1);
    int queries=0, effects=0;
    std::shared_ptr<EventCatalog> events() override {++queries;return data;}
    EventObject default_event_args() override {return nullptr;}
    void empty_event_diagnostic(std::int32_t) override {}
    EventObject dispatch(const EventInstruction& row,const EventContext&) override {
        ++effects;
        if(row->at(0)==1) decoded.money=decoded.money.value()+row->at(1);
        return row->at(0)==8 ? form : nullptr;
    }
    std::int32_t identify_script(const EventScript& script) override {return identify_event_script(*data,script,[]{throw std::logic_error("unknown script");});}
    EventScript resolve_script(std::int32_t id) override {return resolve_event_script(*data,id);}
    std::int32_t read_talk_character() override {return 0;}
    void restore_talk_character(std::size_t,std::int32_t) override {}
};
int main() {
    const auto max=std::numeric_limits<std::int32_t>::max();
    check(advance_select_loop_counter(0)==1 && advance_select_loop_counter(299)==300,"counter advance");
    check(advance_select_loop_counter(max-1)==0 && advance_select_loop_counter(max)==-1 && advance_select_loop_counter(-1)==0,"counter wrap and signed remainder");
    Host h;h.decoded.context_present=true;h.decoded.context_character_count=1;h.decoded.money=0;
    DecodedEventConditionHost conditions(h.decoded);
    h.data=std::make_shared<EventCatalog>(load_event_catalog("0\tdelay\t0\t0\t1\t2\t1\t5\n1\tform\t0\t1\t3\t1\t5\t4\t1\t2\t8\t1",false,parse_event_decimal));
    EventDelayQueue queue;
    SelectEventPassState state{299,false};
    process_select_event_pass(state,queue,h,conditions);
    check(h.queries==0 && !state.event_check_requested,"no selection request skips scan");
    queue.push_back(std::make_shared<DelayedEvent>(DelayedEvent{0,0,1,-1,0,{}}));
    state.loop_counter=300;
    check(process_select_event_pass(state,queue,h,conditions)==h.form,"queue effect changes later eligibility");
    check(queue.empty() && h.decoded.money==5 && state.event_check_requested,"form result retains request");
    state.loop_counter=301;
    check(!process_select_event_pass(state,queue,h,conditions) && !state.event_check_requested,"null scan result clears retained request");
    for(auto n:{0,-300,600}) {
        h.queries=0;state={n,false};process_select_event_pass(state,queue,h,conditions);
        check(h.queries>0,"periodic signed counter requests scan");
    }
    state={302,true};h.data.reset();
    try {process_select_event_pass(state,queue,h,conditions);check(false,"expected catalog failure");}
    catch(const std::invalid_argument&) {}
    check(state.event_check_requested,"failure does not clear request");
}
