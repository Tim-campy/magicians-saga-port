#pragma once
#include "event_flow.hpp"

namespace saga::core {
// AppData.AddMoney/AddCrystal decoded-value contract: sign-extend int32, wrap64, no clamp.
std::int64_t add_decoded_currency(std::int64_t value,std::int32_t amount);
// AppData.CheckCrystal 0x1109c80: signed int64 amount <=0 succeeds without reading storage.
bool check_decoded_crystals(std::int64_t value,std::int64_t amount);
std::int64_t subtract_decoded_crystals(std::int64_t value,std::int32_t amount,std::int32_t yugami);
struct DiscoveryState {
    std::int32_t state;
    bool is_new;
};
// QuestData.Find 0x1172f0c: bool argument ignored; preserve every nonzero state.
inline void find_quest_discovery(std::int32_t& state,bool& is_new,bool){
    if(state==0){state=1;is_new=true;}
}
struct EventProgression {
    std::int64_t money;
    std::int64_t crystals;
    // Decoded SecureInt selected by AppData.SYSSAVEI_YUGAMI. No assumed default.
    std::int32_t crystal_subtraction_mode;
    std::vector<std::shared_ptr<DiscoveryState>> areas;
    std::vector<std::shared_ptr<DiscoveryState>> quests;
};

// Unit-dispatch preprocessing, AppData.ExecuteScript34379 VA0x110cc4c:
// clone the row, substitute INT_MIN..INT_MIN+99 in arguments (not opcode).
EventIntegers resolve_event_arguments(const EventIntegers& row,
                                     const std::shared_ptr<EventIntegers>& arguments);

// Effects for opcodes1/2/4/15 on decoded state, after argument substitution.
// True means handled; false explicitly leaves remaining opcodes to the host.
// This is not SecureProperty serialization, UI notification or a full dispatcher.
bool apply_event_progression(const EventIntegers& row, EventProgression& state);
}
