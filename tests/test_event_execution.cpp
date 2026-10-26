#include "core/gameplay/event_execution.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool v,const char* message) {if (!v) {std::cerr<<message;std::exit(1);}}
template<class F> void fails(F f) {try {f();}catch(const std::exception&) {return;}check(false,"expected failure");}
struct Host : EventExecutionHost {
    std::shared_ptr<EventCatalog> data=std::make_shared<EventCatalog>();
    DecodedEventConditionState state;
    EventObject fallback=std::make_shared<int>(42),seen,form=std::make_shared<int>(7);
    std::shared_ptr<EventIntegers> seen_argi;
    int defaults=0,diagnostics=0,dispatches=0;
    std::function<std::shared_ptr<EventCatalog>()> catalog_hook;
    std::function<void()> prepare_hook;
    std::function<void()> dispatch_hook;
    std::shared_ptr<EventCatalog> events() override {return catalog_hook ? catalog_hook() : data;}
    EventObject default_event_args() override {++defaults;if(prepare_hook)prepare_hook();return fallback;}
    void empty_event_diagnostic(std::int32_t) override {++diagnostics;}
    EventObject dispatch(const EventInstruction& row,const EventContext& c) override {
        ++dispatches;seen=c.args;seen_argi=c.argi;
        if (dispatch_hook) dispatch_hook();
        if (row->at(0)==1) state.money=state.money.value()+row->at(1);
        return row->at(0)==8 ? form : nullptr;
    }
    std::int32_t identify_script(const EventScript& s) override {return identify_event_script(*data,s,[]{throw std::logic_error("unknown script");});}
    EventScript resolve_script(std::int32_t id) override {return resolve_event_script(*data,id);}
    std::int32_t read_talk_character() override {return 2;}
    void restore_talk_character(std::size_t,std::int32_t) override {}
    void load(std::string_view text) {data=std::make_shared<EventCatalog>(load_event_catalog(text,false,parse_event_decimal));}
};
int main() {
    Host h;EventDelayQueue q;
    h.state.context_present=true;h.state.context_character_count=1;h.state.money=0;
    DecodedEventConditionHost conditions(h.state);
    h.load("0\tx\t0\t0\t0");
    check(!execute_indexed_event(-1,{},q,h) && !execute_indexed_event(3,{},q,h) && h.defaults==0,"bounds skip defaults");
    execute_indexed_event(0,{},q,h);
    check(h.defaults==1 && h.diagnostics==1 && h.data->at(0)->execution_count==1,"empty effects still counted");
    h.data->at(0)->execution_count=std::numeric_limits<std::int32_t>::max();
    execute_indexed_event(0,{},q,h);
    check(h.data->at(0)->execution_count==std::numeric_limits<std::int32_t>::min(),"counter wraps");
    h.load("0\tx\t0\t0\t1\t2\t1\t4");
    auto args=std::make_shared<int>(9);auto argi=std::make_shared<EventIntegers>(EventIntegers{5});
    const auto defaults=h.defaults;
    execute_indexed_event(0,{args,argi},q,h);
    check(h.seen==args && h.seen_argi==argi && h.defaults==defaults && h.state.money==4,"direct execution bypasses empty terms and keeps context");
    h.prepare_hook=[&]{h.load("0\treplacement\t0\t0\t1\t2\t1\t3");};
    execute_indexed_event(0,{},q,h);h.prepare_hook={};
    check(h.state.money==7 && h.data->at(0)->execution_count==1 && h.seen==h.fallback,"context callback can replace catalog");
    h.prepare_hook=[] {throw std::runtime_error("missing default context");};
    fails([&]{execute_indexed_event(0,{},q,h);});h.prepare_hook={};
    check(h.data->at(0)->execution_count==1,"context failure precedes increment");
    h.state.money=0;
    h.load("0\ta\t0\t1\t3\t1\t0\t4\t1\t2\t1\t5\n2\tb\t0\t1\t3\t1\t5\t4\t1\t2\t8\t9\n3\tc\t0\t1\t3\t1\t5\t4\t1\t2\t1\t99");
    check(select_and_execute_events(q,h,conditions)==h.form,"scan continues after null result and stops at form");
    check(h.state.money==5 && h.data->at(0)->execution_count==1 && h.data->at(2)->execution_count==1 && h.data->at(3)->execution_count==0,"later eligibility sees previous effects, gap skipped");
    h.load("0\ta\t0\t1\t3\t1\t5\t4\t1\t2\t10\t1\n1\tb\t0\t1\t3\t1\t5\t4\t1\t2\t1\t2");
    select_and_execute_events(q,h,conditions);
    check(q.size()==1 && h.state.money==7,"scheduled delay does not stop selection");
    h.state.money=0;
    h.load("0\ta\t0\t1\t3\t1\t0\t4\t1\t2\t1\t0");
    h.dispatch_hook=[&]{h.load("0\tx\t0\t0\t0\n1\tb\t0\t1\t3\t1\t0\t4\t1\t2\t8\t9");};
    check(select_and_execute_events(q,h,conditions)==h.form,"scan observes replacement length");
    h.dispatch_hook={};
    auto first=std::make_shared<EventCatalog>(load_event_catalog("0\tx\t0\t0\t1\t2\t1\t3",false,parse_event_decimal));
    auto second=std::make_shared<EventCatalog>(load_event_catalog("0\tx\t0\t0\t0",false,parse_event_decimal));
    auto third=std::make_shared<EventCatalog>(load_event_catalog("0\tx\t0\t0\t0",false,parse_event_decimal));
    third->at(0)->execution_count=9;
    int reads=0;
    h.catalog_hook=[&]{++reads;return reads<=2 ? first : reads==3 ? second : third;};
    execute_indexed_event(0,{args,{}},q,h);
    check(reads==4 && second->at(0)->execution_count==10 && first->at(0)->execution_count==0 && third->at(0)->execution_count==9,"distinct script, counter target and counter source reads");
    h.catalog_hook={};h.data=first;
    h.dispatch_hook=[] {throw std::runtime_error("effect failure");};
    fails([&]{execute_indexed_event(0,{args,{}},q,h);});
    check(first->at(0)->execution_count==1,"effect failure does not roll back counter");
    h.dispatch_hook={};h.data.reset();
    fails([&]{select_and_execute_events(q,h,conditions);});
    check(!execute_indexed_event(-1,{},q,h),"negative id does not read null catalog");
}
