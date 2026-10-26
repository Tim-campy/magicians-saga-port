#include "core/gameplay/select_guide.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool value){if(!value){std::cerr<<"guide contract failure\n";std::exit(1);}}
template<class F> void fails(F f){try{f();check(false);}catch(const std::exception&) {}}
struct Host:SelectGuideHost {
    std::int32_t challenges=0;
    std::shared_ptr<BuildingSlots> slots=std::make_shared<BuildingSlots>();
    std::shared_ptr<GuideTowns> records=std::make_shared<GuideTowns>();
    std::vector<int> calls;std::function<void()> sum_hook,enable_hook;
    std::int32_t sum_quest_challenges() override {calls.push_back(1);if(sum_hook)sum_hook();return challenges;}
    std::shared_ptr<BuildingSlots> building_slots() override {calls.push_back(2);return slots;}
    bool can_build(std::int32_t id) override {calls.push_back(100+id);if(enable_hook)enable_hook();return id==7;}
    std::shared_ptr<GuideTowns> towns() override {calls.push_back(3);return records;}
    void row(std::int32_t id,std::int32_t building){slots->push_back(std::make_shared<EventIntegers>(EventIntegers{id,building}));}
};
int main(){
    {Host h;SelectGuideState s;fails([&]{check_select_guide(s,false,h);});check(h.calls.empty());}
    {Host h;SelectGuideState s;h.row(7,-1);h.row(8,4);h.records->push_back(std::make_shared<GuideTownRecord>(GuideTownRecord{1,4}));check_select_guide(s,true,h);check(s.flags->at(0)==1 && s.flags->at(1)==1);check((h.calls==std::vector<int>{1,2,107,3,2}));}
    {Host h;SelectGuideState s;h.challenges=1;check_select_guide(s,true,h);check(s.flags->at(0)==0 && s.flags->at(1)==0);h.calls.clear();h.challenges=0;h.row(7,-1);check_select_guide(s,false,h);check(h.calls.empty());check_select_guide(s,true,h);check(s.flags->at(0)==1 && s.flags->at(1)==1);}
    {Host h;SelectGuideState s;h.row(8,4);h.slots->push_back(std::make_shared<EventIntegers>());fails([&]{check_select_guide(s,true,h);});check(s.flags->at(0)==1 && s.flags->at(1)==0);}
    {Host h;SelectGuideState s;h.slots->push_back(nullptr);h.records->push_back(nullptr);check_select_guide(s,true,h);check((h.calls==std::vector<int>{1,2,3,2}));} // Still enumerate towns/occupied with no available slot.
    {Host h;auto captured=std::make_shared<std::vector<std::uint8_t>>(2,1);SelectGuideState s{captured};h.sum_hook=[&]{s.flags=std::make_shared<std::vector<std::uint8_t>>(2,0);};check_select_guide(s,false,h);check(captured->at(0)==1 && s.flags->at(0)==0 && h.calls.size()==1);}
    {Host h;SelectGuideState s;h.row(7,-1);auto fresh=std::make_shared<BuildingSlots>();fresh->push_back(std::make_shared<EventIntegers>(EventIntegers{8,9}));h.enable_hook=[&]{h.slots=fresh;};h.records->push_back(std::make_shared<GuideTownRecord>(GuideTownRecord{1,9}));check_select_guide(s,true,h);check(s.flags->at(1)==1);} // Second building query observes replacement.
    check(!guide_active_town(std::make_shared<GuideTownRecord>(GuideTownRecord{0,4})));
    check(!guide_active_town(std::make_shared<GuideTownRecord>(GuideTownRecord{1,-1})));
    check(guide_active_town(std::make_shared<GuideTownRecord>(GuideTownRecord{3,0})));
}
