#include "core/gameplay/event_dispatch.hpp"
#include "core/gameplay/event_execution.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
using namespace saga::core;

// Research interchange protocol, NOT the game's Event.txt loader or save format.
template<class T> T read() {
    T value{};
    if (!(std::cin >> value)) throw std::runtime_error("invalid scenario input");
    return value;
}
std::int32_t count() {
    auto value = read<std::int32_t>();
    if (value < 0 || value > 100000) throw std::runtime_error("scenario count outside tool limit");
    return value;
}
struct Host : EventFlowHost, EventEffectsHost {
    EventProgression state{};
    EventCatalog catalog;
    DecodedEventConditionState conditions;
    DecodedEventExecutionState execution;
    EventDelayQueue queue;
    std::int64_t stamina = 0;
    std::int32_t item_state = 0, talk = 0;
    EventProgression& progression() override { return state; }
    std::int32_t stamina_limit() override { return stamina_limit_from_item_state(item_state); }
    std::int64_t read_stamina() override { return stamina; }
    void write_stamina(std::int64_t value) override { stamina = value; }
    EventObject make_talk_form(std::int32_t id, EventObject) override {
        std::cout << "{\"effect\":\"request_talk\",\"id\":" << id << "}\n";
        return std::make_shared<int>(id);
    }
    EventObject make_event_form(std::int32_t id, EventObject) override {
        std::cout << "{\"effect\":\"request_event\",\"id\":" << id << "}\n";
        return std::make_shared<int>(id);
    }
    void push_form(EventObject) override {
        std::cout << "{\"effect\":\"observe_push_request\"}\n";
    }
    EventObject dispatch(const EventInstruction& row, const EventContext& context) override {
        auto result = dispatch_known_event_instruction(row, context, *this);
        if (!result.handled) throw std::runtime_error("unsupported scenario opcode");
        sync_conditions();
        return result.value;
    }
    std::int32_t identify_script(const EventScript& script) override {
        return identify_event_script(catalog,script,[]{
            throw std::runtime_error("GetDelayId fallback context not supplied");
        });
    }
    EventScript resolve_script(std::int32_t id) override {
        return resolve_event_script(catalog,id);
    }
    std::int32_t read_talk_character() override { return talk; }
    void restore_talk_character(std::size_t slot, std::int32_t value) override {
        if (slot!=7) throw std::logic_error("unexpected talk slot");
        talk=value;
        std::cout << "{\"effect\":\"restore_talk\",\"value\":" << value << "}\n";
    }
    void sync_conditions() {
        conditions.money=state.money;conditions.crystals=state.crystals;conditions.stamina=stamina;
        if (conditions.area_states)
            for (std::size_t i=0;i<state.areas.size();++i)
                if (state.areas[i]) *conditions.area_states->at(i)=state.areas[i]->state;
    }
    void snapshot(std::int32_t pass) {
        std::cout << "{\"pass\":" << pass << ",\"money\":" << state.money
                  << ",\"crystals\":" << state.crystals << ",\"stamina\":" << stamina
                  << ",\"pending\":" << queue.size() << ",\"waits\":[";
        bool first=true;
        for (const auto& delayed:queue) {
            if (!first) std::cout << ',';
            first=false;
            std::cout << delayed->wait;
        }
        std::cout << "],\"execution_counts\":[";
        for (std::size_t i=0;i<catalog.size();++i) {
            if (i) std::cout << ',';
            if (catalog[i]) std::cout << catalog[i]->execution_count; else std::cout << "null";
        }
        std::cout << "]}\n";
    }
};
bool boolean() {
    const auto value=read<int>();
    if (value!=0 && value!=1) throw std::invalid_argument("expected boolean");
    return value==1;
}
std::string unhex(const std::string& encoded) {
    if (encoded.size()%2) throw std::invalid_argument("odd hex length");
    auto nibble=[](char c) -> unsigned {
        if (c>='0' && c<='9') return static_cast<unsigned>(c-'0');
        if (c>='a' && c<='f') return static_cast<unsigned>(c-'a'+10);
        throw std::invalid_argument("invalid hex");
    };
    std::string out;
    for (std::size_t i=0;i<encoded.size();i+=2)
        out.push_back(static_cast<char>((nibble(encoded[i])<<4U)|nibble(encoded[i+1])));
    return out;
}
void fixture(Host& h) {
    for (auto key=read<std::string>();key!="end";key=read<std::string>()) {
        auto& c=h.conditions;auto& e=h.execution;
        if (key=="context_present") c.context_present=boolean();
        else if (key=="context_character_count") c.context_character_count=count();
        else if (key=="normal_save_present") e.normal_save_present=boolean();
        else if (key=="print_enabled") e.print_enabled=boolean();
        else if (key=="debug_enabled") c.debug_enabled=boolean();
        else if (key=="debug_rescue_count") c.debug_rescue_count=read<std::int32_t>();
        else if (key=="hero_level") c.hero_level=read<std::int32_t>();
        else if (key=="high_score") c.high_score=read<std::int64_t>();
        else if (key=="town_capacity") c.town_capacity=read<std::int32_t>();
        else if (key=="quest_data_count") e.quest_data_count=count();
        else if (key=="execution_counts") {
            for (auto n=count();n>0;--n) {
                const auto id=count();const auto value=read<std::int32_t>();
                auto record=h.catalog.at(static_cast<std::size_t>(id));
                if (!record) throw std::invalid_argument("counter for null event");
                record->execution_count=value;
            }
        } else if (key=="normal_save_strings") {
            e.normal_save_strings=std::make_shared<std::vector<EventObject>>();
            for (auto n=count();n>0;--n) {
                const auto token=read<std::string>();
                if (token=="null") e.normal_save_strings->push_back(nullptr);
                else if (token.front()=='x') e.normal_save_strings->push_back(std::make_shared<std::string>(unhex(token.substr(1))));
                else throw std::invalid_argument("invalid string fixture");
            }
        } else if (key=="areas") {
            c.area_states=std::make_shared<ConditionCatalog<std::int32_t>>();h.state.areas.clear();
            for (auto n=count();n>0;--n) {
                if (!boolean()) {c.area_states->push_back(nullptr);h.state.areas.push_back(nullptr);continue;}
                const auto value=read<std::int32_t>();const auto fresh=boolean();
                c.area_states->push_back(std::make_shared<std::int32_t>(value));
                h.state.areas.push_back(std::make_shared<DiscoveryState>(DiscoveryState{value,fresh}));
            }
        } else if (key=="quests") {
            c.quests=std::make_shared<ConditionCatalog<QuestConditionState>>();h.state.quests.clear();
            for (auto n=count();n>0;--n) {
                if (!boolean()) {c.quests->push_back(nullptr);h.state.quests.push_back(nullptr);continue;}
                const auto value=read<std::int32_t>();const auto fresh=boolean();
                const auto flags=count();if (flags>255) throw std::invalid_argument("quest flags");
                const auto completed=boolean();std::optional<QuestConditionRecord> record;
                if (boolean()) record=QuestConditionRecord{read<std::int32_t>(),read<std::int32_t>(),read<std::int32_t>()};
                c.quests->push_back(std::make_shared<QuestConditionState>(QuestConditionState{static_cast<std::uint8_t>(flags),completed,record}));
                h.state.quests.push_back(std::make_shared<DiscoveryState>(DiscoveryState{value,fresh}));
            }
        } else if (key=="characters") {
            c.characters=std::make_shared<ConditionCatalog<CharacterConditionState>>();
            for (auto n=count();n>0;--n) {
                if (!boolean()) {c.characters->push_back(nullptr);continue;}
                const auto sub=read<std::int32_t>();const auto flags=count();
                if (flags>255) throw std::invalid_argument("character flags");
                c.characters->push_back(std::make_shared<CharacterConditionState>(CharacterConditionState{sub,static_cast<std::uint8_t>(flags)}));
            }
        } else if (key=="town_flags") {
            c.town_flags=std::make_shared<ConditionCatalog<std::uint32_t>>();
            for (auto n=count();n>0;--n) c.town_flags->push_back(boolean() ? std::make_shared<std::uint32_t>(read<std::uint32_t>()) : nullptr);
        } else if (key=="buildings") {
            c.buildings=std::make_shared<std::vector<EventInstruction>>();
            for (auto n=count();n>0;--n) {
                if (!boolean()) {c.buildings->push_back(nullptr);continue;}
                auto row=std::make_shared<EventIntegers>();
                for (auto size=count();size>0;--size) row->push_back(read<std::int32_t>());
                c.buildings->push_back(row);
            }
        } else throw std::invalid_argument("unknown fixture field: "+key);
    }
}
int main() {
    try {
        if (read<std::string>()!="SAGA_EVENT_TRACE_3") throw std::runtime_error("protocol version");
        Host host;
        const auto mode=read<std::string>();
        if (mode!="auto" && mode!="inject") throw std::invalid_argument("execution mode");
        const auto selected=count(), passes=count();
        host.state.money=read<std::int64_t>(); host.state.crystals=read<std::int64_t>();
        host.state.crystal_subtraction_mode=read<std::int32_t>();
        host.stamina=read<std::int64_t>(); host.item_state=read<std::int32_t>();
        host.talk=read<std::int32_t>();
        const auto table=unhex(read<std::string>());
        host.catalog=load_event_catalog(table,false,parse_event_decimal);
        host.execution.catalog=std::make_shared<EventCatalog>(host.catalog);
        fixture(host);
        host.sync_conditions();
        host.execution.diagnostic=[](EventDiagnostic kind,std::int32_t id) {
            std::cout << "{\"diagnostic\":\"" << (kind==EventDiagnostic::EmptyScript ? "empty_script" : "unknown_script") << "\",\"id\":" << id << "}\n";
        };
        DecodedEventExecutionHost execution(host.execution,host);
        DecodedEventConditionHost conditions(host.conditions);
        std::string extra;
        if (std::cin >> extra) throw std::runtime_error("trailing scenario input");
        if (mode=="auto") {
            const auto form=select_and_execute_events(host.queue,execution,conditions);
            std::cout << "{\"selection_returned_form\":" << (form ? "true" : "false") << "}\n";
        } else execute_event_script(execution.resolve_script(selected),0,{},host.queue,execution);
        host.snapshot(0);
        for (std::int32_t pass=1;pass<=passes;++pass) {
            process_counted_event_delays(host.queue,execution); host.snapshot(pass);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
