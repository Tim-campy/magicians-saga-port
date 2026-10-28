#include "dynamic_subsets.hpp"
#include <stdexcept>

namespace saga::core {
bool ComponentGroupFilter::equals(GroupFilter& other){
    auto* concrete=dynamic_cast<ComponentGroupFilter*>(&other);
    return concrete&&identity.equals(&concrete->identity,diagnostic);
}
bool ComponentGroupFilter::matches(ComponentSlots* e){
    const auto& f=identity.lists;return matches_component_filter(e,f.all,f.any,f.none);
}
std::shared_ptr<DynamicSubsets::Entry> DynamicSubsets::find(std::uint32_t hash,GroupFilter& filter){
    const auto at=buckets_.find(hash);
    auto entry=at==buckets_.end()?nullptr:at->second;
    for(;entry;entry=entry->next)if(entry->filter->equals(filter))return entry;
    return nullptr;
}
EntitySubset& DynamicSubsets::get(std::shared_ptr<GroupFilter> filter){
    if(!filter)throw std::invalid_argument("missing subset filter");
    if(initialized_){
        const auto hash=static_cast<std::uint32_t>(filter->hash())&0x7fffffffU;
        if(auto hit=find(hash,*filter))return *hit->group;
    }
    auto group=std::make_unique<EntitySubset>([filter](ComponentSlots* e){return filter->matches(e);},identity_);
    {
        auto cursor=registry_.values();
        while(auto entity=cursor.next())group->add(*entity); // Silent population, in registry order.
    }
    ++version_;initialized_=true; // Dictionary.TryInsert changes version before hashing/duplicate failure.
    const auto hash=static_cast<std::uint32_t>(filter->hash())&0x7fffffffU;
    if(find(hash,*filter))throw std::invalid_argument("duplicate subset filter at publication");
    auto& head=buckets_[hash];
    auto entry=std::make_shared<Entry>(Entry{std::move(filter),std::move(group),head});
    order_.push_back(entry);head=entry;
    return *entry->group;
}
void DynamicSubsets::added(ComponentSlots& e,std::int32_t index,Component* c){
    const auto version=version_;
    for(std::size_t i=0;;++i){
        if(version!=version_)throw std::runtime_error("subset directory changed during notification");
        if(i>=order_.size())break;
        const auto entry=order_[i];
        if(!entry->group->add(&e,index,c))entry->group->remove(&e,index,c);
    }
}
void DynamicSubsets::removed(ComponentSlots& e,std::int32_t i,Component* c){added(e,i,c);}
void DynamicSubsets::changed(ComponentSlots& e,std::int32_t index,Component* old,Component* c){
    const auto version=version_;
    for(std::size_t i=0;;++i){
        if(version!=version_)throw std::runtime_error("subset directory changed during notification");
        if(i>=order_.size())break;
        const auto entry=order_[i];entry->group->change(e,index,old,c);
    }
}
}
