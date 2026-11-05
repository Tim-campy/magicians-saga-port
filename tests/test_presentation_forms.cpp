#include "core/gameplay/presentation_forms.hpp"
#include "core/gameplay/event_dispatch.hpp"
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace saga::core;
void check(bool ok, const char* message) {
    if (!ok) { std::cerr << message << '\n'; std::exit(1); }
}
template<class E, class F> void expect(F action, const char* message) {
    try { action(); } catch (const E&) { return; }
    check(false, message);
}
struct Host : PresentationFormHost, EventEffectsHost, EventFlowHost {
    std::vector<std::u16string> parsed;
    int translations = 0;
    EventProgression state{0, 0, 10000, {}, {}};
    EventDelayQueue queue;
    EventScript script = std::make_shared<std::vector<EventInstruction>>();
    std::vector<EventObject> pushed;
    std::shared_ptr<PreparedTalkForm> talk;
    std::shared_ptr<PreparedEventForm> event;
    NullablePresentationText translate_argument(const std::u16string& text) override {
        ++translations; return text; // Explicit test translator, not product fallback.
    }
    bool remove_duplicate_honorific() override { return false; }
    std::int32_t parse_event_type(const std::u16string& text) override {
        parsed.push_back(text);
        if (text == u"1") return 1;
        if (text == u"2 ") return 2;
        throw std::invalid_argument("test parser has no rule for this value");
    }
    EventProgression& progression() override { return state; }
    std::int32_t stamina_limit() override { throw std::logic_error("not used"); }
    std::int64_t read_stamina() override { throw std::logic_error("not used"); }
    void write_stamina(std::int64_t) override { throw std::logic_error("not used"); }
    EventObject make_talk_form(std::int32_t id, EventObject args) override {
        check(id == 0, "test talk id");
        const auto argument = std::static_pointer_cast<std::u16string>(args);
        talk = std::make_shared<PreparedTalkForm>(prepare_talk_form(3, u" hello <0> \t next ",
            argument ? NullablePresentationText(*argument) : std::nullopt, *this));
        return talk;
    }
    EventObject make_event_form(std::int32_t id, EventObject args) override {
        check(id == 0, "test event id");
        const auto argument = std::static_pointer_cast<std::u16string>(args);
        event = std::make_shared<PreparedEventForm>(prepare_event_form(u"1&<0>",
            argument ? NullablePresentationText(*argument) : std::nullopt, *this));
        return event;
    }
    void push_form(EventObject form) override { pushed.push_back(form); }
    EventObject dispatch(const EventInstruction& row, const EventContext& context) override {
        const auto result = dispatch_known_event_instruction(row, context, *this);
        if (!result.handled) throw std::logic_error("unsupported test opcode");
        return result.value;
    }
    std::int32_t identify_script(const EventScript& value) override { return value == script ? 0 : -1; }
    EventScript resolve_script(std::int32_t id) override {
        if (id != 0) throw std::out_of_range("test script id");
        return script;
    }
    std::int32_t read_talk_character() override { return 0; }
    void restore_talk_character(std::size_t, std::int32_t) override {}
};
struct PageHost final:TalkPageHost {
    std::vector<std::string> calls;
    bool pulse=false,held_enabled=false,held=false,ended=false,fail_set=false;
    std::u16string text;
    PreparedTalkForm* continuation=nullptr;
    void text_frame()override{calls.push_back("text_frame");}
    void portrait_frame()override{calls.push_back("portrait_frame");}
    bool advance_pulse()override{calls.push_back("pulse");return pulse;}
    bool held_advance_enabled()override{calls.push_back("held_enabled");return held_enabled;}
    bool advance_held()override{calls.push_back("held");return held;}
    bool text_ended()override{calls.push_back("ended");return ended;}
    void flush_text()override{calls.push_back("flush");}
    void set_text(const std::u16string& value)override{
        calls.push_back("set_text");if(fail_set)throw std::runtime_error("synthetic SetText failure");text=value;
    }
    void continue_talk(PreparedTalkForm& form)override{calls.push_back("continue");continuation=&form;}
};
struct NoticeHost final:EventNoticeHost {
    std::vector<std::string> calls;
    std::vector<bool> debug_values{false},held_values{false};
    std::size_t debug_queries=0,held_queries=0;
    bool pulse=false,cancel=false,reject_negative=false;
    std::string failure;
    std::int32_t selected=-1;
    void call(const char* name){calls.emplace_back(name);if(failure==name)throw std::runtime_error("synthetic notice failure");}
    bool debug_command_enabled()override{
        call("debug");const auto i=debug_queries++;return debug_values.at(i<debug_values.size()?i:debug_values.size()-1);
    }
    bool held_advance()override{
        call("held");const auto i=held_queries++;return held_values.at(i<held_values.size()?i:held_values.size()-1);
    }
    void frame_resource()override{call("frame");}
    bool advance_pulse()override{call("pulse");return pulse;}
    void set_event(std::int32_t page)override{
        call("set");selected=page;if(reject_negative&&page<0)throw std::out_of_range("synthetic negative page");
    }
    bool cancel_pressed()override{call("cancel");return cancel;}
};
struct PopupHost final:PopupContentsHost {
    bool pulse=false;
    std::vector<std::string> calls;
    std::function<void()> pulse_action,pop_action;
    bool popup_close_pulse()override {
        calls.push_back("pulse");if(pulse_action)pulse_action();return pulse;
    }
    void pop_popup()override {calls.push_back("pop");if(pop_action)pop_action();}
};
void test_popup_contents() {
    {PopupProductionState production;PopupHost h;std::int32_t animation=71;
        for(int i=1;i<=5;++i) {
            check(!update_popup_contents(animation,production,h),"production returns false to the enclosing SubForm.Update");
            check(production.step==(i==5?0:1)&&production.frame==(i==5?0:i),"opening clears both production fields on fifth update");
        }
        check(animation==71&&h.calls.empty(),"opening neither advances idle animation nor queries close input");
        h.pulse=true;check(update_popup_contents(animation,production,h),"idle update returns true even when close is requested");
        check(animation==72&&production.step==2&&production.frame==0&&h.calls==std::vector<std::string>{"pulse"},"close pulse changes only production step after the animation clock");
        h.calls.clear();
        for(int i=1;i<=5;++i) {
            check(!update_popup_contents(animation,production,h),"closing keeps the production return value");
            check(production.step==(i==5?0:2)&&production.frame==(i==5?0:i),"closing retains state until fifth update");
        }
        check(animation==72&&h.calls==std::vector<std::string>{"pop"},"closing calls Pop once without querying pulse");
    }
    for(const auto step:{0,3,-1,std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max()}) {
        PopupProductionState production{step,99};PopupHost h;
        check(!update_popup_production(production,h)&&production.step==step&&production.frame==99&&h.calls.empty(),"other steps leave production untouched");
        std::int32_t animation=4;
        check(update_popup_contents(animation,production,h)&&animation==5&&production.step==step&&production.frame==99&&h.calls==std::vector<std::string>{"pulse"},"all nonproduction steps run the idle branch");
    }
    {PopupProductionState production{2,4};PopupHost h;
        h.pop_action=[&]{check(production.step==2&&production.frame==5,"Pop observes incremented closing counter");production={7,80};};
        check(update_popup_production(production,h)&&production.step==0&&production.frame==0,"successful Pop mutations are overwritten by the native paired clear");
    }
    {PopupProductionState production{2,4};PopupHost h;
        h.pop_action=[]{throw std::runtime_error("Pop failure");};
        expect<std::runtime_error>([&]{update_popup_production(production,h);},"Pop failure propagates");
        check(production.step==2&&production.frame==5&&h.calls==std::vector<std::string>{"pop"},"failed Pop keeps incremented state without clearing");
        h.pop_action={};check(update_popup_production(production,h)&&h.calls.size()==2&&production.step==0&&production.frame==0,"a failed Pop is retried after another wrapped counter increment");
    }
    for(const auto step:{1,2}) {
        PopupProductionState production{step,std::numeric_limits<std::int32_t>::max()};PopupHost h;
        check(update_popup_production(production,h)&&production.step==step&&production.frame==std::numeric_limits<std::int32_t>::min()&&h.calls.empty(),"production increment wraps before the signed five-frame comparison");
    }
    {PopupProductionState production{1,8};PopupHost h;
        check(update_popup_production(production,h)&&production.step==0&&production.frame==0&&h.calls.empty(),"opening also completes from a counter beyond five");
    }
    struct Boundary {std::int32_t initial,expected;};
    for(const auto boundary:{Boundary{-1,0},Boundary{0,1},Boundary{std::numeric_limits<std::int32_t>::max()-1,0},
            Boundary{std::numeric_limits<std::int32_t>::max(),-1},Boundary{std::numeric_limits<std::int32_t>::min(),0}}) {
        PopupProductionState production{0,12};PopupHost h;auto animation=boundary.initial;
        h.pulse_action=[&]{check(animation==boundary.expected,"pulse observes wrapped animation modulo INT32_MAX");};
        check(update_popup_contents(animation,production,h)&&animation==boundary.expected&&production.frame==12,"idle clock retains native signed remainder");
    }
    {PopupProductionState production{0,3};PopupHost h;std::int32_t animation=6;h.pulse=true;
        h.pulse_action=[&]{production={9,81};};update_popup_contents(animation,production,h);
        check(production.step==2&&production.frame==81,"close pulse replaces only the live step and preserves callback counter mutations");
    }
    {PopupProductionState production{0,3};PopupHost h;std::int32_t animation=6;
        h.pulse_action=[]{throw std::runtime_error("pulse failure");};
        expect<std::runtime_error>([&]{update_popup_contents(animation,production,h);},"pulse failure propagates");
        check(animation==7&&production.step==0&&production.frame==3&&h.calls==std::vector<std::string>{"pulse"},"failed input retains the animation prefix without initiating close");
    }
}
struct KingHost final:KingHallHost {
    std::vector<std::string> calls;
    std::function<void(const std::string&)> action;
    NullablePresentationText voice=u"king speech";
    bool pulse=false,back=false,notes=false;
    std::int32_t eased=80,easing_frame=0,interval=0;
    void call(std::string name){calls.push_back(name);if(action)action(name);}
    NullablePresentationText king_hall_voice()override{call("voice");return voice;}
    void king_hall_initialize_news(std::int32_t value)override{interval=value;call("initialize_news");}
    std::int32_t king_hall_easing(std::int32_t value)override{easing_frame=value;call("easing");return eased;}
    void execute_king_rewards()override{call("rewards");}
    bool king_hall_confirm_pulse()override{call("pulse");return pulse;}
    void king_hall_play_sound()override{call("sound");}
    void king_hall_save_game()override{call("save_game");}
    void king_hall_save_system()override{call("save_system");}
    bool king_hall_soft_label(std::int32_t id)override{call("soft"+std::to_string(id));return id==3?back:notes;}
    void king_hall_push_notes()override{call("notes");}
    void king_hall_push_tapjoy()override{call("tapjoy");}
    void king_hall_pop()override{call("pop");}
    void king_hall_update_news()override{call("news");}
};
void test_king_hall() {
    {KingHallState state;state.frame=12;state.movement_frame=17;state.selection=2;KingHost h;
        initialize_king_hall(state,h);
        check(state.production_step==1&&state.entrance_x==-40&&state.speech==u"king speech"&&h.interval==60,
              "hall Init selects speech after its scalar prefix, then creates news60");
        check(state.frame==12&&state.movement_frame==17&&state.selection==2&&h.calls==std::vector<std::string>({"voice","initialize_news"}),
              "reached Init block preserves counters/selection owned by common Init or allocation");
    }
    for(const auto* failure:{"voice","initialize_news"}) {
        KingHallState state;state.speech=u"old";KingHost h;
        h.action=[&](const auto& call){if(call==failure)throw std::runtime_error(call);};
        expect<std::runtime_error>([&]{initialize_king_hall(state,h);},"hall Init callback failure propagates");
        check(state.production_step==1&&state.entrance_x==-40,"Init scalar stores survive either later failure");
        check(state.speech==(std::string(failure)=="voice"?u"old":u"king speech"),"speech changes only after a successful voice call");
    }
    {KingHallState state;KingHost h;h.voice.reset();initialize_king_hall(state,h);
        check(!state.speech&&h.calls.back()=="initialize_news","nullable voice text is retained without inventing text");}
    {KingHallState state;state.production_step=1;KingHost h;
        for(int i=1;i<=8;++i) {
            check(update_king_hall(state,h),"hall update returns true during opening");
            check(state.frame==(i==8?0:i)&&state.production_step==(i==8?0:1),"opening lasts eight updates and clears the animation counter first");
        }
        check(state.movement_frame==0&&h.calls==std::vector<std::string>(8,"news"),"opening still updates news without querying input/rewards");
    }
    {KingHallState state;KingHost h;h.back=true;state.movement_frame=9;
        update_king_hall(state,h);
        check(state.production_step==2&&state.movement_frame==10&&state.frame==0&&
            h.calls==std::vector<std::string>({"easing","pulse","soft3","sound","news"}),"back short-circuits soft11 and starts closing after sound");
        h.calls.clear();
        for(int i=0;i<8;++i)update_king_hall(state,h);
        auto expected=std::vector<std::string>(7,"news");expected.push_back("pop");expected.push_back("news");
        check(state.production_step==0&&state.frame==0&&h.calls==expected,"closing Pop precedes the final news update on its eighth pass");
    }
    for(const auto selection:{-1,0,1,2,3}) {
        KingHallState state;state.selection=selection;KingHost h;h.pulse=true;h.back=h.notes=true;
        update_king_hall(state,h);
        std::vector<std::string> expected{"easing","pulse","sound","save_game","save_system"};
        if(selection==1)expected.push_back("notes");else if(selection==2)expected.push_back("tapjoy");expected.push_back("news");
        check(state.production_step==0&&h.calls==expected,"confirm saves and routes selection without closing or checking softlabels");
    }
    {KingHallState state;KingHost h;h.notes=true;update_king_hall(state,h);
        check(h.calls==std::vector<std::string>({"easing","pulse","soft3","soft11","notes","news"})&&state.production_step==0,
              "soft11 pushes Notes without save or sound");}
    {KingHallState state;state.movement_frame=38;KingHost h;
        update_king_hall(state,h);
        check(h.easing_frame==39&&state.entrance_x==80&&h.calls==std::vector<std::string>({"easing","rewards","pulse","soft3","soft11","news"}),
              "reward grant occurs exactly at movement frame39 after publishing entrance position");
        h.calls.clear();update_king_hall(state,h);
        check(h.easing_frame==40&&h.calls==std::vector<std::string>({"easing","pulse","soft3","soft11","news"}),"subsequent frames do not repeat the reward request");
    }
    {KingHallState state;state.movement_frame=8;KingHost h;
        h.action=[&](const auto& call){if(call=="easing")state.movement_frame=39;};update_king_hall(state,h);
        check(h.easing_frame==9&&h.calls[1]=="rewards","reward equality uses the live counter after Easing");}
    {KingHallState state;state.selection=1;KingHost h;h.pulse=true;
        h.action=[&](const auto& call){if(call=="save_system")state.selection=2;};update_king_hall(state,h);
        check(h.calls.at(5)=="tapjoy","confirm routing reads selection after both saves");}
    {KingHallState state;state.production_step=2;state.frame=7;KingHost h;
        h.action=[&](const auto& call){if(call=="pop"){
            check(state.frame==0&&state.production_step==2,"Pop observes reset frame but closing step");state.frame=61;state.production_step=9;
        }};
        update_king_hall(state,h);
        check(state.frame==61&&state.production_step==0&&h.calls==std::vector<std::string>({"pop","news"}),"post-Pop step clear preserves live frame mutations");}
    {KingHallState state;state.production_step=2;state.frame=7;KingHost h;
        h.action=[](const auto& call){if(call=="pop")throw std::runtime_error(call);};
        expect<std::runtime_error>([&]{update_king_hall(state,h);},"hall Pop failure propagates");
        check(state.frame==0&&state.production_step==2&&h.calls==std::vector<std::string>{"pop"},"failed Pop retains the reset frame/closing state and skips news");}
    {KingHallState state;KingHost h;h.back=true;
        h.action=[](const auto& call){if(call=="sound")throw std::runtime_error(call);};
        expect<std::runtime_error>([&]{update_king_hall(state,h);},"back sound failure propagates");
        check(state.production_step==0&&h.calls==std::vector<std::string>({"easing","pulse","soft3","sound"}),"failed back sound prevents the closing-step store and news suffix");}
    {KingHallState state;state.production_step=1;state.frame=7;KingHost h;
        h.action=[](const auto& call){if(call=="news")throw std::runtime_error(call);};
        expect<std::runtime_error>([&]{update_king_hall(state,h);},"news failure propagates after opening completion");
        check(state.frame==0&&state.production_step==0,"news failure does not roll back completed production");}
    {KingHallState state;state.production_step=2;state.frame=std::numeric_limits<std::int32_t>::max();KingHost h;
        update_king_hall(state,h);
        check(state.frame==std::numeric_limits<std::int32_t>::min()&&state.production_step==2&&h.calls==std::vector<std::string>{"news"},"hall production increment wraps before signed eight-frame comparison");}
    {KingHallState state;state.movement_frame=std::numeric_limits<std::int32_t>::max();KingHost h;update_king_hall(state,h);
        check(state.movement_frame==std::numeric_limits<std::int32_t>::min()&&h.easing_frame==state.movement_frame,"idle movement counter wraps without modulo");}
    const std::vector<std::string> order{"easing","rewards","pulse","sound","save_game","save_system","notes","news"};
    for(const auto& failure:order) {
        KingHallState state;state.movement_frame=38;state.entrance_x=7;state.selection=1;KingHost h;h.pulse=true;
        h.action=[&](const auto& call){if(call==failure)throw std::runtime_error(call);};
        expect<std::runtime_error>([&]{update_king_hall(state,h);},"hall callback failure propagates");
        std::vector<std::string> prefix;
        for(const auto& call:order){prefix.push_back(call);if(call==failure)break;}
        check(h.calls==prefix&&state.movement_frame==39&&state.production_step==0,"hall failure retains only its ordered prefix");
        check(state.entrance_x==(failure=="easing"?7:80),"position store follows successful Easing only");
    }
}
struct ConstructionHost final:BuildListHost {
    std::vector<std::string> calls;
    std::function<void(const std::string&)> action;
    bool first=false,second=false,optional=false,back=false,affordable=true;
    std::int32_t stock=2,price_calls=0,checked=0,subtracted=0,installed_slot=-1,installed_id=-1,completed_id=-1,stock_id=-1;
    std::vector<std::int32_t> prices{10,20};
    void call(std::string name){calls.push_back(name);if(action)action(name);}
    void fill_build_list(std::vector<std::int32_t>& rows)override{rows.push_back(7);call("fill");rows.push_back(8);}
    bool build_key_pulse(std::int32_t mask)override{
        if(mask==0x02000000){call("confirm25");return first;}
        if(mask==0x100000){call("confirm20");return second;}
        check(mask==0x04000000,"only the native optional key is queried");call("optional");return optional;
    }
    std::int32_t build_price(std::int32_t id)override{
        check(id==7,"selection maps through the ID list");++price_calls;call("price"+std::to_string(price_calls));return prices.at(static_cast<std::size_t>(price_calls-1));
    }
    bool check_build_crystals(std::int64_t price)override{checked=static_cast<std::int32_t>(price);call("check");return affordable;}
    void subtract_build_crystals(std::int32_t price)override{subtracted=price;call("subtract");}
    void install_building(std::int32_t slot,std::int32_t id)override{installed_slot=slot;installed_id=id;call("install");}
    void build_sound()override{call("sound");}
    void push_building_complete(std::int32_t id)override{completed_id=id;call("complete");}
    std::int32_t& construction_stock(std::int32_t id)override{stock_id=id;call("stock");return stock;}
    void pop_build_list()override{call("pop");}
    bool build_back_pressed()override{call("back");return back;}
    void push_build_tapjoy()override{call("tapjoy");}
};
BuildListState selected_build_list(){BuildListState state;state.selected=0;state.town_slot=3;state.entries=std::make_shared<std::vector<std::int32_t>>(std::initializer_list<std::int32_t>{7,8});return state;}
struct CompleteHost final:BuildingCompleteHost {
    std::vector<std::string> calls;std::function<void(const std::string&)> action;bool close=false;
    void call(std::string name){calls.push_back(name);if(action)action(name);}
    void completion_sound()override{call("sound");}
    bool completion_close_pulse()override{call("pulse");return close;}
    void pop_building_complete()override{call("pop");}
};
void test_construction_forms() {
    {auto state=selected_build_list();state.production_step=9;state.frame=6;const auto old=state.entries;ConstructionHost h;
        initialize_build_list(state,h);
        check(state.entries!=old&&*old==std::vector<std::int32_t>({7,8})&&*state.entries==std::vector<std::int32_t>({7,8}),"Init replaces the list rather than editing the old alias");
        check(state.selected==-1&&state.production_step==1&&state.town_slot==3&&state.frame==6,"Init tail selects none and starts production while preserving factory/common fields");}
    {auto state=selected_build_list();state.production_step=9;const auto old=state.entries;ConstructionHost h;
        h.action=[](const auto& call){if(call=="fill")throw std::runtime_error(call);};
        expect<std::runtime_error>([&]{initialize_build_list(state,h);},"eligibility failure propagates");
        check(state.entries!=old&&*state.entries==std::vector<std::int32_t>{7}&&state.selected==0&&state.production_step==9,"fresh list and partial choices are published before a failed Init tail");}
    {auto state=selected_build_list();ConstructionHost h;h.first=true;
        check(update_build_list(state,h),"build update returns true");
        check(h.calls==std::vector<std::string>({"confirm25","price1","check","price2","subtract","install","sound","complete","stock","pop","optional","back"}),"commit keeps price/check/debit/install/push/consume/parent-Pop order and continues input suffix");
        check(h.checked==10&&h.subtracted==20&&h.installed_slot==3&&h.installed_id==7&&h.completed_id==7&&h.stock_id==7&&h.stock==1,"check and debit use distinct live prices and captured building identity");}
    {auto state=selected_build_list();state.production_step=1;ConstructionHost h;h.first=true;update_build_list(state,h);
        check(state.frame==1&&state.production_step==1&&h.completed_id==7,"production does not suppress confirmation");}
    {auto state=selected_build_list();ConstructionHost h;h.second=true;h.affordable=false;update_build_list(state,h);
        check(h.calls==std::vector<std::string>({"confirm25","confirm20","price1","check","optional","back"})&&h.stock==2&&h.installed_id==-1,
              "insufficient balance has no debit, install, sound, dialog or Pop");}
    {BuildListState state;state.selected=-1;ConstructionHost h;h.first=h.optional=true;h.back=true;update_build_list(state,h);
        check(h.calls==std::vector<std::string>({"confirm25","optional","tapjoy","back","sound"})&&state.production_step==2,"no selection can open optional Tapjoy before back starts closing");}
    {auto state=selected_build_list();ConstructionHost h;h.optional=true;update_build_list(state,h);
        check(h.calls==std::vector<std::string>({"confirm25","confirm20","optional","back"}),"optional action is ignored while an entry is selected");}
    {auto state=selected_build_list();ConstructionHost h;h.first=true;h.optional=true;
        h.action=[&](const auto& call){if(call=="subtract")state.town_slot=11;if(call=="complete"){state.selected=-1;h.stock=7;}};
        update_build_list(state,h);
        check(h.installed_slot==11&&h.stock==6&&h.calls.at(h.calls.size()-2)=="tapjoy","install reads live town slot and stock after callbacks; suffix reads live selection");}
    {auto state=selected_build_list();ConstructionHost h;h.first=true;h.stock=std::numeric_limits<std::int32_t>::min();update_build_list(state,h);
        check(h.stock==std::numeric_limits<std::int32_t>::max(),"stock decrement wraps int32");}
    {BuildListState state;state.production_step=2;state.frame=7;state.selected=-1;ConstructionHost h;
        h.action=[&](const auto& call){if(call=="pop"){check(state.frame==0&&state.production_step==2,"closing Pop sees the precleared frame");state.frame=77;}};
        update_build_list(state,h);
        check(state.frame==77&&state.production_step==0&&h.calls==std::vector<std::string>({"pop","confirm25","confirm20","optional","back"}),"production Pop still reaches input and preserves callback frame mutations");}
    {BuildListState state;state.production_step=2;state.frame=7;ConstructionHost h;
        h.action=[](const auto& call){if(call=="pop")throw std::runtime_error(call);};
        expect<std::runtime_error>([&]{update_build_list(state,h);},"closing Pop failure propagates");
        check(state.frame==0&&state.production_step==2&&h.calls==std::vector<std::string>{"pop"},"failed closing Pop skips its state clear and all input");}
    {BuildListState state;state.selected=-1;state.production_step=1;state.frame=std::numeric_limits<std::int32_t>::max();ConstructionHost h;update_build_list(state,h);
        check(state.frame==std::numeric_limits<std::int32_t>::min()&&state.production_step==1,"build production wraps before its signed threshold");}
    {BuildListState state;state.selected=0;ConstructionHost h;h.first=true;
        expect<std::invalid_argument>([&]{update_build_list(state,h);},"null choice owner fails after confirm");
        check(h.calls==std::vector<std::string>{"confirm25"},"missing list cannot query price");}
    const std::vector<std::string> commit{"confirm25","price1","check","price2","subtract","install","sound","complete","stock","pop","optional","back"};
    for(std::size_t failure=0;failure<commit.size();++failure) {
        auto state=selected_build_list();ConstructionHost h;h.first=true;
        h.action=[&](const auto& call){if(call==commit[failure])throw std::runtime_error(call);};
        expect<std::runtime_error>([&]{update_build_list(state,h);},"construction callback failure propagates");
        check(h.calls==std::vector<std::string>(commit.begin(),commit.begin()+static_cast<std::ptrdiff_t>(failure+1)),"construction retains only completed call prefix");
        check(h.stock==(failure>8?1:2),"stock changes only after successful stock lookup and is retained across Pop/suffix failures");
    }
    {BuildingCompleteState state{7,9,22};initialize_building_complete(state);
        check(state.building_id==7&&state.frame==22&&state.production_step==1,"complete Init sets only production step");}
    {MagicShopNoticeState state{1,0};CompleteHost h;h.close=true;
        for(int frame=1;frame<=19;++frame)update_magic_shop_notice(state,h);
        check(h.calls.empty()&&state.production_step==0,"magic purchase opens at6 without construction sound at15");
        update_magic_shop_notice(state,h);check(h.calls==std::vector<std::string>{"sound"},"magic purchase sound at20");
        for(int frame=21;frame<=31;++frame)update_magic_shop_notice(state,h);
        check(h.calls==std::vector<std::string>({"sound","pulse","pop"}),"magic purchase closes only after31 on acknowledgement");}
    {BuildingCompleteState state;initialize_building_complete(state);CompleteHost h;
        for(int frame=1;frame<=30;++frame){update_building_complete(state,h);check(state.frame==frame&&state.production_step==(frame<6?1:0),"complete opening retains the total frame clock");}
        check(h.calls==std::vector<std::string>{"sound"},"complete plays sound once at15 and accepts no early close input");
        update_building_complete(state,h);check(h.calls==std::vector<std::string>({"sound","pulse"}),"frame31 starts querying close input");
        h.close=true;update_building_complete(state,h);check(h.calls==std::vector<std::string>({"sound","pulse","pulse","pop"}),"complete confirmation Pops immediately without another animation");}
    {BuildingCompleteState state{7,2,5};CompleteHost h;update_building_complete(state,h);
        check(state.production_step==0&&state.frame==6&&h.calls.empty(),"complete step2 is cleared like opening, without an implicit Pop");}
    {BuildingCompleteState state{7,0,14};CompleteHost h;h.close=true;h.action=[&](const auto& call){if(call=="sound")state.frame=31;};
        update_building_complete(state,h);check(h.calls==std::vector<std::string>({"sound","pulse","pop"}),"completion gate rereads the frame after sound");}
    for(const auto* failure:{"sound","pulse","pop"}) {
        BuildingCompleteState state{7,0,std::string(failure)=="sound"?14:30};CompleteHost h;h.close=true;
        h.action=[&](const auto& call){if(call==failure)throw std::runtime_error(call);};
        expect<std::runtime_error>([&]{update_building_complete(state,h);},"completion callback failure propagates");
        const auto expected=std::string(failure)=="sound"?std::vector<std::string>{"sound"}:std::string(failure)=="pulse"?std::vector<std::string>{"pulse"}:std::vector<std::string>{"pulse","pop"};
        check(h.calls==expected&&state.frame==(std::string(failure)=="sound"?15:31),"completion failure retains incremented frame and stops its suffix");
    }
}
int main() {
    test_construction_forms();
    test_king_hall();
    test_popup_contents();
    Host h;
    std::vector<std::string> decoded_tokens;
    const PresentationStringDecoder decode=[&](std::string_view token){
        decoded_tokens.emplace_back(token);
        if(token=="bad")throw std::invalid_argument("synthetic decoding failure");
        if(token=="unicode")return std::u16string(u"\u00e9\U0001f600");
        return std::u16string(token.begin(),token.end()); // Synthetic ASCII fixture only.
    };
    {
        const auto rows=load_talk_table("\xef\xbb\xbf" "2\t7\t3\t a \t\tlast\r\n0\t-1\t0\n2\t9\t2\tunicode\t\n",false,parse_event_decimal,decode);
        check(rows.size()==3&&rows[0]&&!rows[1]&&rows[2]&&rows[2]->id==2&&rows[2]->character_index==9,
              "talk table preserves id gaps and last duplicate");
        check(rows[0]->texts&&rows[0]->texts->empty()&&join_talk_text(*rows[0]).empty(),
              "zero count supplies a nonnull empty array and empty join");
        check(join_talk_text(*rows[2])==u"\u00e9\U0001f600\t"&&decoded_tokens==std::vector<std::string>({" a ","","last\r","unicode",""}),
              "loader delegates decoding, keeps whitespace and joins empty trailing text");
        auto prepared=prepare_talk_form(rows[2]->character_index,join_talk_text(*rows[2]),{},h);
        check(prepared.character_index==9&&prepared.texts==std::vector<std::u16string>({u"\u00e9\U0001f600",u""}),
              "table text reaches the existing talk factory with its page boundary");
        const auto raw=load_talk_table("0\t3\t1\tpage\r\n",false,parse_event_decimal,decode);
        const auto normalized=load_talk_table("0\t3\t1\tpage\r\n",true,parse_event_decimal,decode);
        check(raw[0]->texts->at(0)==u"page\r"&&normalized[0]->texts->at(0)==u"page",
              "talk reader uses the explicit table CRLF normalization gate");
        expect<std::out_of_range>([&]{load_talk_table("-1\t0\t0",false,parse_event_decimal,decode);},
                                  "negative talk id fails catalog placement");
        expect<std::out_of_range>([&]{load_talk_table("0\t1\t2\tfirst",false,parse_event_decimal,decode);},
                                  "truncated counted strings fail");
    }
    {
        TalkDefinition row;load_talk_definition(row,"4\t8\t2\tfirst\tsecond\tignored",parse_event_decimal,decode);
        auto previous=row.texts;
        check(join_talk_text(row)==u"first\tsecond","talk reader ignores suffix tokens");
        load_talk_definition(row,std::nullopt,parse_event_decimal,decode);
        load_talk_definition(row,"",parse_event_decimal,decode);
        check(row.id==4&&row.character_index==8&&row.texts==previous,"null or empty talk input preserves receiver");
        expect<std::invalid_argument>([&]{load_talk_definition(row,"5\t9\t2\tnew\tbad",parse_event_decimal,decode);},
                                      "decoder failure propagates");
        check(row.id==5&&row.character_index==9&&row.texts==previous,
              "later array failure preserves scalar writes and previous array identity");
        load_talk_definition(row,"6\t-1\t-7\tignored",parse_event_decimal,decode);
        check(row.id==6&&row.character_index==-1&&!row.texts,"any negative string count means null");
        expect<std::invalid_argument>([&]{join_talk_text(row);},"null talk array fails join");
    }
    {
        EventMessageDefinition row;
        check(row.id==-1&&row.type==0&&row.text.empty(),"event message defaults");
        decoded_tokens.clear();
        const auto rows=load_event_message_table("\xef\xbb\xbf" "2\t-8\tunicode\tignored\n0\t7\t zero \r\n2\t9\tlast\n",
                                                 false,parse_event_decimal,decode);
        check(rows.size()==3&&rows[0]&&!rows[1]&&rows[2]&&rows[2]->id==2&&rows[2]->type==9&&rows[2]->text==u"last",
              "event message catalog retains gaps and last duplicate");
        check(rows[0]->text==u" zero \r"&&decoded_tokens==std::vector<std::string>({"unicode"," zero \r","last"}),
              "event message decoding follows record order and ignores suffix tokens");
        const auto normalized=load_event_message_table("0\t3\t\r\n",true,parse_event_decimal,decode);
        check(normalized[0]->type==3&&normalized[0]->text.empty(),"event message CRLF gate preserves empty text");
        expect<std::out_of_range>([&]{load_event_message_table("-1\t0\ttext",false,parse_event_decimal,decode);},
                                  "negative event message id fails catalog placement");
        row={4,8,u"previous"};
        load_event_message_definition(row,std::nullopt,parse_event_decimal,decode);
        load_event_message_definition(row,"",parse_event_decimal,decode);
        check(row.id==4&&row.type==8&&row.text==u"previous","null and empty event message preserve receiver");
        expect<std::out_of_range>([&]{load_event_message_definition(row,"5",parse_event_decimal,decode);},"missing message type fails");
        check(row.id==5&&row.type==8&&row.text==u"previous","id store precedes missing type failure");
        expect<std::invalid_argument>([&]{load_event_message_definition(row,"6\tbad\tnew",parse_event_decimal,decode);},"invalid message type fails");
        check(row.id==6&&row.type==8&&row.text==u"previous","id store survives type parser failure");
        expect<std::out_of_range>([&]{load_event_message_definition(row,"7\t-9",parse_event_decimal,decode);},"missing message text fails");
        check(row.id==7&&row.type==-9&&row.text==u"previous","both scalars precede missing text failure");
        expect<std::invalid_argument>([&]{load_event_message_definition(row,"8\t-10\tbad",parse_event_decimal,decode);},"message decoder failure propagates");
        check(row.id==8&&row.type==-10&&row.text==u"previous","decoder failure keeps immediate scalars and previous text");
        check(&load_event_message_definition(row,"9\t-11\tunicode\tignored",parse_event_decimal,decode)==&row&&
              row.id==9&&row.type==-11&&row.text==u"\u00e9\U0001f600","message loader returns its receiver after decoding only text2");
    }
    {
        const PreparedEventForm form{{4,0,0,0},{11,22},{u"first",u"second"},{0,0,0,0}};
        std::int32_t page=0,frame=16,close=-1;
        NoticeHost gated;gated.pulse=gated.cancel=true;
        update_event_notice(form,page,frame,close,gated);
        check(gated.calls==std::vector<std::string>({"debug"})&&page==0&&frame==16&&close==-1,
              "frame16 gates resource, advance and cancel without advancing the frame");
        NoticeHost unheld;unheld.debug_values={true};
        update_event_notice(form,page,frame,close,unheld);
        check(unheld.calls==std::vector<std::string>({"debug","held"})&&frame==16,"debug without held input does not bypass opening");
        frame=17;NoticeHost idle;
        update_event_notice(form,page,frame,close,idle);
        check(idle.calls==std::vector<std::string>({"debug","frame","pulse","debug","cancel"})&&frame==17&&page==0&&close==-1,
              "frame17 frames resource before input and rereads disabled config");
        frame=16;NoticeHost forced;forced.debug_values={true,false};forced.held_values={true};
        update_event_notice(form,page,frame,close,forced);
        check(frame==17&&page==0&&close==-1&&forced.calls==std::vector<std::string>({"debug","held","frame","pulse","debug","cancel"}),
              "opening force reads config again before deciding whether to advance");
        NoticeHost fresh;fresh.debug_values={false,true};fresh.held_values={true};
        update_event_notice(form,page,frame,close,fresh);
        check(page==1&&frame==17&&fresh.selected==1&&close==-1&&
              fresh.calls==std::vector<std::string>({"debug","frame","pulse","debug","held","set","cancel"}),
              "fresh enabled held input advances using page index and preserves animation frame");
        page=0;NoticeHost pulse;pulse.pulse=true;
        update_event_notice(form,page,frame,close,pulse);
        check(page==1&&pulse.selected==1&&pulse.calls==std::vector<std::string>({"debug","frame","pulse","set","cancel"}),
              "pulse skips the second config and held reads");
        pulse.calls.clear();pulse.cancel=true;
        update_event_notice(form,page,frame,close,pulse);
        check(page==2&&close==8&&pulse.calls==std::vector<std::string>({"debug","frame","pulse"}),
              "last event schedules close8 and returns before cancel or SetEvent");
        page=0;close=-1;frame=29;NoticeHost cancel;cancel.cancel=true;
        update_event_notice(form,page,frame,close,cancel);
        check(page==0&&frame==29&&close==8&&cancel.calls.back()=="cancel","cancel schedules close without changing page or frame");
        page=0;close=-1;NoticeHost set_cancel;set_cancel.pulse=set_cancel.cancel=true;
        update_event_notice(form,page,frame,close,set_cancel);
        check(page==1&&set_cancel.selected==1&&close==8&&
              set_cancel.calls==std::vector<std::string>({"debug","frame","pulse","set","cancel"}),
              "next-page SetEvent precedes cancel checking");
        page=std::numeric_limits<std::int32_t>::max();close=-1;NoticeHost wrapped;wrapped.pulse=true;wrapped.reject_negative=true;
        expect<std::out_of_range>([&]{update_event_notice(form,page,frame,close,wrapped);},"wrapped page reaches SetEvent failure");
        check(page==std::numeric_limits<std::int32_t>::min()&&wrapped.selected==page&&close==-1&&wrapped.calls.back()=="set",
              "signed wrapped page is compared before forwarding unchanged to SetEvent");
        for(const auto* failure:{"debug","held","frame","pulse","set","cancel"}){
            page=0;frame=16;close=-1;NoticeHost failing;
            failing.debug_values={true};failing.held_values={true};failing.pulse=true;failing.failure=failure;
            expect<std::runtime_error>([&]{update_event_notice(form,page,frame,close,failing);},"notice callback failure propagates");
            const std::string name=failure;
            const std::vector<std::string> sequence{"debug","held","frame","pulse","set","cancel"};
            std::vector<std::string> prefix;
            for(const auto& call:sequence){prefix.push_back(call);if(call==name)break;}
            check(failing.calls==prefix&&close==-1,"notice failure retains only its ordered call prefix");
            check(frame==((name=="debug"||name=="held")?16:17),"opening force survives only later failures");
            check(page==((name=="set"||name=="cancel")?1:0),"page increment survives SetEvent and cancel failures");
        }
    }
    {
        auto form=prepare_talk_form(2,u"first\tsecond",{},h);
        PageHost host;std::int32_t close=-1;
        update_talk_page(form,close,host);
        check(host.calls==std::vector<std::string>({"text_frame","portrait_frame","pulse","held_enabled"})&&form.text_index==0&&close==-1,
              "no input advances text and portrait clocks without querying reveal end");
        host.calls.clear();host.held_enabled=true;
        update_talk_page(form,close,host);
        check(host.calls==std::vector<std::string>({"text_frame","portrait_frame","pulse","held_enabled","held"}),
              "held key is queried only after its config gate");
        host.calls.clear();host.pulse=true;
        update_talk_page(form,close,host);
        check(host.calls==std::vector<std::string>({"text_frame","portrait_frame","pulse","ended","flush"})&&form.text_index==0&&close==-1,
              "pulse short-circuits held input and unfinished reveal only flushes");
        host.calls.clear();host.pulse=false;host.held=true;host.ended=true;
        update_talk_page(form,close,host);
        check(host.calls==std::vector<std::string>({"text_frame","portrait_frame","pulse","held_enabled","held","ended","set_text"})&&
              form.text_index==1&&host.text==u"second"&&close==-1,
              "enabled held advance selects the next page after reveal completion");
        host.calls.clear();host.pulse=true;
        update_talk_page(form,close,host);
        check(form.text_index==2&&close==8&&host.calls==std::vector<std::string>({"text_frame","portrait_frame","pulse","ended"}),
              "last page schedules eight close updates without an immediate pop");
        form.text_index=0;close=-1;host.fail_set=true;
        expect<std::runtime_error>([&]{update_talk_page(form,close,host);},"SetText failure propagates");
        check(form.text_index==1&&close==-1,"failed next-page SetText retains the incremented index");
        host.fail_set=false;host.calls.clear();form.next_index=17;
        update_talk_page(form,close,host);
        check(host.continuation==&form&&form.text_index==2&&form.next_index==17&&close==-1&&host.calls.back()=="continue",
              "nonterminal nextIndex dispatches the explicit continuation boundary");
        form.text_index=std::numeric_limits<std::int32_t>::max();
        expect<std::out_of_range>([&]{update_talk_page(form,close,host);},"wrapped negative page index fails bounds");
        check(form.text_index==std::numeric_limits<std::int32_t>::min(),"page increment wraps int32 before failure");
    }
    auto talk = prepare_talk_form(-1, u" a \t\tb\t", {}, h);
    check(talk.initial.type == 0 && talk.initial.x == 0 && talk.initial.y == 0 &&
          talk.initial.selection == 0 && talk.character_index == -1 &&
          talk.text_index == 0 && talk.next_index == -1 && !talk.arguments,
          "talk preparation preserves explicit initial fields");
    check(talk.texts == std::vector<std::u16string>({u"a", u"", u"b", u""}) && h.translations == 0,
          "talk retains empty segments; absent args skip translation");
    expect<std::out_of_range>([&] { prepare_talk_form(0, {}, {}, h); }, "null talk text lacks first page");
    expect<std::out_of_range>([&] { prepare_talk_form(0, u"", {}, h); }, "empty talk text lacks first page");
    check(prepare_talk_form(0, u" ", {}, h).texts == std::vector<std::u16string>({u""}),
          "nonempty whitespace text supplies one empty page");
    auto event = prepare_event_form(u" 1&hello&ignored\t2 & ", {}, h);
    check(event.initial.type == 4 && event.initial.x == 0 && event.initial.y == 0 &&
          event.initial.selection == 0 && event.animation == std::array<std::int32_t,4>{0,0,0,0},
          "event factory initializes type, coordinates, selection and animation");
    check(event.event_ids == std::vector<std::int32_t>({1,2}) &&
          event.texts == std::vector<std::u16string>({u"hello", u""}) &&
          h.parsed == std::vector<std::u16string>({u"1", u"2 "}),
          "parser receives untrimmed inner type; extra fields ignored");
    check(prepare_event_form({}, {}, h).event_ids.empty() &&
          prepare_event_form(u"", {}, h).texts.empty(), "event accepts zero outer segments");
    h.parsed.clear();
    expect<std::out_of_range>([&] { prepare_event_form(u"1", {}, h); }, "missing event text fails");
    check(h.parsed == std::vector<std::u16string>({u"1"}), "numeric parse occurs before missing field failure");
    h.parsed.clear();
    expect<std::out_of_range>([&] { prepare_event_form(u"1&a\t", {}, h); }, "trailing empty event segment fails");
    check(h.parsed.size() == 1, "empty segment fails before calling numeric parser");
    expect<std::invalid_argument>([&] { prepare_event_form(u"bad&a", {}, h); }, "numeric parser failure propagates");

    // Integrated executor -> dispatcher -> actual preparation -> presentation push.
    for (const auto& values : {EventIntegers{8,0}, {10,1}, {12,0}})
        h.script->push_back(std::make_shared<EventIntegers>(values));
    const auto args = std::make_shared<std::u16string>(u"world");
    const auto first = execute_event_script(h.script, 0, {args, {}}, h.queue, h);
    check(first == h.talk && h.pushed.size() == 1 && h.talk->texts[0] == u"hello world" &&
          h.talk->arguments == u"world" && !h.event, "prepared talk pushed before delay");
    process_counted_event_delays(h.queue, h);
    check(h.queue.empty() && h.pushed.size() == 2 && h.pushed[1] == h.event &&
          h.event->event_ids == std::vector<std::int32_t>({1}) && h.event->texts[0] == u"world",
          "continuation prepares event data through real text pipeline");
    std::cout << "presentation form preparation contracts passed\n";
}
