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
GaugePoint& effect_to_screen_position(ContactPosition* input,EffectProjectionHost& host){
    auto* first=host.effect_screen_work();
    auto& position=required(input);
    const auto x=host.effect_projection_x(position.x,position.z);
    required(first).x=factory_cell_coordinate(x,1);
    auto* second=host.effect_screen_work();
    const auto projected_y=host.effect_projection_y(position.x,position.z);
    auto& y_point=required(second);
    const float y=projected_y-position.y;
    y_point.y=factory_cell_coordinate(y,1);
    auto& result=required(host.effect_screen_work());
    host.validate_effect_draw_context();
    const auto camera=host.effect_camera_offset();
    const auto subtract=[](std::int32_t a,std::int32_t b){
        const auto bits=static_cast<std::uint32_t>(a)-static_cast<std::uint32_t>(b);
        return static_cast<std::int32_t>(bits<=0x7fffffffU?static_cast<std::int64_t>(bits):static_cast<std::int64_t>(bits)-0x100000000LL);
    };
    result.x=subtract(result.x,camera.x);result.y=subtract(result.y,camera.y);
    return result;
}
void draw_number_effect(ComponentSlots* input,EffectDrawingHost& host){
    auto& entity=required(input);
    auto* captured_effect=dynamic_cast<EffectComponent*>(entity.get(26));
    auto* position=dynamic_cast<ContactPosition*>(entity.get(0));
    auto& point=host.effect_screen_position(position);
    const auto context=host.capture_effect_numbers();
    auto& sprite=required(dynamic_cast<EntitySprite*>(entity.get(2)));
    auto& value=required(captured_effect);
    if(!context)throw std::invalid_argument("missing effect number application");
    host.effect_draw_number(context,sprite.animation.clip,static_cast<std::int64_t>(value.value),
        point.x,point.y,0,2,100,-1,-1);
}
void draw_effects(EffectSystemState& state,EffectDrawingHost& host){
    auto cursor=required(state.subset).cursor();
    while(const auto entry=cursor.next()){
        auto& entity=required(*entry);
        switch(static_cast<std::uint32_t>(effect(&entity).type)){
        case 0:host.effect_draw_normal(entity);break;
        case 1:host.effect_draw_damage(entity);break;
        case 2:host.effect_draw_time(entity);break;
        case 3:draw_number_effect(&entity,host);break;
        case 5:host.effect_draw_jump(entity);break;
        case 7:host.effect_draw_string_background(entity);break;
        default:break; // LevelUp4 and WarehouseBalloon6 return without touching the entity.
        }
    }
}
}
