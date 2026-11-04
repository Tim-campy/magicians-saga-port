#include "core/gameplay/stage_menu.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;
namespace {
void check(bool value){if(!value){std::cerr<<"stage-menu contract failure\n";std::exit(1);}}
template<class F>void fails(F action){try{action();check(false);}catch(const std::exception&){}}
struct Host final:StageMenuHost {
    StageMenuState& state;
    std::vector<std::string> calls;
    std::string failure;
    EventInstruction saves=std::make_shared<EventIntegers>(12,0);
    std::shared_ptr<AreaTable> area_rows=std::make_shared<AreaTable>(2);
    std::shared_ptr<QuestTable> quest_rows=std::make_shared<QuestTable>(4);
    bool debug_value=false,confirm=false,formation=false,cancel=false,pop_value=false,recover_save=false;
    std::int32_t balance=10,spent=0;
    std::uint32_t fallback_flags=0;
    int quest_queries=0;
    std::function<void()> on_quests;
    StageMenuScrollParameters specification{};
    EventObject created_scroll=std::make_shared<int>(7),created_renderer=std::make_shared<int>(8),child=std::make_shared<int>(9);
    const StageMenuTouch* forwarded_touch=nullptr;
    explicit Host(StageMenuState& value):state(value){
        saves->at(8)=1;
        area_rows->at(1)=AreaDefinition{};
        area_rows->at(1)->stages=std::make_shared<EventIntegers>(EventIntegers{0,1,-1,2,3});
        for(std::size_t i=0;i<quest_rows->size();++i){
            quest_rows->at(i)=QuestDefinition{};auto& quest=*quest_rows->at(i);
            quest.id=static_cast<std::int32_t>(i)+20;quest.discovery_state=1;quest.need_stamina=5;
        }
        quest_rows->at(1)->stage.flags=2;quest_rows->at(2)->discovery_state=2;
    }
    void call(const std::string& name){calls.push_back(name);if(failure==name)throw std::runtime_error(name);}
    void base_initialize()override{call("init");}
    void set_background_white()override{call("white");}
    void prepare_resources()override{call("resources");}
    EventInstruction save_integers()override{call("save");return saves;}
    void system_flag_on(std::uint32_t value)override{check(value==256);call("flag");}
    std::shared_ptr<AreaTable> areas()override{call("areas");check(state.entries!=nullptr);return area_rows;}
    std::shared_ptr<QuestTable> quests()override{call("quests");++quest_queries;if(on_quests)on_quests();return quest_rows;}
    bool debug()override{call("debug");return debug_value;}
    void set_soft_labels(std::int32_t left,std::int32_t right)override{call("labels:"+std::to_string(left)+":"+std::to_string(right));}
    EventObject make_scroll(const StageMenuScrollParameters& value)override{call("make_scroll");specification=value;return created_scroll;}
    void scroll_element_size(const EventObject& value,std::int32_t width,std::int32_t height)override{check(value==created_scroll&&width==210&&height==40);call("size");}
    void scroll_touch_flags(const EventObject& value,std::int32_t flags)override{check(value==created_scroll&&flags==17);call("flags");}
    EventObject make_renderer()override{call("renderer");check(state.scroll==created_scroll);return created_renderer;}
    void base_update()override{call("base");++state.base_update_counter;}
    void reset_image_clocks()override{call("reset");}
    void advance_image_clocks()override{call("clocks");}
    void update_main_blur()override{call("blur");}
    void update_scroll(const EventObject& value,bool input)override{check(value==state.scroll&&input);call("scroll");}
    bool pressed(std::uint32_t mask)override{check(mask==0x100000U);call("pressed");return confirm;}
    void play_button_sound()override{call("sound");}
    std::int32_t stamina()override{call("stamina");check(saves->at(9)==23);return balance;}
    void use_stamina(std::int32_t value)override{call("use");spent+=value;balance-=value;}
    EventObject make_stamina_dialog()override{call("stamina_dialog");return child;}
    void push_form(const EventObject& value)override{check(value==child);call("push");}
    bool soft_label_pressed(std::int32_t label)override{call("soft:"+std::to_string(label));return label==18?formation:cancel;}
    EventObject make_formation()override{call("formation");return child;}
    bool pop_form()override{call("pop");return pop_value;}
    void play_bgm()override{call("bgm");}
    void save_game(std::int32_t slot,bool option)override{check(slot==0&&option);call("save_game");}
    void save_system()override{call("save_system");}
    bool recover_save_exception(std::exception_ptr value)override{check(value!=nullptr);call("save_exception");return recover_save;}
    void reset_town_camera()override{call("reset_camera");}
    void change_to_quest()override{call("quest");}
    void base_set_touch_value(std::uint32_t flags,std::int32_t value)override{fallback_flags=flags;check(value==7);call("set_base");}
    std::int32_t base_get_touch_value(std::uint32_t flags,std::int32_t fallback)override{fallback_flags=flags;call("get_base");return fallback+1;}
    void scroll_camera_touch(const EventObject& value,std::int32_t type,std::int32_t id,const StageMenuTouch& event)override{check(value==state.scroll&&type==8&&id==19);forwarded_touch=&event;call("camera");}
    void scroll_list_touch(const EventObject& value,std::int32_t type,std::int32_t id,const StageMenuTouch& event)override{check(value==state.scroll&&type==8&&id==19);forwarded_touch=&event;call("list");}
    void base_resume()override{call("resume");}
    void base_finish()override{call("finish");}
    void end_main_blur()override{call("end_blur");}
};
void initialization(){
    StageMenuState state;Host host(state);state.scroll_value=81;
    const auto old=std::make_shared<EventIntegers>(EventIntegers{99});state.entries=old;
    check(initialize_stage_menu(state,host));
    check(state.state==0&&state.selected==-1&&state.area==1&&state.frame==0&&state.production_frame==0&&state.scroll_value==81);
    check(*state.entries==EventIntegers({3,0})&&state.entries!=old&&*old==EventIntegers({99}));
    check(state.scroll==host.created_scroll&&state.renderer==host.created_renderer);
    check(host.specification.integers==std::array<std::int32_t,8>{{1,3,2,2,40,5,0x20000,0}});
    check(host.specification.option&&!host.specification.listener&&host.quest_queries==7);
    host.debug_value=true;check(initialize_stage_menu(state,host));check(*state.entries==EventIntegers({3,1,0}));
}
void initialization_failures(){
    StageMenuState state;Host host(state);const auto old=std::make_shared<int>(42);
    state.scroll=state.renderer=old;host.failure="flags";
    fails([&]{initialize_stage_menu(state,host);});
    check(*state.entries==EventIntegers({3,0})&&state.scroll==old&&state.renderer==old);
    host.failure.clear();host.quest_rows->at(0).reset();
    fails([&]{initialize_stage_menu(state,host);});check(*state.entries==EventIntegers({3}));
    host.area_rows->at(1)->stages.reset();fails([&]{initialize_stage_menu(state,host);});check(state.entries->empty());
    state.state=1;host.failure="labels:18:3";fails([&]{change_stage_menu_state(state,0,host);});check(state.state==1);
    host.failure.clear();fails([&]{change_stage_menu_state(state,-1,host);});check(state.state==1);
}
void live_catalog(){
    StageMenuState state;Host host(state);
    host.on_quests=[&]{if(host.quest_queries==2){host.quest_rows=std::make_shared<QuestTable>(*host.quest_rows);host.quest_rows->at(3)->discovery_state=0;}};
    initialize_stage_menu(state,host);check(*state.entries==EventIntegers({0}));
}
void confirmation_and_production(){
    StageMenuState state;Host host(state);initialize_stage_menu(state,host);host.calls.clear();state.selected=0;host.confirm=true;
    check(update_stage_menu(state,host,{11,22}));
    check(state.error_function==11&&state.error_line==22&&state.state==1&&state.production_frame==0&&host.spent==5&&host.saves->at(9)==23);
    check(host.calls==std::vector<std::string>({"base","reset","clocks","blur","scroll","pressed","quests","sound","save","stamina","use","labels:0:0","soft:18","soft:3","bgm"}));
    host.calls.clear();state.production_frame=39;update_stage_menu(state,host,{0,0});
    check(state.production_frame==40&&host.calls==std::vector<std::string>({"base"}));
    host.calls.clear();host.quest_rows->at(3).reset();update_stage_menu(state,host,{0,0});
    check(state.production_frame==41&&host.calls==std::vector<std::string>({"base","quests","save_game","save_system","reset_camera","quest"}));
    host.calls.clear();state.production_frame=40;host.failure="save_system";
    fails([&]{update_stage_menu(state,host,{0,0});});
    check(state.production_frame==41&&host.calls.back()=="save_exception"&&state.state==1);
    host.calls.clear();state.production_frame=40;host.recover_save=true;
    update_stage_menu(state,host,{0,0});
    check(host.calls==std::vector<std::string>({"base","quests","save_game","save_system","save_exception","reset_camera","quest"}));
}
void insufficient_and_continuation(){
    StageMenuState state;Host host(state);initialize_stage_menu(state,host);state.selected=0;host.confirm=true;host.balance=4;host.calls.clear();
    update_stage_menu(state,host,{0,0});
    check(state.state==0&&host.spent==0&&host.saves->at(9)==23);
    check(std::find(host.calls.begin(),host.calls.end(),"stamina_dialog")!=host.calls.end());
    host.balance=10;host.formation=host.cancel=true;host.calls.clear();
    check(!update_stage_menu(state,host,{0,0}));
    check(state.state==1&&host.spent==5&&host.calls.back()=="pop");
    check(std::find(host.calls.begin(),host.calls.end(),"formation")!=host.calls.end());
    check(std::find(host.calls.begin(),host.calls.end(),"bgm")==host.calls.end());
}
void touches_and_lifecycle(){
    StageMenuState state;Host host(state);initialize_stage_menu(state,host);
    set_stage_menu_touch_value(state,0x20071U,8,host);set_stage_menu_touch_value(state,0x40015U,9,host);
    check(get_stage_menu_touch_value(state,0x20002U,-5,host)==8&&get_stage_menu_touch_value(state,0x400ffU,-5,host)==9);
    set_stage_menu_touch_value(state,0x500feU,7,host);check(host.fallback_flags==0x50000U);
    check(get_stage_menu_touch_value(state,0x50011U,7,host)==8&&host.fallback_flags==0x50000U);
    InputControl control{17,19};StageMenuTouch event{{8,&control},1,2,3,4,5};
    check(!stage_menu_touch(state,&event,host)&&host.forwarded_touch==&event&&host.calls.back()=="camera");
    control.group=10;check(stage_menu_touch(state,&event,host)&&host.calls.back()=="list");
    state.scroll.reset();check(stage_menu_touch(state,&event,host));control.group=17;check(!stage_menu_touch(state,&event,host));
    fails([&]{stage_menu_touch(state,nullptr,host);});event.input.component=nullptr;fails([&]{stage_menu_touch(state,&event,host);});
    host.calls.clear();check(resume_stage_menu(host)&&finish_stage_menu(host));
    check(host.calls==std::vector<std::string>({"resume","finish","end_blur"}));
}
void update_failures_and_widths(){
    StageMenuState state;Host host(state);initialize_stage_menu(state,host);host.calls.clear();host.failure="base";
    fails([&]{update_stage_menu(state,host,{9,7});});check(state.error_function==9&&state.error_line==7&&state.frame==0);
    host.failure.clear();state.frame=state.loop_counter=std::numeric_limits<std::int32_t>::max();
    update_stage_menu(state,host,{0,0});check(state.frame==std::numeric_limits<std::int32_t>::min()&&state.loop_counter==-1);
    state.state=1;state.production_frame=std::numeric_limits<std::int32_t>::max();host.calls.clear();
    update_stage_menu(state,host,{0,0});check(state.production_frame==std::numeric_limits<std::int32_t>::min()&&host.calls==std::vector<std::string>({"base"}));
}
void neutral_scroll(){
    for(auto count:{0,1,2,3,5}){
        StageNeutralScroll state;state.count=count;std::vector<std::uint32_t> calls;
        const auto key=[&](std::uint32_t mask,bool repeat){calls.push_back(mask|(repeat?0x80000000U:0));return false;};
        for(int i=0;i<20;++i){update_neutral_stage_scroll(state,true,false,key);if(i<10)check(state.color==255);}
        check(state.color==128&&state.fade_progress==10&&state.first_visible==0&&state.offset_y==0);
        check(calls.size()==120&&std::equal(calls.begin(),calls.begin()+6,std::vector<std::uint32_t>{0x80080000U,0x80020000U,0x80040000U,0x80010000U,0x80000U,0x20000U}.begin()));
    }
    StageNeutralScroll state;state.count=5;state.offset_y=-200;state.damping=49;
    update_neutral_stage_scroll(state,false,false,[](auto,bool){return false;});
    check(state.first_visible==2&&state.offset_y==-90&&state.damping==34&&state.fade_progress==-9);
    fails([&]{update_neutral_stage_scroll(state,true,false,[](auto,bool repeat){return repeat;});});
    check(state.fade_progress==-9);
}
}
int main(){initialization();initialization_failures();live_catalog();confirmation_and_production();insufficient_and_continuation();touches_and_lifecycle();update_failures_and_widths();neutral_scroll();}
