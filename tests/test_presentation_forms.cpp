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
int main() {
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
