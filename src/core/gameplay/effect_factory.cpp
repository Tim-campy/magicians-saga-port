#include "effect_factory.hpp"
#include <cmath>
#include <limits>

namespace saga::core {
namespace {
std::int32_t bits(std::uint32_t v){return static_cast<std::int32_t>(v<=0x7fffffffU?static_cast<std::int64_t>(v):static_cast<std::int64_t>(v)-0x100000000LL);}

}
ComponentSlots& create_effect(EffectRequest r,EntityIdState& ids,EffectFactoryHost& host){
    auto max_frame=r.max_frame;
    if(max_frame==0&&!r.loop)max_frame=bits(static_cast<std::uint32_t>(host.clip_max_frame(r.resource,r.clip))-1U);
    auto& entity=create_entity(ids,0,false,host);
    auto& effect=host.acquire_effect();
    effect.type=r.type;effect.value=r.value;effect.depth=r.depth;effect.frame=r.frame;effect.max_frame=max_frame;effect.parent=r.parent;
    entity.add(26,&effect);
    auto& position=host.acquire_position();position.x=r.position.x;position.y=r.position.y;position.z=r.position.z;
    position.offset_x=0;position.offset_y=0;position.offset_z=0;position.parent=nullptr;entity.add(0,&position);
    auto& speed=host.acquire_speed();speed.x=0;speed.y=0;speed.z=0;entity.add(1,&speed);
    auto& sprite=host.acquire_sprite();sprite.animation={r.resource,r.clip,r.frame};sprite.layers=std::make_shared<std::vector<std::int32_t>>(1,-1);entity.add(2,&sprite);
    if(r.depth){
        host.add_cell(entity,factory_cell_coordinate(r.position.x),factory_cell_coordinate(r.position.z));
        auto& depth=host.acquire_depth();depth.value=0;depth.extra=40;entity.add(4,&depth);
    }
    if(!r.loop)add_garbage_lifetime(entity,max_frame,host);
    if(r.image!=-1)host.add_image(entity,{{r.image,r.resource,-1,-1,-1,-1}});
    if(r.animate)host.add_playback(entity,1,-1);
    return entity;
}
ComponentSlots& create_visual_effect(ProjectilePoint p,std::int32_t clip,std::int32_t image,bool loop,EntityIdState& ids,EffectFactoryHost& host){
    return create_effect({p,0,0,20,clip,true,loop,0,0,nullptr,image,true},ids,host);
}
}
