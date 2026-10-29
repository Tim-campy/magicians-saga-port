#include "effect_frame.hpp"
#include <limits>
#include <stdexcept>

namespace saga::core {
namespace {
template<class T>T& required(T* p){if(!p)throw std::invalid_argument("missing effect update input");return *p;}
EffectComponent& effect(ComponentSlots* e){return required(dynamic_cast<EffectComponent*>(required(e).get(26)));}
std::int32_t increment(std::int32_t value){const auto bits=static_cast<std::uint32_t>(value)+1U;return static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);}
void age(ComponentSlots* e){auto& value=effect(e);value.frame=increment(value.frame);}
}
void initialize_effects(EffectSystemState& s,EffectInitializationWorld* world){
    s.subset=required(world).effect_subset({26},{4});s.expired=std::make_shared<EmitterExpiryQueue>();
}
void update_normal_effect(ComponentSlots* e){
    if(effect(e).depth)return;
    auto& output=effect(e);const auto next=increment(effect(e).frame),maximum=effect(e).max_frame;
    // ARM64 SDIV/MSUB: zero divisor leaves next; INT_MIN/-1 has remainder zero.
    output.frame=maximum==0?next:(maximum==-1&&next==std::numeric_limits<std::int32_t>::min()?0:next%maximum);
}
void update_damage_number_effect(ComponentSlots* e){
    auto& position=required(dynamic_cast<ContactPosition*>(required(e).get(0)));position.y=position.y+1.0F;
    age(e);
}
void update_time_effect(ComponentSlots*) noexcept {}
void update_level_up_effect(ComponentSlots* e){age(e);}
void update_jump_number_effect(ComponentSlots* e){age(e);}
void update_warehouse_balloon_effect(ComponentSlots* e){age(e);}
void update_string_background_effect(ComponentSlots* e){age(e);}
void update_effects(EffectSystemState& s,EffectFrameHost& host){
    {
        auto cursor=required(s.subset).cursor();
        while(const auto entry=cursor.next()){
            auto* e=*entry;auto* parent=effect(e).parent;
            if(parent&&host.effect_parent_destroyed(*parent))required(s.expired.get()).add(e);
            switch(effect(e).type){
            case 0:update_normal_effect(e);break;
            case 1:update_damage_number_effect(e);break;
            case 4:update_level_up_effect(e);break;
            case 5:update_jump_number_effect(e);break;
            case 6:update_warehouse_balloon_effect(e);break;
            case 7:update_string_background_effect(e);break;
            default:break; // 2, 3 and unknown types have no update body.
            }
        }
    }
    auto pending=s.expired;auto& list=required(pending.get());if(!list.size())return;
    const auto version=list.version();
    for(std::size_t i=0;;++i){
        if(version!=list.version())throw std::runtime_error("effect expiry list changed during enumeration");
        if(i==list.size())break;
        host.destroy_effect_entity(required(list.at(i)));
    }
    required(s.expired.get()).clear();
}
}
