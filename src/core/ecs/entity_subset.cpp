#include "entity_subset.hpp"
#include <stdexcept>

namespace saga::core {
EntitySubset::EntitySubset(Predicate predicate,Identity identity):identity_(std::move(identity)),predicate_(std::move(predicate)){
    if(!predicate_)throw std::invalid_argument("missing subset predicate");
}
// Slot reuse/order follows public Microsoft HashSet reference source; std::unordered_map
// handles membership. See docs/subsystems/ecs.md and third_party/licenses/dotnet-referencesource.txt.
EntitySubset::Key EntitySubset::key(ComponentSlots* e)const{
    return {e,e?(static_cast<std::uint32_t>(identity_(*e))&0x7fffffffU):0U};
}
bool EntitySubset::matches(ComponentSlots* e)const{return predicate_?predicate_(e):matches_component_filter(e,filter_->all,filter_->any,filter_->none);}
void EntitySubset::insert(ComponentSlots* e){
    const auto k=key(e);const auto slot=free_==no_slot?slots_.size():free_;
    const auto result=lookup_.emplace(k,slot);if(!result.second)return;
    if(free_==no_slot){
        try{slots_.push_back({e,no_slot});}catch(...){lookup_.erase(result.first);throw;}
    }else{free_=slots_[slot].next_free;slots_[slot]={e,no_slot};}
    ++version_;
}
bool EntitySubset::erase(ComponentSlots* e){
    const auto found=lookup_.find(key(e));if(found==lookup_.end())return false;
    const auto index=found->second;lookup_.erase(found);slots_[index]={nullptr,free_};free_=index;++version_;
    if(lookup_.empty()){slots_.clear();free_=no_slot;}
    return true;
}
bool EntitySubset::add(ComponentSlots* e){
    if(!matches(e))return false;
    insert(e);cached_entities=nullptr;return true;
}
bool EntitySubset::add(ComponentSlots* e,std::int32_t index,Component* c){
    if(!add(e))return false;
    if(notifications)notifications->added(*e,index,c);
    return true;
}
void EntitySubset::remove(ComponentSlots* e,std::int32_t index,Component* c){
    if(erase(e)&&notifications)notifications->removed(*e,index,c);
    cached_entities=nullptr; // A throwing removal callback skips this store.
}
void EntitySubset::change(ComponentSlots& e,std::int32_t index,Component* old,Component* value){
    if(notifications)notifications->changed(e,index,old,value);
}
std::optional<ComponentSlots*> EntitySubset::Cursor::next(){
    if(version_!=owner_.version_)throw std::runtime_error("entity subset changed during enumeration");
    while(index_<owner_.slots_.size())if(auto* e=owner_.slots_[index_++].entity)return e;
    return std::nullopt;
}
void EntitySubset::clear_entities(){
    lookup_.clear();slots_.clear();free_=no_slot;++version_;cached_entities=nullptr;
}
ComponentSlots* EntitySubset::first_entity()const{
    if(size()==0)return nullptr;
    auto iterator=cursor();const auto first=iterator.next();
    if(!first)throw std::runtime_error("nonempty subset has no first entity");
    return *first;
}
void SubsetMembership::added(ComponentSlots& e,std::int32_t index,Component* c){
    for(auto* group:groups_){if(!group)throw std::invalid_argument("missing entity subset");if(!group->add(&e,index,c))group->remove(&e,index,c);}
}
void SubsetMembership::removed(ComponentSlots& e,std::int32_t index,Component* c){added(e,index,c);}
void SubsetMembership::changed(ComponentSlots& e,std::int32_t index,Component* old,Component* value){
    for(auto* group:groups_){if(!group)throw std::invalid_argument("missing entity subset");group->change(e,index,old,value);}
}
}
