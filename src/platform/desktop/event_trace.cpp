#include "core/gameplay/event_dispatch.hpp"
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
    std::vector<EventScript> scripts;
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
        if (row && !row->empty() && (row->at(0)==4 || row->at(0)==15))
            throw std::runtime_error("scenario requires area/quest state not supplied by this tool");
        auto result = dispatch_known_event_instruction(row, context, *this);
        if (!result.handled) throw std::runtime_error("unsupported scenario opcode");
        return result.value;
    }
    std::int32_t identify_script(const EventScript& script) override {
        for (std::size_t i=0; i<scripts.size(); ++i)
            if (scripts[i]==script) return static_cast<std::int32_t>(i);
        throw std::runtime_error("script outside supplied catalog");
    }
    EventScript resolve_script(std::int32_t id) override {
        if (id<0) throw std::out_of_range("negative scenario catalog index");
        return scripts.at(static_cast<std::size_t>(id));
    }
    std::int32_t read_talk_character() override { return talk; }
    void restore_talk_character(std::size_t slot, std::int32_t value) override {
        if (slot!=7) throw std::logic_error("unexpected talk slot");
        talk=value;
        std::cout << "{\"effect\":\"restore_talk\",\"value\":" << value << "}\n";
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
        std::cout << "]}\n";
    }
};
int main() {
    try {
        if (read<std::string>()!="SAGA_EVENT_TRACE_1") throw std::runtime_error("protocol version");
        Host host;
        const auto selected=count(), passes=count();
        host.state.money=read<std::int64_t>(); host.state.crystals=read<std::int64_t>();
        host.state.crystal_subtraction_mode=read<std::int32_t>();
        host.stamina=read<std::int64_t>(); host.item_state=read<std::int32_t>();
        host.talk=read<std::int32_t>();
        const auto records=count();
        for (std::int32_t i=0;i<records;++i) {
            const auto rows=read<std::int32_t>();
            if (rows==-1) { host.scripts.push_back(nullptr); continue; }
            if (rows<0 || rows>100000) throw std::runtime_error("invalid script count");
            auto script=std::make_shared<std::vector<EventInstruction>>();
            for (std::int32_t r=0;r<rows;++r) {
                const auto cells=count();
                auto row=std::make_shared<EventIntegers>();
                for (std::int32_t c=0;c<cells;++c) row->push_back(read<std::int32_t>());
                script->push_back(row);
            }
            host.scripts.push_back(script);
        }
        std::string extra;
        if (std::cin >> extra) throw std::runtime_error("trailing scenario input");
        execute_event_script(host.resolve_script(selected),0,{},host.queue,host);
        host.snapshot(0);
        for (std::int32_t pass=1;pass<=passes;++pass) {
            process_counted_event_delays(host.queue,host); host.snapshot(pass);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
