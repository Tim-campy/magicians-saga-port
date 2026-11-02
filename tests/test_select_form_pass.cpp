#include "core/gameplay/select_form_pass.hpp"
#include "core/gameplay/select_navigation.hpp"
#include "core/gameplay/select_late.hpp"
#include "core/gameplay/select_guide.hpp"
#include <cstdlib>
#include <cmath>
#include <limits>
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
struct LateBindings final:SelectBuildingHost,SelectTownButtonsHost,SelectGuidanceSaveHost,SelectCameraHost,SelectReviewHost,SelectGuideHost,SelectActiveAreaHost,SelectNotificationHost {
    int save_calls=0,platform_calls=0,guide_sums=0,notification_reads=0;
    EventObject notification_manager() override {return std::make_shared<int>(1);}
    std::int32_t value_count(EventObject) override {++notification_reads;return 0;}
    NotificationText value(EventObject,std::int32_t) override {throw std::logic_error("unexpected notification value");}
    bool starts_currency_sale(NotificationText) override {throw std::logic_error("unexpected sale prefix");}
    EventObject billing_manager() override {throw std::logic_error("unexpected billing");}
    std::shared_ptr<NotificationApplyTask> make_apply_task(NotificationText) override {throw std::logic_error("unexpected task");}
    void add_apply_task(EventObject,std::shared_ptr<NotificationApplyTask>,bool) override {throw std::logic_error("unexpected task add");}
    void show_currency_sale(NotificationText) override {throw std::logic_error("unexpected sale");}
    void show_free_movie() override {throw std::logic_error("unexpected movie");}
    void remove_value(EventObject,std::int32_t) override {throw std::logic_error("unexpected notification removal");}
    void update_apply_tasks(EventObject,bool) override {throw std::logic_error("unexpected task update");}
    std::shared_ptr<EventIntegers> normal_save_integers() override {return std::make_shared<EventIntegers>(20,0);}
    std::shared_ptr<CameraAreas> camera_areas() override {return std::make_shared<CameraAreas>();}
    bool pressed(std::uint32_t) override {return false;}
    void play_button_sound() override {throw std::logic_error("unexpected sound");}
    std::shared_ptr<BuildingSlots> building_slots() override {return std::make_shared<BuildingSlots>();}
    std::shared_ptr<GuideTowns> towns() override {return std::make_shared<GuideTowns>();}
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
    std::int32_t sum_quest_challenges() override {++guide_sums;return 0;}
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
struct BackgroundHost final:SelectBackgroundHost {
    struct Draw {SelectBackgroundResource resource;std::int32_t clip,frame;float x,y;};
    std::int32_t camera_x=0,camera_y=0,left=0,right=1,view_y=0;
    std::int32_t width=320,height=240,view_height=240,clip=7;
    int owner=0;
    SelectBackgroundResource resource=&owner;
    std::array<float,3> pixel{0,0,0};
    std::array<std::int32_t,4> color{};
    std::array<std::int32_t,2> fill{};
    std::vector<Draw> draws;
    std::vector<std::string> log;
    std::function<void(const std::string&)> on_call;
    std::size_t fail_call=0;
    void call(const std::string& name) {
        log.push_back(name);
        if(on_call)on_call(name);
        if(fail_call==log.size())throw std::runtime_error("background service failed");
    }
    SelectBackgroundState state() const {return {camera_x,camera_y,left,right,view_y};}
    void run() {draw_select_background(state(),*this);}
    std::array<float,3> background_pixel() override {const auto result=pixel;call("pixel");return result;}
    void background_color(std::int32_t r,std::int32_t g,std::int32_t b,std::int32_t a) override {color={r,g,b,a};call("color");}
    std::int32_t background_width() override {const auto result=width;call("width");return result;}
    std::int32_t background_height() override {const auto result=height;call("height");return result;}
    void fill_background(std::int32_t w,std::int32_t h) override {fill={w,h};call("fill");}
    SelectBackgroundResource background_resource() override {const auto result=resource;call("resource");return result;}
    std::int32_t background_view_height() override {const auto result=view_height;call("view_height");return result;}
    std::int32_t background_clip() override {const auto result=clip;call("clip");return result;}
    void draw_background(SelectBackgroundResource r,std::int32_t c,std::int32_t f,float x,float y) override {
        draws.push_back({r,c,f,x,y});call(f==0?"draw0":"draw1");
    }
};
void test_select_background() {
    // _Draw 0x11f0850..0x11f0a6c. This host supplies no RNG service.
    const auto lo=std::numeric_limits<std::int32_t>::min();
    const auto hi=std::numeric_limits<std::int32_t>::max();
    const auto inf=std::numeric_limits<float>::infinity();
    const auto nan=std::numeric_limits<float>::quiet_NaN();
    struct ColorCase {std::array<float,3> input;std::array<std::int32_t,4> expected;};
    const ColorCase colors[]={
        {{0.5F,-0.5F,2.0F},{127,-127,510,255}},
        {{nan,inf,-inf},{0,lo,lo,255}},
        {{1.0e10F,-1.0e10F,std::numeric_limits<float>::max()},{hi,lo,lo,255}},
        // The first binary32 product rounds to257; binary64 multiplication would truncate to256.
        {{257.0F/255.0F,std::nextafter(1.0F,0.0F),-0.0F},{257,254,0,255}},
    };
    for(const auto& item:colors) {
        BackgroundHost h;h.right=0;h.pixel=item.input;h.run();
        check(h.color==item.expected,"background RGB uses binary32 products and ARM64 conversion without clamping");
        check(h.draws.size()==1 && h.draws[0].frame==1,"empty tile range still draws frame1");
    }
    {
        BackgroundHost h;h.right=0;h.width=-17;h.height=4097;
        h.on_call=[&](const std::string& name){
            if(name=="width"){h.width=999;h.height=5001;}
            if(name=="height")h.height=888;
            if(name=="fill"){h.camera_y=10;h.view_y=20;h.camera_x=-59;}
        };
        h.run();
        check(h.log==std::vector<std::string>({"pixel","color","width","height","fill","resource","clip","draw1"}),"fill and empty-range query ordering");
        check(h.fill==std::array<std::int32_t,2>{-17,5001},"width captured before live height and dimensions passed unchanged");
        check(h.draws[0].x==29.0F && h.draws[0].y==50.0F,"background captures coordinates after the fill");
    }
    {
        BackgroundHost h;int next_owner=1,last_owner=2,unused_owner=3;
        const auto first=h.resource;
        h.camera_x=-60;h.camera_y=10;h.view_y=20;h.left=-240;h.right=1;h.view_height=300;h.clip=4;
        int resources=0;
        h.on_call=[&](const std::string& name){
            if(name=="resource") {
                ++resources;
                if(resources==1) {
                    h.camera_y=900;h.view_y=800;h.left=700;h.right=-999;h.resource=&next_owner;
                } else if(resources==3) {
                    h.right=1000;h.camera_x=-121;h.clip=7;h.resource=&unused_owner;
                }
            } else if(name=="draw0" && h.draws.size()==1) {
                h.right=481;h.view_height=310;h.clip=5;h.camera_x=-91;
            } else if(name=="draw0" && h.draws.size()==2) {
                h.right=1;h.resource=&last_owner;h.camera_x=-119;
            } else if(name=="clip" && resources==1)h.view_height=9000;
            else if(name=="clip" && resources==3)h.camera_x=9000;
        };
        h.run();
        check(h.log==std::vector<std::string>({"pixel","color","width","height","fill","resource","view_height","clip","draw0","resource","view_height","clip","draw0","resource","clip","draw1"}),"resource is read at every captured-limit test including the false test");
        check(h.draws.size()==3 && resources==3,"right extent is live but resource callback cannot change its captured comparison");
        const auto& a=h.draws[0];const auto& b=h.draws[1];const auto& c=h.draws[2];
        check(a.resource==first && a.clip==4 && a.frame==0 && a.x==-240.0F && a.y==205.0F,"first tile retains captured resource, left extent and baseY");
        check(b.resource==&next_owner && b.clip==5 && b.frame==0 && b.x==0.0F && b.y==215.0F,"240-step tile rereads resource, view height and clip");
        check(c.resource==&last_owner && c.clip==7 && c.frame==1 && c.x==26.0F && c.y==50.0F,"frame1 retains false-test resource and baseY but captures fresh camera before clip");
    }
    {
        BackgroundHost h;h.camera_y=hi-10;h.view_y=25;h.left=hi-100;h.right=hi;h.view_height=1000;
        h.on_call=[&](const std::string& name){
            if(name=="draw0" && h.draws.size()==1)h.right=lo+200;
            if(name=="draw0" && h.draws.size()==2)h.camera_x=lo;
        };
        h.run();
        check(h.draws.size()==3,"wrapped tile increment continues under signed live bound");
        check(h.draws[0].x==static_cast<float>(hi-100) && h.draws[1].x==static_cast<float>(lo+139),"tile X addition wraps int32 before binary32 conversion");
        check(h.draws[0].y==static_cast<float>(lo+889) && h.draws[1].y==h.draws[0].y,"baseY and tile Y additions wrap int32");
        check(h.draws[2].x==static_cast<float>(-71582758) && h.draws[2].y==static_cast<float>(lo+34),"negative camera division truncates toward zero and frame1 Y wraps");
    }
    {
        const std::vector<std::string> expected{"pixel","color","width","height","fill","resource","view_height","clip","draw0","resource","clip","draw1"};
        for(std::size_t fail=1;fail<=expected.size();++fail) {
            BackgroundHost h;h.fail_call=fail;
            bool failed=false;try{h.run();}catch(const std::runtime_error&){failed=true;}
            check(failed && h.log==std::vector<std::string>(expected.begin(),expected.begin()+fail),"background service failure preserves only its native call prefix");
        }
    }
    for(const bool tile:{false,true}) {
        BackgroundHost h;h.resource=nullptr;h.right=tile?1:0;
        bool failed=false;try{h.run();}catch(const std::exception&){failed=true;}
        auto expected=std::vector<std::string>{"pixel","color","width","height","fill","resource"};
        if(tile)expected.push_back("view_height");
        check(failed && h.log==expected && h.draws.empty(),"null captured resource fails after tile view height and before clip or drawing");
    }
    {
        BackgroundHost h;
        h.on_call=[&](const std::string& name){if(name=="draw0")h.resource=nullptr;};
        bool failed=false;try{h.run();}catch(const std::exception&){failed=true;}
        check(failed && h.draws.size()==1 && h.log==std::vector<std::string>({"pixel","color","width","height","fill","resource","view_height","clip","draw0","resource"}),"final false-test null resource prevents frame1 after retaining the completed tile");
    }
}
int main() {
    test_select_background();
    {
        auto h=std::make_shared<Host>();h->room_button=false;h->state.events.loop_counter=0;
        SelectPreludeState prelude{0,0,0,-10};SelectTouchMoveState movement{false,0,0,0,-1};
        auto owner=std::shared_ptr<SelectFormPassState>(h,&h->state);
        SelectBuildingState building{-1,prelude.camera_x,0,movement.drag_distance,-1,3};
        SelectCameraState camera{false,2,0,prelude.camera_x,building.camera_y,0,1000};
        LateBindings late;SelectGuideState guide_state;SelectRecoveredPassHost guide_pass{*h,guide_state,late,camera,late,late};
        SelectUpdateBindings bindings{prelude,*h,owner,guide_pass,*h,*h,h->queue,*h,h->conditions,movement,*h,*h,h};
        SelectTownButtonsState town{-1,0};TownBubbleState bubble{-1,nullptr,3};
        h->state.auto_save=true;
        check(update_select_form(bindings,building,late,town,bubble,late,late,camera,late,late),"full Update return");
        check(prelude.camera_x==0 && camera.left_extent==-449 && camera.right_extent==320 && building.unlock_info_frames==2 && bubble.frames==2,"full update reaches movement and lifetimes");
        check(late.notification_reads==1 && late.save_calls==1 && late.platform_calls==1 && !h->state.auto_save,"full update saves before late gates");
        check(late.guide_sums==2 && guide_state.flags->at(0)==1 && guide_state.flags->at(1)==0 && h->log.back()=="bgm","forced and late recovered guide calls before BGM");
        h->state.form.dialog=h->child;h->state.auto_save=true;h->log.clear();
        update_select_form(bindings,building,late,town,bubble,late,late,camera,late,late);
        check(late.save_calls==1 && late.platform_calls==1 && bubble.frames==2 && prelude.camera_x==0,"dialog skips the entire late continuation");
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
