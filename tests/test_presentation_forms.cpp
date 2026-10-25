#include "core/gameplay/presentation_forms.hpp"
#include "core/gameplay/event_dispatch.hpp"
#include <cstdlib>
#include <iostream>
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
int main() {
    Host h;
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
