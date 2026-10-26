#include "core/gameplay/select_town_buttons.hpp"
#include "core/gameplay/event_progression.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool b) {if(!b){std::cerr<<"town buttons contract failure\n";std::exit(1);}}
template<class F> void fails(F f) {try{f();check(false);}catch(const std::exception&) {}}
struct Host:SelectBuildingHost,SelectTownButtonsHost,FormStackHost {
    std::uint32_t buttons=0;std::vector<std::uint32_t> log;
    std::int32_t camera=10;float drag=7.9F;
    SelectBuildingState building{2,camera,20,drag,8,60};
    SelectTownButtonsState state{4,12};
    SelectFormPassState form{};
    TownBubbleState bubble{0,std::make_shared<const std::string>("old"),10};
    EventProgression progress{};
    std::shared_ptr<EventIntegers> save=std::make_shared<EventIntegers>(13,0),system=std::make_shared<EventIntegers>(5,0);
    std::shared_ptr<TownHumanRows> rows=std::make_shared<TownHumanRows>();
    std::shared_ptr<TownVoices> voice=std::make_shared<TownVoices>();
    std::shared_ptr<TownSpeakers> speaker=std::make_shared<TownSpeakers>();
    EventObject child=std::make_shared<int>(1);
    bool fail_area=false,fail_push=false,fail_sum=false,mutate_present=false;
    Host() {
        form.form.manager=this;
        rows->push_back(std::make_shared<EventIntegers>(11,0));
        voice->push_back(std::make_shared<TownVoice>(TownVoice{0,std::make_shared<std::vector<FormText>>(1,std::make_shared<const std::string>("hello"))}));
        speaker->push_back(std::make_shared<TownSpeaker>(TownSpeaker{0,0,0}));
    }
    bool run() {return process_select_town_buttons(state,building,form,bubble,*this,*this);}
    bool pressed(std::uint32_t m) override {log.push_back(m);return (buttons&m)!=0;}
    void play_button_sound() override {log.push_back(1);}
    std::shared_ptr<EventIntegers> save_integers() override {log.push_back(2);return save;}
    EventObject area_form() override {log.push_back(3);check(state.selected_area==-1);if(fail_area)throw std::runtime_error("AppData");return child;}
    EventObject make_obstacle(std::int32_t i) override {log.push_back(4);check(i==2 && building.unlock_info_number==-1 && building.unlock_info_frames==-1);return child;}
    EventObject make_store_invite() override {log.push_back(5);return child;}
    std::int32_t sum_quest_challenges() override {log.push_back(6);if(fail_sum)throw std::runtime_error("quests");return 31;}
    std::shared_ptr<EventIntegers> system_save_integers() override {log.push_back(7);return system;}
    std::int32_t store_limit_slot() override {log.push_back(8);return 3;}
    void add_money(std::int32_t n) override {log.push_back(9);check(rows->at(0)->at(8)==0);apply_event_progression({1,n},progress);if(mutate_present)state.resident_present_value=99;}
    FormText format_present_value(std::int32_t n) override {log.push_back(10);return std::make_shared<const std::string>(std::to_string(n));}
    EventObject make_present_popup(FormText s) override {log.push_back(11);check(*s==std::to_string(state.resident_present_value));return child;}
    void push(EventObject,EventObject c,bool option) override {log.push_back(12);check(c==child && !option && form.auto_save && form.events.event_check_requested);if(fail_push)throw std::runtime_error("manager");}
    std::shared_ptr<TownHumanRows> town_humans() override {log.push_back(13);return rows;}
    std::shared_ptr<TownVoices> voices() override {log.push_back(14);return voice;}
    std::shared_ptr<TownSpeakers> speakers() override {return speaker;}
    std::int32_t game_width() override {throw std::logic_error("no lifetime here");}
    std::int32_t game_height() override {throw std::logic_error("no lifetime here");}
    std::shared_ptr<BuildingSlots> building_slots() override {throw std::logic_error("unused");}
    std::shared_ptr<BuildingMenuCatalog> buildings() override {throw std::logic_error("unused");}
    bool can_build(std::int32_t) override {throw std::logic_error("unused");}
    std::shared_ptr<std::vector<EventObject>> living_members() override {throw std::logic_error("unused");}
    std::int32_t system_share_value() override {throw std::logic_error("unused");}
    EventObject make_shop(std::int32_t) override {throw std::logic_error("unused");}
    void set_shop_selection(EventObject,std::int32_t) override {throw std::logic_error("unused");}
    EventObject make_build_list(std::int32_t) override {throw std::logic_error("unused");}
    EventObject make_human_house(EventObject,std::int32_t,bool) override {throw std::logic_error("unused");}
    EventObject make_subform(std::int32_t) override {throw std::logic_error("unused");}
    EventObject make_share_window(bool) override {throw std::logic_error("unused");}
};
int main() {
    {Host h;check(!h.run() && h.log==std::vector<std::uint32_t>({0x10000000,0x20000000,0x40000000,0x02000000}));}
    {Host h;h.buttons=0x72000000;check(h.run());check(h.log==std::vector<std::uint32_t>({0x10000000,1,2,2,2,2,3,12}));check(h.save->at(8)==4 && h.save->at(10)==7 && h.save->at(11)==10 && h.save->at(12)==20 && h.state.selected_area==-1 && h.building.selected_chip==2);}
    {Host h;h.buttons=0x10000000;h.fail_area=true;fails([&]{h.run();});check(h.state.selected_area==-1 && h.save->at(8)==4 && !h.form.auto_save);}
    {Host h;h.buttons=0x20000000;h.fail_push=true;fails([&]{h.run();});check(h.building.selected_chip==2 && h.building.unlock_info_frames==-1 && h.form.auto_save);}
    {Host h;h.buttons=0x20000000;check(h.run() && h.building.selected_chip==-1);}
    {Host h;h.buttons=0x40000000;h.rows->at(0)->at(8)=1;h.rows->push_back(std::make_shared<EventIntegers>(*h.rows->at(0)));check(h.run());check(h.rows->at(0)->at(8)==2 && h.rows->at(1)->at(8)==1 && h.system->at(3)==31 && h.save->at(3)==0);check(h.bubble.index==-1 && h.bubble.frames==0 && h.bubble.text->empty());check(h.log.at(4)==12 && h.log.at(5)==13);}
    {Host h;h.buttons=0x40000000;h.rows->clear();check(h.run() && h.system->at(3)==31);}
    {Host h;h.buttons=0x40000000;h.rows->at(0)->at(8)=1;h.fail_push=true;fails([&]{h.run();});check(h.rows->at(0)->at(8)==1 && h.bubble.frames==10);}
    {Host h;h.buttons=0x40000000;h.rows->at(0)->at(8)=1;h.fail_sum=true;fails([&]{h.run();});check(h.rows->at(0)->at(8)==2 && h.bubble.frames==10 && h.form.auto_save);}
    {Host h;h.buttons=0x02000000;check(h.run() && h.progress.money==0 && !h.form.auto_save);}
    {Host h;h.buttons=0x02000000;h.rows->at(0)->at(8)=3;h.mutate_present=true;check(h.run());check(h.progress.money==12 && h.state.resident_present_value==99 && h.bubble.frames==120 && *h.bubble.text=="hello");}
    {Host h;h.buttons=0x02000000;h.rows->at(0)->at(8)=3;h.fail_push=true;fails([&]{h.run();});check(h.rows->at(0)->at(8)==0 && h.progress.money==12 && h.bubble.frames==10);}
    {Host h;h.buttons=0x02000000;h.rows->at(0)->at(8)=3;h.voice.reset();fails([&]{h.run();});check(h.progress.money==12 && h.form.auto_save && h.bubble.frames==10);}
}
