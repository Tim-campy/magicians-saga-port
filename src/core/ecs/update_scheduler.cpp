#include "update_scheduler.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace saga::core {
namespace {
template<class T>T& require(T* p){if(!p)throw std::invalid_argument("missing system update input");return *p;}
}
void update_systems(UpdateSchedule& schedule,std::int32_t pass){
    // Indexed traversal intentionally observes the current list/size each iteration.
    for(std::size_t index=0;index<require(schedule.systems).size();++index){
        auto* current=schedule.systems->at(index); // Capture before stopwatch callbacks.
        if(pass==0){require(schedule.stopwatch).reset();require(schedule.stopwatch).start();}
        if(require(current).needs_update(pass))current->update(pass);
        if(pass==0)require(schedule.stopwatch).stop(); // Not a finally block.
    }
}
bool gameplay_needs_update(std::int32_t index,bool enabled,std::int32_t pass){
    if(std::find(gameplay_update_order.begin(),gameplay_update_order.end(),index)==gameplay_update_order.end())
        throw std::invalid_argument("factory entry has no update interface");
    const bool first_only=index==0||index==8||index==20||index==21||index==26;
    return enabled&&(!first_only||pass==0);
}
GameplayUpdate::GameplayUpdate(std::int32_t index,const bool& enabled,std::function<void(std::int32_t)> body)
    :index_(index),enabled_(enabled),body_(std::move(body)){
    gameplay_needs_update(index_,false,0);
    if(!body_)throw std::invalid_argument("missing gameplay update body");
}
bool GameplayUpdate::needs_update(std::int32_t pass){return gameplay_needs_update(index_,enabled_,pass);}
std::vector<UpdateSystem*> make_gameplay_update_list(const std::array<UpdateSystem*,32>& factory){
    std::vector<UpdateSystem*> result;result.reserve(gameplay_update_order.size());
    for(const auto index:gameplay_update_order)result.push_back(&require(factory[static_cast<std::size_t>(index)]));
    return result;
}
}
