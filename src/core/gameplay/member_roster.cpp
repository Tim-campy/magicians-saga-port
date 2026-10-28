#include "member_roster.hpp"
#include <algorithm>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing party state");return *p;}
template<class T>T& component(ComponentSlots& e,std::int32_t slot){return required(dynamic_cast<T*>(e.get(slot)));}
std::int32_t word(std::uint32_t bits){
    return static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::int32_t add(std::int32_t a,std::int32_t b){return word(static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b));}
}
bool PartyMembers::contains(ComponentSlots* e)const{return std::find(entries_.begin(),entries_.end(),e)!=entries_.end();}
void PartyMembers::append(ComponentSlots* e){++version_;entries_.push_back(e);}
bool PartyMembers::remove(ComponentSlots* e){
    const auto at=std::find(entries_.begin(),entries_.end(),e);
    if(at==entries_.end())return false;
    entries_.erase(at);++version_;return true;
}
void PartyMembers::clear(){entries_.clear();++version_;}
void add_member(MemberRoster& state,ComponentSlots* e){
    if(!required(state.members.get()).contains(e))state.members->append(e);
    if(!required(e).contains(30)&&!required(state.sub_members.get()).contains(e))state.sub_members->append(e);
}
void remove_member(MemberRoster& state,ComponentSlots* e){
    required(state.members.get()).remove(e);
    required(state.sub_members.get()).remove(e);
}
bool set_formation_character(MemberRoster& state,std::int32_t index,std::int32_t character,
                             const std::function<bool(std::int32_t)>& is_main){
    if(index<0)return false;
    auto* formation=&required(state.formation.get());
    if(static_cast<std::size_t>(index)>=formation->size())return false;
    const auto previous=formation->at(static_cast<std::size_t>(index));
    if(character!=-1){if(previous!=-1)return false;}
    else if(previous!=-1){
        if(is_main(previous))return false;
        formation=&required(state.formation.get());
    }
    formation->at(static_cast<std::size_t>(index))=character;
    return true;
}
bool set_formation_empty_slot(MemberRoster& state,std::int32_t character,
                              const std::function<bool(std::int32_t)>& is_main){
    if(character==-1)return false;
    for(std::size_t index=0;index<required(state.formation.get()).size();++index){
        if(set_formation_character(state,static_cast<std::int32_t>(index),character,is_main))return true;
    }
    return false;
}
void swap_formation_slots(MemberRoster& state,std::int32_t first,std::int32_t second){
    if(first<0||second<0||first==second)return;
    auto& formation=required(state.formation.get());
    if(static_cast<std::size_t>(first)>=formation.size()||static_cast<std::size_t>(second)>=formation.size())return;
    std::swap(formation[static_cast<std::size_t>(first)],formation[static_cast<std::size_t>(second)]);
}
std::int32_t formation_number(const MemberRoster& state,std::int32_t character){
    if(character==-1)return -1;
    const auto& formation=required(state.formation.get());
    const auto at=std::find(formation.begin(),formation.end(),character);
    return at==formation.end()?-1:static_cast<std::int32_t>(at-formation.begin());
}
void fill_parameter(DamageParameter& value){if(value.maximum!=0)value.value=value.maximum;}
void initialize_members(MemberRoster& state,MemberInitializationHost& host){
    const auto left=host.formation_left();
    const auto top=host.formation_top();
    const auto list=state.members;
    required(list.get());
    const auto version=list->version();
    for(std::size_t index=0;;++index){
        if(version!=list->version())throw std::runtime_error("party changed during initialization");
        if(index>=list->entities().size())break;
        auto& entity=required(list->entities()[index]);
        auto* position=dynamic_cast<ContactPosition*>(entity.get(0));
        const auto x=component<HumanRecordLink>(entity,14).formation_x;
        const auto z=component<HumanRecordLink>(entity,14).formation_z;
        required(position).x=static_cast<float>(add(x,left));
        position->y=0.0F;
        position->z=static_cast<float>(add(z,top));
        auto* zero=resolve_combat_parameter(entity,0);
        const auto zero_max=host.total_parameter(entity,0);
        required(zero).maximum=zero_max;
        auto* one=resolve_combat_parameter(entity,1);
        const auto one_max=host.total_parameter(entity,1);
        required(one).maximum=one_max;
        fill_parameter(required(resolve_combat_parameter(entity,0)));
        fill_parameter(required(resolve_combat_parameter(entity,1)));
        component<ContactDirection>(entity,12).direction=0;
        host.change_animation(entity,36,true);
        change_human_state(entity,0,required(host.human_system()));
        auto& human=component<HumanRecordLink>(entity,14);
        human.attack_frame=0;
        human.opponent=nullptr;
        auto& sorties=required(host.sortie_count(human));
        sorties=add(sorties,1);
        human.invincible_time=0;
    }
}
} // namespace saga::core
