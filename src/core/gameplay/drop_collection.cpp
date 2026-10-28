#include "drop_collection.hpp"
#include "projectile_motion.hpp"
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* value,const char* message) {
    if(!value)throw std::invalid_argument(message);
    return *value;
}
ComponentSlots& entity(ComponentSlots* e){return required(e,"missing collected entity");}
DropComponent& drop(ComponentSlots& e){return required(dynamic_cast<DropComponent*>(e.get(34)),"missing drop");}
ContactPosition* position_pointer(ComponentSlots& e){return dynamic_cast<ContactPosition*>(e.get(0));}
ContactPosition& position(ComponentSlots& e){return required(position_pointer(e),"missing drop position");}
std::int32_t add_word(std::int32_t a,std::int32_t b) {
    const auto bits=static_cast<std::uint32_t>(a)+static_cast<std::uint32_t>(b);
    return static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);
}
std::vector<ComponentSlots*>& list(DropCollectionState& state){return required(state.collecting.get(),"missing collecting list");}
void remove_at(std::vector<ComponentSlots*>& entries,std::int32_t index) {
    const auto offset=static_cast<std::size_t>(index);
    if(offset>=entries.size())throw std::out_of_range("collecting list index");
    entries.erase(entries.begin()+static_cast<std::ptrdiff_t>(offset));
}
}
bool drop_is_collecting(const DropComponent& value) noexcept{return value.frame>=0;}
bool treasure_is_boss_reward(const TreasureRecord& value) noexcept{return (value.flags&1)!=0;}
void on_collected_drop(DropCollectionState& state,ComponentSlots* input,DropCollectionHost& host) {
    auto& e=entity(input);
    const auto type=drop(e).type;
    auto& incoming=drop(e);
    if(type>=0&&type<=3) {
        const auto value=incoming.value;
        record_collected_drop(required(state.rewards,"missing stage rewards"),type,value);
        if(type==0||type==1) {
            auto* world=host.effect_world();
            const auto x=position(e).x,y=position(e).y,z=position(e).z;
            const float raised=y+20.0F;
            auto& binding=required(world,"missing effect world");
            create_effect({{x,raised,z},3,value,2,58,false,false,30,0,nullptr,-1,false},binding.ids,binding.factory);
        }
    }
    auto* world=host.effect_world();
    const auto x=position(e).x,y=position(e).y,z=position(e).z;
    auto& binding=required(world,"missing effect world");
    create_visual_effect({x,y,z},0,25,false,binding.ids,binding.factory);
    add_garbage_lifetime(e,0,host);
    host.play_collection_sound();
}
void collect_drops(DropCollectionState& state,DropCollectionHost& host) {
    auto* front=host.front_line_human();
    auto& group=required(host.drop_subset(),"missing drop subset");
    auto cursor=group.cursor();
    while(const auto entry=cursor.next()) {
        auto& e=entity(*entry);
        if(drop_is_collecting(drop(e)))continue;
        const auto type=drop(e).type;
        const auto value=drop(e).value;
        if(type==3&&treasure_is_boss_reward(host.treasure(value))) {
            const auto front_z=position(entity(front)).z;
            const auto drop_z=position(e).z;
            if(!(front_z<=drop_z)||!state.boss_defeated)continue; // Ordered comparison rejects NaN.
            on_collected_drop(state,&e,host);
            state.reward_box_picked_up=true;
            host.play_reward_jingle();
        } else {
            const auto front_z=position(entity(front)).z;
            const float threshold=position(e).z+100.0F;
            if(!(front_z<=threshold))continue;
            drop(e).frame=0;
            list(state).push_back(&e);
        }
    }
}
void update_collecting_drops(DropCollectionState& state,DropCollectionHost& host) {
    auto& target=entity(host.hero());
    const auto x=position(target).x,z=position(target).z; // One hero snapshot for the whole pass, even for an empty list.
    std::int32_t index=0;
    while(static_cast<std::int64_t>(index)<static_cast<std::int64_t>(list(state).size())) {
        auto& e=entity(list(state).at(static_cast<std::size_t>(index)));
        auto* destination=position_pointer(e);
        const auto x0=drop(e).x0,frame_x=static_cast<float>(drop(e).frame);
        const auto eased_x=easing_float(x0,x,30.0F,frame_x,-80.0F);
        required(destination,"missing drop position").x=eased_x;
        destination=position_pointer(e);
        const auto z0=drop(e).z0,frame_z=static_cast<float>(drop(e).frame);
        const auto eased_z=easing_float(z0,z,30.0F,frame_z,-80.0F);
        required(destination,"missing drop position").z=eased_z;
        auto& current=drop(e);current.frame=add_word(current.frame,1);
        if(drop(e).frame>=31) {
            on_collected_drop(state,&e,host);
            remove_at(list(state),index); // Callbacks may replace the list; do not erase from a captured old list.
            index=add_word(index,-1);
        }
        index=add_word(index,1);
    }
}
} // namespace saga::core
