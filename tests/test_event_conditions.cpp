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
    std::int64_t value=0;
    std::int32_t bonus=0,capacity=0;
    ConditionCatalog<std::uint32_t> towns;
    std::int64_t scalar(ConditionScalar k) override {calls.push_back(100+static_cast<int>(k));return value;}
    std::int32_t rescue_adjustment() override {calls.push_back(7);return bonus;}
    const ConditionCatalog<std::uint32_t>* town_flags() override {calls.push_back(21);return &towns;}
    std::int32_t town_capacity() override {calls.push_back(19);return capacity;}
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
    h.q={nullptr,std::make_shared<QuestConditionState>(QuestConditionState{2,false,QuestConditionRecord{3,2,5}}),
        std::make_shared<QuestConditionState>(QuestConditionState{0,true,QuestConditionRecord{4,1,6}})};
    check(check_event_condition(row({15,1}),h) && check_event_condition(row({15,2}),h),"either quest completion path");
    h.q[2]->completed=false;
    check(!check_event_condition(row({15,2}),h),"quest without completion flags");
    check(check_event_condition(row({16,7,4}),h),"challenge counters summed, null quest skipped");
    h.q[1]->record->challenges=std::numeric_limits<std::int32_t>::max();
    check(check_event_condition(row({16,0,3}),h),"sum wraps32 before comparison");
    h.q[1]->record->challenges=3;
    check(check_event_condition(row({18,3,4}),h),"clear count skips null quest");
    check(check_event_condition(row({19,4,4}),h),"loss is challenges minus clears");
    fails([&]{check_event_condition(row({7,11,4}),h);}); // Rescue does not skip gaps.
    h.q.erase(h.q.begin());h.bonus=-2;
    check(check_event_condition(row({7,9,4}),h),"rescue count plus signed adjustment");
    h.q.insert(h.q.begin(),nullptr);
    for (int opcode:{1,2,32}) {
        h.value=0x100000000LL;
        check(check_event_condition(row({opcode,0,0}),h),"64-bit scalar is not truncated");
        h.value=-1;
        check(check_event_condition(row({opcode,-1,4}),h),"threshold sign extension");
    }
    for (int opcode:{5,31}) {
        h.value=0xffffffffLL;
        check(check_event_condition(row({opcode,-1,4}),h),"int32 comparison uses low word");
    }
    h.towns={std::make_shared<std::uint32_t>(1),std::make_shared<std::uint32_t>(2)};
    h.capacity=3;h.calls.clear();
    check(check_event_condition(row({21,2,4}),h) && h.calls==std::vector<int>({0,21,19}),"capacity minus occupied flags and query order");
    h.capacity=std::numeric_limits<std::int32_t>::min();
    check(check_event_condition(row({21,std::numeric_limits<std::int32_t>::max(),4}),h),"capacity subtraction wraps32");
    h.towns.push_back(nullptr);h.calls.clear();
    fails([&]{check_event_condition(row({21,0,4}),h);});
    check(h.calls==std::vector<int>({0,21}),"null town fails before capacity read");
    h.q[1]->record->challenges=-5;
    check(check_event_condition(row({16,-1,4}),h),"challenge counter is signed, not an array length");
    h.q[1]->record->clears=std::numeric_limits<std::int32_t>::max();
    check(check_event_condition(row({18,std::numeric_limits<std::int32_t>::min(),4}),h),"clear total wraps32");
    h.value=0;h.calls.clear();
    fails([&]{check_event_condition(row({32}),h);});
    check(h.calls==std::vector<int>({0,104}),"high score queried before argument failure");
    h.calls.clear();
    check(!check_event_condition(row({31,0,6}),h) && h.calls==std::vector<int>({0,103}),"invalid comparison still reads stamina");
    h.q[1]->record.reset(); fails([&]{check_event_condition(row({16}),h);});
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
    DecodedEventConditionState state;
    DecodedEventConditionHost decoded(state);
    fails([&]{check_event_condition(row({1,0,4}),decoded);});
    state.context_present=true;state.context_character_count=0;
    fails([&]{check_event_condition(row({1,0,4}),decoded);});
    state.context_character_count=1;
    fails([&]{check_event_condition(row({1,0,4}),decoded);}); // Missing balance, not zero.
    state.money=42;state.crystals=7;state.hero_level=3;state.stamina=0xffffffffLL;state.high_score=99;
    for (auto condition: {row({1,42,4}),row({2,7,4}),row({5,3,4}),row({31,-1,4}),row({32,99,4})})
        check(check_event_condition(condition,decoded),"decoded scalar binding");
    state.quests=std::make_shared<ConditionCatalog<QuestConditionState>>();
    state.quests->push_back(std::make_shared<QuestConditionState>(QuestConditionState{0,false,QuestConditionRecord{4,2,3}}));
    fails([&]{check_event_condition(row({7,3,4}),decoded);}); // Debug flag absent.
    state.debug_enabled=false;
    check(check_event_condition(row({7,3,4}),decoded),"disabled debug does not require rescue bonus");
    state.debug_enabled=true;
    fails([&]{check_event_condition(row({7,3,4}),decoded);});
    state.debug_rescue_count=-1;
    check(check_event_condition(row({7,2,4}),decoded),"debug rescue adjustment");
    state.area_states=std::make_shared<ConditionCatalog<std::int32_t>>();
    state.area_states->push_back(std::make_shared<std::int32_t>(0));
    *terms={row({1,42,4}),row({4,0})};
    check(!check_event_conditions(terms,decoded),"live decoded state before discovery");
    *state.area_states->at(0)=1;
    check(check_event_conditions(terms,decoded),"live decoded state after discovery");
    fails([&]{check_event_condition(row({999}),decoded);});

}
