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
struct StaminaHost final:StageStaminaHost {
    std::shared_ptr<StageStaminaValues> values=std::make_shared<StageStaminaValues>(6,0);
    std::vector<std::string> calls;
    std::vector<std::int32_t> limits{10,10,10};
    std::size_t limit_queries=0;
    std::function<void(std::size_t)> on_values,on_limit;
    std::function<void()> on_clock;
    std::size_t value_queries=0;
    std::int64_t clock=123456789;
    std::shared_ptr<StageStaminaValues> stamina_values()override{
        calls.push_back("values");++value_queries;if(on_values)on_values(value_queries);return values;
    }
    std::int32_t stamina_limit()override{
        calls.push_back("limit");++limit_queries;if(on_limit)on_limit(limit_queries);return limits.at(limit_queries-1);
    }
    std::int64_t server_time()override{calls.push_back("clock");if(on_clock)on_clock();return clock;}
};
void stamina_order(){
    StaminaHost host;host.values->at(1)=10;
    host.on_clock=[&]{check(host.values->at(1)==10&&host.values->at(5)==0);};
    host.on_limit=[&](auto n){if(n==2)check(host.values->at(1)==5&&host.values->at(5)==host.clock&&host.values->at(0)==0);};
    use_stage_stamina(5,host);
    check(*host.values==StageStaminaValues({900,5,0,0,0,host.clock}));
    check(host.calls==std::vector<std::string>({"values","limit","values","clock","values","values","limit","values","values","values","values","values"}));
    StaminaHost zero;zero.values->at(1)=10;zero.values->at(0)=-4;
    use_stage_stamina(0,zero);check(zero.values->at(1)==10&&zero.values->at(0)==0&&zero.values->at(5)==zero.clock);
    StaminaHost negative;negative.values->at(1)=9;negative.limits={10,10,20};
    use_stage_stamina(-5,negative);
    check(negative.values->at(1)==20&&negative.values->at(0)==0&&negative.values->at(5)==0&&negative.limit_queries==3);
    check(std::find(negative.calls.begin(),negative.calls.end(),"clock")==negative.calls.end());
}
void stamina_widths_and_timer(){
    const auto minimum=std::numeric_limits<std::int64_t>::min(),maximum=std::numeric_limits<std::int64_t>::max();
    struct Case{std::int64_t balance;std::int32_t cost;std::int64_t expected;};
    for(const auto sample:{Case{maximum,-1,0},Case{minimum,1,10},Case{minimum,-1,0},
            Case{0,std::numeric_limits<std::int32_t>::min(),10},Case{0,std::numeric_limits<std::int32_t>::max(),0},Case{3,5,0}}){
        StaminaHost host;host.values->at(1)=sample.balance;use_stage_stamina(sample.cost,host);check(host.values->at(1)==sample.expected);
    }
    for(const auto timer:{minimum,std::int64_t{-1},std::int64_t{0},std::int64_t{1},std::int64_t{900},std::int64_t{901},maximum}){
        for(const auto cost:{-1,0,1}){
            StaminaHost host;host.values->at(1)=5;host.values->at(0)=timer;use_stage_stamina(cost,host);
            const auto expected=timer>900?900:timer<=0?(cost>0?900:0):timer;
            check(host.values->at(0)==expected);
        }
    }
    // Native comparisons zero-extend capacity, but the final SecureLong(int32) store sign-extends it.
    StaminaHost limits;limits.values->at(1)=10;limits.limits={-1,-1,10};
    use_stage_stamina(0,limits);check(limits.limit_queries==2&&limits.values->at(5)==0&&limits.values->at(1)==10);
    StaminaHost replacement;replacement.values->at(1)=9;replacement.limits={10,10,-1};
    use_stage_stamina(-5,replacement);check(replacement.values->at(1)==-1);
}
void stamina_live_storage(){
    StaminaHost first;first.values->at(1)=3;const auto old=first.values;
    first.on_limit=[&](auto n){if(n==1){first.values=std::make_shared<StageStaminaValues>(6,0);first.values->at(1)=10;}};
    use_stage_stamina(2,first);check(old->at(1)==3&&first.values->at(1)==8&&first.values->at(5)==0);
    StaminaHost clock;clock.values->at(1)=10;const auto clock_destination=clock.values;
    clock.on_clock=[&]{clock.values=std::make_shared<StageStaminaValues>(6,0);clock.values->at(1)=8;};
    use_stage_stamina(3,clock);
    check(clock_destination->at(1)==10&&clock_destination->at(5)==clock.clock&&clock.values->at(1)==5&&clock.values->at(5)==0);
    StaminaHost cap;cap.values->at(1)=9;const auto cap_destination=cap.values;
    cap.on_limit=[&](auto n){
        if(n==2)check(cap_destination->at(1)==14);
        if(n==3){cap.values=std::make_shared<StageStaminaValues>(6,0);cap.values->at(1)=7;cap.values->at(0)=901;}
    };
    use_stage_stamina(-5,cap);
    check(cap_destination->at(1)==10&&cap_destination->at(0)==0&&cap.values->at(1)==7&&cap.values->at(0)==900);
}
void stamina_failures(){
    StaminaHost missing;missing.values.reset();fails([&]{use_stage_stamina(1,missing);});
    check(missing.calls==std::vector<std::string>({"values"}));
    StaminaHost limit;limit.values->at(1)=10;
    limit.on_limit=[](auto){throw std::runtime_error("limit");};
    fails([&]{use_stage_stamina(5,limit);});check(limit.values->at(1)==10&&limit.values->at(5)==0);
    StaminaHost clock;clock.values->at(1)=10;clock.on_clock=[] {throw std::runtime_error("clock");};
    fails([&]{use_stage_stamina(5,clock);});check(clock.values->at(1)==10&&clock.values->at(5)==0);
    StaminaHost after_store;after_store.values->at(1)=10;
    after_store.on_limit=[](auto n){if(n==2)throw std::runtime_error("limit2");};
    fails([&]{use_stage_stamina(12,after_store);});
    check(after_store.values->at(1)==0&&after_store.values->at(5)==after_store.clock&&after_store.values->at(0)==0);
    StaminaHost cap;cap.values->at(1)=9;
    cap.on_limit=[](auto n){if(n==3)throw std::runtime_error("limit3");};
    fails([&]{use_stage_stamina(-5,cap);});check(cap.values->at(1)==14&&cap.values->at(0)==0);
    // Array validation follows the clock/third-limit callback for captured destinations.
    StaminaHost null_clock;null_clock.values->at(1)=10;
    null_clock.on_values=[&](auto n){if(n==2)null_clock.values.reset();};
    fails([&]{use_stage_stamina(5,null_clock);});check(null_clock.calls.back()=="clock");
    StaminaHost short_clock;short_clock.values->resize(2);short_clock.values->at(1)=10;
    fails([&]{use_stage_stamina(5,short_clock);});check(short_clock.calls.back()=="clock"&&short_clock.values->at(1)==10);
    StaminaHost null_cap;null_cap.values->at(1)=9;
    null_cap.on_limit=[&](auto n){if(n==2)null_cap.values->at(1)=14;};
    null_cap.on_values=[&](auto n){if(n==5)null_cap.values.reset();};
    fails([&]{use_stage_stamina(-5,null_cap);});check(null_cap.calls.back()=="limit"&&null_cap.limit_queries==3);
}
struct DrawHost final:StageMenuDrawHost {
    std::vector<std::string> calls;
    std::vector<StageMenuRowRegion> rows;
    std::vector<InputRectangle> clips;
    std::vector<std::array<std::int32_t,3>> sprites;
    std::shared_ptr<AreaTable> area_rows=std::make_shared<AreaTable>(1);
    std::shared_ptr<QuestTable> quest_rows=std::make_shared<QuestTable>(5);
    std::int32_t view_height=640,maximum=3;
    bool first=false;
    std::string failure;
    CameraTouchRegion camera;
    InputRectangle bar;
    std::array<std::int32_t,4> bar_values{};
    std::array<std::int32_t,2> origin{{180,200}};
    std::function<void()> on_background;
    DrawHost(){for(int i=0;i<5;++i){quest_rows->at(i)=QuestDefinition{};auto& q=*quest_rows->at(i);q.name="quest"+std::to_string(i);q.stage.base_level=10+i;}}
    void call(std::string name){calls.push_back(name);if(failure==name)throw std::runtime_error(name);}
    std::int32_t raw_width()override{call("width");return 600;}
    std::int32_t raw_height()override{call("height");return view_height;}
    std::int32_t center_x()override{call("center");return 120;}
    std::shared_ptr<AreaTable> areas()override{call("areas");return area_rows;}
    std::shared_ptr<QuestTable> quests()override{call("quests");return quest_rows;}
    void draw_scroll_bar(InputRectangle r,std::int32_t total,std::int32_t visible,std::int32_t first_row,std::int32_t color)override{
        call("bar");bar=r;bar_values={total,visible,first_row,color};}
    bool first_camera_touch()override{call("first");return first;}
    void add_camera_touch(const CameraTouchRegion& r)override{call("camera");camera=r;}
    void push_clip(InputRectangle r)override{call("clip");clips.push_back(r);}
    void pop_clip()override{call("pop_clip");}
    void validate_row_bounds()override{call("bounds");}
    void draw_row_background(std::int32_t x,std::int32_t y)override{call("background");sprites.push_back({0,x,y});if(on_background)on_background();}
    void set_row_color()override{call("color");}
    void draw_quest_name(const std::string& name,std::int32_t x,std::int32_t y)override{call(name);sprites.push_back({1,x,y});}
    void draw_level(std::int32_t level,std::int32_t x,std::int32_t y)override{call("level:"+std::to_string(level));sprites.push_back({2,x,y});}
    void draw_completed(std::int32_t x,std::int32_t y)override{call("completed");sprites.push_back({3,x,y});}
    std::int32_t pending_max_frame()override{call("maximum");return maximum;}
    void draw_pending(std::int32_t frame,std::int32_t x,std::int32_t y)override{call("pending:"+std::to_string(frame));sprites.push_back({4,x,y});}
    void add_row_touch(const StageMenuRowRegion& row)override{call("touch");rows.push_back(row);}
    void begin_stage_draw()override{call("begin");}
    std::array<std::int32_t,2> draw_origin()override{call("origin");return origin;}
    void set_draw_origin(std::int32_t x,std::int32_t y)override{call("set_origin");origin={x,y};}
    void draw_heading(std::int32_t center)override{check(center==120);call("heading");}
    void draw_selected_details(const StageMenuState& state)override{check(state.selected>=0);call("details");}
    void end_stage_draw(const StageMenuState&)override{call("end");}
};
StageMenuState draw_state(){
    StageMenuState state;state.selected=-1;state.scroll=std::make_shared<int>(1);state.renderer=std::make_shared<int>(2);
    state.entries=std::make_shared<EventIntegers>(EventIntegers{4,0,2,1,3});return state;
}
void drawing_rows(){
    auto state=draw_state();state.frame=20;DrawHost host;
    host.quest_rows->at(4)->stage.completed=1;
    draw_stage_menu_contents(state,0,999,32,host);
    check(host.calls==std::vector<std::string>({"areas","bounds","center","quests","background","color","quest4","level:14","completed"}));
    check(host.sprites==std::vector<std::array<std::int32_t,3>>({{0,15,32},{1,40,46},{2,125,46},{3,181,53}}));
    host.calls.clear();host.sprites.clear();draw_stage_menu_contents(state,1,-500,77,host);
    check(host.calls.back()=="pending:2"&&host.sprites.back()==std::array<std::int32_t,3>({4,184,90}));
    host.calls.clear();host.maximum=0;draw_stage_menu_contents(state,1,0,77,host);check(host.calls.back()=="pending:5");
    state.frame=-20;host.maximum=3;draw_stage_menu_contents(state,1,0,77,host);check(host.calls.back()=="pending:-2");
}
void drawing_list(){
    auto state=draw_state();StageNeutralScroll scroll;scroll.count=5;DrawHost host;
    draw_stage_menu_list(state,scroll,15,32,host);
    check(host.calls.front()=="height"&&host.calls.back()=="pop_clip"&&host.rows.size()==3);
    check(host.bar.x==228&&host.bar.y==32&&host.bar.width==6&&host.bar.height==130);
    check(host.bar_values==std::array<std::int32_t,4>({5,3,0,255}));
    check(host.camera.bounds.x==15&&host.camera.bounds.y==32&&host.camera.bounds.height==130&&host.camera.flags==1);
    check(host.clips.at(0).height==131);
    for(std::size_t i=0;i<host.rows.size();++i){const auto& row=host.rows[i];
        check(row.control.group==10&&row.control.value==static_cast<std::int32_t>(0x20000+i)&&row.option_flags==5&&row.priority==2);
        check(row.bounds.x==15&&row.bounds.y==32+45*static_cast<std::int32_t>(i)&&row.bounds.width==210&&row.bounds.height==40);
        check(row.margins==std::array<std::int32_t,4>({0,0,0,0}));}
    auto touch=std::find(host.calls.begin(),host.calls.end(),"touch");check(touch!=host.calls.end()&&*(touch-1)=="pending:0");
    DrawHost shifted;scroll.offset_y=-5;draw_stage_menu_list(state,scroll,15,32,shifted);
    check(shifted.rows.size()==4&&shifted.rows[0].bounds.y==30&&shifted.rows[0].bounds.height==37);
    check(shifted.rows[3].bounds.y==162&&shifted.rows[3].bounds.height==2);
    DrawHost culled;scroll.offset_y=-41;draw_stage_menu_list(state,scroll,15,32,culled);
    check(culled.rows.front().control.value==0x20001);
}
void drawing_limits_and_empty(){
    auto state=draw_state();StageNeutralScroll scroll;scroll.count=0;DrawHost host;host.view_height=150;host.first=true;
    draw_stage_menu_list(state,scroll,15,32,host);
    check(scroll.maximum_height==76&&host.bar.height==76&&host.clips[0].height==76&&host.rows.empty());
    check(host.calls==std::vector<std::string>({"height","height","bar","first","camera","clip","pop_clip"}));
    check(host.camera.bounds.x==-1000&&host.camera.bounds.width==2000&&host.camera.flags==32);
    host.view_height=640;draw_stage_menu_list(state,scroll,15,32,host);check(scroll.maximum_height==76);
}
void drawing_failures_and_caller(){
    auto state=draw_state();StageNeutralScroll scroll;scroll.count=1;DrawHost host;
    state.area=1;fails([&]{draw_stage_menu_contents(state,0,0,0,host);});check(host.calls==std::vector<std::string>({"areas"}));
    state.area=0;host.calls.clear();host.quest_rows->at(4).reset();
    fails([&]{draw_stage_menu_contents(state,0,0,0,host);});
    check(host.calls==std::vector<std::string>({"areas","bounds","center","quests","background","color"}));
    DrawHost invalid;state.entries->at(0)=99;fails([&]{draw_stage_menu_contents(state,0,0,0,invalid);});
    check(invalid.calls.back()=="quests"&&invalid.sprites.empty());state.entries->at(0)=4;
    DrawHost missing_renderer;const auto renderer=state.renderer;state.renderer.reset();
    fails([&]{draw_stage_menu_list(state,scroll,15,32,missing_renderer);});
    check(missing_renderer.calls.back()=="clip"&&missing_renderer.rows.empty());state.renderer=renderer;
    DrawHost broken;broken.failure="quest4";fails([&]{draw_stage_menu_list(state,scroll,15,32,broken);});
    check(broken.rows.empty()&&std::find(broken.calls.begin(),broken.calls.end(),"pop_clip")==broken.calls.end());
    DrawHost caller;caller.origin={7,9};draw_stage_menu(state,scroll,caller);
    check(caller.calls.front()=="begin"&&caller.calls.back()=="end"&&caller.origin==std::array<std::int32_t,2>({180,200}));
    check(std::find(caller.calls.begin(),caller.calls.end(),"details")==caller.calls.end());
    caller.calls.clear();caller.origin={7,9};state.selected=0;draw_stage_menu(state,scroll,caller);
    check(caller.origin==std::array<std::int32_t,2>({7,9})&&*(caller.calls.end()-3)=="details"&&*(caller.calls.end()-2)=="set_origin");
    caller.failure="details";caller.origin={7,9};fails([&]{draw_stage_menu(state,scroll,caller);});check(caller.origin==std::array<std::int32_t,2>({180,200}));
    StageMenuTouch event;InputControl control{10,0x2ffff};event.input.component=&control;
    touch_stage_scroll_list(scroll,1,nullptr);check(scroll.last_touched==-1);
    touch_stage_scroll_list(scroll,2,&event);check(scroll.last_touched==65535);
    fails([&]{touch_stage_scroll_list(scroll,0,nullptr);});
}
}
int main(){initialization();initialization_failures();live_catalog();confirmation_and_production();insufficient_and_continuation();touches_and_lifecycle();update_failures_and_widths();neutral_scroll();stamina_order();stamina_widths_and_timer();stamina_live_storage();stamina_failures();drawing_rows();drawing_list();drawing_limits_and_empty();drawing_failures_and_caller();}
