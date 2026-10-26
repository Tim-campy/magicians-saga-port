#include "core/gameplay/event_conditions.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool value,const char* message) {
    if (!value) { std::cerr<<message<<'\n'; std::exit(1); }
}
EventInstruction row(EventIntegers values) { return std::make_shared<EventIntegers>(values); }
template<class F> void fails(F f) {
    try { f(); } catch (const std::exception&) { return; }
    check(false,"expected condition failure");
}
struct Host:EventConditionHost {
    ConditionCatalog<QuestConditionState> q;
    ConditionCatalog<std::int32_t> a;
    ConditionCatalog<CharacterConditionState> c;
    EventScript b=std::make_shared<std::vector<EventInstruction>>();
    std::vector<int> calls;
    void require_condition_context() override { calls.push_back(0); }
    const ConditionCatalog<QuestConditionState>* quests() override { calls.push_back(15);return &q; }
    const ConditionCatalog<std::int32_t>* area_states() override { calls.push_back(4);return &a; }
    const ConditionCatalog<CharacterConditionState>* characters() override { calls.push_back(22);return &c; }
    EventScript buildings() override { calls.push_back(20);return b; }
    bool evaluate_other_condition(const EventInstruction&) override { throw std::logic_error("unrecovered"); }
};
int main() {
    Host h;
    auto terms=std::make_shared<std::vector<EventInstruction>>();
    check(!check_event_conditions(nullptr,h) && !check_event_conditions(terms,h) && h.calls.empty(),"empty is false without host reads");
    for (int code=0;code<6;++code) {
        const bool equal[]={false,true,true,false,true,false};
        const bool less[]={false,false,true,true,false,true};
        const bool greater[]={true,true,false,false,false,true};
        check(compare_event_values(5,5,code)==equal[code],"comparison equality boundary");
        check(compare_event_values(-5,5,code)==less[code],"comparison less than");
        check(compare_event_values(5,-5,code)==greater[code],"comparison greater than");
    }
    check(compare_event_values(std::numeric_limits<std::int64_t>::min(),0,3) &&
          compare_event_values(std::numeric_limits<std::int64_t>::max(),0,0) &&
          !compare_event_values(1,0,-1) && !compare_event_values(1,0,6),"signed64 and invalid codes");
    h.q={nullptr,std::make_shared<QuestConditionState>(QuestConditionState{2,false,3}),
        std::make_shared<QuestConditionState>(QuestConditionState{0,true,4})};
    check(check_event_condition(row({15,1}),h) && check_event_condition(row({15,2}),h),"either quest completion path");
    h.q[2]->completed=false;
    check(!check_event_condition(row({15,2}),h),"quest without completion flags");
    check(check_event_condition(row({16,7,4}),h),"challenge array lengths summed, null quest skipped");
    h.q[1]->challenge_count=std::numeric_limits<std::int32_t>::max();
    check(check_event_condition(row({16,0,3}),h),"sum wraps32 before comparison");
    h.q[1]->challenge_count.reset(); fails([&]{check_event_condition(row({16}),h);});
    h.a={std::make_shared<std::int32_t>(2)};
    *terms={row({4,0}),nullptr}; h.calls.clear();
    check(!check_event_conditions(terms,h) && h.calls==std::vector<int>({0,4}),"AND short circuits before null row");
    *h.a[0]=1;fails([&]{check_event_conditions(terms,h);});
    *terms={row({4,0})};
    ConditionCatalog<EventConditionRecord> records{nullptr,std::make_shared<EventConditionRecord>(EventConditionRecord{-1,terms})};
    check(check_event_eligibility(&records,1,h) && !check_event_eligibility(&records,-1,h) &&
          !check_event_eligibility(&records,2,h) &&
          !check_event_eligibility(&records,0,h) && !check_event_eligibility(nullptr,0,h),"record guards, negative count allowed");
    records[1]->execution_count=1;h.calls.clear();
    check(!check_event_eligibility(&records,1,h) && h.calls.empty(),"positive count skips conditions");
    h.c={nullptr,std::make_shared<CharacterConditionState>(CharacterConditionState{-1,4}),
         std::make_shared<CharacterConditionState>(CharacterConditionState{0,4}),
         std::make_shared<CharacterConditionState>(CharacterConditionState{1,0})};
    check(check_event_condition(row({22,1,4}),h),"only nonzero-sub recruited characters counted");
    check(!check_event_condition(row({20}),h),"empty building collection never reads missing type argument");
    *h.b={row({0,7}),row({1,8})};
    check(check_event_condition(row({20,7}),h),"matching building type");
    h.b->push_back(nullptr);fails([&]{check_event_condition(row({20,7}),h);});
    h.calls.clear();fails([&]{check_event_condition(row({15}),h);});
    check(h.calls==std::vector<int>({0,15}),"quest getter precedes missing id failure");
    fails([&]{check_event_condition(row({999}),h);});
}
