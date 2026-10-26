#include "select_triggers.hpp"
#include <stdexcept>
namespace saga::core {
namespace {
template<class T> T& required(const std::shared_ptr<T>& value) {
    if(!value)throw std::invalid_argument("null trigger data");
    return *value;
}
std::int32_t signed_bits(std::uint32_t bits) {
    return bits<=0x7fffffffU ? static_cast<std::int32_t>(bits)
        : static_cast<std::int32_t>(static_cast<std::int64_t>(bits)-0x100000000LL);
}
}
// AppData.CheckAlreadyEvent, 0x110c4c4: two independent catalog accesses.
bool event_already_executed(std::int32_t id,EventExecutionHost& host) {
    if(id<0)return false;
    if(static_cast<std::size_t>(id)>=required(host.events()).size())return false;
    const auto item=required(host.events()).at(static_cast<std::size_t>(id));
    return required(item).execution_count>0;
}
void process_select_triggers(SelectTriggerHost& host,EventDelayQueue& queue,EventExecutionHost& execution) {
    if(static_cast<std::uint32_t>(host.stamina())==1U && !event_already_executed(1118,execution)) {
        execute_indexed_event(1118,{},queue,execution); // Return deliberately discarded.
        if((static_cast<std::uint32_t>(required(host.save_integers()).at(0)) & 0x80U)==0) {
            // AppData.On retains the fresh normal-save array through read/OR/write.
            const auto save=host.save_integers();
            const auto bits=static_cast<std::uint32_t>(required(save).at(0));
            save->at(0)=signed_bits(bits | 0x80U);
        }
    }
    if((host.system_flags() & 0x2000U)!=0 && !event_already_executed(1133,execution))
        execute_indexed_event(1133,{},queue,execution);
}
}
