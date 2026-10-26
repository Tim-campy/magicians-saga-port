#include "core/gameplay/select_form_pass.hpp"
#include "core/gameplay/select_navigation.hpp"
#include "core/gameplay/select_late.hpp"
#include <cstdlib>
#include <iostream>
#include <stdexcept>
using namespace saga::core;
void check(bool v,const char* message) {if(!v){std::cerr<<message<<'\n';std::exit(1);}}
struct Host:SelectFormPassHost,SelectCloudHost,SelectMarkerHost,EventExecutionHost,FormStackHost,SelectPreludeHost,SelectTouchMoveHost,SelectTriggerHost,SelectNavigationHost {
    SelectFormPassState state{0,false,false,{nullptr,this,nullptr},{false,false,nullptr},{0,false}};
    EventDelayQueue queue;
    DecodedEventConditionState decoded;
    DecodedEventConditionHost conditions{decoded};
    std::vector<std::string> log;
    bool input=false,label=false,balance=false,fail_push=false,room_button=true;
    int town_count=0,effects=0;
    EventObject child=std::make_shared<int>(7);
    std::shared_ptr<SelectClouds> cloud_list=std::make_shared<SelectClouds>();
    std::shared_ptr<EventCatalog> catalog=std::make_shared<EventCatalog>(2);
    Host() {decoded.context_present=true;decoded.context_character_count=1;decoded.money=0;}
    std::function<void()> pending;
    bool pressed(std::uint32_t mask) override {log.push_back("button");return room_button && mask==0x04000000U;}
    void play_button_sound() override {log.push_back("sound");}
    void make_transition(std::int32_t,bool,std::uint32_t,std::function<void()> action) override {log.push_back("transition");pending=std::move(action);}
    EventObject room_form() override {return child;}
    EventObject make_shop(std::int32_t) override {return child;}
    std::int64_t stamina() override {log.push_back("stamina");return 0;}
    std::uint32_t system_flags() override {log.push_back("sysflags");return 0;}
    std::shared_ptr<EventIntegers> save_integers() override {throw std::logic_error("unexpected save");}
    std::shared_ptr<TownHumanRows> town_humans() override {log.push_back("collision");return std::make_shared<TownHumanRows>();}
    void set_talk_bubble(std::int32_t,bool) override {throw std::logic_error("unexpected bubble");}
    std::int32_t saved_orientation() override {log.push_back("orientation");return 0;}
    bool enable_landscape() override {throw std::logic_error("unexpected landscape");}
    void set_orientation(std::int32_t) override {throw std::logic_error("unexpected orientation write");}
    std::int32_t game_width() override {log.push_back("width");return 240;}
    std::int32_t view_x() override {log.push_back("view");return 10;}
    std::shared_ptr<ImageAnimationValues> image_animation_values() override {log.push_back("images");return std::make_shared<ImageAnimationValues>();}
    SelectPassResult run() {return process_select_form_pass(state,*this,*this,*this,queue,*this,conditions);}
    bool capture_input(std::uint32_t mask) override {check(mask==0x100000U,"capture mask");log.push_back("input");return input;}
    void change_state(std::int32_t n) override {log.push_back("change");state.state=n;}
    bool soft_label(std::int32_t n) override {check(n==3,"soft label");log.push_back("label");return label;}
    void finish_capture() override {log.push_back("finish");}
    void capture() override {log.push_back("capture");}
    void play_bgm() override {log.push_back("bgm");}
    void update_notifications() override {log.push_back("notify");}
    bool acquire_balance() override {log.push_back("balance");return balance;}
    void set_active_x() override {log.push_back("active");}
    void update_decorations() override {log.push_back("decor");}
    void update_town_humans() override {log.push_back("humans");}
    std::int32_t town_enable_count() override {log.push_back("count");return town_count;}
    bool update_town_enable(std::int32_t n) override {log.push_back("enable"+std::to_string(n));return true;}
    void check_guide_arrow(bool force) override {log.push_back(force?"guide":"guide_late");}
    std::shared_ptr<SelectClouds> clouds() override {log.push_back("clouds");return cloud_list;}
    std::int32_t right_edge() override {return 100;}
    std::int32_t cloud_resource() override {return 1;}
    std::int32_t game_height() override {return 500;}
    bool low_aspect() override {return false;}
    std::int32_t random(std::int32_t) override {return 0;}
    bool is_top_form() override {log.push_back("top");return true;}
    bool debug_enabled() override {return false;}
    std::shared_ptr<ConditionCatalog<AreaMarkerRecord>> areas() override {return std::make_shared<ConditionCatalog<AreaMarkerRecord>>();}
    std::shared_ptr<AreaQuestState> quest(std::int32_t) override {throw std::logic_error("unexpected quest");}
    std::shared_ptr<ConditionCatalog<EquipmentMarkerState>> equipment(EquipmentMarkerKind) override {return std::make_shared<ConditionCatalog<EquipmentMarkerState>>();}
    std::shared_ptr<EventCatalog> events() override {return catalog;}
    EventObject default_event_args() override {return nullptr;}
    void empty_event_diagnostic(std::int32_t) override {}
    EventObject dispatch(const EventInstruction& row,const EventContext&) override {
        ++effects;log.push_back("effect");
        if(row->at(0)==1) {decoded.money=decoded.money.value()+1;return nullptr;}
        return child;
    }
    std::int32_t identify_script(const EventScript& script) override {return identify_event_script(*catalog,script,[]{return -1;});}
    EventScript resolve_script(std::int32_t id) override {return resolve_event_script(*catalog,id);}
    std::int32_t read_talk_character() override {return 0;}
    void restore_talk_character(std::size_t slot,std::int32_t) override {check(slot==7,"restore slot");log.push_back("restore");}
    void push(EventObject,EventObject value,bool option) override {
        check(value==child && !option && state.auto_save && state.events.event_check_requested,"flags precede push");
        log.push_back("push");if(fail_push)throw std::runtime_error("manager failed");
    }
    void add_event(int opcode) {
        auto record=std::make_shared<EventConditionRecord>();
        record->terms=std::make_shared<std::vector<EventInstruction>>();
        record->terms->push_back(std::make_shared<EventIntegers>(EventIntegers{1,0,4}));
        record->effects=std::make_shared<std::vector<EventInstruction>>();
        record->effects->push_back(std::make_shared<EventIntegers>(EventIntegers{opcode}));
        catalog->at(1)=record;
    }
};
struct LateBindings final:SelectBuildingHost,SelectTownButtonsHost,SelectGuidanceSaveHost,SelectCameraHost,SelectReviewHost {
    int save_calls=0,platform_calls=0;
    bool pressed(std::uint32_t) override {return false;}
    void play_button_sound() override {throw std::logic_error("unexpected sound");}
    std::shared_ptr<BuildingSlots> building_slots() override {throw std::logic_error("unexpected slots");}
    std::shared_ptr<EventIntegers> save_integers() override {throw std::logic_error("unexpected save array");}
    std::shared_ptr<BuildingMenuCatalog> buildings() override {throw std::logic_error("unexpected buildings");}
    bool can_build(std::int32_t) override {throw std::logic_error("unexpected build");}
    std::shared_ptr<std::vector<EventObject>> living_members() override {throw std::logic_error("unexpected members");}
    std::int32_t system_share_value() override {throw std::logic_error("unexpected share");}
    EventObject make_shop(std::int32_t) override {throw std::logic_error("unexpected shop");}
    void set_shop_selection(EventObject,std::int32_t) override {throw std::logic_error("unexpected shop selection");}
    EventObject make_build_list(std::int32_t) override {throw std::logic_error("unexpected build list");}
    EventObject make_human_house(EventObject,std::int32_t,bool) override {throw std::logic_error("unexpected house");}
    EventObject make_subform(std::int32_t) override {throw std::logic_error("unexpected subform");}
    EventObject make_share_window(bool) override {throw std::logic_error("unexpected share window");}
    EventObject area_form() override {throw std::logic_error("unexpected area form");}
    EventObject make_obstacle(std::int32_t) override {throw std::logic_error("unexpected obstacle");}
    EventObject make_store_invite() override {throw std::logic_error("unexpected invite");}
    std::int32_t sum_quest_challenges() override {throw std::logic_error("unexpected quests");}
    std::shared_ptr<EventIntegers> system_save_integers() override {throw std::logic_error("unexpected system save");}
    std::int32_t store_limit_slot() override {throw std::logic_error("unexpected limit");}
    void add_money(std::int32_t) override {throw std::logic_error("unexpected money");}
    FormText format_present_value(std::int32_t) override {throw std::logic_error("unexpected formatting");}
    EventObject make_present_popup(FormText) override {throw std::logic_error("unexpected popup");}
    std::shared_ptr<TownHumanRows> town_humans() override {throw std::logic_error("unexpected bubble row");}
    std::shared_ptr<TownVoices> voices() override {throw std::logic_error("unexpected voices");}
    std::shared_ptr<TownSpeakers> speakers() override {throw std::logic_error("unexpected speakers");}
    std::int32_t game_width() override {return 320;}
    std::int32_t game_height() override {return 240;}
    std::int32_t view_width() override {return 320;}
    std::int32_t view_height() override {return 240;}
    bool is_top_form() override {return true;}
    void save_game(bool automatic,bool option) override {check(automatic && option,"autosave options");++save_calls;}
    void report_autosave_error(const SelectSaveError&) override {throw std::logic_error("unexpected save failure");}
    bool soft_label(std::int32_t n) override {check(n==19,"late label");return false;}
    void save_system() override {throw std::logic_error("unexpected system store");}
    void show_offerwall() override {throw std::logic_error("unexpected offerwall");}
    bool print_enabled() override {throw std::logic_error("unexpected print");}
    void report_offerwall_error(const SelectSaveError&) override {throw std::logic_error("unexpected offer error");}
    std::int32_t random(std::int32_t) override {throw std::logic_error("unexpected review RNG");}
    std::int32_t area_state(std::int32_t) override {throw std::logic_error("unexpected area");}
    std::int32_t reviewed_slot() override {throw std::logic_error("unexpected reviewed");}
    std::int32_t review_coin_slot() override {throw std::logic_error("unexpected coin");}
    std::int32_t platform_code() override {++platform_calls;return 0;}
    EventObject make_reviewed(bool) override {throw std::logic_error("unexpected review form");}
};
int main() {
    {
        auto h=std::make_shared<Host>();h->room_button=false;h->state.events.loop_counter=2;
        SelectPreludeState prelude{0,0,0,-10};SelectTouchMoveState movement{false,0,0,0,-1};
        auto owner=std::shared_ptr<SelectFormPassState>(h,&h->state);
        SelectUpdateBindings bindings{prelude,*h,owner,*h,*h,*h,h->queue,*h,h->conditions,movement,*h,*h,h};
        SelectBuildingState building{-1,prelude.camera_x,0,movement.drag_distance,-1,3};
        SelectTownButtonsState town{-1,0};TownBubbleState bubble{-1,nullptr,3};
        SelectCameraState camera{false,2,0,prelude.camera_x,building.camera_y,0,1000};LateBindings late;
        h->state.auto_save=true;
        check(update_select_form(bindings,building,late,town,bubble,late,late,camera,late,late),"full Update return");
        check(prelude.camera_x==-9 && building.unlock_info_frames==2 && bubble.frames==2,"full update reaches movement and lifetimes");
        check(late.save_calls==1 && late.platform_calls==1 && !h->state.auto_save,"full update saves before late gates");
        check(h->log.at(h->log.size()-2)=="guide_late" && h->log.back()=="bgm","late guide and BGM order");
        h->state.form.dialog=h->child;h->state.auto_save=true;h->log.clear();
        update_select_form(bindings,building,late,town,bubble,late,late,camera,late,late);
        check(late.save_calls==1 && late.platform_calls==1 && bubble.frames==2 && prelude.camera_x==-9,"dialog skips the entire late continuation");
        h->room_button=true;
        update_select_form(bindings,building,late,town,bubble,late,late,camera,late,late);
        check(bool(h->pending) && late.save_calls==1 && bubble.frames==2,"button return skips guidance and late continuation");
        h->pending={};
    }

    {
        auto h=std::make_shared<Host>();SelectPreludeState prelude{0,0,0,0};SelectTouchMoveState movement{false,0,0,0,-1};
        auto owner=std::shared_ptr<SelectFormPassState>(h,&h->state);
        SelectUpdateBindings bindings{prelude,*h,owner,*h,*h,*h,h->queue,*h,h->conditions,movement,*h,*h,h};
        h->state.form.dialog=h->child;
        check(update_select_through_navigation(bindings).result==SelectNavigationResult::ReturnTrue && !h->pending,"dialog bypasses navigation");
        h->state.events.loop_counter=2;h->log.clear();
        check(update_select_through_navigation(bindings).result==SelectNavigationResult::ReturnTrue && bool(h->pending),"composed update schedules room");
        check(!h->state.auto_save && h->log.back()=="transition","room push stays deferred");
        auto action=std::move(h->pending);h->pending={};action();
        check(h->state.auto_save && h->state.events.event_check_requested,"deferred callback reaches existing Push");
    }
    {
        Host h;SelectPreludeState prelude{0,0,0,20};h.state.form.dialog=h.child;
        SelectTouchMoveState movement{true,0,10,0,-1};
        const auto result=update_select_form_prefix(prelude,h,h.state,h,h,h,h.queue,h,h.conditions,movement,h,h);
        check(result.pass==SelectPassResult::ReturnTrue && result.snapshot.camera_x==20,"prefix snapshot and return");
        check(prelude.update_counter==1 && h.state.events.loop_counter==0 && !h.state.form.dialog,"base counter advances even on dialog return");
        check(h.log==std::vector<std::string>({"orientation","width","view","images","images"}),"common work precedes dialog gate");
        check(movement.witch_x==0,"dialog return skips touch movement");
        h.log.clear();h.state.events.loop_counter=2;
        const auto next=update_select_form_prefix(prelude,h,h.state,h,h,h,h.queue,h,h.conditions,movement,h,h);
        check(next.pass==SelectPassResult::ContinueInput && movement.witch_x==6 && h.log.back()=="sysflags","normal continuation reaches triggers");
        check(h.log.at(h.log.size()-3)=="collision" && h.log.at(h.log.size()-2)=="stamina","movement precedes triggers");
    }
    {
        Host h;h.state.form.dialog=h.child;
        check(h.run()==SelectPassResult::ReturnTrue && !h.state.form.dialog && h.log.empty(),"dialog clears before state dispatch");
        h.state.state=3;h.state.captured=true;
        check(h.run()==SelectPassResult::ReturnTrue && h.log.empty(),"captured gate");
        h.state.captured=false;h.input=true;h.run();
        check(h.state.state==4 && h.log==std::vector<std::string>({"input","change"}),"capture transition returns without bgm");
        h.log.clear();h.run();check(h.log==std::vector<std::string>({"capture"}),"state4");
        h.log.clear();h.state.state=3;h.input=false;h.label=true;h.run();
        check(h.log==std::vector<std::string>({"input","label","finish","bgm"}),"capture finish ordering");
        h.log.clear();h.state.state=-1;h.run();check(h.log==std::vector<std::string>({"bgm"}),"other states skip events");
    }
    {
        Host h;h.balance=true;h.state.markers.scene_resume=true;
        h.queue.push_back(std::make_shared<DelayedEvent>(DelayedEvent{1,0,2,-1,0,{}}));
        check(h.run()==SelectPassResult::ReturnTrue && h.state.events.loop_counter==1,"counter precedes balance gate");
        check(h.log==std::vector<std::string>({"clouds","top","notify","balance"}),"clouds and markers precede notifications and balance");
        check(h.queue.at(0)->wait==2,"balance gate must not consume delay");
    }
    {
        Host h;h.add_event(8);h.town_count=2;
        check(h.run()==SelectPassResult::ContinueInput,"initial direct event result discarded");
        check(h.effects==2 && h.catalog->at(1)->execution_count==2 && !h.state.auto_save,"initial events bypass eligibility and do not select-push");
        check(h.log==std::vector<std::string>({"clouds","notify","balance","active","decor","humans","count","enable0","effect","count","enable1","effect","count","guide"}),"early counter initialization ordering");
    }
    {
        Host h;h.add_event(8);h.state.events={299,false};h.fail_push=true;
        try{h.run();check(false,"push should fail");}catch(const std::runtime_error&){}
        check(h.effects==1 && h.state.auto_save && h.state.events.event_check_requested,"automatic event push retains mutations on failure");
        check(h.log.back()=="push","event result reaches manager");
    }
    {
        Host h;h.add_event(1);h.state.events={2,false};
        h.queue.push_back(std::make_shared<DelayedEvent>(DelayedEvent{1,0,1,-1,0,{}}));
        check(h.run()==SelectPassResult::ContinueInput && h.queue.empty() && h.decoded.money==1,"queue resumes after counter2; not gated by initialization");
        check(h.log==std::vector<std::string>({"clouds","notify","balance","active","decor","humans","restore","effect"}),"queue order");
    }
}
