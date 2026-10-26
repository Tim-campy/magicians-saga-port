#include "core/gameplay/select_markers.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool v,const char* m) {if(!v) {std::cerr<<m;std::exit(1);}}
template<class F> void fails(F f) {try{f();}catch(const std::exception&){return;}check(false,"expected failure");}
struct Host:SelectMarkerHost {
    bool top=true,debug=false;int top_calls=0,debug_calls=0,quest_calls=0;
    std::shared_ptr<ConditionCatalog<AreaMarkerRecord>> a=std::make_shared<ConditionCatalog<AreaMarkerRecord>>();
    ConditionCatalog<AreaQuestState> q;
    std::shared_ptr<ConditionCatalog<EquipmentMarkerState>> items=std::make_shared<ConditionCatalog<EquipmentMarkerState>>();
    std::vector<EquipmentMarkerKind> queried;
    bool fail_cloth=false;
    bool is_top_form() override {++top_calls;return top;}
    bool debug_enabled() override {++debug_calls;return debug;}
    std::shared_ptr<ConditionCatalog<AreaMarkerRecord>> areas() override {return a;}
    std::shared_ptr<AreaQuestState> quest(std::int32_t id) override {++quest_calls;return q.at(static_cast<std::size_t>(id));}
    std::shared_ptr<ConditionCatalog<EquipmentMarkerState>> equipment(EquipmentMarkerKind k) override {
        queried.push_back(k);if(fail_cloth && k==EquipmentMarkerKind::Cloth)return nullptr;return items;
    }
};
int main() {
    Host h;AreaMarkerRecord area{std::make_shared<EventIntegers>()};
    check(area_all_completed(area,h) && h.debug_calls==0,"empty quest list is complete");
    *area.quest_ids={-1};check(area_all_completed(area,h) && h.debug_calls==0,"sentinel skips all queries");
    h.q={nullptr};*area.quest_ids={0};
    fails([&]{area_all_completed(area,h);});
    h.debug=true;check(area_all_completed(area,h),"DEBUG null quest skipped");
    h.q[0]=std::make_shared<AreaQuestState>(AreaQuestState{1,0,false});
    check(!area_all_completed(area,h),"found incomplete quest fails");
    h.q[0]->state=0;check(area_all_completed(area,h),"undiscovered quest ignored");
    h.q[0]->state=1;h.q[0]->completed=true;check(area_all_completed(area,h),"completed boolean");
    h.q[0]->completed=false;h.q[0]->flags=2;h.debug=false;h.quest_calls=0;
    check(area_all_completed(area,h) && h.quest_calls==1,"completion bit bypasses remaining queries outside DEBUG");
    h.q[0]->flags=0;
    h.a->push_back(nullptr);h.a->push_back(std::make_shared<AreaMarkerRecord>(area));
    h.a->push_back(std::make_shared<AreaMarkerRecord>(AreaMarkerRecord{std::make_shared<EventIntegers>()}));
    SelectMarkerState state{false,true,nullptr};
    check(!refresh_select_markers(state,h) && h.top_calls==0,"resume flag precedes top query");
    state.scene_resume=true;h.top=false;
    check(!refresh_select_markers(state,h) && state.scene_resume && !state.area_marks,"non-top keeps dirty flag");h.top=true;
    h.items->push_back(std::make_shared<EquipmentMarkerState>(EquipmentMarkerState{1,4,true}));
    check(refresh_select_markers(state,h) && *state.area_marks==EventIntegers({0,1,2}),"null and completed area markers");
    check(state.weapon_new && !state.scene_resume && h.queried.size()==3,"all equipment catalogs visited; hats ignore equipment bit");
    check(!equipment_marker(h.items->at(0),EquipmentMarkerKind::Equipment),"equipment exclusion bit");
    h.items->at(0)->quantity=-1;
    check(!equipment_marker(h.items->at(0),EquipmentMarkerKind::Hat),"positive stock required");
    h.items->at(0)->quantity=1;h.items->at(0)->flags=0;
    state.scene_resume=true;state.weapon_new=false;h.fail_cloth=true;
    fails([&]{refresh_select_markers(state,h);});
    check(state.scene_resume && !state.weapon_new && state.area_marks->at(2)==2,"later failure retains refreshed areas but old flag and dirty state");
    area.quest_ids.reset();fails([&]{area_all_completed(area,h);});
}
