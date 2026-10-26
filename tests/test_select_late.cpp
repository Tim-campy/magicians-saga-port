#include "core/gameplay/select_late.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace saga::core;
void check(bool value){if(!value){std::cerr<<"late update contract failure\n";std::exit(1);}}
template<class F> void fails(F f){try{f();check(false);}catch(const std::exception&) {}}
struct Host final:SelectReviewHost,SelectCameraHost,TownBubbleHost,SelectFormPassHost,EventExecutionHost {
    std::vector<std::string> calls;
    std::shared_ptr<EventIntegers> save=std::make_shared<EventIntegers>(2,0);
    std::shared_ptr<TownHumanRows> humans=std::make_shared<TownHumanRows>();
    std::shared_ptr<EventCatalog> catalog=std::make_shared<EventCatalog>(15);
    std::int32_t roll=0,area=1,platform=0;
    bool fail_effect=false,fail_camera=false;
    std::function<void()> guide_hook,coin_hook;
    EventObject child=std::make_shared<int>(8);
    Host(){auto r=std::make_shared<EventConditionRecord>();r->execution_count=0;r->effects=std::make_shared<std::vector<EventInstruction>>();r->effects->push_back(std::make_shared<EventIntegers>(EventIntegers{1}));catalog->at(14)=r;}
    void record(const std::string& s){calls.push_back(s);}
    std::int32_t random(std::int32_t bound) override {check(bound==100);record("random");return roll;}
    std::int32_t area_state(std::int32_t index) override {check(index==4);record("area");return area;}
    std::shared_ptr<EventIntegers> system_save_integers() override {record("save");return save;}
    std::int32_t reviewed_slot() override {record("reviewed");return 0;}
    std::int32_t review_coin_slot() override {record("coin");if(coin_hook)coin_hook();return 1;}
    std::int32_t platform_code() override {record("platform");return platform;}
    EventObject make_reviewed(bool option) override {check(!option);record("make");return child;}
    std::int32_t game_width() override {record("width");if(fail_camera)throw std::runtime_error("camera");return 320;}
    std::int32_t game_height() override {record("height");return 240;}
    std::int32_t view_width() override {record("view_width");return 320;}
    std::int32_t view_height() override {record("view_height");return 240;}
    std::shared_ptr<TownHumanRows> town_humans() override {return humans;}
    std::shared_ptr<TownVoices> voices() override {throw std::logic_error("unexpected voices");}
    std::shared_ptr<TownSpeakers> speakers() override {throw std::logic_error("unexpected speakers");}
    void check_guide_arrow(bool force) override {check(!force);record("guide");if(guide_hook)guide_hook();}
    void play_bgm() override {record("bgm");}
    bool capture_input(std::uint32_t) override {throw std::logic_error("capture");}
    void change_state(std::int32_t) override {throw std::logic_error("change");}
    bool soft_label(std::int32_t) override {throw std::logic_error("label");}
    void finish_capture() override {throw std::logic_error("finish");}
    void capture() override {throw std::logic_error("capture");}
    void update_notifications() override {throw std::logic_error("notify");}
    bool acquire_balance() override {throw std::logic_error("balance");}
    void set_active_x() override {throw std::logic_error("active");}
    void update_decorations() override {throw std::logic_error("decor");}
    void update_town_humans() override {throw std::logic_error("town");}
    std::int32_t town_enable_count() override {throw std::logic_error("count");}
    bool update_town_enable(std::int32_t) override {throw std::logic_error("enable");}
    std::shared_ptr<EventCatalog> events() override {record("events");return catalog;}
    EventObject default_event_args() override {return nullptr;}
    void empty_event_diagnostic(std::int32_t) override {record("empty");}
    EventObject dispatch(const EventInstruction&,const EventContext&) override {record("effect");check(save->at(1)==1);if(fail_effect)throw std::runtime_error("effect");return child;}
    std::int32_t identify_script(const EventScript&) override {return -1;}
    EventScript resolve_script(std::int32_t) override {throw std::logic_error("delay");}
    std::int32_t read_talk_character() override {return 0;}
    void restore_talk_character(std::size_t,std::int32_t) override {}
};
struct Fixture {
    std::int32_t x=0,y=0;float drag=0;
    SelectBuildingState building{-1,x,0,drag,-1,3};
    SelectCameraState camera{false,0,0,x,building.camera_y,0,1000};
    SelectFormPassState form{};
    TownBubbleState bubble{-1,std::make_shared<const std::string>("synthetic"),3};
    Host host;EventDelayQueue queue;
    void run(SelectPreludeSnapshot snapshot={0,0}) {process_select_late(form,building,bubble,snapshot,camera,host,host,host,host,queue,host);}
};
int main() {
    {Fixture f;f.run();check(f.building.unlock_info_frames==2 && f.bubble.frames==2);check((f.host.calls==std::vector<std::string>{"guide","platform","bgm"}));}
    {Fixture f;f.building.unlock_info_frames=0;f.bubble.frames=0;f.run();check(f.building.unlock_info_frames==-1 && f.bubble.frames==-1 && *f.bubble.text=="synthetic");}
    {Fixture f;f.host.guide_hook=[&]{f.form.events.loop_counter=90;};f.host.roll=-1;f.run();check(f.form.auto_save && f.form.events.event_check_requested);check((f.host.calls==std::vector<std::string>{"guide","random","area","save","reviewed","save","coin","make"}));}
    {Fixture f;f.form.events.loop_counter=90;f.host.roll=40;f.host.save.reset();f.run();check((f.host.calls==std::vector<std::string>{"guide","random","platform","bgm"}));}
    {Fixture f;f.form.events.loop_counter=90;f.host.area=2;f.run();check((f.host.calls==std::vector<std::string>{"guide","random","area","platform","bgm"}));}
    {Fixture f;f.form.events.loop_counter=90;f.host.save->at(0)=2;f.host.save->at(1)=-1;f.run();check(f.form.auto_save && f.host.calls.back()=="make");} // Flags are exactly 1, not nonzero.
    {Fixture f;f.host.platform=2;f.host.save->at(0)=1;f.run();check(f.host.save->at(1)==1 && f.host.catalog->at(14)->execution_count==1 && f.form.auto_save);check(!f.form.events.event_check_requested && f.host.calls.back()=="bgm");} // Returned event form is discarded.
    {Fixture f;f.host.platform=2;f.host.save->at(0)=1;f.host.fail_effect=true;fails([&]{f.run();});check(f.host.save->at(1)==1 && !f.form.auto_save && f.host.calls.back()=="effect");}
    {Fixture f;f.host.platform=2;f.host.save->at(0)=1;f.host.catalog->at(14)->execution_count=1;f.run();check(f.host.save->at(1)==0 && !f.form.auto_save);}
    {Fixture f;f.host.save.reset();fails([&]{select_is_reviewed(f.host);});check((f.host.calls==std::vector<std::string>{"save"}));}
    {Fixture f;const auto captured=f.host.save;captured->at(1)=1;f.host.coin_hook=[&]{f.host.save=std::make_shared<EventIntegers>(2,0);};check(select_has_review_coin(f.host));}
    {Fixture f;f.camera.velocity_x=2;f.host.fail_camera=true;fails([&]{f.run();});check(f.building.unlock_info_frames==3 && f.bubble.frames==3 && f.host.calls.size()==1);}
    {Fixture f;f.x=-600;f.camera.velocity_x=2;f.bubble.index=0;f.host.humans->push_back(std::make_shared<EventIntegers>(EventIntegers{0,20}));f.run({0,0});check(f.bubble.index==0 && f.bubble.frames==2);} // Old snapshot remains visible despite the new negative camera.
}
