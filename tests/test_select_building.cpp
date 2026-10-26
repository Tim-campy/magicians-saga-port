#include "core/gameplay/select_building.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace saga::core;
void check(bool b) {if(!b){std::cerr<<"building contract failure\n";std::exit(1);}}
template<class F> void fails(F f) {try{f();check(false);}catch(const std::exception&) {}}
struct Host:SelectBuildingHost,FormStackHost {
    std::int32_t camera=12;float drag=4.9F;
    SelectBuildingState state{0,camera,13,drag,9,60};
    SelectFormPassState form{};
    bool button=true,enabled=true,push_failure=false;
    std::vector<int> log;
    int catalog_reads=0,change_kind=0,share_value=0;
    EventObject child=std::make_shared<int>(3);
    std::shared_ptr<BuildingSlots> slots=std::make_shared<BuildingSlots>();
    std::shared_ptr<EventIntegers> save=std::make_shared<EventIntegers>(13,0);
    std::shared_ptr<BuildingMenuCatalog> data=std::make_shared<BuildingMenuCatalog>();
    std::shared_ptr<std::vector<EventObject>> members=std::make_shared<std::vector<EventObject>>(1);
    Host() {form.form.manager=this;slots->push_back(std::make_shared<EventIntegers>(EventIntegers{0,0}));data->push_back(std::make_shared<BuildingMenuRecord>(BuildingMenuRecord{1}));}
    bool run() {return process_select_building_button(state,form,*this);}
    bool pressed(std::uint32_t mask) override {check(mask==0x08000000U);log.push_back(1);return button;}
    void play_button_sound() override {log.push_back(2);}
    std::shared_ptr<BuildingSlots> building_slots() override {log.push_back(3);return slots;}
    std::shared_ptr<EventIntegers> save_integers() override {log.push_back(4);return save;}
    std::shared_ptr<BuildingMenuCatalog> buildings() override {++catalog_reads;if(change_kind && catalog_reads==2)data->at(0)->kind=change_kind;return data;}
    bool can_build(std::int32_t i) override {check(i==0);log.push_back(5);return enabled;}
    std::shared_ptr<std::vector<EventObject>> living_members() override {return members;}
    std::int32_t system_share_value() override {return share_value;}
    EventObject make_shop(std::int32_t type) override {log.push_back(100+type);return child;}
    void set_shop_selection(EventObject shop,std::int32_t i) override {check(shop==child && i==0);log.push_back(6);}
    EventObject make_build_list(std::int32_t i) override {check(i==0);log.push_back(7);return child;}
    EventObject make_human_house(EventObject m,std::int32_t i,bool option) override {check(m==members->at(0) && i==0 && !option);log.push_back(8);return child;}
    EventObject make_subform(std::int32_t type) override {check(type==15);log.push_back(9);return child;}
    EventObject make_share_window(bool first) override {check(first==(share_value==0));log.push_back(10);return child;}
    void push(EventObject,EventObject value,bool option) override {check(value==child && !option && form.auto_save && form.events.event_check_requested);log.push_back(11);if(push_failure)throw std::runtime_error("manager");}
};
int main() {
    for(auto kind:{1,2,3,4,5,9,10})check(building_is_shop(kind));
    for(auto kind:{-1,0,6,7,8,11,12,std::numeric_limits<int>::max()})check(!building_is_shop(kind));
    {Host h;h.button=false;check(!h.run() && h.log==std::vector<int>({1}));}
    {Host h;h.state.selected_chip=-1;h.slots.reset();check(!h.run() && h.log==std::vector<int>({1,2,3}));}
    {Host h;h.state.selected_chip=1;check(!h.run() && h.state.selected_chip==1 && h.save->at(11)==0);}
    {Host h;h.change_kind=10;check(h.run());check(h.state.selected_chip==-1 && h.catalog_reads==2 && h.log==std::vector<int>({1,2,3,4,4,4,110,11}));check(h.save->at(11)==12 && h.save->at(12)==13 && h.save->at(10)==4);}
    {Host h;h.slots->at(0).reset();check(h.run() && h.state.selected_chip==-1 && !h.form.auto_save);}
    {Host h;h.slots->at(0)->at(1)=-1;h.enabled=false;check(h.run() && h.state.unlock_info_frames==60 && !h.form.auto_save);}
    {Host h;h.slots->at(0)->at(1)=-1;h.push_failure=true;fails([&]{h.run();});check(h.state.selected_chip==0 && h.state.unlock_info_number==-1 && h.state.unlock_info_frames==-1 && h.form.auto_save);}
    {Host h;h.data->at(0)->kind=6;check(h.run() && h.log.at(h.log.size()-3)==106 && h.log.at(h.log.size()-2)==6);}
    {Host h;h.data->at(0)->kind=6;h.members->at(0)=std::make_shared<int>(1);check(h.run() && h.log.at(h.log.size()-2)==8);}
    {Host h;h.data->at(0)->kind=6;h.child.reset();fails([&]{h.run();});check(h.state.selected_chip==0 && !h.form.auto_save);}
    for(auto kind:{7,12,99}) {Host h;h.data->at(0)->kind=kind;check(h.run() && h.state.selected_chip==-1);check(h.form.auto_save==(kind!=99));check(h.catalog_reads==(kind==7 ? 3:4));}
    {Host h;h.save->resize(12);fails([&]{h.run();});check(h.save->at(11)==12 && h.save->at(10)==0 && h.state.selected_chip==0);}
    {Host h;h.drag=std::numeric_limits<float>::infinity();save_select_position(h.state,h);check(h.save->at(10)==std::numeric_limits<int>::min());h.drag=2147483648.0F;save_select_position(h.state,h);check(h.save->at(10)==std::numeric_limits<int>::max());h.drag=std::numeric_limits<float>::quiet_NaN();save_select_position(h.state,h);check(h.save->at(10)==0);}
}
