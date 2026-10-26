#include "event_execution.hpp"
#include <limits>
#include <stdexcept>
namespace saga::core {
namespace {
std::shared_ptr<EventCatalog> catalog(EventExecutionHost& host) {
    auto result=host.events();
    if (!result) throw std::invalid_argument("null event catalog");
    return result;
}
std::shared_ptr<EventConditionRecord> record(EventExecutionHost& host,std::int32_t id) {
    return catalog(host)->at(static_cast<std::size_t>(id));
}
const EventConditionRecord& required(const std::shared_ptr<EventConditionRecord>& value) {
    if (!value) throw std::invalid_argument("null event record");
    return *value;
}
// CheckTerm(int), 0x110b3e0: bounds, saved terms record, fresh counter record.
bool eligible(std::int32_t id,EventExecutionHost& host,EventConditionHost& conditions) {
    const auto initial=host.events();
    if (!initial || id<0 || static_cast<std::size_t>(id)>=initial->size()) return false;
    const auto terms_record=record(host,id);
    if (!terms_record) return false;
    if (required(record(host,id)).execution_count>0) return false;
    return check_event_conditions(terms_record->terms,conditions);
}
}
// AppData.ExecEvent(index,args,argi), 0x110c668. Direct execution bypasses terms.
EventObject execute_indexed_event(std::int32_t id, EventContext context,
                                 EventDelayQueue& queue, EventExecutionHost& host) {
    if (id<0) return nullptr;
    if (static_cast<std::size_t>(id)>=catalog(host)->size()) return nullptr;
    if (!context.args) context.args=host.default_event_args();
    const auto script_record=record(host,id);
    const auto counter_record=record(host,id);
    const auto count=required(record(host,id)).execution_count;
    required(counter_record);
    counter_record->execution_count=count==std::numeric_limits<std::int32_t>::max()
        ? std::numeric_limits<std::int32_t>::min() : count+1;
    if (!script_record || !script_record->effects || script_record->effects->empty()) {
        host.empty_event_diagnostic(id);
        return nullptr;
    }
    return execute_event_script(script_record->effects,0,context,queue,host);
}
// AppData.ExecEvent(), 0x110c580. Keep scanning after null-returning effects.
EventObject select_and_execute_events(EventDelayQueue& queue, EventExecutionHost& host,
                                      EventConditionHost& conditions) {
    for (std::size_t i=0;i<catalog(host)->size();++i) {
        const auto id=static_cast<std::int32_t>(i);
        if (!record(host,id) || !eligible(id,host,conditions)) continue;
        auto result=execute_indexed_event(id,{},queue,host);
        if (result) return result;
    }
    return nullptr;
}
std::shared_ptr<EventCatalog> DecodedEventExecutionHost::events() { return state_.catalog; }
EventObject DecodedEventExecutionHost::default_event_args() {
    if (!state_.sys_save_present.value()) return nullptr;
    if (!state_.sys_save_strings) throw std::invalid_argument("null sysSave string array");
    return state_.sys_save_strings->at(0);
}
void DecodedEventExecutionHost::diagnose(EventDiagnostic kind,std::int32_t id) {
    if (state_.print_enabled.value()) state_.diagnostic(kind,id);
}
void DecodedEventExecutionHost::empty_event_diagnostic(std::int32_t id) {
    diagnose(EventDiagnostic::EmptyScript,id);
}
EventObject DecodedEventExecutionHost::dispatch(const EventInstruction& row,const EventContext& context) {
    return flow_.dispatch(row,context);
}
std::int32_t DecodedEventExecutionHost::identify_script(const EventScript& script) {
    // GetDelayId 0x110caa0: fresh catalog accesses, then a length-only fallback.
    for (std::size_t i=0;i<catalog(*this)->size();++i) {
        if (!record(*this,static_cast<std::int32_t>(i))) continue;
        if (required(record(*this,static_cast<std::int32_t>(i))).effects==script)
            return static_cast<std::int32_t>(i);
    }
    const auto count=state_.quest_data_count.value();
    if (count>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("questData length exceeds managed array bounds");
    // 0x110cbc8..0x110cbd8 increments a local index without loading any element.
    // Under valid managed-array bounds the loop has no observable side effect.
    diagnose(EventDiagnostic::UnknownScript,-1);
    return -1;
}
EventScript DecodedEventExecutionHost::resolve_script(std::int32_t id) {
    // Preserve the original early guard even if the catalog is absent.
    if (id>999) return nullptr;
    return resolve_event_script(*catalog(*this),id);
}
std::int32_t DecodedEventExecutionHost::read_talk_character() {return flow_.read_talk_character();}
void DecodedEventExecutionHost::restore_talk_character(std::size_t slot,std::int32_t value) {
    flow_.restore_talk_character(slot,value);
}

}
