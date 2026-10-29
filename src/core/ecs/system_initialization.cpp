#include "system_initialization.hpp"
#include <stdexcept>

namespace saga::core {
void InitList::append(InitSystem* system){entries_.push_back(system);++version_;}
void InitList::replace(std::size_t index,InitSystem* system){entries_.at(index)=system;++version_;}
void InitList::erase(std::size_t index){
    if(index>=entries_.size())throw std::out_of_range("initialization list index");
    entries_.erase(entries_.begin()+static_cast<std::ptrdiff_t>(index));++version_;
}
void InitList::clear(){entries_.clear();++version_;}
void append_gameplay_initializers(InitList& list,const std::array<InitSystem*,32>& factory){
    for(auto* system:factory)if(!system)throw std::invalid_argument("missing native system initializer");
    for(auto* system:factory)list.append(system);
}
void initialize_systems(InitializationSchedule& schedule,InitializationWorld* world){
    auto* list=schedule.systems;
    if(!list)throw std::invalid_argument("missing initialization list");
    const auto version=list->version_;
    for(std::size_t index=0;;++index){
        // List<T>.Enumerator checks version even for the final MoveNext.
        if(version!=list->version_)throw std::runtime_error("system initialization list changed");
        if(index>=list->entries_.size())break;
        auto* system=list->entries_[index];
        if(!system)throw std::invalid_argument("missing initialization system");
        system->init(world);
    }
    schedule.paused=false;
}
}
