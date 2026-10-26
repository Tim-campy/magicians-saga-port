#include "core/gameplay/select_triggers.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool b) {if(!b){std::cerr<<"trigger contract failure\n";std::exit(1);}}
template<class F> void fails(F f) {try{f();check(false);}catch(const std::exception&) {}}
struct Host:SelectTriggerHost,EventExecutionHost {
    std::int64_t energy=0;
    std::uint32_t flags=0;
    int queries=0,save_reads=0;
    bool set_system_flag=false,replace=false;
    std::vector<int> log;
    std::shared_ptr<EventCatalog> catalog=std::make_shared<EventCatalog>(1134);
    std::shared_ptr<EventIntegers> save=std::make_shared<EventIntegers>(1,0),replacement;
    std::int64_t stamina() override {log.push_back(1);return energy;}
    std::uint32_t system_flags() override {log.push_back(2);return flags;}
    std::shared_ptr<EventIntegers> save_integers() override {log.push_back(3);++save_reads;return save_reads==2 && replacement ? replacement : save;}
    std::shared_ptr<EventCatalog> events() override {++queries;if(replace && queries==2)catalog=std::make_shared<EventCatalog>();return catalog;}
    EventObject default_event_args() override {return nullptr;}
    void empty_event_diagnostic(std::int32_t id) override {log.push_back(id);}
    EventObject dispatch(const EventInstruction& row,const EventContext&) override {log.push_back(row->at(0));if(set_system_flag)flags=0x2000;return std::make_shared<int>(1);}
    std::int32_t identify_script(const EventScript&) override {return -1;}
    EventScript resolve_script(std::int32_t) override {throw std::logic_error("unexpected delay");}
    std::int32_t read_talk_character() override {return 0;}
    void restore_talk_character(std::size_t,std::int32_t) override {}
    void event(int id,int counter=0) {
        auto r=std::make_shared<EventConditionRecord>();r->execution_count=counter;
        r->effects=std::make_shared<std::vector<EventInstruction>>();
        r->effects->push_back(std::make_shared<EventIntegers>(EventIntegers{id}));
        catalog->at(static_cast<std::size_t>(id))=r;
    }
};
int main() {
    EventDelayQueue queue;
    {
        Host h;h.energy=0x100000001LL;h.event(1118,-1);h.event(1133);h.set_system_flag=true;
        process_select_triggers(h,queue,h);
        check(h.log==std::vector<int>({1,1118,3,3,2,1133}));
        check(h.save->at(0)==128 && h.catalog->at(1118)->execution_count==0 && h.catalog->at(1133)->execution_count==1);
        // Both returned forms are discarded; first effect changes the second gate.
    }
    {
        Host h;h.energy=1;h.catalog->resize(229);h.flags=0x2000;
        process_select_triggers(h,queue,h);
        check(h.save->at(0)==128 && h.log==std::vector<int>({1,3,3,2})); // Out-of-range events do not suppress flag write.
    }
    {
        Host h;h.energy=1;h.event(1118,1);h.flags=0;h.save.reset();
        process_select_triggers(h,queue,h);check(h.save_reads==0 && h.log==std::vector<int>({1,2}));
        h.energy=2;h.catalog.reset();process_select_triggers(h,queue,h); // Neither gate consults missing catalog.
    }
    {
        Host h;h.energy=1;h.catalog->clear();h.replacement=std::make_shared<EventIntegers>(1,-2147483648);
        process_select_triggers(h,queue,h);check(h.save->at(0)==0 && h.replacement->at(0)==-2147483520);
        h.save_reads=0;h.save->at(0)=128;process_select_triggers(h,queue,h);check(h.save_reads==1);
    }
    {
        Host h;check(!event_already_executed(-1,h) && h.queries==0);
        fails([&]{event_already_executed(0,h);}); // In-range null is an error.
        h.queries=0;h.replace=true;fails([&]{event_already_executed(0,h);}); // Fresh catalog can shrink.
    }
    {
        Host h;h.energy=1;h.catalog->clear();h.save.reset();
        fails([&]{process_select_triggers(h,queue,h);});check(h.log==std::vector<int>({1,3})); // Stops before system flag read.
    }
}
